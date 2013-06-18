// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_output_surface.h"

#include "base/bind.h"
#include "base/message_loop.h"
#include "cc/output/compositor_frame_ack.h"
#include "cc/output/output_surface_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {

FakeOutputSurface::FakeOutputSurface(
    scoped_ptr<WebKit::WebGraphicsContext3D> context3d,
    bool delegated_rendering)
    : OutputSurface(context3d.Pass()),
      num_sent_frames_(0),
      needs_begin_frame_(false),
      forced_draw_to_software_device_(false),
      fake_weak_ptr_factory_(this) {
  if (delegated_rendering) {
    capabilities_.delegated_rendering = true;
    capabilities_.max_frames_pending = 1;
  }
}

FakeOutputSurface::FakeOutputSurface(
    scoped_ptr<SoftwareOutputDevice> software_device, bool delegated_rendering)
    : OutputSurface(software_device.Pass()),
      num_sent_frames_(0),
      forced_draw_to_software_device_(false),
      fake_weak_ptr_factory_(this) {
  if (delegated_rendering) {
    capabilities_.delegated_rendering = true;
    capabilities_.max_frames_pending = 1;
  }
}

FakeOutputSurface::FakeOutputSurface(
    scoped_ptr<WebKit::WebGraphicsContext3D> context3d,
    scoped_ptr<SoftwareOutputDevice> software_device,
    bool delegated_rendering)
    : OutputSurface(context3d.Pass(), software_device.Pass()),
      num_sent_frames_(0),
      forced_draw_to_software_device_(false),
      fake_weak_ptr_factory_(this) {
  if (delegated_rendering) {
    capabilities_.delegated_rendering = true;
    capabilities_.max_frames_pending = 1;
  }
}

FakeOutputSurface::~FakeOutputSurface() {}

void FakeOutputSurface::SwapBuffers(CompositorFrame* frame) {
  if (frame->software_frame_data || frame->delegated_frame_data ||
      !context3d()) {
    frame->AssignTo(&last_sent_frame_);
    ++num_sent_frames_;
    PostSwapBuffersComplete();
    DidSwapBuffers();
  } else {
    OutputSurface::SwapBuffers(frame);
    frame->AssignTo(&last_sent_frame_);
    ++num_sent_frames_;
  }
}

void FakeOutputSurface::SetNeedsBeginFrame(bool enable) {
  needs_begin_frame_ = enable;
  OutputSurface::SetNeedsBeginFrame(enable);

  // If there is not BeginFrame emulation from the FrameRateController,
  // then we just post a BeginFrame to emulate it as part of the test.
  if (enable && !frame_rate_controller_) {
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE, base::Bind(&FakeOutputSurface::OnBeginFrame,
                              fake_weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(16));
  }
}

void FakeOutputSurface::OnBeginFrame() {
  OutputSurface::BeginFrame(base::TimeTicks::Now());
}


bool FakeOutputSurface::ForcedDrawToSoftwareDevice() const {
  return forced_draw_to_software_device_;
}

}  // namespace cc
