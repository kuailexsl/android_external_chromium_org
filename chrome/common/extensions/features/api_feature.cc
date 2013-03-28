// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/features/api_feature.h"

namespace extensions {

APIFeature::APIFeature() {
}

APIFeature::~APIFeature() {
}

std::string APIFeature::Parse(const DictionaryValue* value) {
  std::string error = SimpleFeature::Parse(value);
  if (!error.empty())
    return error;

  if (GetContexts()->empty())
    return name() + ": API features must specify at least one context.";

  return "";
}

}  // namespace
