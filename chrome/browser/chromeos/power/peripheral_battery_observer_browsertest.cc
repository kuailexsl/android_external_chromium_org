// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/peripheral_battery_observer.h"

#include "base/command_line.h"
#include "base/message_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/cros/cros_in_process_browser_test.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chromeos/dbus/mock_dbus_thread_manager.h"
#include "chromeos/dbus/mock_update_engine_client.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SaveArg;

namespace {

const char kTestBatteryPath[] = "/sys/class/power_supply/hid-AA:BB:CC-battery";
const char kTestBatteryAddress[] = "cc:bb:aa";
const char kTestDeviceName[] = "test device";

}  // namespace

namespace chromeos {

class PeripheralBatteryObserverTest : public CrosInProcessBrowserTest {
 public:
  PeripheralBatteryObserverTest () {}
  virtual ~PeripheralBatteryObserverTest () {}

  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    MockDBusThreadManager* mock_dbus_thread_manager = new MockDBusThreadManager;
    DBusThreadManager::InitializeForTesting(mock_dbus_thread_manager);
    CrosInProcessBrowserTest::SetUpInProcessBrowserTestFixture();
  }

  virtual void SetUpOnMainThread() OVERRIDE {
    observer_.reset(new PeripheralBatteryObserver());
  }

  virtual void CleanUpOnMainThread() OVERRIDE {
    observer_.reset();
  }

  virtual void TearDownInProcessBrowserTestFixture() OVERRIDE {
    CrosInProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    DBusThreadManager::Shutdown();
  }

 protected:
  scoped_ptr<PeripheralBatteryObserver> observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PeripheralBatteryObserverTest);
};

IN_PROC_BROWSER_TEST_F(PeripheralBatteryObserverTest, Basic) {
  base::SimpleTestTickClock clock;
  observer_->set_testing_clock(&clock);

  NotificationUIManager* notification_manager =
      g_browser_process->notification_ui_manager();

  // Level 50 at time 100, no low-battery notification.
  clock.Advance(base::TimeDelta::FromSeconds(100));
  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, 50);
  EXPECT_EQ(observer_->batteries_.count(kTestBatteryAddress), 1u);

  const PeripheralBatteryObserver::BatteryInfo& info =
      observer_->batteries_[kTestBatteryAddress];

  EXPECT_EQ(info.name, kTestDeviceName);
  EXPECT_EQ(info.level, 50);
  EXPECT_EQ(info.last_notification_timestamp, base::TimeTicks());
  EXPECT_FALSE(notification_manager->DoesIdExist(kTestBatteryAddress));

  // Level 5 at time 110, low-battery notification.
  clock.Advance(base::TimeDelta::FromSeconds(10));
  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, 5);
  EXPECT_EQ(info.level, 5);
  EXPECT_EQ(info.last_notification_timestamp, clock.NowTicks());
  EXPECT_TRUE(notification_manager->DoesIdExist(kTestBatteryAddress));

  // Level -1 at time 115, cancel previous notification
  clock.Advance(base::TimeDelta::FromSeconds(5));
  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, -1);
  EXPECT_EQ(info.level, 5);
  EXPECT_EQ(info.last_notification_timestamp,
            clock.NowTicks() - base::TimeDelta::FromSeconds(5));
  EXPECT_FALSE(notification_manager->DoesIdExist(kTestBatteryAddress));

  // Level 50 at time 120, no low-battery notification.
  clock.Advance(base::TimeDelta::FromSeconds(5));
  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, 50);
  EXPECT_EQ(info.level, 50);
  EXPECT_EQ(info.last_notification_timestamp,
            clock.NowTicks() - base::TimeDelta::FromSeconds(10));
  EXPECT_FALSE(notification_manager->DoesIdExist(kTestBatteryAddress));

  // Level 5 at time 130, no low-battery notification (throttling).
  clock.Advance(base::TimeDelta::FromSeconds(10));
  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, 5);
  EXPECT_EQ(info.level, 5);
  EXPECT_EQ(info.last_notification_timestamp,
            clock.NowTicks() - base::TimeDelta::FromSeconds(20));
  EXPECT_FALSE(notification_manager->DoesIdExist(kTestBatteryAddress));
}

IN_PROC_BROWSER_TEST_F(PeripheralBatteryObserverTest, InvalidBatteryInfo) {
  observer_->PeripheralBatteryStatusReceived("invalid-path", kTestDeviceName,
                                             10);
  EXPECT_TRUE(observer_->batteries_.empty());

  observer_->PeripheralBatteryStatusReceived(
      "/sys/class/power_supply/hid-battery", kTestDeviceName, 10);
  EXPECT_TRUE(observer_->batteries_.empty());

  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, -2);
  EXPECT_TRUE(observer_->batteries_.empty());

  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, 101);
  EXPECT_TRUE(observer_->batteries_.empty());

  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, -1);
  EXPECT_TRUE(observer_->batteries_.empty());
}

IN_PROC_BROWSER_TEST_F(PeripheralBatteryObserverTest, DeviceRemove) {
  NotificationUIManager* notification_manager =
      g_browser_process->notification_ui_manager();

  observer_->PeripheralBatteryStatusReceived(kTestBatteryPath,
                                             kTestDeviceName, 5);
  EXPECT_EQ(observer_->batteries_.count(kTestBatteryAddress), 1u);
  EXPECT_TRUE(notification_manager->DoesIdExist(kTestBatteryAddress));

  observer_->RemoveBattery(kTestBatteryAddress);
  EXPECT_FALSE(notification_manager->DoesIdExist(kTestBatteryAddress));
}

}  // namespace chromeos
