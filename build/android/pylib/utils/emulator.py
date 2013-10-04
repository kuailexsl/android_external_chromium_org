#!/usr/bin/env python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides an interface to start and stop Android emulator.

Assumes system environment ANDROID_NDK_ROOT has been set.

  Emulator: The class provides the methods to launch/shutdown the emulator with
            the android virtual device named 'avd_armeabi' .
"""

import logging
import os
import shutil
import signal
import subprocess
import sys
import time

import time_profile
# TODO(craigdh): Move these pylib dependencies to pylib/utils/.
from pylib import android_commands
from pylib import cmd_helper
from pylib import constants
from pylib import pexpect

import errors
import run_command

# Android API level
API_TARGET = 'android-%s' % constants.ANDROID_SDK_VERSION

# SD card size
SDCARD_SIZE = '512M'

class EmulatorLaunchException(Exception):
  """Emulator failed to launch."""
  pass

def _KillAllEmulators():
  """Kill all running emulators that look like ones we started.

  There are odd 'sticky' cases where there can be no emulator process
  running but a device slot is taken.  A little bot trouble and and
  we're out of room forever.
  """
  emulators = android_commands.GetAttachedDevices(hardware=False)
  if not emulators:
    return
  for emu_name in emulators:
    cmd_helper.RunCmd(['adb', '-s', emu_name, 'emu', 'kill'])
  logging.info('Emulator killing is async; give a few seconds for all to die.')
  for i in range(5):
    if not android_commands.GetAttachedDevices(hardware=False):
      return
    time.sleep(1)


def DeleteAllTempAVDs():
  """Delete all temporary AVDs which are created for tests.

  If the test exits abnormally and some temporary AVDs created when testing may
  be left in the system. Clean these AVDs.
  """
  avds = android_commands.GetAVDs()
  if not avds:
    return
  for avd_name in avds:
    if 'run_tests_avd' in avd_name:
      cmd = ['android', '-s', 'delete', 'avd', '--name', avd_name]
      cmd_helper.RunCmd(cmd)
      logging.info('Delete AVD %s' % avd_name)


class PortPool(object):
  """Pool for emulator port starting position that changes over time."""
  _port_min = 5554
  _port_max = 5585
  _port_current_index = 0

  @classmethod
  def port_range(cls):
    """Return a range of valid ports for emulator use.

    The port must be an even number between 5554 and 5584.  Sometimes
    a killed emulator "hangs on" to a port long enough to prevent
    relaunch.  This is especially true on slow machines (like a bot).
    Cycling through a port start position helps make us resilient."""
    ports = range(cls._port_min, cls._port_max, 2)
    n = cls._port_current_index
    cls._port_current_index = (n + 1) % len(ports)
    return ports[n:] + ports[:n]


def _GetAvailablePort():
  """Returns an available TCP port for the console."""
  used_ports = []
  emulators = android_commands.GetAttachedDevices(hardware=False)
  for emulator in emulators:
    used_ports.append(emulator.split('-')[1])
  for port in PortPool.port_range():
    if str(port) not in used_ports:
      return port


def LaunchEmulators(emulator_count, abi, wait_for_boot=True):
  """Launch multiple emulators and wait for them to boot.

  Args:
    emulator_count: number of emulators to launch.
    abi: the emulator target platform
    wait_for_boot: whether or not to wait for emulators to boot up

  Returns:
    List of emulators.
  """
  emulators = []
  for n in xrange(emulator_count):
    t = time_profile.TimeProfile('Emulator launch %d' % n)
    # Creates a temporary AVD.
    avd_name = 'run_tests_avd_%d' % n
    logging.info('Emulator launch %d with avd_name=%s', n, avd_name)
    emulator = Emulator(avd_name, abi)
    emulator.Launch(kill_all_emulators=n == 0)
    t.Stop()
    emulators.append(emulator)
  # Wait for all emulators to boot completed.
  if wait_for_boot:
    for emulator in emulators:
      emulator.ConfirmLaunch(True)
  return emulators


class Emulator(object):
  """Provides the methods to launch/shutdown the emulator.

  The emulator has the android virtual device named 'avd_armeabi'.

  The emulator could use any even TCP port between 5554 and 5584 for the
  console communication, and this port will be part of the device name like
  'emulator-5554'. Assume it is always True, as the device name is the id of
  emulator managed in this class.

  Attributes:
    emulator: Path of Android's emulator tool.
    popen: Popen object of the running emulator process.
    device: Device name of this emulator.
  """

  # Signals we listen for to kill the emulator on
  _SIGNALS = (signal.SIGINT, signal.SIGHUP)

  # Time to wait for an emulator launch, in seconds.  This includes
  # the time to launch the emulator and a wait-for-device command.
  _LAUNCH_TIMEOUT = 120

  # Timeout interval of wait-for-device command before bouncing to a a
  # process life check.
  _WAITFORDEVICE_TIMEOUT = 5

  # Time to wait for a "wait for boot complete" (property set on device).
  _WAITFORBOOT_TIMEOUT = 300

  def __init__(self, avd_name, abi):
    """Init an Emulator.

    Args:
      avd_name: name of the AVD to create
      abi: target platform for emulator being created
    """
    android_sdk_root = os.path.join(constants.EMULATOR_SDK_ROOT, 'sdk')
    self.emulator = os.path.join(android_sdk_root, 'tools', 'emulator')
    self.android = os.path.join(android_sdk_root, 'tools', 'android')
    self.popen = None
    self.device = None
    self.abi = abi
    self.avd_name = avd_name
    self._CreateAVD()

  def _DeviceName(self):
    """Return our device name."""
    port = _GetAvailablePort()
    return ('emulator-%d' % port, port)

  def _CreateAVD(self):
    """Creates an AVD with the given name.

    Return avd_name.
    """

    if self.abi == 'arm':
      abi_option = 'armeabi-v7a'
    else:
      abi_option = 'x86'

    avd_command = [
        self.android,
        '--silent',
        'create', 'avd',
        '--name', self.avd_name,
        '--abi', abi_option,
        '--target', API_TARGET,
        '--sdcard', SDCARD_SIZE,
        '--force',
    ]
    avd_cmd_str = ' '.join(avd_command)
    logging.info('Create AVD command: %s', avd_cmd_str)
    avd_process = pexpect.spawn(avd_cmd_str)

    # Instead of creating a custom profile, we overwrite config files.
    avd_process.expect('Do you wish to create a custom hardware profile')
    avd_process.sendline('no\n')
    avd_process.expect('Created AVD \'%s\'' % self.avd_name)

    # Setup test device as default Galaxy Nexus AVD
    avd_config_dir = os.path.join(constants.DIR_SOURCE_ROOT, 'build', 'android',
                                  'avd_configs')
    avd_config_ini = os.path.join(avd_config_dir,
                                  'AVD_for_Galaxy_Nexus_by_Google_%s.avd' %
                                  self.abi, 'config.ini')

    # Replace current configuration with default Galaxy Nexus config.
    avds_dir = os.path.join(os.path.expanduser('~'), '.android', 'avd')
    ini_file = os.path.join(avds_dir, '%s.ini' % self.avd_name)
    new_config_ini = os.path.join(avds_dir, '%s.avd' % self.avd_name,
                                  'config.ini')

    # Remove config files with defaults to replace with Google's GN settings.
    os.unlink(ini_file)
    os.unlink(new_config_ini)

    # Create new configuration files with Galaxy Nexus by Google settings.
    with open(ini_file, 'w') as new_ini:
      new_ini.write('avd.ini.encoding=ISO-8859-1\n')
      new_ini.write('target=%s\n' % API_TARGET)
      new_ini.write('path=%s/%s.avd\n' % (avds_dir, self.avd_name))
      new_ini.write('path.rel=avd/%s.avd\n' % self.avd_name)

    shutil.copy(avd_config_ini, new_config_ini)
    return self.avd_name


  def _DeleteAVD(self):
    """Delete the AVD of this emulator."""
    avd_command = [
        self.android,
        '--silent',
        'delete',
        'avd',
        '--name', self.avd_name,
    ]
    logging.info('Delete AVD command: %s', ' '.join(avd_command))
    cmd_helper.RunCmd(avd_command)


  def Launch(self, kill_all_emulators):
    """Launches the emulator asynchronously. Call ConfirmLaunch() to ensure the
    emulator is ready for use.

    If fails, an exception will be raised.
    """
    if kill_all_emulators:
      _KillAllEmulators()  # just to be sure
    self._AggressiveImageCleanup()
    (self.device, port) = self._DeviceName()
    emulator_command = [
        self.emulator,
        # Speed up emulator launch by 40%.  Really.
        '-no-boot-anim',
        # The default /data size is 64M.
        # That's not enough for 8 unit test bundles and their data.
        '-partition-size', '512',
        # Use a familiar name and port.
        '-avd', self.avd_name,
        '-port', str(port),
        # Wipe the data.  We've seen cases where an emulator gets 'stuck' if we
        # don't do this (every thousand runs or so).
        '-wipe-data',
        # Enable GPU by default.
        '-gpu', 'on',
        '-qemu', '-m', '1024',
        ]
    if self.abi == 'x86':
      emulator_command.extend([
          # For x86 emulator --enable-kvm will fail early, avoiding accidental
          # runs in a slow mode (i.e. without hardware virtualization support).
          '--enable-kvm',
          ])

    logging.info('Emulator launch command: %s', ' '.join(emulator_command))
    self.popen = subprocess.Popen(args=emulator_command,
                                  stderr=subprocess.STDOUT)
    self._InstallKillHandler()

  def _AggressiveImageCleanup(self):
    """Aggressive cleanup of emulator images.

    Experimentally it looks like our current emulator use on the bot
    leaves image files around in /tmp/android-$USER.  If a "random"
    name gets reused, we choke with a 'File exists' error.
    TODO(jrg): is there a less hacky way to accomplish the same goal?
    """
    logging.info('Aggressive Image Cleanup')
    emulator_imagedir = '/tmp/android-%s' % os.environ['USER']
    if not os.path.exists(emulator_imagedir):
      return
    for image in os.listdir(emulator_imagedir):
      full_name = os.path.join(emulator_imagedir, image)
      if 'emulator' in full_name:
        logging.info('Deleting emulator image %s', full_name)
        os.unlink(full_name)

  def ConfirmLaunch(self, wait_for_boot=False):
    """Confirm the emulator launched properly.

    Loop on a wait-for-device with a very small timeout.  On each
    timeout, check the emulator process is still alive.
    After confirming a wait-for-device can be successful, make sure
    it returns the right answer.
    """
    seconds_waited = 0
    number_of_waits = 2  # Make sure we can wfd twice
    adb_cmd = "adb -s %s %s" % (self.device, 'wait-for-device')
    while seconds_waited < self._LAUNCH_TIMEOUT:
      try:
        run_command.RunCommand(adb_cmd,
                               timeout_time=self._WAITFORDEVICE_TIMEOUT,
                               retry_count=1)
        number_of_waits -= 1
        if not number_of_waits:
          break
      except errors.WaitForResponseTimedOutError as e:
        seconds_waited += self._WAITFORDEVICE_TIMEOUT
        adb_cmd = "adb -s %s %s" % (self.device, 'kill-server')
        run_command.RunCommand(adb_cmd)
      self.popen.poll()
      if self.popen.returncode != None:
        raise EmulatorLaunchException('EMULATOR DIED')
    if seconds_waited >= self._LAUNCH_TIMEOUT:
      raise EmulatorLaunchException('TIMEOUT with wait-for-device')
    logging.info('Seconds waited on wait-for-device: %d', seconds_waited)
    if wait_for_boot:
      # Now that we checked for obvious problems, wait for a boot complete.
      # Waiting for the package manager is sometimes problematic.
      a = android_commands.AndroidCommands(self.device)
      a.WaitForSystemBootCompleted(self._WAITFORBOOT_TIMEOUT)

  def Shutdown(self):
    """Shuts down the process started by launch."""
    self._DeleteAVD()
    if self.popen:
      self.popen.poll()
      if self.popen.returncode == None:
        self.popen.kill()
      self.popen = None

  def _ShutdownOnSignal(self, signum, frame):
    logging.critical('emulator _ShutdownOnSignal')
    for sig in self._SIGNALS:
      signal.signal(sig, signal.SIG_DFL)
    self.Shutdown()
    raise KeyboardInterrupt  # print a stack

  def _InstallKillHandler(self):
    """Install a handler to kill the emulator when we exit unexpectedly."""
    for sig in self._SIGNALS:
      signal.signal(sig, self._ShutdownOnSignal)
