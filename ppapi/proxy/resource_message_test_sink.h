// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_RESOURCE_MESSAGE_TEST_SINK_H_
#define PPAPI_PROXY_RESOURCE_MESSAGE_TEST_SINK_H_

#include "ipc/ipc_listener.h"
#include "ipc/ipc_test_sink.h"
#include "ppapi/c/pp_stdint.h"

namespace ppapi {
namespace proxy {

class ResourceMessageCallParams;
class ResourceMessageReplyParams;
class SerializedHandle;

// Extends IPC::TestSink to add extra capabilities for searching for and
// decoding resource messages.
class ResourceMessageTestSink : public IPC::TestSink {
 public:
  ResourceMessageTestSink();
  virtual ~ResourceMessageTestSink();

  // IPC::TestSink.
  // Overridden to handle sync messages.
  virtual bool Send(IPC::Message* msg) OVERRIDE;

  // Sets the reply message that will be returned to the next sync message sent.
  // This test sink owns any reply messages passed into this method.
  void SetSyncReplyMessage(IPC::Message* reply_msg);

  // Searches the queue for the first resource call message with a nested
  // message matching the given ID. On success, returns true and populates the
  // givem params and nested message.
  bool GetFirstResourceCallMatching(
      uint32 id,
      ResourceMessageCallParams* params,
      IPC::Message* nested_msg) const;

  // Like GetFirstResourceCallMatching except for replies.
  bool GetFirstResourceReplyMatching(
      uint32 id,
      ResourceMessageReplyParams* params,
      IPC::Message* nested_msg);

  // Searches the queue for the next resource call message with a nested
  // message matching the given ID. On success, returns true and populates the
  // givem params and nested message. The first time this is called, it is
  // equivalent to GetFirstResourceCallMatching.
  bool GetNextResourceCallMatching(
      uint32 id,
      ResourceMessageCallParams* params,
      IPC::Message* nested_msg);

  // Like GetNextResourceCallMatching except for replies.
  bool GetNextResourceReplyMatching(
      uint32 id,
      ResourceMessageReplyParams* params,
      IPC::Message* nested_msg);

 private:
  scoped_ptr<IPC::Message> sync_reply_msg_;
  int next_resource_call_;
  int next_resource_reply_;
};

// This is a message handler which generates reply messages for synchronous
// resource calls. This allows unit testing of the plugin side of resources
// which send sync messages. If you want to reply to a sync message type named
// |PpapiHostMsg_X_Y| with |PpapiPluginMsg_X_YReply| then usage would be as
// follows (from within |PluginProxyTest|s):
//
// PpapiHostMsg_X_YReply my_reply;
// ResourceSyncCallHandler handler(&sink(),
//                                 PpapiHostMsg_X_Y::ID,
//                                 PP_OK,
//                                 my_reply);
// sink().AddFilter(&handler);
// // Do stuff to send a sync message ...
// // You can check handler.last_handled_msg() to ensure the correct message was
// // handled.
// sink().RemoveFilter(&handler);
class ResourceSyncCallHandler : public IPC::Listener {
 public:
  ResourceSyncCallHandler(ResourceMessageTestSink* test_sink,
                          uint32 incoming_type,
                          int32_t result,
                          const IPC::Message& reply_msg);
  virtual ~ResourceSyncCallHandler();

  // IPC::Listener.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  IPC::Message last_handled_msg() { return last_handled_msg_; }

  // Sets a handle to be appended to the ReplyParams. The pointer is owned by
  // the caller.
  void set_serialized_handle(const SerializedHandle* serialized_handle) {
    serialized_handle_ = serialized_handle;
  }

 private:
  ResourceMessageTestSink* test_sink_;
  uint32 incoming_type_;
  int32_t result_;
  const SerializedHandle* serialized_handle_;  // Non-owning pointer.
  IPC::Message reply_msg_;
  IPC::Message last_handled_msg_;
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_RESOURCE_MESSAGE_TEST_SINK_H_
