// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_WIN_HWND_SUBCLASS_H_
#define UI_BASE_WIN_HWND_SUBCLASS_H_
#pragma once

#include <vector>
#include <windows.h>

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "ui/base/ui_export.h"
#include "ui/base/view_prop.h"

namespace ui {

// Classes implementing this interface get the opportunity to handle and consume
// messages before they are sent to their target HWND.
class UI_EXPORT HWNDMessageFilter {
 public:
  virtual ~HWNDMessageFilter();

  // A derived class overrides this method to perform filtering of the messages.
  // Return true to prevent other HWNDMessageFilter's of the target HWND and the
  // system message handler |original_wnd_proc_| from receiving the message.
  // Return false to propagate the message further to other HWNDMessageFilters
  // and eventually to |original_wnd_proc|.
  // The order in which HWNDMessageFilters are added in HWNDSubclass::AddFilter
  // determines which filter gets to see the message first (a filter added first
  // will see the message first).
  virtual bool FilterMessage(HWND hwnd,
                             UINT message,
                             WPARAM w_param,
                             LPARAM l_param,
                             LRESULT* l_result) = 0;
};

// An object that instance-subclasses a window. If the window has already been
// instance-subclassed, that subclassing is lost.
class UI_EXPORT HWNDSubclass {
 public:
  ~HWNDSubclass();

  // Adds |filter| to the HWNDSubclass of |target|. Caller retains ownership of
  // |filter|. See the comment about the order in which filters are added in
  // HWNDMessageFilter::FilterMessage.
  static void AddFilterToTarget(HWND target, HWNDMessageFilter* filter);

  // Removes |filter| from any HWNDSubclass that has it.
  static void RemoveFilterFromAllTargets(HWNDMessageFilter* filter);

  // Returns a non-null HWNDSubclass corresponding to the HWND |target|. Creates
  // one if none exists. Retains ownership of the returned pointer.
  static HWNDSubclass* GetHwndSubclassForTarget(HWND target);

  // Adds |filter| if not already added to this HWNDSubclass. Caller retains
  // ownership of |filter|. See the comment about the order in which filters are
  // added in HWNDMessageFilter::FilterMessage.
  void AddFilter(HWNDMessageFilter* filter);

  // Removes |filter|  from this HWNDSubclass instance if present.
  void RemoveFilter(HWNDMessageFilter* filter);

  LRESULT OnWndProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);

 private:
  class HWNDSubclassFactory;
  friend class HWNDSubclassFactory;

  explicit HWNDSubclass(HWND target);

  HWND target_;
  std::vector<HWNDMessageFilter*> filters_;
  WNDPROC original_wnd_proc_;
  ui::ViewProp prop_;

  DISALLOW_COPY_AND_ASSIGN(HWNDSubclass);
};

}  // namespace ui

#endif  // UI_BASE_WIN_HWND_SUBCLASS_H_
