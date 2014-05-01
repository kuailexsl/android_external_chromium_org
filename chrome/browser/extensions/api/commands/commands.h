// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_COMMANDS_COMMANDS_H_
#define CHROME_BROWSER_EXTENSIONS_API_COMMANDS_COMMANDS_H_

#include "chrome/browser/extensions/chrome_extension_function.h"

class GetAllCommandsFunction : public ChromeSyncExtensionFunction {
  virtual ~GetAllCommandsFunction() {}
  virtual bool RunSync() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION("commands.getAll", COMMANDS_GETALL)
};

#endif  // CHROME_BROWSER_EXTENSIONS_API_COMMANDS_COMMANDS_H_
