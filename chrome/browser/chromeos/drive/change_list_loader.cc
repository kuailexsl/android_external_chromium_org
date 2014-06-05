// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/change_list_loader.h"

#include <set>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/drive/change_list_loader_observer.h"
#include "chrome/browser/chromeos/drive/change_list_processor.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/drive/job_scheduler.h"
#include "chrome/browser/chromeos/drive/resource_metadata.h"
#include "chrome/browser/drive/event_logger.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/gdata_wapi_parser.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace drive {
namespace internal {

typedef base::Callback<void(FileError, ScopedVector<ChangeList>)>
    FeedFetcherCallback;

class ChangeListLoader::FeedFetcher {
 public:
  virtual ~FeedFetcher() {}
  virtual void Run(const FeedFetcherCallback& callback) = 0;
};

namespace {

// Fetches all the (currently available) resource entries from the server.
class FullFeedFetcher : public ChangeListLoader::FeedFetcher {
 public:
  explicit FullFeedFetcher(JobScheduler* scheduler)
      : scheduler_(scheduler),
        weak_ptr_factory_(this) {
  }

  virtual ~FullFeedFetcher() {
  }

  virtual void Run(const FeedFetcherCallback& callback) OVERRIDE {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    DCHECK(!callback.is_null());

    // Remember the time stamp for usage stats.
    start_time_ = base::TimeTicks::Now();

    // This is full resource list fetch.
    scheduler_->GetAllResourceList(
        base::Bind(&FullFeedFetcher::OnFileListFetched,
                   weak_ptr_factory_.GetWeakPtr(), callback));
  }

 private:
  void OnFileListFetched(
      const FeedFetcherCallback& callback,
      google_apis::GDataErrorCode status,
      scoped_ptr<google_apis::ResourceList> resource_list) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    DCHECK(!callback.is_null());

    FileError error = GDataToFileError(status);
    if (error != FILE_ERROR_OK) {
      callback.Run(error, ScopedVector<ChangeList>());
      return;
    }

    DCHECK(resource_list);
    change_lists_.push_back(new ChangeList(*resource_list));

    GURL next_url;
    if (resource_list->GetNextFeedURL(&next_url) && !next_url.is_empty()) {
      // There is the remaining result so fetch it.
      scheduler_->GetRemainingFileList(
          next_url,
          base::Bind(&FullFeedFetcher::OnFileListFetched,
                     weak_ptr_factory_.GetWeakPtr(), callback));
      return;
    }

    UMA_HISTOGRAM_LONG_TIMES("Drive.FullFeedLoadTime",
                             base::TimeTicks::Now() - start_time_);

    // Note: The fetcher is managed by ChangeListLoader, and the instance
    // will be deleted in the callback. Do not touch the fields after this
    // invocation.
    callback.Run(FILE_ERROR_OK, change_lists_.Pass());
  }

  JobScheduler* scheduler_;
  ScopedVector<ChangeList> change_lists_;
  base::TimeTicks start_time_;
  base::WeakPtrFactory<FullFeedFetcher> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(FullFeedFetcher);
};

// Fetches the delta changes since |start_change_id|.
class DeltaFeedFetcher : public ChangeListLoader::FeedFetcher {
 public:
  DeltaFeedFetcher(JobScheduler* scheduler, int64 start_change_id)
      : scheduler_(scheduler),
        start_change_id_(start_change_id),
        weak_ptr_factory_(this) {
  }

  virtual ~DeltaFeedFetcher() {
  }

  virtual void Run(const FeedFetcherCallback& callback) OVERRIDE {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    DCHECK(!callback.is_null());

    scheduler_->GetChangeList(
        start_change_id_,
        base::Bind(&DeltaFeedFetcher::OnChangeListFetched,
                   weak_ptr_factory_.GetWeakPtr(), callback));
  }

 private:
  void OnChangeListFetched(
      const FeedFetcherCallback& callback,
      google_apis::GDataErrorCode status,
      scoped_ptr<google_apis::ResourceList> resource_list) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    DCHECK(!callback.is_null());

    FileError error = GDataToFileError(status);
    if (error != FILE_ERROR_OK) {
      callback.Run(error, ScopedVector<ChangeList>());
      return;
    }

