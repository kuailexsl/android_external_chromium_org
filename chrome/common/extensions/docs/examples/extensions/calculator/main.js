// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.experimental.runtime.onLaunched.addListener(function() {
  chrome.windows.create({
      url: 'calculator.html',
      type: 'shell',
      width: 217,
      height: 223
  });
});
