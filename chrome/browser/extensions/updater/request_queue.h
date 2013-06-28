// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_UPDATER_REQUEST_QUEUE_H_
#define CHROME_BROWSER_EXTENSIONS_UPDATER_REQUEST_QUEUE_H_

#include <deque>
#include <utility>

#include "base/callback.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "net/base/backoff_entry.h"

namespace extensions {

// This class keeps track of a queue of requests, and contains the logic to
// retry requests with some backoff policy. Each request has a
// net::BackoffEntry instance associated with it.
//
// The general flow when using this class would be something like this:
//   - requests are queued up by calling ScheduleRequest.
//   - when a request is ready to be executed, RequestQueue removes the
//     request from the queue, assigns it as active request, and calls
//     the callback that was passed to the constructor.
//   - (optionally) when a request has completed unsuccessfully call
//     RetryRequest to put the request back in the queue, using the
//     backoff policy and minimum backoff delay to determine when to
//     next schedule this request.
//   - call reset_active_request() to indicate that the active request has
//     been dealt with.
//   - call StartNextRequest to schedule the next pending request (if any).
template<typename T>
class RequestQueue {
 public:
  class iterator;

  RequestQueue(const net::BackoffEntry::Policy* backoff_policy,
               const base::Closure& start_request_callback);
  ~RequestQueue();

  // Returns the request that is currently being processed.
  T* active_request();

  // Returns the number of times the current request has been retried already.
  int active_request_failure_count();

  // Signals RequestQueue that processing of the current request has completed.
  scoped_ptr<T> reset_active_request();

  // Add the given request to the queue, and starts the next request if no
  // request is currently being processed.
  void ScheduleRequest(scoped_ptr<T> request);

  bool empty() const;
  size_t size() const;

  // Returns the earliest release time of all requests currently in the queue.
  base::TimeTicks NextReleaseTime() const;

  // Starts the next request, if no request is currently active. This will
  // synchronously call the start_request_callback if the release time of the
  // earliest available request is in the past, otherwise it will call that
  // callback asynchronously after enough time has passed.
  void StartNextRequest();

  // Tell RequestQueue to put the current request back in the queue, after
  // applying the backoff policy to determine when to next try this request.
  // If the policy results in a backoff delay smaller than |min_backoff_delay|,
  // that delay is used instead.
  void RetryRequest(const base::TimeDelta& min_backoff_delay);

  iterator begin();
  iterator end();

  // Change the backoff policy used by the queue.
  void set_backoff_policy(const net::BackoffEntry::Policy* backoff_policy);

 private:
  struct Request {
    Request(net::BackoffEntry* backoff_entry, T* request)
        : backoff_entry(backoff_entry), request(request) {}
    linked_ptr<net::BackoffEntry> backoff_entry;
    linked_ptr<T> request;
  };

  // Compares the release time of two pending requests.
  static bool CompareRequests(const Request& a,
                              const Request& b);

  // Pushes a request with a given backoff entry onto the queue.
  void PushImpl(scoped_ptr<T> request,
                scoped_ptr<net::BackoffEntry> backoff_entry);

  // The backoff policy used to determine backoff delays.
  const net::BackoffEntry::Policy* backoff_policy_;

  // Callback to call when a new request has become the active request.
  base::Closure start_request_callback_;

  // Priority queue of pending requests. Not using std::priority_queue since
  // the code needs to be able to iterate over all pending requests.
  std::deque<Request> pending_requests_;

  // Active request and its associated backoff entry.
  scoped_ptr<T> active_request_;
  scoped_ptr<net::BackoffEntry> active_backoff_entry_;

  // Timer to schedule calls to StartNextRequest, if the first pending request
  // hasn't passed its release time yet.
  base::Timer timer_;
};

// Iterator class that wraps a std::deque<> iterator, only giving access to the
// actual request part of each item.
template<typename T>
class RequestQueue<T>::iterator {
 public:
  iterator() {}

  T* operator*() { return it_->request.get(); }
  T* operator->() { return it_->request.get(); }
  iterator& operator++() {
    ++it_;
    return *this;
  }
  bool operator!=(const iterator& b) const {
    return it_ != b.it_;
  }

 private:
  friend class RequestQueue<T>;
  typedef std::deque<typename RequestQueue<T>::Request> Container;

  explicit iterator(const typename Container::iterator& it)
      : it_(it) {}

  typename Container::iterator it_;
};


}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_UPDATER_REQUEST_QUEUE_H_
