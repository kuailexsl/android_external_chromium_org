// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants related to the Chrome mini installer testing.

#ifndef CHROME_TEST_MINI_INSTALLER_TEST_MINI_INSTALLER_TEST_CONSTANTS_H__
#define CHROME_TEST_MINI_INSTALLER_TEST_MINI_INSTALLER_TEST_CONSTANTS_H__

namespace mini_installer_constants {

// Path and process names
extern const wchar_t kChromeAppDir[];
extern const wchar_t kChromeSetupExecutable[];
extern const wchar_t kChromeMiniInstallerExecutable[];
extern const wchar_t kChromeMetaInstallerExecutable[];
extern const wchar_t kGoogleUpdateExecutable[];
extern const wchar_t kIEExecutable[];

// Window names.
extern const wchar_t kBrowserAppName[];
extern const wchar_t kBrowserTabName[];
extern const wchar_t kChromeBuildType[];
extern const wchar_t kChromeFirstRunUI[];
extern const wchar_t kInstallerWindow[];
extern const wchar_t kChromeUninstallDialogName[];

// Shortcut names
extern const wchar_t kChromeLaunchShortcut[];
extern const wchar_t kChromeUninstallShortcut[];

// Chrome install types
extern const wchar_t kSystemInstall[];
extern const wchar_t kStandaloneInstaller[];
extern const wchar_t kUserInstall[];
extern const wchar_t kUntaggedInstallerPattern[];
extern const wchar_t kDiffInstallerPattern[];
extern const wchar_t kFullInstallerPattern[];
extern const wchar_t kDevChannelBuild[];
extern const wchar_t kStableChannelBuild[];
extern const wchar_t kFullInstall[];
extern const wchar_t kDiffInstall[];

// Google Chrome meta installer location.
extern const wchar_t kChromeApplyTagExe[];
extern const wchar_t kChromeMetaInstallerExe[];
extern const wchar_t kChromeStandAloneInstallerLocation[];
extern const wchar_t kChromeApplyTagParameters[];
extern const wchar_t kChromeDiffInstallerLocation[];
}

#endif  // CHROME_TEST_MINI_INSTALLER_TEST_MINI_INSTALLER_TEST_CONSTANTS_H__