    DCHECK(resource_list);
    change_lists_.push_back(new ChangeList(*resource_list));

    GURL next_url;
    if (resource_list->GetNextFeedURL(&next_url) && !next_url.is_empty()) {
      // There is the remaining result so fetch it.
      scheduler_->GetRemainingChangeList(
          next_url,
          base::Bind(&DeltaFeedFetcher::OnChangeListFetched,
                     weak_ptr_factory_.GetWeakPtr(), callback));
      return;
    }

    // Note: The fetcher is managed by ChangeListLoader, and the instance
    // will be deleted in the callback. Do not touch the fields after this
    // invocation.
    callback.Run(FILE_ERROR_OK, change_lists_.Pass());
  }

  JobScheduler* scheduler_;
  int64 start_change_id_;
  ScopedVector<ChangeList> change_lists_;
  base::WeakPtrFactory<DeltaFeedFetcher> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DeltaFeedFetcher);
};

}  // namespace

LoaderController::LoaderController()
    : lock_count_(0),
      weak_ptr_factory_(this) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

LoaderController::~LoaderController() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

scoped_ptr<base::ScopedClosureRunner> LoaderController::GetLock() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  ++lock_count_;
  return make_scoped_ptr(new base::ScopedClosureRunner(
      base::Bind(&LoaderController::Unlock,
                 weak_ptr_factory_.GetWeakPtr())));
}

void LoaderController::ScheduleRun(const base::Closure& task) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!task.is_null());

  if (lock_count_ > 0) {
    pending_tasks_.push_back(task);
  } else {
    task.Run();
  }
}

void LoaderController::Unlock() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK_LT(0, lock_count_);

  if (--lock_count_ > 0)
    return;

  std::vector<base::Closure> tasks;
  tasks.swap(pending_tasks_);
  for (size_t i = 0; i < tasks.size(); ++i)
    tasks[i].Run();
}

AboutResourceLoader::AboutResourceLoader(JobScheduler* scheduler)
    : scheduler_(scheduler),
      weak_ptr_factory_(this) {
}

AboutResourceLoader::~AboutResourceLoader() {}

void AboutResourceLoader::GetAboutResource(
    const google_apis::AboutResourceCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  if (cached_about_resource_) {
    base::MessageLoopProxy::current()->PostTask(
        FROM_HERE,
        base::Bind(
            callback,
            google_apis::HTTP_NO_CONTENT,
            base::Passed(scoped_ptr<google_apis::AboutResource>(
                new google_apis::AboutResource(*cached_about_resource_)))));
  } else {
    UpdateAboutResource(callback);
  }
}

void AboutResourceLoader::UpdateAboutResource(
    const google_apis::AboutResourceCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  scheduler_->GetAboutResource(
      base::Bind(&AboutResourceLoader::UpdateAboutResourceAfterGetAbout,
                 weak_ptr_factory_.GetWeakPtr(),
                 callback));
}

void AboutResourceLoader::UpdateAboutResourceAfterGetAbout(
    const google_apis::AboutResourceCallback& callback,
    google_apis::GDataErrorCode status,
    scoped_ptr<google_apis::AboutResource> about_resource) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());
  FileError error = GDataToFileError(status);

  if (error == FILE_ERROR_OK) {
    if (cached_about_resource_ &&
        cached_about_resource_->largest_change_id() >
        about_resource->largest_change_id()) {
      LOG(WARNING) << "Local cached about resource is fresher than server, "
                   << "local = " << cached_about_resource_->largest_change_id()
                   << ", server = " << about_resource->largest_change_id();
    }

    cached_about_resource_.reset(
        new google_apis::AboutResource(*about_resource));
  }

  callback.Run(status, about_resource.Pass());
}

ChangeListLoader::ChangeListLoader(
    EventLogger* logger,
    base::SequencedTaskRunner* blocking_task_runner,
    ResourceMetadata* resource_metadata,
    JobScheduler* scheduler,
    AboutResourceLoader* about_resource_loader,
    LoaderController* loader_controller)
    : logger_(logger),
      blocking_task_runner_(blocking_task_runner),
      resource_metadata_(resource_metadata),
      scheduler_(scheduler),
      about_resource_loader_(about_resource_loader),
      loader_controller_(loader_controller),
      loaded_(false),
      weak_ptr_factory_(this) {
}

