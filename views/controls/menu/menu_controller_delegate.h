// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_CONTROLS_MENU_MENU_CONTROLLER_DELEGATE_H_
#define VIEWS_CONTROLS_MENU_MENU_CONTROLLER_DELEGATE_H_
#pragma once

namespace views {

class MenuItemView;

// This is internal as there should be no need for usage of this class outside
// of views.
namespace internal {

// Used by MenuController to notify of interesting events that are intended for
// the class using MenuController. This is implemented by MenuRunnerImpl.
class MenuControllerDelegate {
 public:
  enum NotifyType {
    NOTIFY_DELEGATE,
    DONT_NOTIFY_DELEGATE
  };

  // Invoked when MenuController closes a menu and the MenuController was
  // configured for drop (MenuRunner::FOR_DROP).
  virtual void DropMenuClosed(NotifyType type, MenuItemView* menu) = 0;

  // Invoked when the MenuDelegate::GetSiblingMenu() returns non-NULL.
  virtual void SiblingMenuCreated(MenuItemView* menu) = 0;

 protected:
  virtual ~MenuControllerDelegate() {}
};

}  // namespace internal

}  // namespace view

#endif  // VIEWS_CONTROLS_MENU_MENU_CONTROLLER_DELEGATE_H_
