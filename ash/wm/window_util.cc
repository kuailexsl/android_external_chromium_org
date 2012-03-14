// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_util.h"

#include "ash/shell.h"
#include "ash/wm/activation_controller.h"
#include "ui/aura/client/activation_client.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/root_window.h"
#include "ui/aura/window.h"
#include "ui/aura/window_property.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/screen.h"

DECLARE_WINDOW_PROPERTY_TYPE(bool);

namespace ash {
DEFINE_WINDOW_PROPERTY_KEY(bool, kOpenWindowSplitKey, false);

namespace wm {

void ActivateWindow(aura::Window* window) {
  aura::client::GetActivationClient(Shell::GetRootWindow())->ActivateWindow(
      window);
}

void DeactivateWindow(aura::Window* window) {
  aura::client::GetActivationClient(Shell::GetRootWindow())->DeactivateWindow(
      window);
}

bool IsActiveWindow(aura::Window* window) {
  return GetActiveWindow() == window;
}

aura::Window* GetActiveWindow() {
  return aura::client::GetActivationClient(Shell::GetRootWindow())->
      GetActiveWindow();
}

aura::Window* GetActivatableWindow(aura::Window* window) {
  return internal::ActivationController::GetActivatableWindow(window);
}

bool IsWindowNormal(aura::Window* window) {
  return window->GetProperty(aura::client::kShowStateKey) ==
          ui::SHOW_STATE_NORMAL ||
      window->GetProperty(aura::client::kShowStateKey) ==
          ui::SHOW_STATE_DEFAULT;
}

bool IsWindowMaximized(aura::Window* window) {
  return window->GetProperty(aura::client::kShowStateKey) ==
      ui::SHOW_STATE_MAXIMIZED;
}

bool IsWindowMinimized(aura::Window* window) {
  return window->GetProperty(aura::client::kShowStateKey) ==
      ui::SHOW_STATE_MINIMIZED;
}

bool IsWindowFullscreen(aura::Window* window) {
  return window->GetProperty(aura::client::kShowStateKey) ==
      ui::SHOW_STATE_FULLSCREEN;
}

void MaximizeWindow(aura::Window* window) {
  window->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
}

void RestoreWindow(aura::Window* window) {
  window->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
}

bool HasFullscreenWindow(const WindowSet& windows) {
  for (WindowSet::const_iterator i = windows.begin(); i != windows.end(); ++i) {
    if ((*i)->GetProperty(aura::client::kShowStateKey)
        == ui::SHOW_STATE_FULLSCREEN) {
      return true;
    }
  }
  return false;
}

void SetOpenWindowSplit(aura::Window* window, bool value) {
  window->SetProperty(kOpenWindowSplitKey, value);
}

bool GetOpenWindowSplit(aura::Window* window) {
  return window->GetProperty(kOpenWindowSplitKey);
}

}  // namespace wm
}  // namespace ash
