// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/event_executor.h"

#include <windows.h>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/message_loop.h"
#include "remoting/host/capturer.h"
#include "remoting/proto/event.pb.h"
#include "ui/base/keycodes/keyboard_codes.h"

namespace remoting {

namespace {

using protocol::ClipboardEvent;
using protocol::KeyEvent;
using protocol::MouseEvent;

// USB to XKB keycode map table.
#define USB_KEYMAP(usb, xkb, win, mac) {usb, win}
#include "remoting/host/usb_keycode_map.h"
#undef USB_KEYMAP

// A class to generate events on Windows.
class EventExecutorWin : public EventExecutor {
 public:
  EventExecutorWin(MessageLoop* message_loop, Capturer* capturer);
  virtual ~EventExecutorWin() {}

  // ClipboardStub interface.
  virtual void InjectClipboardEvent(const ClipboardEvent& event) OVERRIDE;

  // InputStub interface.
  virtual void InjectKeyEvent(const KeyEvent& event) OVERRIDE;
  virtual void InjectMouseEvent(const MouseEvent& event) OVERRIDE;

 private:
  HKL GetForegroundKeyboardLayout();
  void HandleKey(const KeyEvent& event);
  void HandleMouse(const MouseEvent& event);

  MessageLoop* message_loop_;
  Capturer* capturer_;

