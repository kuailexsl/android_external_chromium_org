// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/zoom/zoom_controller.h"

#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tab_contents/tab_contents.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_zoom.h"
#include "grit/theme_resources.h"

ZoomController::ZoomController(TabContents* tab_contents)
    : content::WebContentsObserver(tab_contents->web_contents()),
      zoom_percent_(100),
      tab_contents_(tab_contents),
      observer_(NULL) {
  default_zoom_level_.Init(prefs::kDefaultZoomLevel,
                           tab_contents->profile()->GetPrefs(), this);
  registrar_.Add(this, content::NOTIFICATION_ZOOM_LEVEL_CHANGED,
                 content::NotificationService::AllBrowserContextsAndSources());

  UpdateState(false);
}

ZoomController::~ZoomController() {
  default_zoom_level_.Destroy();
  registrar_.RemoveAll();
}

bool ZoomController::IsAtDefaultZoom() const {
  return content::ZoomValuesEqual(tab_contents_->web_contents()->GetZoomLevel(),
                                  default_zoom_level_.GetValue());
}

int ZoomController::GetResourceForZoomLevel() const {
  DCHECK(!IsAtDefaultZoom());
  double zoom = tab_contents_->web_contents()->GetZoomLevel();
  return zoom > default_zoom_level_.GetValue() ? IDR_ZOOM_PLUS : IDR_ZOOM_MINUS;
}

void ZoomController::DidNavigateMainFrame(
    const content::LoadCommittedDetails& details,
    const content::FrameNavigateParams& params) {
  // If the main frame's content has changed, the new page may have a different
  // zoom level from the old one.
  UpdateState(false);
}

void ZoomController::Observe(int type,
                             const content::NotificationSource& source,
                             const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_PREF_CHANGED: {
      std::string* pref_name = content::Details<std::string>(details).ptr();
      DCHECK(pref_name && *pref_name == prefs::kDefaultZoomLevel);
      UpdateState(false);
      break;
    }
    case content::NOTIFICATION_ZOOM_LEVEL_CHANGED:
      UpdateState(!content::Details<std::string>(details)->empty());
      break;
    default:
      NOTREACHED();
  }
}

void ZoomController::UpdateState(bool can_show_bubble) {
  bool dummy;
  zoom_percent_ = tab_contents_->web_contents()->GetZoomPercent(&dummy, &dummy);

  if (observer_)
    observer_->OnZoomChanged(tab_contents_, can_show_bubble);
}
