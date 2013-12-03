// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MODE_INDICATOR_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MODE_INDICATOR_CONTROLLER_H_

#include "base/memory/scoped_ptr.h"
#include "chromeos/ime/input_method_manager.h"
#include "ui/gfx/rect.h"

namespace chromeos {
namespace input_method {

class ModeIndicatorObserver;

// ModeIndicatorController is the controller of ModeIndicatiorDelegateView
// on the MVC model.
class ModeIndicatorController
    : public InputMethodManager::Observer {
 public:
  // This class takes the ownership of |mi_widget|.
  explicit ModeIndicatorController(InputMethodManager* imm);
  virtual ~ModeIndicatorController();

  // Set cursor bounds, which is the base point to display this indicator.
  // Bacisally this indicator is displayed underneath the cursor.
  void SetCursorBounds(const gfx::Rect& cursor_location);

  // Notify the focus state to the mode indicator.
  void FocusStateChanged(bool is_focused);

 private:
  // InputMethodManager::Observer implementation.
  virtual void InputMethodChanged(InputMethodManager* manager,
                                  bool show_message) OVERRIDE;
  virtual void InputMethodPropertyChanged(InputMethodManager* manager) OVERRIDE;

  // Show the mode inidicator with the current ime's short name if all
  // the conditions are cleared.
  void ShowModeIndicator();

  InputMethodManager* imm_;

  // Cursor bounds representing the anchor rect of the mode indicator.
  gfx::Rect cursor_bounds_;

  // True on a text field is focused.
  bool is_focused_;

  // Observer of the widgets created by BubbleDelegateView.  This is used to
  // close the previous widget when a new widget is created.
  scoped_ptr<ModeIndicatorObserver> mi_observer_;

  DISALLOW_COPY_AND_ASSIGN(ModeIndicatorController);
};

}  // namespace input_method
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MODE_INDICATOR_CONTROLLER_H_
