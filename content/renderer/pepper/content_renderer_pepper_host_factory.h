// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_CONTENT_RENDERER_PEPPER_HOST_FACTORY_H_
#define CONTENT_RENDERER_PEPPER_CONTENT_RENDERER_PEPPER_HOST_FACTORY_H_

#include "base/compiler_specific.h"
#include "ppapi/host/host_factory.h"

class RenderViewImpl;

namespace content {

class ContentRendererPepperHostFactory : public ppapi::host::HostFactory {
 public:
  explicit ContentRendererPepperHostFactory(RenderViewImpl* render_view);
  virtual ~ContentRendererPepperHostFactory();

  virtual scoped_ptr<ppapi::host::ResourceHost> CreateResourceHost(
      ppapi::host::PpapiHost* host,
      const ppapi::proxy::ResourceMessageCallParams& params,
      PP_Instance instance,
      const IPC::Message& message) OVERRIDE;

 private:
  RenderViewImpl* render_view_;

  DISALLOW_COPY_AND_ASSIGN(ContentRendererPepperHostFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_CONTENT_RENDERER_PEPPER_HOST_FACTORY_H_
