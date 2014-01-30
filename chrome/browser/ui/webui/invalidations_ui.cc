// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/invalidations_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "grit/invalidations_resources.h"

content::WebUIDataSource* CreateInvalidationsHTMLSource() {
  // This is done once per opening of the page
  // This method does not fire when refreshing the page
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIInvalidationsHost);
  source->AddResourcePath("about_invalidations.js", IDR_ABOUT_INVALIDATIONS_JS);
  source->SetDefaultResource(IDR_ABOUT_INVALIDATIONS_HTML);
  return source;
}

InvalidationsUI::InvalidationsUI(content::WebUI* web_ui)
    : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  if (profile) {
    content::WebUIDataSource::Add(profile, CreateInvalidationsHTMLSource());
  }
}

InvalidationsUI::~InvalidationsUI() { }
