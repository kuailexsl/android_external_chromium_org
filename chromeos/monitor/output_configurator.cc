// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/monitor/output_configurator.h"

#include "base/chromeos/chromeos_version.h"
#include "base/logging.h"
#include "base/message_pump_aurax11.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "dbus/bus.h"
#include "dbus/exported_object.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {
// DPI measurements.
const float kMmInInch = 25.4;
const float kDpi96 = 96.0;
const float kPixelsToMmScale = kMmInInch / kDpi96;

// Prefixes for the built-in displays.
const char kInternal_LVDS[] = "LVDS";
const char kInternal_eDP[] = "eDP";

// TODO: Determine if we need to organize modes in a way which provides better
// than O(n) lookup time.  In many call sites, for example, the "next" mode is
// typically what we are looking for so using this helper might be too
// expensive.
static XRRModeInfo* ModeInfoForID(XRRScreenResources* screen, RRMode modeID) {
  XRRModeInfo* result = NULL;
  for (int i = 0; (i < screen->nmode) && (result == NULL); i++)
    if (modeID == screen->modes[i].id)
      result = &screen->modes[i];

  // We can't fail to find a mode referenced from the same screen.
  CHECK(result != NULL);
  return result;
}

// Identifies the modes which will be used by the respective outputs when in a
// mirror mode.  This means that the two modes will have the same resolution.
// The RROutput IDs |one| and |two| are used to look up the modes and
// |out_one_mode| and |out_two_mode| are the out-parameters for the respective
// modes.
// Returns false if it fails to find a compatible set of modes.
static bool FindMirrorModeForOutputs(Display* display,
                                     XRRScreenResources* screen,
                                     RROutput one,
                                     RROutput two,
                                     RRMode* out_one_mode,
                                     RRMode* out_two_mode) {
  XRROutputInfo* primary = XRRGetOutputInfo(display, screen, one);
  XRROutputInfo* secondary = XRRGetOutputInfo(display, screen, two);

  int one_index = 0;
  int two_index = 0;
  bool found = false;
  while (!found &&
      (one_index < primary->nmode) &&
      (two_index < secondary->nmode)) {
    RRMode one_id = primary->modes[one_index];
    RRMode two_id = secondary->modes[two_index];
    XRRModeInfo* one_mode = ModeInfoForID(screen, one_id);
    XRRModeInfo* two_mode = ModeInfoForID(screen, two_id);
    int one_width = one_mode->width;
    int one_height = one_mode->height;
    int two_width = two_mode->width;
    int two_height = two_mode->height;
    if ((one_width == two_width) && (one_height == two_height)) {
      *out_one_mode = one_id;
      *out_two_mode = two_id;
      found = true;
    } else {
      // The sort order of the modes is NOT by mode area but is sorted by width,
      // then by height within each like width.
      if (one_width > two_width) {
        one_index += 1;
      } else if (one_width < two_width) {
        two_index += 1;
      } else {
        if (one_height > two_height) {
          one_index += 1;
        } else {
          two_index += 1;
        }
      }
    }
  }
  XRRFreeOutputInfo(primary);
  XRRFreeOutputInfo(secondary);
  return found;
}

// A helper to call XRRSetCrtcConfig with the given options but some of our
// default output count and rotation arguments.
static void ConfigureCrtc(Display *display,
                          XRRScreenResources* screen,
                          RRCrtc crtc,
                          int x,
                          int y,
                          RRMode mode,
                          RROutput output) {
  const Rotation kRotate = RR_Rotate_0;
  RROutput* outputs = NULL;
  int num_outputs = 0;

  // Check the output and mode argument - if either are None, we should disable.
  if ((output != None) && (mode != None)) {
    outputs = &output;
    num_outputs = 1;
  }

  XRRSetCrtcConfig(display,
                   screen,
                   crtc,
                   CurrentTime,
                   x,
                   y,
                   mode,
                   kRotate,
                   outputs,
                   num_outputs);
  if (num_outputs == 1) {
    // We are enabling a display so make sure it is turned on.
    CHECK(DPMSEnable(display));
    CHECK(DPMSForceLevel(display, DPMSModeOn));
  }
}

