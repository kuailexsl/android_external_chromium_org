# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file is based on gdb_rsp.py file from NaCl repository.

import re
import socket
import time


def RspChecksum(data):
  checksum = 0
  for char in data:
    checksum = (checksum + ord(char)) % 0x100
  return checksum


class GdbRspConnection(object):

  def __init__(self, addr):
    self._socket = self._Connect(addr)

  def _Connect(self, addr):
    # We have to poll because we do not know when sel_ldr has
    # successfully done bind() on the TCP port.  This is inherently
    # unreliable.
    # TODO(mseaborn): Add a more reliable connection mechanism to
    # sel_ldr's debug stub.
    timeout_in_seconds = 10
    poll_time_in_seconds = 0.1
    for i in xrange(int(timeout_in_seconds / poll_time_in_seconds)):
      # On Mac OS X, we have to create a new socket FD for each retry.
      sock = socket.socket()
      try:
        sock.connect(addr)
      except socket.error:
        # Retry after a delay.
        time.sleep(poll_time_in_seconds)
      else:
        return sock
    raise Exception('Could not connect to sel_ldr\'s debug stub in %i seconds'
                    % timeout_in_seconds)

  def _GetReply(self):
    reply = ''
    while True:
      data = self._socket.recv(1024)
      if len(data) == 0:
        raise AssertionError('EOF on socket reached with '
                             'incomplete reply message: %r' % reply)
      reply += data
      if '#' in data:
        break
    match = re.match('\+\$([^#]*)#([0-9a-fA-F]{2})$', reply)
    if match is None:
      raise AssertionError('Unexpected reply message: %r' % reply)
    reply_body = match.group(1)
    checksum = match.group(2)
    expected_checksum = '%02x' % RspChecksum(reply_body)
    if checksum != expected_checksum:
      raise AssertionError('Bad RSP checksum: %r != %r' %
                           (checksum, expected_checksum))
    # Send acknowledgement.
    self._socket.send('+')
    return reply_body

  # Send an rsp message, but don't wait for or expect a reply.
  def RspSendOnly(self, data):
    msg = '$%s#%02x' % (data, RspChecksum(data))
    return self._socket.send(msg)

  def RspRequest(self, data):
    self.RspSendOnly(data)
    return self._GetReply()

  def RspInterrupt(self):
    self._socket.send('\x03')
    return self._GetReply()
