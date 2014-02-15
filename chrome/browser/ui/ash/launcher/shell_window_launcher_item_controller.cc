// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/shell_window_launcher_item_controller.h"

#include "apps/app_window.h"
#include "apps/ui/native_app_window.h"
#include "ash/shelf/shelf_model.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_app_menu_item.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_app_menu_item_v2app.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/launcher_application_menu_item_model.h"
#include "chrome/browser/ui/ash/launcher/launcher_context_menu.h"
#include "chrome/browser/ui/ash/launcher/launcher_item_controller.h"
#include "content/public/browser/web_contents.h"
#include "skia/ext/image_operations.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/corewm/window_animations.h"

using apps::AppWindow;

namespace {

// Size of the icon in the shelf launcher in display-independent pixels.
const int kAppListIconSize = 24;

// This will return a slightly smaller icon than the app icon to be used in
// the application list menu.
scoped_ptr<gfx::Image> GetAppListIcon(AppWindow* app_window) {
  // TODO(skuhne): We instead might want to use LoadImages in
  // AppWindow::UpdateExtensionAppIcon() to let the extension give us
  // pre-defined icons in the launcher and the launcher list sizes. Since there
  // is no mock yet, doing this now seems a bit premature and we scale for the
  // time being.
  if (app_window->app_icon().IsEmpty())
    return make_scoped_ptr(new gfx::Image());

  SkBitmap bmp =
      skia::ImageOperations::Resize(*app_window->app_icon().ToSkBitmap(),
                                    skia::ImageOperations::RESIZE_BEST,
                                    kAppListIconSize,
                                    kAppListIconSize);
  return make_scoped_ptr(
      new gfx::Image(gfx::ImageSkia::CreateFrom1xBitmap(bmp)));
}

// Functor for std::find_if used in AppLauncherItemController.
class AppWindowHasWindow {
 public:
  explicit AppWindowHasWindow(aura::Window* window) : window_(window) {}

  bool operator()(AppWindow* app_window) const {
    return app_window->GetNativeWindow() == window_;
  }

 private:
  const aura::Window* window_;
};

}  // namespace

ShellWindowLauncherItemController::ShellWindowLauncherItemController(
    Type type,
    const std::string& app_shelf_id,
    const std::string& app_id,
    ChromeLauncherController* controller)
    : LauncherItemController(type, app_id, controller),
      last_active_app_window_(NULL),
      app_shelf_id_(app_shelf_id),
      observed_windows_(this) {}

ShellWindowLauncherItemController::~ShellWindowLauncherItemController() {
}

void ShellWindowLauncherItemController::AddAppWindow(
    AppWindow* app_window,
    ash::ShelfItemStatus status) {
  if (app_window->window_type_is_panel() && type() != TYPE_APP_PANEL)
    LOG(ERROR) << "AppWindow of type Panel added to non-panel launcher item";
  app_windows_.push_front(app_window);
  observed_windows_.Add(app_window->GetNativeWindow());
}

void ShellWindowLauncherItemController::RemoveShellWindowForWindow(
    aura::Window* window) {
  AppWindowList::iterator iter = std::find_if(
      app_windows_.begin(), app_windows_.end(), AppWindowHasWindow(window));
  if (iter != app_windows_.end()) {
    if (*iter == last_active_app_window_)
      last_active_app_window_ = NULL;
    app_windows_.erase(iter);
  }
  observed_windows_.Remove(window);
}

void ShellWindowLauncherItemController::SetActiveWindow(aura::Window* window) {
  AppWindowList::iterator iter = std::find_if(
      app_windows_.begin(), app_windows_.end(), AppWindowHasWindow(window));
  if (iter != app_windows_.end())
    last_active_app_window_ = *iter;
}

bool ShellWindowLauncherItemController::IsOpen() const {
  return !app_windows_.empty();
}

bool ShellWindowLauncherItemController::IsVisible() const {
  // Return true if any windows are visible.
  for (AppWindowList::const_iterator iter = app_windows_.begin();
       iter != app_windows_.end();
       ++iter) {
    if ((*iter)->GetNativeWindow()->IsVisible())
      return true;
  }
  return false;
}

void ShellWindowLauncherItemController::Launch(ash::LaunchSource source,
                                               int event_flags) {
  launcher_controller()->LaunchApp(app_id(),
                                   source,
                                   ui::EF_NONE);
}

bool ShellWindowLauncherItemController::Activate(ash::LaunchSource source) {
  DCHECK(!app_windows_.empty());
  AppWindow* window_to_activate =
      last_active_app_window_ ? last_active_app_window_ : app_windows_.back();
  window_to_activate->GetBaseWindow()->Activate();
  return false;
}

void ShellWindowLauncherItemController::Close() {
  // Note: Closing windows may affect the contents of app_windows_.
  AppWindowList windows_to_close = app_windows_;
  for (AppWindowList::iterator iter = windows_to_close.begin();
       iter != windows_to_close.end();
       ++iter) {
    (*iter)->GetBaseWindow()->Close();
  }
}

void ShellWindowLauncherItemController::ActivateIndexedApp(size_t index) {
  if (index >= app_windows_.size())
    return;
  AppWindowList::iterator it = app_windows_.begin();
  std::advance(it, index);
  ShowAndActivateOrMinimize(*it);
}

