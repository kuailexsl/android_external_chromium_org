// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_WEBREQUEST_API_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_WEBREQUEST_API_H_
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/memory/singleton.h"
#include "base/time.h"
#include "chrome/browser/extensions/extension_function.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/url_pattern_set.h"
#include "ipc/ipc_message.h"
#include "net/base/completion_callback.h"
#include "webkit/glue/resource_type.h"

class ExtensionInfoMap;
class GURL;

namespace base {
class DictionaryValue;
class ListValue;
class StringValue;
}

namespace net {
class HostPortPair;
class HttpRequestHeaders;
class HttpResponseHeaders;
class URLRequest;
}

// This class observes network events and routes them to the appropriate
// extensions listening to those events. All methods must be called on the IO
// thread unless otherwise specified.
class ExtensionWebRequestEventRouter {
 public:
  enum EventTypes {
    kInvalidEvent = 0,
    kOnBeforeRequest = 1 << 0,
    kOnBeforeSendHeaders = 1 << 1,
    kOnRequestSent = 1 << 2,
    kOnBeforeRedirect = 1 << 3,
    kOnResponseStarted = 1 << 4,
    kOnErrorOccurred = 1 << 5,
    kOnCompleted = 1 << 6,
  };

  // Internal representation of the webRequest.RequestFilter type, used to
  // filter what network events an extension cares about.
  struct RequestFilter {
    URLPatternSet urls;
    std::vector<ResourceType::Type> types;
    int tab_id;
    int window_id;

    RequestFilter();
    ~RequestFilter();

    // Returns false if there was an error initializing. If it is a user error,
    // an error message is provided, otherwise the error is internal (and
    // unexpected).
    bool InitFromValue(const base::DictionaryValue& value, std::string* error);
  };

  // Internal representation of the extraInfoSpec parameter on webRequest
  // events, used to specify extra information to be included with network
  // events.
  struct ExtraInfoSpec {
    enum Flags {
      REQUEST_LINE = 1<<0,
      REQUEST_HEADERS = 1<<1,
      STATUS_LINE = 1<<2,
      RESPONSE_HEADERS = 1<<3,
      BLOCKING = 1<<4,
    };

    static bool InitFromValue(const base::ListValue& value,
                              int* extra_info_spec);
  };

  // Contains an extension's response to a blocking event.
  struct EventResponse {
    // ID of the extension that sent this response.
    std::string extension_id;

    // The time that the extension was installed. Used for deciding order of
    // precedence in case multiple extensions respond with conflicting
    // decisions.
    base::Time extension_install_time;

    // Response values. These are mutually exclusive.
    bool cancel;
    GURL new_url;
    scoped_ptr<net::HttpRequestHeaders> request_headers;

    EventResponse(const std::string& extension_id,
                  const base::Time& extension_install_time);
    ~EventResponse();

    DISALLOW_COPY_AND_ASSIGN(EventResponse);
  };

  // Used in testing to allow chrome-extension URLs to be intercepted.
  static void SetAllowChromeExtensionScheme();

  static ExtensionWebRequestEventRouter* GetInstance();

  // Dispatches the OnBeforeRequest event to any extensions whose filters match
  // the given request. Returns net::ERR_IO_PENDING if an extension is
  // intercepting the request, OK otherwise.
  int OnBeforeRequest(void* profile,
                      ExtensionInfoMap* extension_info_map,
                      net::URLRequest* request,
                      net::CompletionCallback* callback,
                      GURL* new_url);

  // Dispatches the onBeforeSendHeaders event. This is fired for HTTP(s)
  // requests only, and allows modification of the outgoing request headers.
  // Returns net::ERR_IO_PENDING if an extension is intercepting the request, OK
  // otherwise.
  int OnBeforeSendHeaders(void* profile,
                          ExtensionInfoMap* extension_info_map,
                          net::URLRequest* request,
                          net::CompletionCallback* callback,
                          net::HttpRequestHeaders* headers);

  // Dispatches the onRequestSent event. This is fired for HTTP(s) requests
  // only.
  void OnRequestSent(void* profile,
                     ExtensionInfoMap* extension_info_map,
                     uint64 request_id,
                     const net::HostPortPair& socket_address,
                     const net::HttpRequestHeaders& headers);

  // Dispatches the onBeforeRedirect event. This is fired for HTTP(s) requests
  // only.
  void OnBeforeRedirect(void* profile,
                        ExtensionInfoMap* extension_info_map,
                        net::URLRequest* request,
                        const GURL& new_location);

  // Dispatches the onResponseStarted event indicating that the first bytes of
  // the response have arrived.
  void OnResponseStarted(void* profile,
                         ExtensionInfoMap* extension_info_map,
                         net::URLRequest* request);

  // Dispatches the onComplete event.
  void OnCompleted(void* profile,
                   ExtensionInfoMap* extension_info_map,
                   net::URLRequest* request);

  // Dispatches an onErrorOccurred event.
  void OnErrorOccurred(void* profile,
                      ExtensionInfoMap* extension_info_map,
                      net::URLRequest* request);

