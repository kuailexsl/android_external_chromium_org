// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_KIOSK_MODE_KIOSK_MODE_SETTINGS_H_
#define CHROME_BROWSER_CHROMEOS_KIOSK_MODE_KIOSK_MODE_SETTINGS_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/time.h"

namespace base {
template <typename T> struct DefaultLazyInstanceTraits;
}

namespace {

const int kMaxIdleLogoutTimeout = 600000; // ms = 600s = 10m.
const int kMinIdleLogoutTimeout = 5000; // ms = 5s.

const int kMaxIdleLogoutWarningDuration = 60000; // ms = 60s.
const int kMinIdleLogoutWarningDuration = 1000; // ms = 1s.

}  // namespace

namespace chromeos {

// This class centralizes all our code to get KioskMode settings; since
// KioskMode interferes with normal operations all over Chrome, having all
// data about it pulled from a central location would make future
// refactorings easier. This class also handles getting trust for the policies
// via it's init method.
//
// Note: If Initialize is not called before the various Getters, we'll return
// invalid values.
class KioskModeSettings {
 public:
  // This method checks if Kiosk Mode is enabled or not.
  virtual bool IsKioskModeEnabled();

  static KioskModeSettings* Get();

  // Initialize the settings; this will call the callback once trust is
  // established with the policy settings provider.
  virtual void Initialize(const base::Closure& notify_initialized);
  virtual bool is_initialized() const;

  // The path to the screensaver extension.
  virtual std::string GetScreensaverPath() const;
  // The timeout before which we'll start showing the screensaver.
  virtual base::TimeDelta GetScreensaverTimeout() const;

  // NOTE: The idle logout timeout is the time 'till' we show the idle dialog
  // box. After we show the dialog box, it remains up for an 'additional'
  // IdleLogoutWarningTimeout seconds, which adds to the total time before the
  // user is logged out.
  // The time to logout the user in on idle.
  virtual base::TimeDelta GetIdleLogoutTimeout() const;
  // The time to show the countdown timer for.
  virtual base::TimeDelta GetIdleLogoutWarningDuration() const;

 protected:
  // Needed here so MockKioskModeSettings can inherit from us.
  KioskModeSettings();
  virtual ~KioskModeSettings();

 private:
  friend struct base::DefaultLazyInstanceTraits<KioskModeSettings>;
  friend class KioskModeSettingsTest;

  bool is_initialized_;

  // Used for testing.
  void set_initialized(bool value) { is_initialized_ = value; }

  std::string screensaver_id_;
  std::string screensaver_path_;
  base::TimeDelta screensaver_timeout_;
  base::TimeDelta idle_logout_timeout_;
  base::TimeDelta idle_logout_warning_duration_;

  DISALLOW_COPY_AND_ASSIGN(KioskModeSettings);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_KIOSK_MODE_KIOSK_MODE_SETTINGS_H_
