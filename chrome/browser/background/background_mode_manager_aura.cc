// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background/background_mode_manager.h"

#include "chrome/common/extensions/extension.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

// No background jobs for aura for now.

void BackgroundModeManager::EnableLaunchOnStartup(bool should_launch) {
  NOTIMPLEMENTED();
}

void BackgroundModeManager::DisplayAppInstalledNotification(
    const extensions::Extension* extension) {
  NOTIMPLEMENTED();
}

string16 BackgroundModeManager::GetPreferencesMenuLabel() {
  return l10n_util::GetStringUTF16(IDS_SETTINGS);
}