  // Notifications when objects are going away.
  void OnURLRequestDestroyed(void* profile, net::URLRequest* request);
  void OnHttpTransactionDestroyed(void* profile, uint64 request_id);

  // Called when an event listener handles a blocking event and responds.
  void OnEventHandled(
      void* profile,
      const std::string& extension_id,
      const std::string& event_name,
      const std::string& sub_event_name,
      uint64 request_id,
      EventResponse* response);

  // Adds a listener to the given event. |event_name| specifies the event being
  // listened to. |sub_event_name| is an internal event uniquely generated in
  // the extension process to correspond to the given filter and
  // extra_info_spec.
  void AddEventListener(
      void* profile,
      const std::string& extension_id,
      const std::string& event_name,
      const std::string& sub_event_name,
      const RequestFilter& filter,
      int extra_info_spec,
      base::WeakPtr<IPC::Message::Sender> ipc_sender);

  // Removes the listener for the given sub-event.
  void RemoveEventListener(
      void* profile,
      const std::string& extension_id,
      const std::string& sub_event_name);

  // Called when an incognito profile is created or destroyed.
  void OnOTRProfileCreated(void* original_profile,
                           void* otr_profile);
  void OnOTRProfileDestroyed(void* original_profile,
                             void* otr_profile);

 private:
  friend struct DefaultSingletonTraits<ExtensionWebRequestEventRouter>;
  struct EventListener;
  struct BlockedRequest;
  typedef std::map<std::string, std::set<EventListener> > ListenerMapForProfile;
  typedef std::map<void*, ListenerMapForProfile> ListenerMap;
  typedef std::map<uint64, BlockedRequest> BlockedRequestMap;
  typedef std::map<uint64, net::URLRequest*> HttpRequestMap;
  // Map of request_id -> bit vector of EventTypes already signaled
  typedef std::map<uint64, int> SignaledRequestMap;
  typedef std::map<void*, void*> CrossProfileMap;

  ExtensionWebRequestEventRouter();
  ~ExtensionWebRequestEventRouter();

  bool DispatchEvent(
      void* profile,
      net::URLRequest* request,
      const std::vector<const EventListener*>& listeners,
      const base::ListValue& args);

  // Returns a list of event listeners that care about the given event, based
  // on their filter parameters. |extra_info_spec| will contain the combined
  // set of extra_info_spec flags that every matching listener asked for.
  std::vector<const EventListener*> GetMatchingListeners(
      void* profile,
      ExtensionInfoMap* extension_info_map,
      const std::string& event_name,
      const GURL& url,
      int tab_id,
      int window_id,
      ResourceType::Type resource_type,
      int* extra_info_spec);

  // Same as above, but retrieves the filter parameters from the request.
  std::vector<const EventListener*> GetMatchingListeners(
      void* profile,
      ExtensionInfoMap* extension_info_map,
      const std::string& event_name,
      net::URLRequest* request,
      int* extra_info_spec);

  // Helper for the above functions. This is called twice: once for the profile
  // of the event, the next time for the "cross" profile (i.e. the incognito
  // profile if the event is originally for the normal profile, or vice versa).
  void GetMatchingListenersImpl(
      void* profile,
      ExtensionInfoMap* extension_info_map,
      bool crosses_incognito,
      const std::string& event_name,
      const GURL& url,
      int tab_id,
      int window_id,
      ResourceType::Type resource_type,
      int* extra_info_spec,
      std::vector<const ExtensionWebRequestEventRouter::EventListener*>*
          matching_listeners);

  // Decrements the count of event handlers blocking the given request. When the
  // count reaches 0, we stop blocking the request and proceed it using the
  // method requested by the extension with the highest precedence. Precedence
  // is decided by extension install time. If |response| is non-NULL, this
  // method assumes ownership.
  void DecrementBlockCount(uint64 request_id, EventResponse* response);

  // Sets the flag that |event_type| has been signaled for |request_id|.
  // Returns the value of the flag before setting it.
  bool GetAndSetSignaled(uint64 request_id, EventTypes event_type);

  // Clears the flag that |event_type| has been signaled for |request_id|.
  void ClearSignaled(uint64 request_id, EventTypes event_type);

  // A map for each profile that maps an event name to a set of extensions that
  // are listening to that event.
  ListenerMap listeners_;

  // A map of network requests that are waiting for at least one event handler
  // to respond.
  BlockedRequestMap blocked_requests_;

  // A map of HTTP(s) network requests. We use this to look up the URLRequest
  // from the request ID given to us for HTTP-specific events.
  HttpRequestMap http_requests_;

  // A map of request ids to a bitvector indicating which events have been
  // signaled and should not be sent again.
  SignaledRequestMap signaled_requests_;

  // A map of original profile -> corresponding incognito profile (and vice
  // versa).
  CrossProfileMap cross_profile_map_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionWebRequestEventRouter);
};

class WebRequestAddEventListener : public SyncIOThreadExtensionFunction {
 public:
  virtual bool RunImpl();
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.webRequest.addEventListener");
};

class WebRequestEventHandled : public SyncIOThreadExtensionFunction {
 public:
  virtual bool RunImpl();
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.webRequest.eventHandled");
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_WEBREQUEST_API_H_
