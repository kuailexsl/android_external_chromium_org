// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_I18N_I18N_API_H__
#define CHROME_BROWSER_EXTENSIONS_API_I18N_I18N_API_H__

#include "chrome/browser/extensions/extension_function.h"

class GetAcceptLanguagesFunction : public SyncExtensionFunction {
  virtual ~GetAcceptLanguagesFunction() {}
  virtual bool RunImpl() OVERRIDE;
  DECLARE_EXTENSION_FUNCTION_NAME("i18n.getAcceptLanguages")
};

#endif  // CHROME_BROWSER_EXTENSIONS_API_I18N_I18N_API_H__
