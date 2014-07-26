// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "extensions/browser/api/dns/host_resolver_wrapper.h"
#include "extensions/browser/api/dns/mock_host_resolver_creator.h"
#include "extensions/browser/api/sockets_tcp/sockets_tcp_api.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/spawned_test_server/spawned_test_server.h"

using extensions::Extension;

namespace utils = extension_function_test_utils;

namespace {

// TODO(jschuh): Hanging plugin tests. crbug.com/244653
#if defined(OS_WIN) && defined(ARCH_CPU_X86_64)
#define MAYBE(x) DISABLED_##x
#else
#define MAYBE(x) x
#endif

const std::string kHostname = "127.0.0.1";

class SocketsTcpApiTest : public ExtensionApiTest {
 public:
  SocketsTcpApiTest() : resolver_event_(true, false),
                        resolver_creator_(
                            new extensions::MockHostResolverCreator()) {
  }

  virtual void SetUpOnMainThread() OVERRIDE {
    extensions::HostResolverWrapper::GetInstance()->SetHostResolverForTesting(
        resolver_creator_->CreateMockHostResolver());
  }

  virtual void TearDownOnMainThread() OVERRIDE {
    extensions::HostResolverWrapper::GetInstance()->
        SetHostResolverForTesting(NULL);
    resolver_creator_->DeleteMockHostResolver();
  }

 private:
  base::WaitableEvent resolver_event_;

  // The MockHostResolver asserts that it's used on the same thread on which
  // it's created, which is actually a stronger rule than its real counterpart.
  // But that's fine; it's good practice.
  scoped_refptr<extensions::MockHostResolverCreator> resolver_creator_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(SocketsTcpApiTest, SocketsTcpCreateGood) {
  scoped_refptr<extensions::core_api::SocketsTcpCreateFunction>
      socket_create_function(
          new extensions::core_api::SocketsTcpCreateFunction());
  scoped_refptr<Extension> empty_extension(utils::CreateEmptyExtension());

  socket_create_function->set_extension(empty_extension.get());
  socket_create_function->set_has_callback(true);

  scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
      socket_create_function.get(), "[]", browser(), utils::NONE));
  ASSERT_EQ(base::Value::TYPE_DICTIONARY, result->GetType());
  base::DictionaryValue *value =
      static_cast<base::DictionaryValue*>(result.get());
  int socketId = -1;
  EXPECT_TRUE(value->GetInteger("socketId", &socketId));
  ASSERT_TRUE(socketId > 0);
}

IN_PROC_BROWSER_TEST_F(SocketsTcpApiTest, SocketTcpExtension) {
  scoped_ptr<net::SpawnedTestServer> test_server(
      new net::SpawnedTestServer(
          net::SpawnedTestServer::TYPE_TCP_ECHO,
          net::SpawnedTestServer::kLocalhost,
          base::FilePath(FILE_PATH_LITERAL("net/data"))));
  EXPECT_TRUE(test_server->Start());

  net::HostPortPair host_port_pair = test_server->host_port_pair();
  int port = host_port_pair.port();
  ASSERT_TRUE(port > 0);

  // Test that connect() is properly resolving hostnames.
  host_port_pair.set_host("lOcAlHoSt");

  ResultCatcher catcher;
  catcher.RestrictToProfile(browser()->profile());

  ExtensionTestMessageListener listener("info_please", true);

  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("sockets_tcp/api")));
  EXPECT_TRUE(listener.WaitUntilSatisfied());
  listener.Reply(
      base::StringPrintf("tcp:%s:%d", host_port_pair.host().c_str(), port));

  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}

IN_PROC_BROWSER_TEST_F(SocketsTcpApiTest, SocketTcpExtensionTLS) {
  scoped_ptr<net::SpawnedTestServer> test_https_server(
      new net::SpawnedTestServer(
          net::SpawnedTestServer::TYPE_HTTPS,
          net::BaseTestServer::SSLOptions(),
          base::FilePath(FILE_PATH_LITERAL("net/data"))));
  EXPECT_TRUE(test_https_server->Start());

  net::HostPortPair https_host_port_pair = test_https_server->host_port_pair();
  int https_port = https_host_port_pair.port();
  ASSERT_GT(https_port, 0);

  ResultCatcher catcher;
  catcher.RestrictToProfile(browser()->profile());

  ExtensionTestMessageListener listener("info_please", true);

  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("sockets_tcp/api")));
  EXPECT_TRUE(listener.WaitUntilSatisfied());
  listener.Reply(base::StringPrintf(
      "https:%s:%d", https_host_port_pair.host().c_str(), https_port));

  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}
