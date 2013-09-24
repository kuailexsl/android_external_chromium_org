// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom bindings for the feedbackPrivate API.

var binding = require('binding').Binding.create('feedbackPrivate');

var feedbackPrivateNatives = requireNative('feedback_private');

binding.registerCustomHook(function(bindingsAPI) {
  var apiFunctions = bindingsAPI.apiFunctions;
  apiFunctions.setUpdateArgumentsPostValidate(
      "sendFeedback", function(feedbackInfo, callback) {
    var attachedFileBlobUuid = '';
    var screenshotBlobUuid = '';

    if (feedbackInfo.attachedFile)
      attachedFileBlobUuid =
          feedbackPrivateNatives.GetBlobUuid(feedbackInfo.attachedFile.data);
    if (feedbackInfo.screenshot)
      screenshotBlobUuid =
          feedbackPrivateNatives.GetBlobUuid(feedbackInfo.screenshot);

    feedbackInfo.attachedFileBlobUuid = attachedFileBlobUuid;
    feedbackInfo.screenshotBlobUuid = screenshotBlobUuid;

    return [feedbackInfo, callback];
  });
});

exports.binding = binding.generate();