ChangeListLoader::~ChangeListLoader() {
}

bool ChangeListLoader::IsRefreshing() const {
  // Callback for change list loading is stored in pending_load_callback_.
  // It is non-empty if and only if there is an in-flight loading operation.
  return !pending_load_callback_.empty();
}

void ChangeListLoader::AddObserver(ChangeListLoaderObserver* observer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  observers_.AddObserver(observer);
}

void ChangeListLoader::RemoveObserver(ChangeListLoaderObserver* observer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  observers_.RemoveObserver(observer);
}

void ChangeListLoader::CheckForUpdates(const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  if (IsRefreshing()) {
    // There is in-flight loading. So keep the callback here, and check for
    // updates when the in-flight loading is completed.
    pending_update_check_callback_ = callback;
    return;
  }

  if (loaded_) {
    // We only start to check for updates iff the load is done.
    // I.e., we ignore checking updates if not loaded to avoid starting the
    // load without user's explicit interaction (such as opening Drive).
    logger_->Log(logging::LOG_INFO, "Checking for updates");
    Load(callback);
  }
}

void ChangeListLoader::LoadIfNeeded(const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  // If the metadata is not yet loaded, start loading.
  if (!loaded_)
    Load(callback);
}

void ChangeListLoader::Load(const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  // Check if this is the first time this ChangeListLoader do loading.
  // Note: IsRefreshing() depends on pending_load_callback_ so check in advance.
  const bool is_initial_load = (!loaded_ && !IsRefreshing());

  // Register the callback function to be called when it is loaded.
  pending_load_callback_.push_back(callback);

  // If loading task is already running, do nothing.
  if (pending_load_callback_.size() > 1)
    return;

  // Check the current status of local metadata, and start loading if needed.
  int64* local_changestamp = new int64(0);
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_,
      FROM_HERE,
      base::Bind(&ResourceMetadata::GetLargestChangestamp,
                 base::Unretained(resource_metadata_),
                 local_changestamp),
      base::Bind(&ChangeListLoader::LoadAfterGetLargestChangestamp,
                 weak_ptr_factory_.GetWeakPtr(),
                 is_initial_load,
                 base::Owned(local_changestamp)));
}

void ChangeListLoader::LoadAfterGetLargestChangestamp(
    bool is_initial_load,
    const int64* local_changestamp,
    FileError error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (error != FILE_ERROR_OK) {
    OnChangeListLoadComplete(error);
    return;
  }

  if (is_initial_load && *local_changestamp > 0) {
    // The local data is usable. Flush callbacks to tell loading was successful.
    OnChangeListLoadComplete(FILE_ERROR_OK);

    // Continues to load from server in background.
    // Put dummy callbacks to indicate that fetching is still continuing.
    pending_load_callback_.push_back(
        base::Bind(&util::EmptyFileOperationCallback));
  }

  about_resource_loader_->UpdateAboutResource(
      base::Bind(&ChangeListLoader::LoadAfterGetAboutResource,
                 weak_ptr_factory_.GetWeakPtr(),
                 *local_changestamp));
}

void ChangeListLoader::LoadAfterGetAboutResource(
    int64 local_changestamp,
    google_apis::GDataErrorCode status,
    scoped_ptr<google_apis::AboutResource> about_resource) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  FileError error = GDataToFileError(status);
  if (error != FILE_ERROR_OK) {
    OnChangeListLoadComplete(error);
    return;
  }

  DCHECK(about_resource);

  int64 remote_changestamp = about_resource->largest_change_id();
  int64 start_changestamp = local_changestamp > 0 ? local_changestamp + 1 : 0;
  if (local_changestamp >= remote_changestamp) {
    if (local_changestamp > remote_changestamp) {
      LOG(WARNING) << "Local resource metadata is fresher than server, "
                   << "local = " << local_changestamp
                   << ", server = " << remote_changestamp;
    }

    // No changes detected, tell the client that the loading was successful.
    OnChangeListLoadComplete(FILE_ERROR_OK);
  } else {
    // Start loading the change list.
    LoadChangeListFromServer(start_changestamp);
  }
}

