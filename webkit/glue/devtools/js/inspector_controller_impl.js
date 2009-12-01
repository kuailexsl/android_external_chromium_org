// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview DevTools' implementation of the InspectorController API.
 */

goog.provide('devtools.InspectorBackendImpl');
goog.provide('devtools.InspectorFrontendHostImpl');

devtools.InspectorBackendImpl = function() {
  WebInspector.InspectorBackendStub.call(this);
  this.installInspectorControllerDelegate_('clearMessages');
  this.installInspectorControllerDelegate_('copyNode');
  this.installInspectorControllerDelegate_('deleteCookie');
  this.installInspectorControllerDelegate_('disableResourceTracking');
  this.installInspectorControllerDelegate_('disableTimeline');
  this.installInspectorControllerDelegate_('enableResourceTracking');
  this.installInspectorControllerDelegate_('enableTimeline');
  this.installInspectorControllerDelegate_('getChildNodes');
  this.installInspectorControllerDelegate_('getCookies');
  this.installInspectorControllerDelegate_('getDatabaseTableNames');
  this.installInspectorControllerDelegate_('getDOMStorageEntries');
  this.installInspectorControllerDelegate_('getEventListenersForNode');
  this.installInspectorControllerDelegate_('highlightDOMNode');
  this.installInspectorControllerDelegate_('hideDOMNodeHighlight');
  this.installInspectorControllerDelegate_('releaseWrapperObjectGroup');
  this.installInspectorControllerDelegate_('removeAttribute');
  this.installInspectorControllerDelegate_('removeDOMStorageItem');
  this.installInspectorControllerDelegate_('removeNode');
  this.installInspectorControllerDelegate_('setAttribute');
  this.installInspectorControllerDelegate_('setDOMStorageItem');
  this.installInspectorControllerDelegate_('setTextNodeValue');
  this.installInspectorControllerDelegate_('startTimelineProfiler');
  this.installInspectorControllerDelegate_('stopTimelineProfiler');
  this.installInspectorControllerDelegate_('storeLastActivePanel');
};
goog.inherits(devtools.InspectorBackendImpl,
    WebInspector.InspectorBackendStub);


/**
 * {@inheritDoc}.
 */
devtools.InspectorBackendImpl.prototype.toggleNodeSearch = function() {
  WebInspector.InspectorBackendStub.prototype.toggleNodeSearch.call(this);
  this.callInspectorController_.call(this, 'toggleNodeSearch');
  if (!this.searchingForNode()) {
    // This is called from ElementsPanel treeOutline's focusNodeChanged().
    DevToolsHost.activateWindow();
  }
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.debuggerEnabled = function() {
  return true;
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.profilerEnabled = function() {
  return true;
};


devtools.InspectorBackendImpl.prototype.addBreakpoint = function(
    sourceID, line, condition) {
  devtools.tools.getDebuggerAgent().addBreakpoint(sourceID, line, condition);
};


devtools.InspectorBackendImpl.prototype.removeBreakpoint = function(
    sourceID, line) {
  devtools.tools.getDebuggerAgent().removeBreakpoint(sourceID, line);
};

devtools.InspectorBackendImpl.prototype.updateBreakpoint = function(
    sourceID, line, condition) {
  devtools.tools.getDebuggerAgent().updateBreakpoint(
      sourceID, line, condition);
};

devtools.InspectorBackendImpl.prototype.pauseInDebugger = function() {
  devtools.tools.getDebuggerAgent().pauseExecution();
};


devtools.InspectorBackendImpl.prototype.resumeDebugger = function() {
  devtools.tools.getDebuggerAgent().resumeExecution();
};


devtools.InspectorBackendImpl.prototype.stepIntoStatementInDebugger =
    function() {
  devtools.tools.getDebuggerAgent().stepIntoStatement();
};


devtools.InspectorBackendImpl.prototype.stepOutOfFunctionInDebugger =
    function() {
  devtools.tools.getDebuggerAgent().stepOutOfFunction();
};


devtools.InspectorBackendImpl.prototype.stepOverStatementInDebugger =
    function() {
  devtools.tools.getDebuggerAgent().stepOverStatement();
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.pauseOnExceptions = function() {
  return devtools.tools.getDebuggerAgent().pauseOnExceptions();
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.setPauseOnExceptions = function(
    value) {
  return devtools.tools.getDebuggerAgent().setPauseOnExceptions(value);
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.startProfiling = function() {
  devtools.tools.getDebuggerAgent().startProfiling(
      devtools.DebuggerAgent.ProfilerModules.PROFILER_MODULE_CPU);
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.stopProfiling = function() {
  devtools.tools.getDebuggerAgent().stopProfiling(
      devtools.DebuggerAgent.ProfilerModules.PROFILER_MODULE_CPU);
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.getProfileHeaders = function(callId) {
  WebInspector.didGetProfileHeaders(callId, []);
};


/**
 * Emulate WebKit InspectorController behavior. It stores profiles on renderer side,
 * and is able to retrieve them by uid using 'getProfile'.
 */
devtools.InspectorBackendImpl.prototype.addFullProfile = function(profile) {
  WebInspector.__fullProfiles = WebInspector.__fullProfiles || {};
  WebInspector.__fullProfiles[profile.uid] = profile;
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.getProfile = function(callId, uid) {
  if (WebInspector.__fullProfiles && (uid in WebInspector.__fullProfiles)) {
    WebInspector.didGetProfile(callId, WebInspector.__fullProfiles[uid]);
  }
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.takeHeapSnapshot = function() {
  devtools.tools.getDebuggerAgent().startProfiling(
      devtools.DebuggerAgent.ProfilerModules.PROFILER_MODULE_HEAP_SNAPSHOT
      | devtools.DebuggerAgent.ProfilerModules.PROFILER_MODULE_HEAP_STATS
      | devtools.DebuggerAgent.ProfilerModules.PROFILER_MODULE_JS_CONSTRUCTORS);
};


/**
 * @override
 */
devtools.InspectorBackendImpl.prototype.dispatchOnInjectedScript = function(
    callId, methodName, argsString, async) {
  var callback = function(result, isException) {
    WebInspector.didDispatchOnInjectedScript(callId, result, isException);
  };
  RemoteToolsAgent.DispatchOnInjectedScript(
      WebInspector.Callback.wrap(callback),
      async ? methodName + "_async" : methodName,
      argsString);
};


/**
 * Installs delegating handler into the inspector controller.
 * @param {string} methodName Method to install delegating handler for.
 */
devtools.InspectorBackendImpl.prototype.installInspectorControllerDelegate_
    = function(methodName) {
  this[methodName] = goog.bind(this.callInspectorController_, this,
      methodName);
};


/**
 * Bound function with the installInjectedScriptDelegate_ actual
 * implementation.
 */
devtools.InspectorBackendImpl.prototype.callInspectorController_ =
    function(methodName, var_arg) {
  var args = Array.prototype.slice.call(arguments, 1);
  RemoteToolsAgent.DispatchOnInspectorController(
      WebInspector.Callback.wrap(function(){}),
      methodName,
      JSON.stringify(args));
};


InspectorBackend = new devtools.InspectorBackendImpl();
