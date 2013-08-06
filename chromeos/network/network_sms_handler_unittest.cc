// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_sms_handler.h"

#include <set>
#include <string>

#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_device_client.h"
#include "chromeos/dbus/shill_manager_client.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

class TestObserver : public NetworkSmsHandler::Observer {
 public:
  TestObserver() {}
  virtual ~TestObserver() {}

  virtual void MessageReceived(const base::DictionaryValue& message) OVERRIDE {
    std::string text;
    if (message.GetStringWithoutPathExpansion(
            NetworkSmsHandler::kTextKey, &text)) {
      messages_.insert(text);
    }
  }

  void ClearMessages() {
    messages_.clear();
  }

  int message_count() { return messages_.size(); }
  const std::set<std::string>& messages() const {
    return messages_;
  }

 private:
  std::set<std::string> messages_;
};

}  // namespace

class NetworkSmsHandlerTest : public testing::Test {
 public:
  NetworkSmsHandlerTest() {}
  virtual ~NetworkSmsHandlerTest() {}

  virtual void SetUp() OVERRIDE {
    // Append '--sms-test-messages' to the command line to tell
    // SMSClientStubImpl to generate a series of test SMS messages.
    CommandLine* command_line = CommandLine::ForCurrentProcess();
    command_line->AppendSwitch(chromeos::switches::kSmsTestMessages);

    // Initialize DBusThreadManager with a stub implementation.
    DBusThreadManager::InitializeWithStub();
    ShillManagerClient::TestInterface* manager_test =
        DBusThreadManager::Get()->GetShillManagerClient()->GetTestInterface();
    ASSERT_TRUE(manager_test);
    manager_test->AddDevice("stub_cellular_device2");
    ShillDeviceClient::TestInterface* device_test =
        DBusThreadManager::Get()->GetShillDeviceClient()->GetTestInterface();
    ASSERT_TRUE(device_test);
    device_test->AddDevice("stub_cellular_device2", flimflam::kTypeCellular,
                           "/org/freedesktop/ModemManager1/stub/0");

    // This relies on the stub dbus implementations for ShillManagerClient,
    // ShillDeviceClient, GsmSMSClient, ModemMessagingClient and SMSClient.
    // Initialize a sms handler. The stub dbus clients will not send the
    // first test message until RequestUpdate has been called.
    network_sms_handler_.reset(new NetworkSmsHandler());
    network_sms_handler_->Init();
    test_observer_.reset(new TestObserver());
    network_sms_handler_->AddObserver(test_observer_.get());
    network_sms_handler_->RequestUpdate(true);
    message_loop_.RunUntilIdle();
  }

  virtual void TearDown() OVERRIDE {
    network_sms_handler_.reset();
    DBusThreadManager::Shutdown();
  }

 protected:
  base::MessageLoopForUI message_loop_;
  scoped_ptr<NetworkSmsHandler> network_sms_handler_;
  scoped_ptr<TestObserver> test_observer_;
};

TEST_F(NetworkSmsHandlerTest, SmsHandlerDbusStub) {
  EXPECT_EQ(test_observer_->message_count(), 0);

  // Test that no messages have been received yet
  const std::set<std::string>& messages(test_observer_->messages());
  // Note: The following string corresponds to values in
  // ModemMessagingClientStubImpl and SmsClientStubImpl.
  // TODO(stevenjb): Use a TestInterface to set this up to remove dependency.
  const char kMessage1[] = "SMSClientStubImpl: Test Message: /SMS/0";
  EXPECT_EQ(messages.find(kMessage1), messages.end());

  // Test for messages delivered by signals.
  test_observer_->ClearMessages();
  network_sms_handler_->RequestUpdate(false);
  message_loop_.RunUntilIdle();
  EXPECT_GE(test_observer_->message_count(), 1);
  EXPECT_NE(messages.find(kMessage1), messages.end());
}

}  // namespace chromeos