ChromeLauncherAppMenuItems
ShellWindowLauncherItemController::GetApplicationList(int event_flags) {
  ChromeLauncherAppMenuItems items;
  items.push_back(new ChromeLauncherAppMenuItem(GetTitle(), NULL, false));
  int index = 0;
  for (AppWindowList::iterator iter = app_windows_.begin();
       iter != app_windows_.end();
       ++iter) {
    AppWindow* app_window = *iter;
    scoped_ptr<gfx::Image> image(GetAppListIcon(app_window));
    items.push_back(new ChromeLauncherAppMenuItemV2App(
        app_window->GetTitle(),
        image.get(),  // Will be copied
        app_id(),
        launcher_controller(),
        index,
        index == 0 /* has_leading_separator */));
    ++index;
  }
  return items.Pass();
}

bool ShellWindowLauncherItemController::ItemSelected(const ui::Event& event) {
  if (app_windows_.empty())
    return false;
  if (type() == TYPE_APP_PANEL) {
    DCHECK(app_windows_.size() == 1);
    AppWindow* panel = app_windows_.front();
    aura::Window* panel_window = panel->GetNativeWindow();
    // If the panel is attached on another display, move it to the current
    // display and activate it.
    if (ash::wm::GetWindowState(panel_window)->panel_attached() &&
        ash::wm::MoveWindowToEventRoot(panel_window, event)) {
      if (!panel->GetBaseWindow()->IsActive())
        ShowAndActivateOrMinimize(panel);
    } else {
      ShowAndActivateOrMinimize(panel);
    }
  } else {
    AppWindow* window_to_show = last_active_app_window_
                                    ? last_active_app_window_
                                    : app_windows_.front();
    // If the event was triggered by a keystroke, we try to advance to the next
    // item if the window we are trying to activate is already active.
    if (app_windows_.size() >= 1 &&
        window_to_show->GetBaseWindow()->IsActive() &&
        event.type() == ui::ET_KEY_RELEASED) {
      ActivateOrAdvanceToNextShellWindow(window_to_show);
    } else {
      ShowAndActivateOrMinimize(window_to_show);
    }
  }
  return false;
}

base::string16 ShellWindowLauncherItemController::GetTitle() {
  // For panels return the title of the contents if set.
  // Otherwise return the title of the app.
  if (type() == TYPE_APP_PANEL && !app_windows_.empty()) {
    AppWindow* app_window = app_windows_.front();
    if (app_window->web_contents()) {
      base::string16 title = app_window->web_contents()->GetTitle();
      if (!title.empty())
        return title;
    }
  }
  return GetAppTitle();
}

ui::MenuModel* ShellWindowLauncherItemController::CreateContextMenu(
    aura::Window* root_window) {
  ash::ShelfItem item =
      *(launcher_controller()->model()->ItemByID(shelf_id()));
  return new LauncherContextMenu(launcher_controller(), &item, root_window);
}

ash::ShelfMenuModel* ShellWindowLauncherItemController::CreateApplicationMenu(
    int event_flags) {
  return new LauncherApplicationMenuItemModel(GetApplicationList(event_flags));
}

bool ShellWindowLauncherItemController::IsDraggable() {
  if (type() == TYPE_APP_PANEL)
    return true;
  return launcher_controller()->CanPin() ? true : false;
}

bool ShellWindowLauncherItemController::ShouldShowTooltip() {
  if (type() == TYPE_APP_PANEL && IsVisible())
    return false;
  return true;
}

void ShellWindowLauncherItemController::OnWindowPropertyChanged(
    aura::Window* window,
    const void* key,
    intptr_t old) {
  if (key == aura::client::kDrawAttentionKey) {
    ash::ShelfItemStatus status;
    if (ash::wm::IsActiveWindow(window)) {
      status = ash::STATUS_ACTIVE;
    } else if (window->GetProperty(aura::client::kDrawAttentionKey)) {
      status = ash::STATUS_ATTENTION;
    } else {
      status = ash::STATUS_RUNNING;
    }
    launcher_controller()->SetItemStatus(shelf_id(), status);
  }
}

void ShellWindowLauncherItemController::ShowAndActivateOrMinimize(
    AppWindow* app_window) {
  // Either show or minimize windows when shown from the launcher.
  launcher_controller()->ActivateWindowOrMinimizeIfActive(
      app_window->GetBaseWindow(), GetApplicationList(0).size() == 2);
}

void ShellWindowLauncherItemController::ActivateOrAdvanceToNextShellWindow(
    AppWindow* window_to_show) {
  AppWindowList::iterator i(
      std::find(app_windows_.begin(), app_windows_.end(), window_to_show));
  if (i != app_windows_.end()) {
    if (++i != app_windows_.end())
      window_to_show = *i;
    else
      window_to_show = app_windows_.front();
  }
  if (window_to_show->GetBaseWindow()->IsActive()) {
    // Coming here, only a single window is active. For keyboard activations
    // the window gets animated.
    AnimateWindow(window_to_show->GetNativeWindow(),
                  views::corewm::WINDOW_ANIMATION_TYPE_BOUNCE);
  } else {
    ShowAndActivateOrMinimize(window_to_show);
  }
}
