# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.core.backends.chrome import android_browser_finder
from telemetry.core.platform import profiler


class UnableToFindApplicationException(Exception):
  """Exception when unable to find a launched application"""
  pass


class OOMKillerProfiler(profiler.Profiler):
  """Android-specific, Launch the music application and check it is still alive
  at the end of the run."""

  def __init__(self, browser_backend, platform_backend, output_path, state):
    super(OOMKillerProfiler, self).__init__(
        browser_backend, platform_backend, output_path, state)
    if not 'mem_consumer_launched' in state:
      state['mem_consumer_launched'] = True
      self._browser_backend.adb.GoHome()
      self._platform_backend.LaunchApplication(
          'org.chromium.memconsumer/.MemConsumer',
          '--ei memory 10')
      # Bring the browser to the foreground after launching the mem consumer
      self._browser_backend.adb.StartActivity(browser_backend.package,
                                              browser_backend.activity,
                                              True)

  @classmethod
  def name(cls):
    return 'oomkiller'

  @classmethod
  def is_supported(cls, browser_type):
    if browser_type == 'any':
      return android_browser_finder.CanFindAvailableBrowsers()
    return browser_type.startswith('android')

  def CollectProfile(self):
    if not self._AreProcessRunnings():
      raise UnableToFindApplicationException()
    return []

  def _AreProcessRunnings(self):
    must_have_apps = [
        'org.chromium.memconsumer',
        'com.android.launcher',
    ]
    return all([self._platform_backend.IsApplicationRunning(app)
                for app in must_have_apps])
