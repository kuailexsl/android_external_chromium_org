// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_WEB_DIALOG_OBSERVER_H_
#define CHROME_BROWSER_UI_WEBUI_WEB_DIALOG_OBSERVER_H_
#pragma once

namespace content {
class RenderViewHost;
class WebUI;
}

// Implement this class to receive notifications.
class WebDialogObserver {
 public:
  // Invoked when a web dialog has been shown.
  // |webui| is the WebUI with which the dialog is associated.
  // |render_view_host| is the RenderViewHost for the shown dialog.
  virtual void OnDialogShown(content::WebUI* webui,
                             content::RenderViewHost* render_view_host) = 0;

 protected:
  virtual ~WebDialogObserver() {}
};

#endif  // CHROME_BROWSER_UI_WEBUI_WEB_DIALOG_OBSERVER_H_
