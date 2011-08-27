// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "dbus/message.h"
#include "dbus/mock_bus.h"
#include "dbus/mock_object_proxy.h"
#include "dbus/mock_exported_object.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Unused;

class MockTest : public testing::Test {
 public:
  MockTest() {
  }

  virtual void SetUp() {
    // Create a mock bus.
    dbus::Bus::Options options;
    options.bus_type = dbus::Bus::SYSTEM;
    mock_bus_ = new dbus::MockBus(options);

    // Create a mock proxy.
    mock_proxy_ = new dbus::MockObjectProxy(mock_bus_.get(),
                                            "org.chromium.TestService",
                                            "/org/chromium/TestObject");

    // Set an expectation so mock_proxy's CallMethodAndBlock() will use
    // CreateMockProxyResponse() to return responses.
    EXPECT_CALL(*mock_proxy_, CallMethodAndBlock(_, _))
        .WillRepeatedly(Invoke(this, &MockTest::CreateMockProxyResponse));

    // Set an expectation so mock_proxy's CallMethod() will use
    // HandleMockProxyResponseWithMessageLoop() to return responses.
    EXPECT_CALL(*mock_proxy_, CallMethod(_, _, _))
        .WillRepeatedly(
            Invoke(this,
                   &MockTest::HandleMockProxyResponseWithMessageLoop));

    // Set an expectation so mock_bus's GetObjectProxy() for the given
    // service name and the object path will return mock_proxy_.
    EXPECT_CALL(*mock_bus_, GetObjectProxy("org.chromium.TestService",
                                          "/org/chromium/TestObject"))
        .WillOnce(Return(mock_proxy_.get()));

    // ShutdownAndBlock() will be called in TearDown().
    EXPECT_CALL(*mock_bus_, ShutdownAndBlock()).WillOnce(Return());
  }

  virtual void TearDown() {
    mock_bus_->ShutdownAndBlock();
  }

  // Called when the response is received.
  void OnResponse(dbus::Response* response) {
    // |response| will be deleted on exit of the function. Copy the
    // payload to |response_string_|.
    if (response) {
      dbus::MessageReader reader(response);
      ASSERT_TRUE(reader.PopString(&response_string_));
    }
    message_loop_.Quit();
  };

 protected:
  std::string response_string_;
  MessageLoop message_loop_;
  scoped_refptr<dbus::MockBus> mock_bus_;
  scoped_refptr<dbus::MockObjectProxy> mock_proxy_;

 private:
  // Returns a response for the given method call. Used to implement
  // CallMethodAndBlock() for |mock_proxy_|.
  dbus::Response* CreateMockProxyResponse(dbus::MethodCall* method_call,
                                          int timeout_ms) {
    if (method_call->GetInterface() == "org.chromium.TestInterface" &&
        method_call->GetMember() == "Echo") {
      dbus::MessageReader reader(method_call);
      std::string text_message;
      if (reader.PopString(&text_message)) {
        dbus::Response* response = dbus::Response::CreateEmpty();
        dbus::MessageWriter writer(response);
        writer.AppendString(text_message);
        return response;
      }
    }

    LOG(ERROR) << "Unexpected method call: " << method_call->ToString();
    return NULL;
  }

  // Creates a response and runs the given response callback in the
  // message loop with the response. Used to implement for |mock_proxy_|.
  void HandleMockProxyResponseWithMessageLoop(
      dbus::MethodCall* method_call,
      int timeout_ms,
      dbus::ObjectProxy::ResponseCallback response_callback) {
    dbus::Response* response = CreateMockProxyResponse(method_call,
                                                       timeout_ms);
    message_loop_.PostTask(FROM_HERE,
                           base::Bind(&MockTest::RunResponseCallback,
                                      base::Unretained(this),
                                      response_callback,
                                      response));
  }

  // Runs the given response callback with the given response.
  void RunResponseCallback(
      dbus::ObjectProxy::ResponseCallback response_callback,
      dbus::Response* response) {
    response_callback.Run(response);
    delete response;
  }
};

// This test demonstrates how to mock a synchronos method call using the
// mock classes.
TEST_F(MockTest, CallMethodAndBlock) {
  const char kHello[] = "Hello";
  // Get an object proxy from the mock bus.
  dbus::ObjectProxy* proxy = mock_bus_->GetObjectProxy(
      "org.chromium.TestService",
      "/org/chromium/TestObject");

  // Create a method call.
  dbus::MethodCall method_call("org.chromium.TestInterface", "Echo");
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kHello);

  // Call the method.
  scoped_ptr<dbus::Response> response(
      proxy->CallMethodAndBlock(&method_call,
                                dbus::ObjectProxy::TIMEOUT_USE_DEFAULT));

  // Check the response.
  ASSERT_TRUE(response.get());
  dbus::MessageReader reader(response.get());
  std::string text_message;
  ASSERT_TRUE(reader.PopString(&text_message));
  // The text message should be echo'ed back.
  EXPECT_EQ(kHello, text_message);
}

// This test demonstrates how to mock an asynchronos method call using the
// mock classes.
TEST_F(MockTest, CallMethod) {
  const char kHello[] = "hello";

  // Get an object proxy from the mock bus.
  dbus::ObjectProxy* proxy = mock_bus_->GetObjectProxy(
      "org.chromium.TestService",
      "/org/chromium/TestObject");

  // Create a method call.
  dbus::MethodCall method_call("org.chromium.TestInterface", "Echo");
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kHello);

  // Call the method.
  proxy->CallMethod(&method_call,
                    dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                    base::Bind(&MockTest::OnResponse,
                               base::Unretained(this)));
  // Run the message loop to let OnResponse be called.
  message_loop_.Run();

  EXPECT_EQ(kHello, response_string_);
}