void ChangeListLoader::OnChangeListLoadComplete(FileError error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!loaded_ && error == FILE_ERROR_OK) {
    loaded_ = true;
    FOR_EACH_OBSERVER(ChangeListLoaderObserver,
                      observers_,
                      OnInitialLoadComplete());
  }

  for (size_t i = 0; i < pending_load_callback_.size(); ++i) {
    base::MessageLoopProxy::current()->PostTask(
        FROM_HERE,
        base::Bind(pending_load_callback_[i], error));
  }
  pending_load_callback_.clear();

  // If there is pending update check, try to load the change from the server
  // again, because there may exist an update during the completed loading.
  if (!pending_update_check_callback_.is_null()) {
    Load(base::ResetAndReturn(&pending_update_check_callback_));
  }
}

void ChangeListLoader::LoadChangeListFromServer(int64 start_changestamp) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!change_feed_fetcher_);
  DCHECK(about_resource_loader_->cached_about_resource());

  bool is_delta_update = start_changestamp != 0;

  // Set up feed fetcher.
  if (is_delta_update) {
    change_feed_fetcher_.reset(
        new DeltaFeedFetcher(scheduler_, start_changestamp));
  } else {
    change_feed_fetcher_.reset(new FullFeedFetcher(scheduler_));
  }

  // Make a copy of cached_about_resource_ to remember at which changestamp we
  // are fetching change list.
  change_feed_fetcher_->Run(
      base::Bind(&ChangeListLoader::LoadChangeListFromServerAfterLoadChangeList,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Passed(make_scoped_ptr(new google_apis::AboutResource(
                     *about_resource_loader_->cached_about_resource()))),
                 is_delta_update));
}

void ChangeListLoader::LoadChangeListFromServerAfterLoadChangeList(
    scoped_ptr<google_apis::AboutResource> about_resource,
    bool is_delta_update,
    FileError error,
    ScopedVector<ChangeList> change_lists) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(about_resource);

  // Delete the fetcher first.
  change_feed_fetcher_.reset();

  if (error != FILE_ERROR_OK) {
    OnChangeListLoadComplete(error);
    return;
  }

  ChangeListProcessor* change_list_processor =
      new ChangeListProcessor(resource_metadata_);
  // Don't send directory content change notification while performing
  // the initial content retrieval.
  const bool should_notify_changed_directories = is_delta_update;

  logger_->Log(logging::LOG_INFO,
               "Apply change lists (is delta: %d)",
               is_delta_update);
  loader_controller_->ScheduleRun(base::Bind(
      base::IgnoreResult(
          &base::PostTaskAndReplyWithResult<FileError, FileError>),
      blocking_task_runner_,
      FROM_HERE,
      base::Bind(&ChangeListProcessor::Apply,
                 base::Unretained(change_list_processor),
                 base::Passed(&about_resource),
                 base::Passed(&change_lists),
                 is_delta_update),
      base::Bind(&ChangeListLoader::LoadChangeListFromServerAfterUpdate,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Owned(change_list_processor),
                 should_notify_changed_directories,
                 base::Time::Now())));
}

void ChangeListLoader::LoadChangeListFromServerAfterUpdate(
    ChangeListProcessor* change_list_processor,
    bool should_notify_changed_directories,
    const base::Time& start_time,
    FileError error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  const base::TimeDelta elapsed = base::Time::Now() - start_time;
  logger_->Log(logging::LOG_INFO,
               "Change lists applied (elapsed time: %sms)",
               base::Int64ToString(elapsed.InMilliseconds()).c_str());

  if (should_notify_changed_directories) {
    for (std::set<base::FilePath>::iterator dir_iter =
            change_list_processor->changed_dirs().begin();
        dir_iter != change_list_processor->changed_dirs().end();
        ++dir_iter) {
      FOR_EACH_OBSERVER(ChangeListLoaderObserver, observers_,
                        OnDirectoryChanged(*dir_iter));
    }
  }

  OnChangeListLoadComplete(error);

  FOR_EACH_OBSERVER(ChangeListLoaderObserver,
                    observers_,
                    OnLoadFromServerComplete());
}

}  // namespace internal
}  // namespace drive
