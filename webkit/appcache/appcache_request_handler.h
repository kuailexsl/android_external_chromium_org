// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_APPCACHE_APPCACHE_REQUEST_HANDLER_H_
#define WEBKIT_APPCACHE_APPCACHE_REQUEST_HANDLER_H_

#include "net/url_request/url_request.h"
#include "webkit/appcache/appcache_entry.h"
#include "webkit/appcache/appcache_host.h"
#include "webkit/glue/resource_type.h"

class URLRequest;
class URLRequestJob;

namespace appcache {

class AppCacheURLRequestJob;

// An instance is created for each URLRequest. The instance survives all
// http transactions involved in the processing of its URLRequest, and is
// given the opportunity to hijack the request along the way. Callers
// should use AppCacheHost::CreateRequestHandler to manufacture instances
// that can retrieve resources for a particular host.
class AppCacheRequestHandler : public URLRequest::UserData,
                               public AppCacheHost::Observer,
                               public AppCacheStorage::Delegate  {
 public:
  virtual ~AppCacheRequestHandler();

  // These are called on each request intercept opportunity.
  AppCacheURLRequestJob* MaybeLoadResource(URLRequest* request);
  AppCacheURLRequestJob* MaybeLoadFallbackForRedirect(URLRequest* request,
                                                      const GURL& location);
  AppCacheURLRequestJob* MaybeLoadFallbackForResponse(URLRequest* request);

  void GetExtraResponseInfo(int64* cache_id, GURL* manifest_url);

  static bool IsMainResourceType(ResourceType::Type type) {
    return ResourceType::IsFrame(type) ||
           ResourceType::IsSharedWorker(type);
  }

 private:
  friend class AppCacheHost;

  // Callers should use AppCacheHost::CreateRequestHandler.
  AppCacheRequestHandler(AppCacheHost* host, ResourceType::Type resource_type);

  // AppCacheHost::Observer override
  virtual void OnDestructionImminent(AppCacheHost* host);

  // Helpers to instruct a waiting job with what response to
  // deliver for the request we're handling.
  void DeliverAppCachedResponse(const AppCacheEntry& entry, int64 cache_id,
                                const GURL& manifest_url);
  void DeliverNetworkResponse();
  void DeliverErrorResponse();

  // Helper to retrieve a pointer to the storage object.
  AppCacheStorage* storage() const;

  bool is_main_resource() const {
    return IsMainResourceType(resource_type_);
  }

  // Main-resource loading -------------------------------------
  // Frame and SharedWorker main resources are handled here.

  void MaybeLoadMainResource(URLRequest* request);

  // AppCacheStorage::Delegate methods
  virtual void OnMainResponseFound(
      const GURL& url, const AppCacheEntry& entry,
      const AppCacheEntry& fallback_entry,
      int64 cache_id, const GURL& mainfest_url,
      bool was_blocked_by_policy);

  // Sub-resource loading -------------------------------------
  // Dedicated worker and all manner of sub-resources are handled here.

  void MaybeLoadSubResource(URLRequest* request);
  void ContinueMaybeLoadSubResource();

  // AppCacheHost::Observer override
  virtual void OnCacheSelectionComplete(AppCacheHost* host);

  // Data members -----------------------------------------------

  // What host we're servicing a request for.
  AppCacheHost* host_;

  // Frame vs subresource vs sharedworker loads are somewhat different.
  ResourceType::Type resource_type_;

  // Subresource requests wait until after cache selection completes.
  bool is_waiting_for_cache_selection_;

  // Info about the type of response we found for delivery.
  // These are relevant for both main and subresource requests.
  AppCacheEntry found_entry_;
  AppCacheEntry found_fallback_entry_;
  int64 found_cache_id_;
  GURL found_manifest_url_;
  bool found_network_namespace_;

  // The job we use to deliver a response.
  scoped_refptr<AppCacheURLRequestJob> job_;

  friend class AppCacheRequestHandlerTest;
  DISALLOW_COPY_AND_ASSIGN(AppCacheRequestHandler);
};

}  // namespace appcache

#endif  // WEBKIT_APPCACHE_APPCACHE_REQUEST_HANDLER_H_

