# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import distutils.spawn
import re
import subprocess

from telemetry.core.platform import desktop_platform_backend
from telemetry.core.platform import ps_util


class PosixPlatformBackend(desktop_platform_backend.DesktopPlatformBackend):

  # This is an abstract class. It is OK to have abstract methods.
  # pylint: disable=W0223

  def _RunCommand(self, args):
    return subprocess.Popen(args, stdout=subprocess.PIPE).communicate()[0]

  def _GetFileContents(self, path):
    with open(path, 'r') as f:
      return f.read()

  def _GetPsOutput(self, columns, pid=None):
    """Returns output of the 'ps' command as a list of lines.
    Subclass should override this function.

    Args:
      columns: A list of require columns, e.g., ['pid', 'pss'].
      pid: If nont None, returns only the information of the process
         with the pid.
    """
    args = ['ps']
    args.extend(['-p', str(pid)] if pid != None else ['-e'])
    for c in columns:
      args.extend(['-o', c + '='])
    return self._RunCommand(args).splitlines()

  def GetChildPids(self, pid):
    """Returns a list of child pids of |pid|."""
    ps_output = self._GetPsOutput(['pid', 'ppid', 'state'])
    ps_line_re = re.compile(
        '\s*(?P<pid>\d+)\s*(?P<ppid>\d+)\s*(?P<state>\S*)\s*')
    processes = []
    for pid_ppid_state in ps_output:
      m = ps_line_re.match(pid_ppid_state)
      assert m, 'Did not understand ps output: %s' % pid_ppid_state
      processes.append((m.group('pid'), m.group('ppid'), m.group('state')))
    return ps_util.GetChildPids(processes, pid)

  def GetCommandLine(self, pid):
    command = self._GetPsOutput(['command'], pid)
    return command[0] if command else None

  def GetFlushUtilityName(self):
    return 'clear_system_cache'

  def CanRunApplication(self, application):
    return bool(distutils.spawn.find_executable(application))
