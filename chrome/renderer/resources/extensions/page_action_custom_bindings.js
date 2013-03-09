// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the pageAction API.

var binding = require('binding').Binding.create('pageAction');

var setIcon = require('setIcon').setIcon;

binding.registerCustomHook(function(bindingsAPI) {
  var apiFunctions = bindingsAPI.apiFunctions;

  apiFunctions.setHandleRequest('setIcon', function(details, callback) {
    setIcon(details, callback, this.name, this.definition.parameters,
        'page action');
  });
});

exports.binding = binding.generate();
