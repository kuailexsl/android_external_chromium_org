// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

onload = function() {
  var message = {
    messageId: "message-id",
    destinationId: "destination-id",
    timeToLive: 2419200,
    data: {
      "key1": "value1",
      "key2": "value2"
    }
  };

  chrome.gcm.send(message, function(messageId) {});
};
