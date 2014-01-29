// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/detachable_resource_handler.h"

#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/time/time.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_status.h"

namespace {

// This matches the maximum allocation size of AsyncResourceHandler.
const int kReadBufSize = 32 * 1024;

// Enum type for <a ping> result histograms. Only add new values to the end.
enum UMAPingResultType {
  // The ping request completed successfully.
  UMA_PING_RESULT_TYPE_SUCCESS = 0,
  // The ping request received a response, but did not consume the entire body.
  UMA_PING_RESULT_TYPE_RESPONSE_STARTED = 1,
  // The ping request was canceled due to the internal timeout.
  UMA_PING_RESULT_TYPE_TIMEDOUT = 2,
  // The ping request was canceled for some other reason.
  UMA_PING_RESULT_TYPE_CANCELED = 3,
  // The ping request failed for some reason.
  UMA_PING_RESULT_TYPE_FAILED = 4,
  // The ping request was deleted before OnResponseCompleted.
  UMA_PING_RESULT_TYPE_UNCOMPLETED = 5,

  UMA_PING_RESULT_TYPE_MAX,
};

}  // namespace

namespace content {

DetachableResourceHandler::DetachableResourceHandler(
    net::URLRequest* request,
    base::TimeDelta cancel_delay,
    scoped_ptr<ResourceHandler> next_handler)
    : ResourceHandler(request),
      next_handler_(next_handler.Pass()),
      cancel_delay_(cancel_delay),
      is_deferred_(false),
      is_finished_(false),
      timed_out_(false),
      response_started_(false),
      status_(net::URLRequestStatus::IO_PENDING) {
  GetRequestInfo()->set_detachable_handler(this);
}

DetachableResourceHandler::~DetachableResourceHandler() {
  // Cleanup back-pointer stored on the request info.
  GetRequestInfo()->set_detachable_handler(NULL);

  // Record the status of <a ping> requests.
  // http://crbug.com/302816
  if (GetRequestInfo()->GetResourceType() == ResourceType::PING) {
    UMAPingResultType result_type = UMA_PING_RESULT_TYPE_MAX;

    if (status_ == net::URLRequestStatus::SUCCESS) {
      result_type = UMA_PING_RESULT_TYPE_SUCCESS;
    } else if (response_started_) {
      // However the request ended, bucket this under RESPONSE_STARTED because
      // OnResponseStarted was received. Note: OnResponseCompleted is also sent
      // when a request is canceled before completion, so it is possible to
      // receive OnResponseCompleted without OnResponseStarted.
      result_type = UMA_PING_RESULT_TYPE_RESPONSE_STARTED;
    } else if (status_ == net::URLRequestStatus::IO_PENDING) {
      // The request was deleted without OnResponseCompleted and before any
      // response was received.
      result_type = UMA_PING_RESULT_TYPE_UNCOMPLETED;
    } else if (status_ == net::URLRequestStatus::CANCELED) {
      if (timed_out_) {
        result_type = UMA_PING_RESULT_TYPE_TIMEDOUT;
      } else {
        result_type = UMA_PING_RESULT_TYPE_CANCELED;
      }
    } else if (status_ == net::URLRequestStatus::FAILED) {
      result_type = UMA_PING_RESULT_TYPE_FAILED;
    }

    if (result_type < UMA_PING_RESULT_TYPE_MAX) {
      UMA_HISTOGRAM_ENUMERATION("Net.Ping_Result", result_type,
                                UMA_PING_RESULT_TYPE_MAX);
    } else {
      NOTREACHED();
    }
  }
}

void DetachableResourceHandler::Detach() {
  if (is_detached())
    return;

  if (!is_finished_) {
    // Simulate a cancel on the next handler before destroying it.
    net::URLRequestStatus status(net::URLRequestStatus::CANCELED,
                                 net::ERR_ABORTED);
    bool defer_ignored = false;
    next_handler_->OnResponseCompleted(GetRequestID(), status, std::string(),
                                       &defer_ignored);
    DCHECK(!defer_ignored);
    // If |next_handler_| were to defer its shutdown in OnResponseCompleted,
    // this would destroy it anyway. Fortunately, AsyncResourceHandler never
    // does this anyway, so DCHECK it. BufferedResourceHandler and RVH shutdown
    // already ignore deferred ResourceHandler shutdown, but
    // DetachableResourceHandler and the detach-on-renderer-cancel logic
    // introduces a case where this occurs when the renderer cancels a resource.
  }
  // A OnWillRead / OnReadCompleted pair may still be in progress, but
  // OnWillRead passes back a scoped_refptr, so downstream handler's buffer will
  // survive long enough to complete that read. From there, future reads will
  // drain into |read_buffer_|. (If |next_handler_| is an AsyncResourceHandler,
  // the net::IOBuffer takes a reference to the ResourceBuffer which owns the
  // shared memory.)
  next_handler_.reset();

  // Time the request out if it takes too long.
  detached_timer_.reset(new base::OneShotTimer<DetachableResourceHandler>());
  detached_timer_->Start(
      FROM_HERE, cancel_delay_, this, &DetachableResourceHandler::TimedOut);

  // Resume if necessary. The request may have been deferred, say, waiting on a
  // full buffer in AsyncResourceHandler. Now that it has been detached, resume
  // and drain it.
  if (is_deferred_)
    Resume();
}

void DetachableResourceHandler::SetController(ResourceController* controller) {
  ResourceHandler::SetController(controller);

  // Intercept the ResourceController for downstream handlers to keep track of
  // whether the request is deferred.
  if (next_handler_)
    next_handler_->SetController(this);
}

bool DetachableResourceHandler::OnUploadProgress(int request_id,
                                                 uint64 position,
                                                 uint64 size) {
  if (!next_handler_)
    return true;

  return next_handler_->OnUploadProgress(request_id, position, size);
}

bool DetachableResourceHandler::OnRequestRedirected(int request_id,
                                                    const GURL& url,
                                                    ResourceResponse* response,
                                                    bool* defer) {
  DCHECK(!is_deferred_);

  if (!next_handler_)
    return true;

  bool ret = next_handler_->OnRequestRedirected(request_id, url, response,
                                                &is_deferred_);
  *defer = is_deferred_;
  return ret;
}

bool DetachableResourceHandler::OnResponseStarted(int request_id,
                                                  ResourceResponse* response,
                                                  bool* defer) {
  DCHECK(!is_deferred_);
  DCHECK(!response_started_);
  response_started_ = true;

  // Record how long it takes for <a ping> to respond.
  // http://crbug.com/302816
  if (GetRequestInfo()->GetResourceType() == ResourceType::PING) {
    UMA_HISTOGRAM_MEDIUM_TIMES("Net.Ping_ResponseStartedTime",
                               time_since_start_.Elapsed());
  }

  if (!next_handler_)
    return true;

  bool ret =
      next_handler_->OnResponseStarted(request_id, response, &is_deferred_);
  *defer = is_deferred_;
  return ret;
}

bool DetachableResourceHandler::OnWillStart(int request_id, const GURL& url,
                                            bool* defer) {
  DCHECK(!is_deferred_);

  if (!next_handler_)
    return true;

  bool ret = next_handler_->OnWillStart(request_id, url, &is_deferred_);
  *defer = is_deferred_;
  return ret;
}

bool DetachableResourceHandler::OnBeforeNetworkStart(int request_id,
                                                     const GURL& url,
                                                     bool* defer) {
  DCHECK(!is_deferred_);

  if (!next_handler_)
    return true;

  bool ret =
      next_handler_->OnBeforeNetworkStart(request_id, url, &is_deferred_);
  *defer = is_deferred_;
  return ret;
}

bool DetachableResourceHandler::OnWillRead(int request_id,
                                           scoped_refptr<net::IOBuffer>* buf,
                                           int* buf_size,
                                           int min_size) {
  if (!next_handler_) {
    DCHECK_EQ(-1, min_size);
    if (!read_buffer_)
      read_buffer_ = new net::IOBuffer(kReadBufSize);
    *buf = read_buffer_;
    *buf_size = kReadBufSize;
    return true;
  }

  return next_handler_->OnWillRead(request_id, buf, buf_size, min_size);
}

bool DetachableResourceHandler::OnReadCompleted(int request_id, int bytes_read,
                                                bool* defer) {
  DCHECK(!is_deferred_);

  if (!next_handler_)
    return true;

  bool ret =
      next_handler_->OnReadCompleted(request_id, bytes_read, &is_deferred_);
  *defer = is_deferred_;
  return ret;
}

void DetachableResourceHandler::OnResponseCompleted(
    int request_id,
    const net::URLRequestStatus& status,
    const std::string& security_info,
    bool* defer) {
  // No DCHECK(!is_deferred_) as the request may have been cancelled while
  // deferred.

  status_ = status.status();
  DCHECK_NE(net::URLRequestStatus::IO_PENDING, status_);

  if (!next_handler_)
    return;

  is_finished_ = true;

  next_handler_->OnResponseCompleted(request_id, status, security_info,
                                     &is_deferred_);
  *defer = is_deferred_;
}

void DetachableResourceHandler::OnDataDownloaded(int request_id,
                                                 int bytes_downloaded) {
  if (!next_handler_)
    return;

  next_handler_->OnDataDownloaded(request_id, bytes_downloaded);
}

void DetachableResourceHandler::Resume() {
  DCHECK(is_deferred_);
  is_deferred_ = false;
  controller()->Resume();
}

void DetachableResourceHandler::Cancel() {
  controller()->Cancel();
}

void DetachableResourceHandler::CancelAndIgnore() {
  controller()->CancelAndIgnore();
}

void DetachableResourceHandler::CancelWithError(int error_code) {
  controller()->CancelWithError(error_code);
}

void DetachableResourceHandler::TimedOut() {
  timed_out_ = true;
  controller()->Cancel();
}

}  // namespace content
