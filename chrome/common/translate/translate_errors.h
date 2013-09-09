// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_TRANSLATE_TRANSLATE_ERRORS_H_
#define CHROME_COMMON_TRANSLATE_TRANSLATE_ERRORS_H_

// This file consolidates all the error types for translation of a page.
// Note: TranslateErrors is used for UMA and translate_internals.js.
// Assigned numbers should be changed because the number is binded to UMA value.
// enum TranslateError in histograms.xml and errorStrs in translate_internals.js
// should be updated when the type is updated.

class TranslateErrors {
 public:
  enum Type {
    NONE = 0,
    NETWORK,                  // No connectivity.
    INITIALIZATION_ERROR,     // The translation script failed to initialize.
    UNKNOWN_LANGUAGE,         // The page's language could not be detected.
    UNSUPPORTED_LANGUAGE,     // The server detected a language that the browser
                              // does not know.
    IDENTICAL_LANGUAGES,      // The original and target languages are the same.
    TRANSLATION_ERROR,        // An error was reported by the translation script
                              // during translation.
    TRANSLATION_TIMEOUT,      // The library doesn't finish the translation.
    UNEXPECTED_SCRIPT_ERROR,  // The library raises an unexpected exception.
    BAD_ORIGIN,               // The library is blocked because of bad origin.
    SCRIPT_LOAD_ERROR,        // Loader fails to load a dependent JavaScript.
    TRANSLATE_ERROR_MAX,
  };

 private:
  TranslateErrors() {}

  DISALLOW_COPY_AND_ASSIGN(TranslateErrors);
};

#endif  // CHROME_COMMON_TRANSLATE_TRANSLATE_ERRORS_H_
