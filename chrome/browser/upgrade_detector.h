// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UPGRADE_DETECTOR_H_
#define CHROME_BROWSER_UPGRADE_DETECTOR_H_

#include "base/singleton.h"
#include "base/timer.h"

///////////////////////////////////////////////////////////////////////////////
// UpgradeDetector
//
// This class is a singleton class that monitors when an upgrade happens in the
// background. We basically ask Omaha what it thinks the latest version is and
// if our version is lower we send out a notification upon:
//   a) Detecting an upgrade and...
//   b) When we think the user should be notified about the upgrade.
// The latter happens much later, since we don't want to be too annoying.
//
class UpgradeDetector {
 public:
  ~UpgradeDetector();

  bool notify_upgrade() { return notify_upgrade_; }

 private:
  UpgradeDetector();
  friend struct DefaultSingletonTraits<UpgradeDetector>;

  // Checks with Omaha if we have the latest version. If not, sends out a
  // notification and starts a one shot timer to wait until notifying the
  // user.
  void CheckForUpgrade();

  // The function that sends out a notification (after a certain time has
  // elapsed) that lets the rest of the UI know we should start notifying the
  // user that a new version is available.
  void NotifyOnUpgrade();

  // We periodically check to see if Chrome has been upgraded.
  base::RepeatingTimer<UpgradeDetector> detect_upgrade_timer_;

  // After we detect an upgrade we wait a set time before notifying the user.
  base::OneShotTimer<UpgradeDetector> upgrade_notification_timer_;

  // Whether we have detected an upgrade happening while we were running.
  bool upgrade_detected_;

  // Whether we have waited long enough after detecting an upgrade (to see
  // is we should start nagging about upgrading).
  bool notify_upgrade_;

  DISALLOW_COPY_AND_ASSIGN(UpgradeDetector);
};

#endif  // CHROME_BROWSER_UPGRADE_DETECTOR_H_
