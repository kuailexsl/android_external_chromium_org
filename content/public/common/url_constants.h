// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_URL_CONSTANTS_H_
#define CONTENT_PUBLIC_COMMON_URL_CONSTANTS_H_
#pragma once

#include "content/common/content_export.h"

// Contains constants for known URLs and portions thereof.

// TODO(jam): rename this to content.
namespace chrome {

// Canonical schemes you can use as input to GURL.SchemeIs().
// TODO(jam): some of these don't below in the content layer, but are accessed
// from there.
CONTENT_EXPORT extern const char kAboutScheme[];
CONTENT_EXPORT extern const char kBlobScheme[];
CONTENT_EXPORT extern const char kChromeDevToolsScheme[];
CONTENT_EXPORT extern const char kChromeInternalScheme[];
CONTENT_EXPORT extern const char kChromeUIScheme[];  // Used for WebUIs.
CONTENT_EXPORT extern const char kCrosScheme[];      // Used for ChromeOS.
CONTENT_EXPORT extern const char kDataScheme[];
CONTENT_EXPORT extern const char kFileScheme[];
CONTENT_EXPORT extern const char kFileSystemScheme[];
CONTENT_EXPORT extern const char kFtpScheme[];
CONTENT_EXPORT extern const char kHttpScheme[];
CONTENT_EXPORT extern const char kHttpsScheme[];
CONTENT_EXPORT extern const char kJavaScriptScheme[];
CONTENT_EXPORT extern const char kMailToScheme[];
CONTENT_EXPORT extern const char kMetadataScheme[];
CONTENT_EXPORT extern const char kSwappedOutScheme[];
CONTENT_EXPORT extern const char kViewSourceScheme[];

// Used to separate a standard scheme and the hostname: "://".
CONTENT_EXPORT extern const char kStandardSchemeSeparator[];

// About URLs (including schemes).
CONTENT_EXPORT extern const char kAboutBlankURL[];
CONTENT_EXPORT extern const char kChromeUIAppCacheInternalsHost[];
CONTENT_EXPORT extern const char kChromeUIBlobInternalsHost[];
CONTENT_EXPORT extern const char kChromeUIBrowserCrashHost[];
CONTENT_EXPORT extern const char kChromeUINetworkViewCacheHost[];
CONTENT_EXPORT extern const char kChromeUICrashURL[];
CONTENT_EXPORT extern const char kChromeUIGpuCleanURL[];
CONTENT_EXPORT extern const char kChromeUIGpuCrashURL[];
CONTENT_EXPORT extern const char kChromeUIGpuHangURL[];
CONTENT_EXPORT extern const char kChromeUIHangURL[];
CONTENT_EXPORT extern const char kChromeUIKillURL[];
CONTENT_EXPORT extern const char kChromeUINetworkViewCacheURL[];
CONTENT_EXPORT extern const char kChromeUIShorthangURL[];

}  // namespace chrome

namespace content {

// Special URL used to start a navigation to an error page.
extern const char kUnreachableWebDataURL[];

// Special URL used to swap out a view being rendered by another process.
extern const char kSwappedOutURL[];

// Null terminated list of schemes that are savable. This function can be
// invoked on any thread.
CONTENT_EXPORT const char** GetSavableSchemes();

// Note: ContentMainRunner calls this method internally as part of main
// initialziation, so this function generally should not be called by
// embedders. It's exported to facilitate test harnesses that do not
// utilize ContentMainRunner and that do not wish to lock the set
// of standard schemes at init time.
//
// Called near the beginning of startup to register URL schemes that should
// be parsed as "standard" with the googleurl library. Optionally, the set
// of standard schemes is locked down. The embedder can add additional
// schemes by overriding the ContentClient::AddAdditionalSchemes method.
CONTENT_EXPORT void RegisterContentSchemes(bool lock_standard_schemes);

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_URL_CONSTANTS_H_