// Called to set the frame buffer (underling XRR "screen") size.  Has a
// side-effect of disabling all CRTCs.
static void CreateFrameBuffer(Display* display,
                              XRRScreenResources* screen,
                              Window window,
                              int width,
                              int height) {
  // Note that setting the screen size fails if any CRTCs are currently
  // pointing into it so disable them all.
  for (int i = 0; i < screen->ncrtc; ++i) {
    const int x = 0;
    const int y = 0;
    const RRMode kMode = None;
    const RROutput kOutput = None;

    ConfigureCrtc(display,
                  screen,
                  screen->crtcs[i],
                  x,
                  y,
                  kMode,
                  kOutput);
  }
  int mm_width = width * kPixelsToMmScale;
  int mm_height = height * kPixelsToMmScale;
  XRRSetScreenSize(display, window, width, height, mm_width, mm_height);
}
}  // namespace

bool OutputConfigurator::TryRecacheOutputs(Display* display,
                                           XRRScreenResources* screen) {
  bool outputs_did_change = false;
  int previous_connected_count = 0;
  int new_connected_count = 0;

  if (output_count_ != screen->noutput) {
    outputs_did_change = true;
  } else {
    // The outputs might have changed so compare the connected states in the
    // screen to our existing cache.
    for (int i = 0; (i < output_count_) && !outputs_did_change; ++i) {
      RROutput thisID = screen->outputs[i];
      XRROutputInfo* output = XRRGetOutputInfo(display, screen, thisID);
      bool now_connected = (RR_Connected == output->connection);
      outputs_did_change = (now_connected != output_cache_[i].is_connected);
      XRRFreeOutputInfo(output);

      if (output_cache_[i].is_connected)
        previous_connected_count += 1;
      if (now_connected)
        new_connected_count += 1;
    }
  }

  if (outputs_did_change) {
    // We now know that we need to recache so free and re-alloc the buffer.
    output_count_ = screen->noutput;
    if (output_count_ == 0) {
      output_cache_.reset(NULL);
    } else {
      // Ideally, this would be allocated inline in the OutputConfigurator
      // instance since we support at most 2 connected outputs but this dynamic
      // allocation was specifically requested.
      output_cache_.reset(new CachedOutputDescription[output_count_]);
    }

    // TODO: This approach to finding CRTCs only supports two.  Expand on this.
    RRCrtc used_crtc = None;
    primary_output_index_ = -1;
    secondary_output_index_ = -1;

    for (int i = 0; i < output_count_; ++i) {
      RROutput this_id = screen->outputs[i];
      XRROutputInfo* output = XRRGetOutputInfo(display, screen, this_id);
      bool is_connected = (RR_Connected == output->connection);
      RRCrtc crtc = None;
      RRMode ideal_mode = None;
      int x = 0;
      int y = 0;
      bool is_internal = false;

      if (is_connected) {
        for (int j = 0; (j < output->ncrtc) && (None == crtc); ++j) {
          RRCrtc possible = output->crtcs[j];
          if (possible != used_crtc) {
            crtc = possible;
            used_crtc = possible;
          }
        }

        const char* name = output->name;
        is_internal =
            (strncmp(kInternal_LVDS,
                     name,
                     arraysize(kInternal_LVDS) - 1) == 0) ||
            (strncmp(kInternal_eDP,
                     name,
                     arraysize(kInternal_eDP) - 1) == 0);
        if (output->nmode > 0)
          ideal_mode = output->modes[0];

        if (crtc != None) {
          XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, screen, crtc);
          x = crtcInfo->x;
          y = crtcInfo->y;
          XRRFreeCrtcInfo(crtcInfo);
        }

        // Save this for later mirror mode detection.
        if (primary_output_index_ == -1)
          primary_output_index_ = i;
        else if (secondary_output_index_ == -1)
          secondary_output_index_ = i;
      }
      XRRFreeOutputInfo(output);

      // Now save the cached state for this output (we will default to mirror
      // disabled and detect that after we have identified the first two
      // connected outputs).
      VLOG(1) << "Recache output index: " << i
              << ", output id: " << this_id
              << ", crtc id: " << crtc
              << ", ideal mode id: " << ideal_mode
              << ", x: " << x
              << ", y: " << y
              << ", is connected: " << is_connected
              << ", is_internal: " << is_internal;
      output_cache_[i].output = this_id;
      output_cache_[i].crtc = crtc;
      output_cache_[i].mirror_mode = None;
      output_cache_[i].ideal_mode = ideal_mode;
      output_cache_[i].x = x;
      output_cache_[i].y = y;
      output_cache_[i].is_connected = is_connected;
      output_cache_[i].is_powered_on = true;
      output_cache_[i].is_internal = is_internal;
    }

    // Now, detect the mirror modes if we have two connected outputs.
    if ((primary_output_index_ != -1) && (secondary_output_index_ != -1)) {
      mirror_supported_ = FindMirrorModeForOutputs(
          display,
          screen,
          output_cache_[primary_output_index_].output,
          output_cache_[secondary_output_index_].output,
          &output_cache_[primary_output_index_].mirror_mode,
          &output_cache_[secondary_output_index_].mirror_mode);

      RRMode primary_mode = output_cache_[primary_output_index_].mirror_mode;
      RRMode second_mode = output_cache_[secondary_output_index_].mirror_mode;
      VLOG(1) << "Mirror mode supported " << mirror_supported_
              << " primary " << primary_mode
              << " secondary " << second_mode;
    }
  }
  return outputs_did_change;
}

