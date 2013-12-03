// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/system_indicator/system_indicator_manager.h"
#include "chrome/browser/extensions/api/system_indicator/system_indicator_manager_factory.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/extensions/lazy_background_page_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"
#include "extensions/common/switches.h"

class SystemIndicatorApiTest : public ExtensionApiTest {
 public:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    ExtensionApiTest::SetUpCommandLine(command_line);
    // Set shorter delays to prevent test timeouts in tests that need to wait
    // for the event page to unload.
    command_line->AppendSwitchASCII(
        extensions::switches::kEventPageIdleTime, "1000");
    command_line->AppendSwitchASCII(
        extensions::switches::kEventPageSuspendingTime, "1000");
  }

  const extensions::Extension* LoadExtensionAndWait(
      const std::string& test_name) {
    LazyBackgroundObserver page_complete;
    base::FilePath extdir = test_data_dir_.AppendASCII(test_name);
    const extensions::Extension* extension = LoadExtension(extdir);
    if (extension)
      page_complete.Wait();
    return extension;
  }
};

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, SystemIndicator) {
  // Only run this test on supported platforms.  SystemIndicatorManagerFactory
  // returns NULL on unsupported platforms.
  extensions::SystemIndicatorManager* manager =
      extensions::SystemIndicatorManagerFactory::GetForProfile(profile());
  if (manager) {
    ASSERT_TRUE(RunExtensionTest("system_indicator/basics")) << message_;
  }
}

IN_PROC_BROWSER_TEST_F(SystemIndicatorApiTest, SystemIndicator) {
  // Only run this test on supported platforms.  SystemIndicatorManagerFactory
  // returns NULL on unsupported platforms.
  extensions::SystemIndicatorManager* manager =
      extensions::SystemIndicatorManagerFactory::GetForProfile(profile());
  if (manager) {
    ResultCatcher catcher;

    const extensions::Extension* extension =
        LoadExtensionAndWait("system_indicator/unloaded");
    ASSERT_TRUE(extension) << message_;

    // Lazy Background Page has been shut down.
    extensions::ProcessManager* pm =
        extensions::ExtensionSystem::Get(profile())->process_manager();
    EXPECT_FALSE(pm->GetBackgroundHostForExtension(last_loaded_extension_id()));

    EXPECT_TRUE(manager->SendClickEventToExtensionForTest(extension->id()));
    EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
  }
}
