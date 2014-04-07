// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// require cr.js
// require cr/event_target.js
// require cr/util.js

cr.define('chrome.sync', function() {
  'use strict';

  /**
   * A simple timer to measure elapsed time.
   * @constructor
   */
  function Timer() {
    /**
     * The time that this Timer was created.
     * @type {number}
     * @private
     * @const
     */
    this.start_ = Date.now();
  }

  /**
   * @return {number} The elapsed seconds since this Timer was created.
   */
  Timer.prototype.getElapsedSeconds = function() {
    return (Date.now() - this.start_) / 1000;
  };

  /** @return {!Timer} An object which measures elapsed time. */
  var makeTimer = function() {
    return new Timer;
  };

  /**
   * @param {string} name The name of the event type.
   * @param {!Object} details A collection of event-specific details.
   */
  var dispatchEvent = function(name, details) {
    var e = new Event(name);
    e.details = details;
    chrome.sync.events.dispatchEvent(e);
  };

  /**
   * Registers to receive a stream of events through
   * chrome.sync.dispatchEvent().
   */
  var registerForEvents = function() {
    chrome.send('registerForEvents');
  };

  /**
   * Asks the browser to refresh our snapshot of sync state.  Should result
   * in an onAboutInfoUpdated event being emitted.
   */
  var requestUpdatedAboutInfo = function() {
    chrome.send('requestUpdatedAboutInfo');
  };

  /**
   * Asks the browser to send us the list of registered types.  Should result
   * in an onReceivedListOfTypes event being emitted.
   */
  var requestListOfTypes = function() {
    chrome.send('requestListOfTypes');
  };

  /**
   * Counter to uniquely identify requests while they're in progress.
   * Used in the implementation of GetAllNodes.
   */
  var requestId = 0;

  /**
   * A map from counter values to asynchronous request callbacks.
   * Used in the implementation of GetAllNodes.
   * @type {{number: !Function}}
   */
  var requestCallbacks = {};

  /**
   * Asks the browser to send us a copy of all existing sync nodes.
   * Will eventually invoke the given callback with the results.
   *
   * @param {function(!Object)} callback The function to call with the response.
   */
  var getAllNodes = function(callback) {
    requestId++;
    requestCallbacks[requestId] = callback;
    chrome.send('getAllNodes', [requestId]);
  };

  /**
   * Called from C++ with the response to a getAllNodes request.
   * @param {number} id The requestId passed in with the request.
   * @param {Object} response The response to the request.
   */
  var getAllNodesCallback = function(id, response) {
    requestCallbacks[id](response);
    requestCallbacks[id] = undefined;
  };

  return {
    makeTimer: makeTimer,
    dispatchEvent: dispatchEvent,
    events: new cr.EventTarget(),
    getAllNodes: getAllNodes,
    getAllNodesCallback: getAllNodesCallback,
    registerForEvents: registerForEvents,
    requestUpdatedAboutInfo: requestUpdatedAboutInfo,
    requestListOfTypes: requestListOfTypes,
  };
});
