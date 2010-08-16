// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the constants used to process master_preferences files
// used by setup and first run.

#ifndef CHROME_INSTALLER_UTIL_MASTER_PREFERENCES_CONSTANTS_H_
#define CHROME_INSTALLER_UTIL_MASTER_PREFERENCES_CONSTANTS_H_
#pragma once

namespace installer_util {
namespace master_preferences {
// All the preferences below are expected to be inside the JSON "distribution"
// block. Some of them also have equivalent command line option. If same option
// is specified in master preference as well as command line, the commnd line
// value takes precedence.

// Boolean. Use alternate text for the shortcut. Cmd line override present.
extern const char kAltShortcutText[];
// Boolean. Use alternate smaller first run info bubble.
extern const char kAltFirstRunBubble[];
// Boolean. This is to be a Chrome Frame install.
extern const char kChromeFrame[];
// Integer. Icon index from chrome.exe to use for shortcuts.
extern const char kChromeShortcutIconIndex[];
// Boolean. Create Desktop and QuickLaunch shortcuts. Cmd line override present.
extern const char kCreateAllShortcuts[];
// Boolean pref that triggers silent import of the default browser bookmarks.
extern const char kDistroImportBookmarksPref[];
// String pref that triggers silent import of bookmarks from the html file at
// given path.
extern const char kDistroImportBookmarksFromFilePref[];
// Boolean pref that triggers silent import of the default browser history.
extern const char kDistroImportHistoryPref[];
// Boolean pref that triggers silent import of the default browser homepage.
extern const char kDistroImportHomePagePref[];
// Boolean pref that triggers silent import of the default search engine.
extern const char kDistroImportSearchPref[];
// Integer. RLZ ping delay in seconds.
extern const char kDistroPingDelay[];
// Boolean pref that triggers loading the welcome page.
extern const char kDistroShowWelcomePage[];
// Boolean pref that triggers skipping the first run dialogs.
extern const char kDistroSkipFirstRunPref[];
// Boolean. Do not show first run bubble, even if it would otherwise be shown.
extern const char kDistroSuppressFirstRunBubble[];
// Boolean. Do not create Chrome desktop shortcuts. Cmd line override present.
extern const char kDoNotCreateShortcuts[];
// Boolean. Do not launch Chrome after first install. Cmd line override present.
extern const char kDoNotLaunchChrome[];
// Boolean. Do not register with Google Update to have Chrome launched after
// install. Cmd line override present.
extern const char kDoNotRegisterForUpdateLaunch[];
// Boolean. Register Chrome as default browser. Cmd line override present.
extern const char kMakeChromeDefault[];
// Boolean. Register Chrome as default browser for the current user.
extern const char kMakeChromeDefaultForUser[];
// Boolean. Expect to be run by an MSI installer. Cmd line override present.
extern const char kMsi[];
// Boolean. Show EULA dialog before install.
extern const char kRequireEula[];
// Boolean. Use experimental search engine selection dialog.
extern const char kSearchEngineExperimentPref[];
// Boolean. Randomize logos in experimental search engine selection dialog.
extern const char kSearchEngineExperimentRandomizePref[];
// Boolean. Install Chrome to system wise location. Cmd line override present.
extern const char kSystemLevel[];
// Boolean. Run installer in verbose mode. Cmd line override present.
extern const char kVerboseLogging[];
// Name of the block that contains the extensions on the master preferences.
extern const char kExtensionsBlock[];
}  // namespace master_preferences
}  // namespace installer_util

#endif  // CHROME_INSTALLER_UTIL_MASTER_PREFERENCES_CONSTANTS_H_
