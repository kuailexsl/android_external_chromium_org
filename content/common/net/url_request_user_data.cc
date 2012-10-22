// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/net/url_request_user_data.h"

namespace content {

URLRequestUserData::URLRequestUserData(int render_process_id,
                                       int render_view_id)
    : render_process_id_(render_process_id),
      render_view_id_(render_view_id) {}

URLRequestUserData::~URLRequestUserData() {}

// static
const void* URLRequestUserData::kUserDataKey =
    static_cast<const void*>(&URLRequestUserData::kUserDataKey);

}  // namespace content