OutputConfigurator::OutputConfigurator()
    : is_running_on_chrome_os_(false),
      output_count_(0),
      output_cache_(NULL),
      mirror_supported_(false),
      primary_output_index_(-1),
      secondary_output_index_(-1),
      xrandr_event_base_(0),
      output_state_(STATE_INVALID) {
  // We only want to apply our logic if we are running on ChromeOS.
  is_running_on_chrome_os_ = base::chromeos::IsRunningOnChromeOS();
  LOG_IF(ERROR, !is_running_on_chrome_os_)
      << "OutputConfigurator detected not ChromeOS.  All configuration calls "
         "will fail";

  if (is_running_on_chrome_os_) {
    // Send the signal to powerd to tell it that we will take over output
    // control.
    // Note that this can be removed once the legacy powerd support is removed.
    chromeos::DBusThreadManager* manager = chromeos::DBusThreadManager::Get();
    dbus::Bus* bus = manager->GetSystemBus();
    dbus::ExportedObject* remote_object = bus->GetExportedObject(
        dbus::ObjectPath(power_manager::kPowerManagerServicePath));
    dbus::Signal signal(power_manager::kPowerManagerInterface,
                        power_manager::kUseNewMonitorConfigSignal);
    CHECK(signal.raw_message() != NULL);
    remote_object->SendSignal(&signal);

    // Cache the initial output state.
    Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
    CHECK(display != NULL);
    XGrabServer(display);
    Window window = DefaultRootWindow(display);
    XRRScreenResources* screen = XRRGetScreenResources(display, window);
    CHECK(screen != NULL);
    bool did_detect_outputs = TryRecacheOutputs(display, screen);
    CHECK(did_detect_outputs);
    State state = GetDefaultState();
    UpdateCacheAndXrandrToState(display, screen, window, state);
    // Find xrandr_event_base_ since we need it to interpret events, later.
    int error_base_ignored = 0;
    XRRQueryExtension(display, &xrandr_event_base_, &error_base_ignored);
    // Relinquish X resources.
    XRRFreeScreenResources(screen);
    XUngrabServer(display);
  }
}

