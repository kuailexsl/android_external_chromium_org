// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/context_menu_content_type_panel.h"

ContextMenuContentTypePanel::ContextMenuContentTypePanel(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params)
    : ContextMenuContentType(render_frame_host, params, false) {
}

ContextMenuContentTypePanel::~ContextMenuContentTypePanel() {
}

bool ContextMenuContentTypePanel::SupportsGroup(int group) {
  switch (group) {
    case ITEM_GROUP_LINK:
      // Checking link should take precedence before checking selection since on
      // Mac right-clicking a link will also make it selected.
      return params().unfiltered_link_url.is_valid();
    case ITEM_GROUP_EDITABLE:
    case ITEM_GROUP_COPY:
      return ContextMenuContentType::SupportsGroup(group);
    case ITEM_GROUP_CURRENT_EXTENSION:
      return true;
    default:
      return false;
  }
}
