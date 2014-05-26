// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_FSP_INTERNALS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_FSP_INTERNALS_UI_H_

#include "content/public/browser/web_ui_controller.h"

namespace chromeos {

// The WebUI controller for chrome:://fsp-internals, that is used for diagnosing
// issues of file systems provided via chrome.fileSystemProvider API.
class FSPInternalsUI : public content::WebUIController {
 public:
  explicit FSPInternalsUI(content::WebUI* web_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(FSPInternalsUI);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_FSP_INTERNALS_UI_H_
