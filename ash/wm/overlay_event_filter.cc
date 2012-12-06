// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overlay_event_filter.h"

#include "ash/wm/partial_screenshot_view.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/events/event.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace internal {

OverlayEventFilter::OverlayEventFilter()
    : delegate_(NULL) {
}

OverlayEventFilter::~OverlayEventFilter() {
  delegate_ = NULL;
}

ui::EventResult OverlayEventFilter::OnKeyEvent(ui::KeyEvent* event) {
  if (!delegate_)
    return ui::ER_UNHANDLED;

  // Do not consume a translated key event which is generated by an IME (e.g.,
  // ui::VKEY_PROCESSKEY) since the key event is generated in response to a key
  // press or release before showing the ovelay. This is important not to
  // confuse key event handling JavaScript code in a page.
  if (event->type() == ui::ET_TRANSLATED_KEY_PRESS ||
      event->type() == ui::ET_TRANSLATED_KEY_RELEASE) {
    return ui::ER_UNHANDLED;
  }

  if (delegate_ && delegate_->IsCancelingKeyEvent(event))
    Cancel();

  // Handle key events only when they are sent to a child of the delegate's
  // window.
  aura::Window* target = static_cast<aura::Window*>(event->target());
  if (delegate_ && delegate_->GetWindow() &&
      delegate_->GetWindow()->Contains(target))
    target->delegate()->OnKeyEvent(event);

  // Always handled: other windows shouldn't receive input while we're
  // displaying an overlay.
  return ui::ER_CONSUMED;
}

void OverlayEventFilter::OnLoginStateChanged(
    user::LoginStatus status) {
  Cancel();
}

void OverlayEventFilter::OnAppTerminating() {
  Cancel();
}

void OverlayEventFilter::OnLockStateChanged(bool locked) {
  Cancel();
}

void OverlayEventFilter::Activate(Delegate* delegate) {
  delegate_ = delegate;
}

void OverlayEventFilter::Deactivate() {
  delegate_ = NULL;
}

void OverlayEventFilter::Cancel() {
  if (delegate_)
    delegate_->Cancel();
}
}  // namespace internal
}  // namespace ash
