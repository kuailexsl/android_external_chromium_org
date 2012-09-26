// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Callback from the backend with the list of variations to display.
 * This call will build the variations section of the version page, or hide that
 * section if there are none to display.
 * @param {!Array.<string>} variationsList The list of variations.
 */
function returnVariationInfo(variationsList) {
  $('variations-section').hidden = !variationsList.length;
  $('variations-list').appendChild(
      parseHtmlSubset(variationsList.join('<br>'), ['BR']));
}

/**
 * Callback from the backend with the executable and profile paths to display.
 * @param {string} execPath The executable path to display.
 * @param {string} profilePath The profile path to display.
 */
function returnFilePaths(execPath, profilePath) {
  $('executable_path').textContent = execPath;
  $('profile_path').textContent = profilePath;
}

/**
 * Callback from the backend with the Flash version to display.
 * @param {string} flashVersion The Flash version to display.
 */
function returnFlashVersion(flashVersion) {
  $('flash_version').textContent = flashVersion;
}

/**
 * Callback from the backend with the OS version to display.
 * @param {string} osVersion The OS version to display.
 */
function returnOsVersion(osVersion) {
  $('os_version').textContent = osVersion;
}

/* All the work we do onload. */
function onLoadWork() {
  chrome.send('requestVersionInfo');
}

document.addEventListener('DOMContentLoaded', onLoadWork);
