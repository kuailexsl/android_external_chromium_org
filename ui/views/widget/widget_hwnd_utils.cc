// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/widget_hwnd_utils.h"

#include <dwmapi.h>

#include "base/command_line.h"
#include "base/win/windows_version.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/base/ui_base_switches.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/win/hwnd_message_handler.h"

namespace views {

namespace {

void CalculateWindowStylesFromInitParams(
    const Widget::InitParams& params,
    WidgetDelegate* widget_delegate,
    internal::NativeWidgetDelegate* native_widget_delegate,
    DWORD* style,
    DWORD* ex_style,
    DWORD* class_style) {
  *style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  *ex_style = 0;
  *class_style = CS_DBLCLKS;

  // Set type-independent style attributes.
  if (params.child)
    *style |= WS_CHILD;
  if (params.show_state == ui::SHOW_STATE_MAXIMIZED)
    *style |= WS_MAXIMIZE;
  if (params.show_state == ui::SHOW_STATE_MINIMIZED)
    *style |= WS_MINIMIZE;
  if (!params.accept_events)
    *ex_style |= WS_EX_TRANSPARENT;
  if (!params.can_activate)
    *ex_style |= WS_EX_NOACTIVATE;
  if (params.keep_on_top)
    *ex_style |= WS_EX_TOPMOST;
  if (params.mirror_origin_in_rtl)
    *ex_style |= l10n_util::GetExtendedTooltipStyles();
  // Layered windows do not work with Aura. They are basically incompatible
  // with Direct3D surfaces. Officially, it should be impossible to achieve
  // per-pixel alpha compositing with the desktop and 3D acceleration but it
  // has been discovered that since Vista There is a secret handshake between
  // user32 and the DMW. If things are set up just right DMW gets out of the
  // way; it does not create a backbuffer and simply blends our D3D surface
  // and the desktop background. The handshake is as follows:
  // 1- Use D3D9Ex to create device/swapchain, etc. You need D3DFMT_A8R8G8B8.
  // 2- The window must have WS_EX_COMPOSITED in the extended style.
  // 3- The window must have WS_POPUP in its style.
  // 4- The windows must not have WM_SIZEBOX  in its style.
  // 5- When the window is created but before it is presented, call
  //    DwmExtendFrameIntoClientArea passing -1 as the margins.
  if (params.transparent) {
#if defined(USE_AURA)
    if (base::win::GetVersion() >= base::win::VERSION_VISTA) {
      BOOL enabled = FALSE;
      HRESULT hr = DwmIsCompositionEnabled(&enabled);
      if (CommandLine::ForCurrentProcess()->HasSwitch(
              switches::kDisableDwmComposition))
        enabled = FALSE;
      if (SUCCEEDED(hr) && (enabled == TRUE))
        *ex_style |= WS_EX_COMPOSITED;
    }
#else
    *ex_style |= WS_EX_LAYERED;
#endif
  }
  if (params.has_dropshadow) {
    *class_style |= (base::win::GetVersion() < base::win::VERSION_XP) ?
        0 : CS_DROPSHADOW;
  }

  // Set type-dependent style attributes.
  switch (params.type) {
    case Widget::InitParams::TYPE_PANEL:
      *ex_style |= WS_EX_TOPMOST;
      if (params.remove_standard_frame) {
        *style |= WS_POPUP;
        break;
      }
      // Else, no break. Fall through to TYPE_WINDOW.
    case Widget::InitParams::TYPE_WINDOW: {
      *style |= WS_SYSMENU | WS_CAPTION;
      bool can_resize = widget_delegate->CanResize();
      bool can_maximize = widget_delegate->CanMaximize();
      if (can_maximize) {
        *style |= WS_OVERLAPPEDWINDOW;
      } else if (can_resize || params.remove_standard_frame) {
        *style |= WS_OVERLAPPED | WS_THICKFRAME;
      }
      if (native_widget_delegate->IsDialogBox()) {
        *style |= DS_MODALFRAME;
        // NOTE: Turning this off means we lose the close button, which is bad.
        // Turning it on though means the user can maximize or size the window
        // from the system menu, which is worse. We may need to provide our own
        // menu to get the close button to appear properly.
        // style &= ~WS_SYSMENU;

        // Set the WS_POPUP style for modal dialogs. This ensures that the owner
        // window is activated on destruction. This style should not be set for
        // non-modal non-top-level dialogs like constrained windows.
        *style |= native_widget_delegate->IsModal() ? WS_POPUP : 0;
      }
      *ex_style |=
          native_widget_delegate->IsDialogBox() ? WS_EX_DLGMODALFRAME : 0;
      break;
    }
    case Widget::InitParams::TYPE_CONTROL:
      *style |= WS_VISIBLE;
      break;
    case Widget::InitParams::TYPE_WINDOW_FRAMELESS:
      *style |= WS_POPUP;
      break;
    case Widget::InitParams::TYPE_BUBBLE:
      *style |= WS_POPUP;
      *style |= WS_CLIPCHILDREN;
      break;
    case Widget::InitParams::TYPE_POPUP:
      *style |= WS_POPUP;
      *ex_style |= WS_EX_TOOLWINDOW;
      break;
    case Widget::InitParams::TYPE_MENU:
      *style |= WS_POPUP;
      break;
    default:
      NOTREACHED();
  }
}

}  // namespace

bool DidClientAreaSizeChange(const WINDOWPOS* window_pos) {
  return !(window_pos->flags & SWP_NOSIZE) ||
         window_pos->flags & SWP_FRAMECHANGED;
}

void ConfigureWindowStyles(
    HWNDMessageHandler* handler,
    const Widget::InitParams& params,
    WidgetDelegate* widget_delegate,
    internal::NativeWidgetDelegate* native_widget_delegate) {
  // Configure the HWNDMessageHandler with the appropriate
  DWORD style = 0;
  DWORD ex_style = 0;
  DWORD class_style = 0;
  CalculateWindowStylesFromInitParams(params, widget_delegate,
                                      native_widget_delegate, &style, &ex_style,
                                      &class_style);
  handler->set_initial_class_style(class_style);
  handler->set_window_style(handler->window_style() | style);
  handler->set_window_ex_style(handler->window_ex_style() | ex_style);
}

}  // namespace views
