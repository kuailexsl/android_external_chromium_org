// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/event_transformation_handler.h"

#include <cmath>

#include "ash/display/display_info.h"
#include "ash/display/display_manager.h"
#include "ash/shell.h"
#include "ash/wm/coordinate_conversion.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/compositor/dip_util.h"
#include "ui/events/event.h"
#include "ui/gfx/display.h"
#include "ui/gfx/screen.h"

namespace ash {
namespace {

// Boost factor for non-integrated displays.
const float kBoostForNonIntegrated = 1.20f;

}  // namespace

EventTransformationHandler::EventTransformationHandler()
    : transformation_mode_(TRANSFORM_AUTO) {
}

EventTransformationHandler::~EventTransformationHandler() {
}

void EventTransformationHandler::OnScrollEvent(ui::ScrollEvent* event) {
  if (transformation_mode_ == TRANSFORM_NONE)
    return;

  // It is unnecessary to scale the event for the device scale factor since
  // the event locations etc. are already in DIP.
  gfx::Point point_in_screen(event->location());
  aura::Window* target = static_cast<aura::Window*>(event->target());
  wm::ConvertPointToScreen(target, &point_in_screen);
  const gfx::Display& display =
      Shell::GetScreen()->GetDisplayNearestPoint(point_in_screen);

  // Apply some additional scaling if the display is non-integrated.
  if (!display.IsInternal())
    event->Scale(kBoostForNonIntegrated);
}

}  // namespace ash
