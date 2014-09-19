// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/image_raster_worker_pool.h"

#include "base/debug/trace_event.h"
#include "base/debug/trace_event_argument.h"
#include "cc/debug/traced_value.h"
#include "cc/resources/resource.h"

namespace cc {

// static
scoped_ptr<RasterWorkerPool> ImageRasterWorkerPool::Create(
    base::SequencedTaskRunner* task_runner,
    TaskGraphRunner* task_graph_runner,
    ResourceProvider* resource_provider) {
  return make_scoped_ptr<RasterWorkerPool>(new ImageRasterWorkerPool(
      task_runner, task_graph_runner, resource_provider));
}

ImageRasterWorkerPool::ImageRasterWorkerPool(
    base::SequencedTaskRunner* task_runner,
    TaskGraphRunner* task_graph_runner,
    ResourceProvider* resource_provider)
    : task_runner_(task_runner),
      task_graph_runner_(task_graph_runner),
      namespace_token_(task_graph_runner->GetNamespaceToken()),
      resource_provider_(resource_provider),
      raster_tasks_pending_(false),
      raster_tasks_required_for_activation_pending_(false),
      raster_finished_weak_ptr_factory_(this) {}

ImageRasterWorkerPool::~ImageRasterWorkerPool() {}

Rasterizer* ImageRasterWorkerPool::AsRasterizer() { return this; }

void ImageRasterWorkerPool::SetClient(RasterizerClient* client) {
  client_ = client;
}

void ImageRasterWorkerPool::Shutdown() {
  TRACE_EVENT0("cc", "ImageRasterWorkerPool::Shutdown");

  TaskGraph empty;
  task_graph_runner_->ScheduleTasks(namespace_token_, &empty);
  task_graph_runner_->WaitForTasksToFinishRunning(namespace_token_);
}

void ImageRasterWorkerPool::ScheduleTasks(RasterTaskQueue* queue) {
  TRACE_EVENT0("cc", "ImageRasterWorkerPool::ScheduleTasks");

  DCHECK_EQ(queue->required_for_activation_count,
            static_cast<size_t>(
                std::count_if(queue->items.begin(),
                              queue->items.end(),
                              RasterTaskQueue::Item::IsRequiredForActivation)));

  if (!raster_tasks_pending_)
    TRACE_EVENT_ASYNC_BEGIN0("cc", "ScheduledTasks", this);

  raster_tasks_pending_ = true;
  raster_tasks_required_for_activation_pending_ = true;

  unsigned priority = kRasterTaskPriorityBase;

  graph_.Reset();

  // Cancel existing OnRasterFinished callbacks.
  raster_finished_weak_ptr_factory_.InvalidateWeakPtrs();

  scoped_refptr<RasterizerTask>
      new_raster_required_for_activation_finished_task(
          CreateRasterRequiredForActivationFinishedTask(
              queue->required_for_activation_count,
              task_runner_.get(),
              base::Bind(
                  &ImageRasterWorkerPool::OnRasterRequiredForActivationFinished,
                  raster_finished_weak_ptr_factory_.GetWeakPtr())));
  scoped_refptr<RasterizerTask> new_raster_finished_task(
      CreateRasterFinishedTask(
          task_runner_.get(),
          base::Bind(&ImageRasterWorkerPool::OnRasterFinished,
                     raster_finished_weak_ptr_factory_.GetWeakPtr())));

  for (RasterTaskQueue::Item::Vector::const_iterator it = queue->items.begin();
       it != queue->items.end();
       ++it) {
    const RasterTaskQueue::Item& item = *it;
    RasterTask* task = item.task;
    DCHECK(!task->HasCompleted());

    if (item.required_for_activation) {
      graph_.edges.push_back(TaskGraph::Edge(
          task, new_raster_required_for_activation_finished_task.get()));
    }

    InsertNodesForRasterTask(&graph_, task, task->dependencies(), priority++);

    graph_.edges.push_back(
        TaskGraph::Edge(task, new_raster_finished_task.get()));
  }

  InsertNodeForTask(&graph_,
                    new_raster_required_for_activation_finished_task.get(),
                    kRasterRequiredForActivationFinishedTaskPriority,
                    queue->required_for_activation_count);
  InsertNodeForTask(&graph_,
                    new_raster_finished_task.get(),
                    kRasterFinishedTaskPriority,
                    queue->items.size());

  ScheduleTasksOnOriginThread(this, &graph_);
  task_graph_runner_->ScheduleTasks(namespace_token_, &graph_);

  raster_finished_task_ = new_raster_finished_task;
  raster_required_for_activation_finished_task_ =
      new_raster_required_for_activation_finished_task;

  TRACE_EVENT_ASYNC_STEP_INTO1(
      "cc", "ScheduledTasks", this, "rasterizing", "state", StateAsValue());
}

void ImageRasterWorkerPool::CheckForCompletedTasks() {
  TRACE_EVENT0("cc", "ImageRasterWorkerPool::CheckForCompletedTasks");

  task_graph_runner_->CollectCompletedTasks(namespace_token_,
                                            &completed_tasks_);
  for (Task::Vector::const_iterator it = completed_tasks_.begin();
       it != completed_tasks_.end();
       ++it) {
    RasterizerTask* task = static_cast<RasterizerTask*>(it->get());

    task->WillComplete();
    task->CompleteOnOriginThread(this);
    task->DidComplete();

    task->RunReplyOnOriginThread();
  }
  completed_tasks_.clear();
}

SkCanvas* ImageRasterWorkerPool::AcquireCanvasForRaster(RasterTask* task) {
  SkCanvas* canvas = 0;
#ifdef DO_ZERO_COPY_WITH_ATLAS
  WebTech::TextureMemory* texture;
  gfx::Rect texture_rect;
  canvas = resource_provider_->MapImageRasterBuffer(task->resource()->id(), &texture, &texture_rect);
  task->SetTexture(texture, texture_rect);
#else
  canvas = resource_provider_->MapImageRasterBuffer(task->resource()->id());
#endif

#ifdef DO_PARTIAL_RASTERIZATION
#ifndef COPYBACK_ON_WORKER_THREAD
  if (task->copy_from_resource()) {
    ZEROCOPY_LOG_PARTIAL("ImageRasterWorkerPool::AcquireCanvasForRaster has copy resource");
    SkBitmap* copy_bitmap = resource_provider_->AccessImageRasterBuffer(task->copy_from_resource()->id());
    if (copy_bitmap) {
      ZEROCOPY_LOG_PARTIAL("    ImageRasterWorkerPool::AcquireCanvasForRaster copy resource has bitmap");
      canvas->drawBitmap(*copy_bitmap, 0,0);
      delete copy_bitmap;
    }
    resource_provider_->UnlockForCopy(task->copy_from_resource()->id());
  }
#endif
#endif

  return canvas;
}

#ifdef DO_PARTIAL_RASTERIZATION

SkBitmap* ImageRasterWorkerPool::AcquireCopyFromBitmap(RasterTask* task) {
#ifndef COPYBACK_ON_WORKER_THREAD
  return 0;
#else

  if (task->copy_from_resource()) {
    ZEROCOPY_LOG_PARTIAL("ImageRasterWorkerPool::AcquireCopyFromBitmap has copy resource");
    SkBitmap* copy_bitmap = resource_provider_->AccessImageRasterBuffer(task->copy_from_resource()->id());
    return copy_bitmap;
  }

  return 0;
#endif
}

#endif

void ImageRasterWorkerPool::ReleaseCanvasForRaster(RasterTask* task) {
  resource_provider_->UnmapImageRasterBuffer(task->resource()->id());

  // Map/UnmapImageRasterBuffer provides direct access to the memory used by the
  // GPU. Read lock fences are required to ensure that we're not trying to map a
  // resource that is currently in-use by the GPU.
  resource_provider_->EnableReadLockFences(task->resource()->id());

#ifdef DO_PARTIAL_RASTERIZATION
#ifdef COPYBACK_ON_WORKER_THREAD
  if (task->copy_from_resource()) {
    resource_provider_->UnlockForCopy(task->copy_from_resource()->id());
  }
#endif
#endif
}

void ImageRasterWorkerPool::OnRasterFinished() {
  TRACE_EVENT0("cc", "ImageRasterWorkerPool::OnRasterFinished");

  DCHECK(raster_tasks_pending_);
  raster_tasks_pending_ = false;
  TRACE_EVENT_ASYNC_END0("cc", "ScheduledTasks", this);
  client_->DidFinishRunningTasks();
}

void ImageRasterWorkerPool::OnRasterRequiredForActivationFinished() {
  TRACE_EVENT0("cc",
               "ImageRasterWorkerPool::OnRasterRequiredForActivationFinished");

  DCHECK(raster_tasks_required_for_activation_pending_);
  raster_tasks_required_for_activation_pending_ = false;
  TRACE_EVENT_ASYNC_STEP_INTO1(
      "cc", "ScheduledTasks", this, "rasterizing", "state", StateAsValue());
  client_->DidFinishRunningTasksRequiredForActivation();
}

scoped_refptr<base::debug::ConvertableToTraceFormat>
ImageRasterWorkerPool::StateAsValue() const {
  scoped_refptr<base::debug::TracedValue> state =
      new base::debug::TracedValue();

  state->SetBoolean("tasks_required_for_activation_pending",
                    raster_tasks_required_for_activation_pending_);
  return state;
}

}  // namespace cc
