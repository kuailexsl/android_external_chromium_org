# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os

from telemetry import test as test_module
from telemetry.core import exceptions
from telemetry.core import util
from telemetry.page import page
from telemetry.page import page_set
# pylint: disable=W0401,W0614
from telemetry.page import page_test
from telemetry.page.actions.all_page_actions import *

data_path = os.path.join(
    util.GetChromiumSrcDir(), 'content', 'test', 'data', 'gpu')

wait_timeout = 20  # seconds

harness_script = r"""
  var domAutomationController = {};

  domAutomationController._loaded = false;
  domAutomationController._succeeded = false;
  domAutomationController._finished = false;

  domAutomationController.setAutomationId = function(id) {}

  domAutomationController.send = function(msg) {
    msg = msg.toLowerCase()
    if (msg == "loaded") {
      domAutomationController._loaded = true;
    } else if (msg == "success") {
      domAutomationController._succeeded = true;
      domAutomationController._finished = true;
    } else {
      domAutomationController._succeeded = false;
      domAutomationController._finished = true;
    }
  }

  window.domAutomationController = domAutomationController;
  console.log("Harness injected.");
"""

class _ContextLostValidator(page_test.PageTest):
  def __init__(self):
    # Strictly speaking this test doesn't yet need a browser restart
    # after each run, but if more tests are added which crash the GPU
    # process, then it will.
    super(_ContextLostValidator, self).__init__(
      'ValidatePage', needs_browser_restart_after_each_page=True)

  def CustomizeBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(
        '--disable-domain-blocking-for-3d-apis')
    options.AppendExtraBrowserArgs(
        '--disable-gpu-process-crash-limit')
    # Required for about:gpucrash handling from Telemetry.
    options.AppendExtraBrowserArgs('--enable-gpu-benchmarking')

  def ValidatePage(self, page, tab, results):
    if page.kill_gpu_process:
      # Doing the GPU process kill operation cooperatively -- in the
      # same page's context -- is much more stressful than restarting
      # the browser every time.
      for x in range(page.number_of_gpu_process_kills):
        if not tab.browser.supports_tab_control:
          raise page_test.Failure('Browser must support tab control')
        # Reset the test's state.
        tab.EvaluateJavaScript(
          'window.domAutomationController._succeeded = false');
        tab.EvaluateJavaScript(
          'window.domAutomationController._finished = false');
        # Crash the GPU process.
        new_tab = tab.browser.tabs.New()
        # To access these debug URLs from Telemetry, they have to be
        # written using the chrome:// scheme.
        # The try/except is a workaround for crbug.com/368107.
        try:
          new_tab.Navigate('chrome://gpucrash')
        except (exceptions.TabCrashException, Exception):
          print 'Tab crashed while navigating to chrome://gpucrash'
        # Activate the original tab and wait for completion.
        tab.Activate()
        completed = False
        try:
          util.WaitFor(lambda: tab.EvaluateJavaScript(
              'window.domAutomationController._finished'), wait_timeout)
          completed = True
        except util.TimeoutException:
          pass
        # The try/except is a workaround for crbug.com/368107.
        try:
          new_tab.Close()
        except (exceptions.TabCrashException, Exception):
          print 'Tab crashed while closing chrome://gpucrash'
        if not completed:
          raise page_test.Failure(
              'Test didn\'t complete (no context lost event?)')
        if not tab.EvaluateJavaScript(
          'window.domAutomationController._succeeded'):
          raise page_test.Failure(
            'Test failed (context not restored properly?)')

class WebGLContextLostFromGPUProcessExitPage(page.Page):
  def __init__(self, page_set, base_dir):
    super(WebGLContextLostFromGPUProcessExitPage, self).__init__(
      url='file://webgl.html?query=kill_after_notification',
      page_set=page_set,
      base_dir=base_dir)
    self.name = 'ContextLost.WebGLContextLostFromGPUProcessExit'
    self.script_to_evaluate_on_commit = harness_script
    self.kill_gpu_process = True
    self.number_of_gpu_process_kills = 1

  def RunNavigateSteps(self, action_runner):
    action_runner.RunAction(NavigateAction())
    action_runner.RunAction(WaitAction(
      {'javascript': 'window.domAutomationController._loaded'}))


class WebGLContextLostFromLoseContextExtensionPage(page.Page):
  def __init__(self, page_set, base_dir):
    super(WebGLContextLostFromLoseContextExtensionPage, self).__init__(
      url='file://webgl.html?query=WEBGL_lose_context',
      page_set=page_set,
      base_dir=base_dir)
    self.name = 'ContextLost.WebGLContextLostFromLoseContextExtension',
    self.script_to_evaluate_on_commit = harness_script
    self.kill_gpu_process = False

  def RunNavigateSteps(self, action_runner):
    action_runner.RunAction(NavigateAction())
    action_runner.RunAction(WaitAction(
      {'javascript': 'window.domAutomationController._finished'}))


class ContextLost(test_module.Test):
  enabled = True
  test = _ContextLostValidator
  # For the record, this would have been another way to get the pages
  # to repeat. pageset_repeat would be another option.
  # options = {'page_repeat': 5}
  def CreatePageSet(self, options):
    ps = page_set.PageSet(
      file_path=data_path,
      description='Test cases for real and synthetic context lost events',
      user_agent_type='desktop',
      serving_dirs=set(['']))
    ps.AddPage(WebGLContextLostFromGPUProcessExitPage(ps, ps.base_dir))
    ps.AddPage(WebGLContextLostFromLoseContextExtensionPage(ps, ps.base_dir))
    return ps
