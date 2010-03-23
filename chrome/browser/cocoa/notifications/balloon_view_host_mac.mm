// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/cocoa/notifications/balloon_view_host_mac.h"

#include "chrome/browser/notifications/balloon.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/renderer_host/render_widget_host_view.h"
#include "chrome/browser/renderer_host/render_widget_host_view_mac.h"

BalloonViewHost::BalloonViewHost(Balloon* balloon)
    : BalloonHost(balloon) {
}

void BalloonViewHost::UpdateActualSize(const gfx::Size& new_size) {
  NSView* view = render_widget_host_view_->native_view();
  NSRect frame = [view frame];
  frame.size.width = new_size.width();
  frame.size.height = new_size.height();

  [view setFrame:frame];
  [view setNeedsDisplay:YES];
}

void BalloonViewHost::InitRenderWidgetHostView() {
  DCHECK(render_view_host_);
  render_widget_host_view_ = new RenderWidgetHostViewMac(render_view_host_);
}

