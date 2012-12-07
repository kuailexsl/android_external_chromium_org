// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop.h"
#include "remoting/base/auto_thread_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

void SetFlagTask(bool* success) {
  *success = true;
}

}  // namespace

namespace remoting {

TEST(AutoThreadTaskRunnerTest, StartAndStop) {
  // Create a task runner.
  MessageLoop message_loop;
  scoped_refptr<AutoThreadTaskRunner> task_runner =
      new AutoThreadTaskRunner(
          message_loop.message_loop_proxy(), MessageLoop::QuitClosure());

  // Post a task to make sure it is executed.
  bool success = false;
  message_loop.PostTask(FROM_HERE, base::Bind(&SetFlagTask, &success));

  task_runner = NULL;
  message_loop.Run();
  EXPECT_TRUE(success);
}

}  // namespace remoting