  DISALLOW_COPY_AND_ASSIGN(EventExecutorWin);
};

EventExecutorWin::EventExecutorWin(MessageLoop* message_loop,
                                   Capturer* capturer)
    : message_loop_(message_loop),
      capturer_(capturer) {
}

void EventExecutorWin::InjectClipboardEvent(const ClipboardEvent& event) {
  // TODO(simonmorris): Implement clipboard injection.
}

void EventExecutorWin::InjectKeyEvent(const KeyEvent& event) {
  if (MessageLoop::current() != message_loop_) {
    message_loop_->PostTask(
        FROM_HERE,
        base::Bind(&EventExecutorWin::InjectKeyEvent, base::Unretained(this),
                   event));
    return;
  }

  HandleKey(event);
}

void EventExecutorWin::InjectMouseEvent(const MouseEvent& event) {
  if (MessageLoop::current() != message_loop_) {
    message_loop_->PostTask(
        FROM_HERE,
        base::Bind(&EventExecutorWin::InjectMouseEvent, base::Unretained(this),
                   event));
    return;
  }

  HandleMouse(event);
}

HKL EventExecutorWin::GetForegroundKeyboardLayout() {
  HKL layout = 0;

  // Can return NULL if a window is losing focus.
  HWND foreground = GetForegroundWindow();
  if (foreground) {
    // Can return 0 if the window no longer exists.
    DWORD thread_id = GetWindowThreadProcessId(foreground, 0);
    if (thread_id) {
      // Can return 0 if the thread no longer exists, or if we're
      // running on Windows Vista and the window is a command-prompt.
      layout = GetKeyboardLayout(thread_id);
    }
  }

  // If we couldn't determine a layout then use the system default.
  if (!layout) {
    SystemParametersInfo(SPI_GETDEFAULTINPUTLANG, 0, &layout, 0);
  }

  return layout;
}

void EventExecutorWin::HandleKey(const KeyEvent& event) {
  // HostEventDispatcher should filter events missing the pressed field.
  DCHECK(event.has_pressed());

  // Reset the system idle suspend timeout.
  SetThreadExecutionState(ES_SYSTEM_REQUIRED);

  // The mapping between scancodes and VKEY values depends on the foreground
  // window's current keyboard layout.
  HKL layout = GetForegroundKeyboardLayout();

  int key = event.keycode();
  bool down = event.pressed();
  int scancode = kInvalidKeycode;
  if (event.has_usb_keycode()) {
    // If the event contains a USB-style code, map to a Windows scancode, and
    // look up the corresponding VKEY under the foreground layout.
    scancode = UsbKeycodeToNativeKeycode(event.usb_keycode());
    key = MapVirtualKeyEx(scancode, MAPVK_VSC_TO_VK_EX, layout);
    VLOG(3) << "Converting USB keycode: " << std::hex << event.usb_keycode()
            << " to scancode: " << scancode
            << " and VKEY: " << key << std::dec;
  } else {
    // If the event provides only a VKEY, determine the corresponding scancode.
    scancode = MapVirtualKeyEx(key, MAPVK_VK_TO_VSC_EX, layout);
    VLOG(3) << "Converting VKEY: " << std::hex << event.keycode()
            << " to scancode: " << scancode << std::dec;
  }

  // Ignore events with no VK- or USB-keycode, or which can't be mapped.
  if (scancode == kInvalidKeycode)
    return;

  INPUT input;
  memset(&input, 0, sizeof(input));

  input.type = INPUT_KEYBOARD;
  input.ki.time = 0;
  input.ki.wVk = key;
  input.ki.wScan = scancode;

  // Flag to mark extended 'e0' key scancodes. Without this, the left and
  // right windows keys will not be handled properly (on US keyboard).
  if ((scancode & 0xFF00) == 0xE000) {
    input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
  }

  // Flag to mark keyup events. Default is keydown.
  if (!down) {
    input.ki.dwFlags |= KEYEVENTF_KEYUP;
  }

  if (SendInput(1, &input, sizeof(INPUT)) == 0) {
    LOG_GETLASTERROR(ERROR) << "Failed to inject a key event";
  }
}

void EventExecutorWin::HandleMouse(const MouseEvent& event) {
  // Reset the system idle suspend timeout.
  SetThreadExecutionState(ES_SYSTEM_REQUIRED);

  // TODO(garykac) Collapse mouse (x,y) and button events into a single
  // input event when possible.
  if (event.has_x() && event.has_y()) {
    int x = event.x();
    int y = event.y();

    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.time = 0;
    SkISize screen_size = capturer_->size_most_recent();
    if ((screen_size.width() > 1) && (screen_size.height() > 1)) {
      input.mi.dx = static_cast<int>((x * 65535) / (screen_size.width() - 1));
      input.mi.dy = static_cast<int>((y * 65535) / (screen_size.height() - 1));
      input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
      if (SendInput(1, &input, sizeof(INPUT)) == 0) {
        LOG_GETLASTERROR(ERROR) << "Failed to inject a mouse move event";
      }
    }
  }

  if (event.has_wheel_offset_x() && event.has_wheel_offset_y()) {
    INPUT wheel;
    wheel.type = INPUT_MOUSE;
    wheel.mi.time = 0;

    int dx = event.wheel_offset_x();
    int dy = event.wheel_offset_y();

    if (dx != 0) {
      wheel.mi.mouseData = dx * WHEEL_DELTA;
      wheel.mi.dwFlags = MOUSEEVENTF_HWHEEL;
      if (SendInput(1, &wheel, sizeof(INPUT)) == 0) {
        LOG_GETLASTERROR(ERROR) << "Failed to inject a mouse wheel(x) event";
      }
    }
    if (dy != 0) {
      wheel.mi.mouseData = dy * WHEEL_DELTA;
      wheel.mi.dwFlags = MOUSEEVENTF_WHEEL;
      if (SendInput(1, &wheel, sizeof(INPUT)) == 0) {
        LOG_GETLASTERROR(ERROR) << "Failed to inject a mouse wheel(y) event";
      }
    }
  }

  if (event.has_button() && event.has_button_down()) {
    INPUT button_event;
    button_event.type = INPUT_MOUSE;
    button_event.mi.time = 0;
    button_event.mi.dx = 0;
    button_event.mi.dy = 0;

    MouseEvent::MouseButton button = event.button();
    bool down = event.button_down();
    if (button == MouseEvent::BUTTON_LEFT) {
      button_event.mi.dwFlags =
          down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    } else if (button == MouseEvent::BUTTON_MIDDLE) {
      button_event.mi.dwFlags =
          down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    } else if (button == MouseEvent::BUTTON_RIGHT) {
      button_event.mi.dwFlags =
          down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    } else {
      button_event.mi.dwFlags =
          down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    }

    if (SendInput(1, &button_event, sizeof(INPUT)) == 0) {
      LOG_GETLASTERROR(ERROR) << "Failed to inject a mouse button event";
    }
  }
}

}  // namespace

scoped_ptr<protocol::HostEventStub> EventExecutor::Create(
    MessageLoop* message_loop, Capturer* capturer) {
  return scoped_ptr<protocol::HostEventStub>(
      new EventExecutorWin(message_loop, capturer));
}

}  // namespace remoting