void OutputConfigurator::UpdateCacheAndXrandrToState(
    Display* display,
    XRRScreenResources* screen,
    Window window,
    State new_state) {
  // Default rules:
  // - single display = rebuild framebuffer and set to ideal_mode.
  // - multi display = rebuild framebuffer and set to mirror_mode.

  // First, calculate the width and height of the framebuffer (we could retain
  // the existing buffer, if it isn't resizing, but that causes an odd display
  // state where the CRTCs are repositioned over the root windows before Chrome
  // can move them).  It is a feature worth considering, though, and wouldn't
  // be difficult to implement (just check the current framebuffer size before
  // changing it).
  int width = 0;
  int height = 0;
  int primary_height = 0;
  int secondary_height = 0;
  if (new_state == STATE_SINGLE) {
    CHECK_NE(-1, primary_output_index_);

    XRRModeInfo* ideal_mode = ModeInfoForID(
        screen,
        output_cache_[primary_output_index_].ideal_mode);
    width = ideal_mode->width;
    height = ideal_mode->height;
  } else if (new_state == STATE_DUAL_MIRROR) {
    CHECK_NE(-1, primary_output_index_);
    CHECK_NE(-1, secondary_output_index_);

    XRRModeInfo* mirror_mode = ModeInfoForID(
        screen,
        output_cache_[primary_output_index_].mirror_mode);
    width = mirror_mode->width;
    height = mirror_mode->height;
  } else if ((new_state == STATE_DUAL_PRIMARY_ONLY) ||
             (new_state == STATE_DUAL_SECONDARY_ONLY)) {
    CHECK_NE(-1, primary_output_index_);
    CHECK_NE(-1, secondary_output_index_);

    XRRModeInfo* one_ideal = ModeInfoForID(
        screen,
        output_cache_[primary_output_index_].ideal_mode);
    XRRModeInfo* two_ideal = ModeInfoForID(
        screen,
        output_cache_[secondary_output_index_].ideal_mode);
    width = std::max<int>(one_ideal->width, two_ideal->width);
    height = one_ideal->height + two_ideal->height;
    primary_height = one_ideal->height;
    secondary_height = two_ideal->height;
  }
  CreateFrameBuffer(display, screen, window, width, height);

  // Now, tile the outputs appropriately.
  const int x = 0;
  const int y = 0;
  switch (new_state) {
    case STATE_SINGLE:
      ConfigureCrtc(display,
                    screen,
                    output_cache_[primary_output_index_].crtc,
                    x,
                    y,
                    output_cache_[primary_output_index_].ideal_mode,
                    output_cache_[primary_output_index_].output);
      break;
    case STATE_DUAL_MIRROR:
    case STATE_DUAL_PRIMARY_ONLY:
    case STATE_DUAL_SECONDARY_ONLY: {
      RRMode primary_mode = output_cache_[primary_output_index_].mirror_mode;
      RRMode secondary_mode =
          output_cache_[secondary_output_index_].mirror_mode;
      int primary_y = y;
      int secondary_y = y;

      if (new_state != STATE_DUAL_MIRROR) {
        primary_mode = output_cache_[primary_output_index_].ideal_mode;
        secondary_mode = output_cache_[secondary_output_index_].ideal_mode;
      }
      if (new_state == STATE_DUAL_PRIMARY_ONLY)
        secondary_y = y + primary_height;
      if (new_state == STATE_DUAL_SECONDARY_ONLY)
        primary_y = y + secondary_height;

      ConfigureCrtc(display,
                    screen,
                    output_cache_[primary_output_index_].crtc,
                    x,
                    primary_y,
                    primary_mode,
                    output_cache_[primary_output_index_].output);
      ConfigureCrtc(display,
                    screen,
                    output_cache_[secondary_output_index_].crtc,
                    x,
                    secondary_y,
                    secondary_mode,
                    output_cache_[secondary_output_index_].output);
      }
      break;
    case STATE_HEADLESS:
      // Do nothing.
      break;
    default:
      NOTREACHED() << "Unhandled state " << new_state;
  }
  output_state_ = new_state;
}

bool OutputConfigurator::RecacheAndUseDefaultState() {
  Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
  CHECK(display != NULL);
  XGrabServer(display);
  Window window = DefaultRootWindow(display);
  XRRScreenResources* screen = XRRGetScreenResources(display, window);
  CHECK(screen != NULL);

  bool did_detect_change = TryRecacheOutputs(display, screen);
  if (did_detect_change) {
    State state = GetDefaultState();
    UpdateCacheAndXrandrToState(display, screen, window, state);
  }
  XRRFreeScreenResources(screen);
  XUngrabServer(display);
  return did_detect_change;
}

State OutputConfigurator::GetDefaultState() const {
  State state = STATE_HEADLESS;
  if (-1 != primary_output_index_) {
    if (-1 != secondary_output_index_)
      state = mirror_supported_ ? STATE_DUAL_MIRROR : STATE_DUAL_PRIMARY_ONLY;
    else
      state = STATE_SINGLE;
  }
  return state;
}

