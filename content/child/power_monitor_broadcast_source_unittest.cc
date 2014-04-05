// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "base/test/power_monitor_test_base.h"
#include "content/child/power_monitor_broadcast_source.h"
#include "content/common/power_monitor_messages.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class PowerMonitorBroadcastSourceTest : public testing::Test {
 protected:
  PowerMonitorBroadcastSourceTest() {
    power_monitor_source_ = new PowerMonitorBroadcastSource();
    base::PowerMonitor::Initialize(
        scoped_ptr<base::PowerMonitorSource>(power_monitor_source_));
  }
  virtual ~PowerMonitorBroadcastSourceTest() {
    base::PowerMonitor::ShutdownForTesting();
  }

  PowerMonitorBroadcastSource* source() { return power_monitor_source_; }

  base::MessageLoop message_loop_;

 private:
  PowerMonitorBroadcastSource* power_monitor_source_;

  DISALLOW_COPY_AND_ASSIGN(PowerMonitorBroadcastSourceTest);
};

TEST_F(PowerMonitorBroadcastSourceTest, PowerMessageReceiveBroadcast) {
  IPC::ChannelProxy::MessageFilter* message_filter =
    source()->GetMessageFilter();

  base::PowerMonitorTestObserver observer;
  base::PowerMonitor::AddObserver(&observer);

  PowerMonitorMsg_Suspend suspend_msg;
  PowerMonitorMsg_Resume resume_msg;

  // Sending resume when not suspended should have no effect.
  message_filter->OnMessageReceived(resume_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.resumes(), 0);

  // Pretend we suspended.
  message_filter->OnMessageReceived(suspend_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.suspends(), 1);

  // Send a second suspend notification.  This should be suppressed.
  message_filter->OnMessageReceived(suspend_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.suspends(), 1);

  // Pretend we were awakened.
  message_filter->OnMessageReceived(resume_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.resumes(), 1);

  // Send a duplicate resume notification.  This should be suppressed.
  message_filter->OnMessageReceived(resume_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.resumes(), 1);

  PowerMonitorMsg_PowerStateChange on_battery_msg(true);
  PowerMonitorMsg_PowerStateChange off_battery_msg(false);

  // Pretend the device has gone on battery power
  message_filter->OnMessageReceived(on_battery_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.power_state_changes(), 1);
  EXPECT_EQ(observer.last_power_state(), true);

  // Repeated indications the device is on battery power should be suppressed.
  message_filter->OnMessageReceived(on_battery_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.power_state_changes(), 1);

  // Pretend the device has gone off battery power
  message_filter->OnMessageReceived(off_battery_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.power_state_changes(), 2);
  EXPECT_EQ(observer.last_power_state(), false);

  // Repeated indications the device is off battery power should be suppressed.
  message_filter->OnMessageReceived(off_battery_msg);
  message_loop_.RunUntilIdle();
  EXPECT_EQ(observer.power_state_changes(), 2);
}

}  // namespace base
