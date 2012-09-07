// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PRINTING_RESOURCE_H_
#define PPAPI_PROXY_PRINTING_RESOURCE_H_

#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/ppb_printing_api.h"

namespace ppapi {
namespace proxy {

class PPAPI_PROXY_EXPORT PrintingResource
    : public PluginResource,
      public NON_EXPORTED_BASE(thunk::PPB_Printing_API) {
 public:
  PrintingResource(Connection connection,
                   PP_Instance instance);
  virtual ~PrintingResource();

  // Resource overrides.
  virtual thunk::PPB_Printing_API* AsPPB_Printing_API() OVERRIDE;

  // PPB_Printing_API.
  virtual int32_t GetDefaultPrintSettings(
      PP_PrintSettings_Dev* print_settings,
      scoped_refptr<TrackedCallback> callback) OVERRIDE;

 private:
  // PluginResource override.
  virtual void OnReplyReceived(const ResourceMessageReplyParams& params,
                               const IPC::Message& msg) OVERRIDE;

  void OnPluginMsgGetDefaultPrintSettingsReply(
      const ResourceMessageReplyParams& params,
      const PP_PrintSettings_Dev& print_settings);

  PP_PrintSettings_Dev* print_settings_;

  scoped_refptr<TrackedCallback> callback_;

  DISALLOW_COPY_AND_ASSIGN(PrintingResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_PRINTING_RESOURCE_H_
