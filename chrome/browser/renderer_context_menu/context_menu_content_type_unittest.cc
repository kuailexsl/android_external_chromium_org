// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/context_menu_content_type.h"

#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"


using extensions::MenuItem;

class ContextMenuContentTypeTest : public ChromeRenderViewHostTestHarness {
 public:
  static ContextMenuContentType* Create(
      content::RenderFrameHost* render_frame_host,
      content::ContextMenuParams& params) {
    return new ContextMenuContentType(render_frame_host, params, true);
  }
};

// Generates a ContextMenuParams that matches the specified contexts.
content::ContextMenuParams CreateParams(int contexts) {
  content::ContextMenuParams rv;
  rv.is_editable = false;
  rv.media_type = blink::WebContextMenuData::MediaTypeNone;
  rv.page_url = GURL("http://test.page/");

  static const base::string16 selected_text = base::ASCIIToUTF16("sel");
  if (contexts & MenuItem::SELECTION)
    rv.selection_text = selected_text;

  if (contexts & MenuItem::LINK) {
    rv.link_url = GURL("http://test.link/");
    rv.unfiltered_link_url = GURL("http://test.link/");
  }

  if (contexts & MenuItem::EDITABLE)
    rv.is_editable = true;

  if (contexts & MenuItem::IMAGE) {
    rv.src_url = GURL("http://test.image/");
    rv.media_type = blink::WebContextMenuData::MediaTypeImage;
  }

  if (contexts & MenuItem::VIDEO) {
    rv.src_url = GURL("http://test.video/");
    rv.media_type = blink::WebContextMenuData::MediaTypeVideo;
  }

  if (contexts & MenuItem::AUDIO) {
    rv.src_url = GURL("http://test.audio/");
    rv.media_type = blink::WebContextMenuData::MediaTypeAudio;
  }

  if (contexts & MenuItem::FRAME)
    rv.frame_url = GURL("http://test.frame/");

  return rv;
}

TEST_F(ContextMenuContentTypeTest, CheckTypes) {
  {
    content::ContextMenuParams params = CreateParams(MenuItem::LINK);
    scoped_ptr<ContextMenuContentType> content_type(Create(main_rfh(), params));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_LINK));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_ALL_EXTENSION));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_CURRENT_EXTENSION));
  }

  {
    content::ContextMenuParams params = CreateParams(MenuItem::SELECTION);
    scoped_ptr<ContextMenuContentType> content_type(Create(main_rfh(), params));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_LINK));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_COPY));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_EDITABLE));
  }

  {
    content::ContextMenuParams params = CreateParams(MenuItem::EDITABLE);
    scoped_ptr<ContextMenuContentType> content_type(Create(main_rfh(), params));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_LINK));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_COPY));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_EDITABLE));
  }

  {
    content::ContextMenuParams params = CreateParams(MenuItem::IMAGE);
    scoped_ptr<ContextMenuContentType> content_type(Create(main_rfh(), params));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_IMAGE));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_SEARCHWEBFORIMAGE));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_PRINT));

    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_VIDEO));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_AUDIO));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_PLUGIN));
  }

  {
    content::ContextMenuParams params = CreateParams(MenuItem::VIDEO);
    scoped_ptr<ContextMenuContentType> content_type(Create(main_rfh(), params));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_VIDEO));

    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_IMAGE));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_AUDIO));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_PLUGIN));
  }

  {
    content::ContextMenuParams params = CreateParams(MenuItem::AUDIO);
    scoped_ptr<ContextMenuContentType> content_type(Create(main_rfh(), params));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_AUDIO));

    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_IMAGE));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_VIDEO));
    EXPECT_FALSE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_MEDIA_PLUGIN));
  }

  {
    content::ContextMenuParams params = CreateParams(MenuItem::FRAME);
    scoped_ptr<ContextMenuContentType> content_type(Create(main_rfh(), params));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_FRAME));
    EXPECT_TRUE(content_type->SupportsGroup(
                    ContextMenuContentType::ITEM_GROUP_PAGE));
  }
}
