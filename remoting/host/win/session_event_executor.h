// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_WIN_SESSION_EVENT_EXECUTOR_H_
#define REMOTING_HOST_WIN_SESSION_EVENT_EXECUTOR_H_

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/ref_counted.h"
#include "remoting/host/event_executor.h"

namespace base {
class SingleThreadTaskRunner;
} // namespace base

namespace remoting {

// Monitors and passes key/mouse events to a nested event executor. Injects
// Secure Attention Sequence (SAS) when Ctrl+Alt+Del key combination has been
// detected.
class SessionEventExecutorWin : public EventExecutor {
 public:
  // |inject_sas| is invoked on |inject_sas_task_runner| to generate SAS on
  // Vista+.
  SessionEventExecutorWin(
      scoped_refptr<base::SingleThreadTaskRunner> input_task_runner,
      scoped_ptr<EventExecutor> nested_executor,
      scoped_refptr<base::SingleThreadTaskRunner> inject_sas_task_runner,
      const base::Closure& inject_sas);
  virtual ~SessionEventExecutorWin();

  // EventExecutor implementation.
  virtual void Start(
      scoped_ptr<protocol::ClipboardStub> client_clipboard) OVERRIDE;

  // protocol::ClipboardStub implementation.
  virtual void InjectClipboardEvent(
      const protocol::ClipboardEvent& event) OVERRIDE;

  // protocol::InputStub implementation.
  virtual void InjectKeyEvent(const protocol::KeyEvent& event) OVERRIDE;
  virtual void InjectMouseEvent(const protocol::MouseEvent& event) OVERRIDE;

 private:
  // The actual implementation resides in SessionEventExecutorWin::Core class.
  class Core;
  scoped_refptr<Core> core_;

  DISALLOW_COPY_AND_ASSIGN(SessionEventExecutorWin);
};

}  // namespace remoting

#endif  // REMOTING_HOST_WIN_SESSION_EVENT_EXECUTOR_H_
