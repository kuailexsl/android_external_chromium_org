// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/extension_urls.h"

#include "base/strings/utf_string_conversions.h"
#include "extensions/common/constants.h"
#include "url/gurl.h"

namespace extensions {

const char kEventBindings[] = "event_bindings";

const char kSchemaUtils[] = "schemaUtils";

bool IsSourceFromAnExtension(const base::string16& source) {
  return GURL(source).SchemeIs(kExtensionScheme) ||
         source == base::UTF8ToUTF16(kEventBindings) ||
         source == base::UTF8ToUTF16(kSchemaUtils);
}

}  // namespace extensions