bool OutputConfigurator::CycleDisplayMode() {
  VLOG(1) << "CycleDisplayMode";
  bool did_change = false;
  if (is_running_on_chrome_os_) {
    Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
    CHECK(display != NULL);
    XGrabServer(display);
    Window window = DefaultRootWindow(display);
    XRRScreenResources* screen = XRRGetScreenResources(display, window);
    CHECK(screen != NULL);

    // Rules:
    // - if there are 0 or 1 displays, do nothing and return false.
    // - use y-coord of CRTCs to determine if we are mirror, primary-first, or
    // secondary-first.  The cycle order is:
    //   mirror->primary->secondary->mirror.
    State new_state = STATE_INVALID;
    switch (output_state_) {
      case STATE_DUAL_MIRROR:
        new_state = STATE_DUAL_PRIMARY_ONLY;
        break;
      case STATE_DUAL_PRIMARY_ONLY:
        new_state = STATE_DUAL_SECONDARY_ONLY;
        break;
      case STATE_DUAL_SECONDARY_ONLY:
        new_state = mirror_supported_ ?
            STATE_DUAL_MIRROR :
            STATE_DUAL_PRIMARY_ONLY;
        break;
      default:
        // Do nothing - we aren't in a mode which we can rotate.
        break;
    }
    if (STATE_INVALID != new_state) {
      UpdateCacheAndXrandrToState(display,
                                  screen,
                                  window,
                                  new_state);
      did_change = true;
    }

    XRRFreeScreenResources(screen);
    XUngrabServer(display);
  }
  return did_change;
}

bool OutputConfigurator::ScreenPowerSet(bool power_on, bool all_displays) {
  VLOG(1) << "OutputConfigurator::SetScreensOn " << power_on
          << " all displays " << all_displays;
  bool success = false;
  if (is_running_on_chrome_os_) {
    Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
    CHECK(display != NULL);
    XGrabServer(display);
    Window window = DefaultRootWindow(display);
    XRRScreenResources* screen = XRRGetScreenResources(display, window);
    CHECK(screen != NULL);

    // Set the CRTCs based on whether we want to turn the power on or off and
    // select the outputs to operate on by name or all_displays.
    for (int i = 0; i < output_count_; ++i) {
      if (all_displays || output_cache_[i].is_internal) {
        const int x = output_cache_[i].x;
        const int y = output_cache_[i].y;
        RROutput output = output_cache_[i].output;
        RRCrtc crtc = output_cache_[i].crtc;
        RRMode mode = None;
        if (power_on) {
          mode = (STATE_DUAL_MIRROR == output_state_) ?
              output_cache_[i].mirror_mode :
              output_cache_[i].ideal_mode;
        }

        VLOG(1) << "SET POWER crtc: " << crtc
                << ", mode " << mode
                << ", output " << output
                << ", x " << x
                << ", y " << y;
        ConfigureCrtc(display,
                      screen,
                      crtc,
                      x,
                      y,
                      mode,
                      output);
        output_cache_[i].is_powered_on = power_on;
        success = true;
      }
    }

    // Force the DPMS on since the driver doesn't always detect that it should
    // turn on.
    if (power_on) {
      CHECK(DPMSEnable(display));
      CHECK(DPMSForceLevel(display, DPMSModeOn));
    }

    XRRFreeScreenResources(screen);
    XUngrabServer(display);
  }
  return success;
}

bool OutputConfigurator::Dispatch(const base::NativeEvent& event) {
  // Ignore this event if the Xrandr extension isn't supported.
  if (is_running_on_chrome_os_ &&
      (event->type - xrandr_event_base_ == RRNotify)) {
    XEvent* xevent = static_cast<XEvent*>(event);
    XRRNotifyEvent* notify_event =
        reinterpret_cast<XRRNotifyEvent*>(xevent);
    if (notify_event->subtype == RRNotify_OutputChange) {
      XRROutputChangeNotifyEvent* output_change_event =
          reinterpret_cast<XRROutputChangeNotifyEvent*>(xevent);
      if ((output_change_event->connection == RR_Connected) ||
          (output_change_event->connection == RR_Disconnected)) {
        RecacheAndUseDefaultState();
      }
      // Ignore the case of RR_UnkownConnection.
    }
  }
  return true;
}

}  // namespace chromeos

