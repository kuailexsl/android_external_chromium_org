// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/state_store.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "third_party/skia/include/core/SkColor.h"

namespace extensions {

namespace {

// A key into the StateStore; we don't use any results, but need to know when
// it's initialized.
const char kBrowserActionStorageKey[] = "browser_action";
// The name of the extension we add.
const char kExtensionName[] = "Default Persistence Test Extension";

void QuitMessageLoop(content::MessageLoopRunner* runner,
                     scoped_ptr<base::Value> value) {
  runner->Quit();
}

// We need to wait for the state store to initialize and respond to requests
// so we can see if the preferences persist. Do this by posting our own request
// to the state store, which should be handled after all others.
void WaitForStateStore(Profile* profile, const std::string& extension_id) {
  scoped_refptr<content::MessageLoopRunner> runner =
      new content::MessageLoopRunner;
  ExtensionSystem::Get(profile)->state_store()->GetExtensionValue(
      extension_id,
      kBrowserActionStorageKey,
      base::Bind(&QuitMessageLoop, runner));
  runner->Run();
}

}  // namespace

// Setup for the test by loading an extension, which should set the browser
// action background to blue.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest,
                       PRE_BrowserActionDefaultPersistence) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("api_test")
                        .AppendASCII("browser_action")
                        .AppendASCII("default_persistence"));
  ASSERT_TRUE(extension);
  ASSERT_EQ(kExtensionName, extension->name());
  WaitForStateStore(profile(), extension->id());

  ExtensionAction* extension_action =
      ExtensionActionManager::Get(profile())->GetBrowserAction(*extension);
  ASSERT_TRUE(extension_action);
  EXPECT_EQ(SK_ColorBLUE, extension_action->GetBadgeBackgroundColor(0));
}

// When Chrome restarts, the Extension will immediately update the browser
// action, but will not modify the badge background color. Thus, the background
// should remain blue (persisting the default set in onInstalled()).
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, BrowserActionDefaultPersistence) {
  // Find the extension (it's a shame we don't have an ID for this, but it
  // was generated in the last test).
  const Extension* extension = NULL;
  const ExtensionSet& extension_set =
      ExtensionRegistry::Get(profile())->enabled_extensions();
  for (ExtensionSet::const_iterator iter = extension_set.begin();
       iter != extension_set.end();
       ++iter) {
    if ((*iter)->name() == kExtensionName) {
      extension = *iter;
      break;
    }
  }
  ASSERT_TRUE(extension) << "Could not find extension in registry.";

  // If this log becomes frequent, this test is losing its effectiveness, and
  // we need to find a more invasive way of ensuring the test's StateStore
  // initializes after extensions get their onStartup event.
  if (ExtensionSystem::Get(profile())->state_store()->IsInitialized())
    LOG(WARNING) << "State store already initialized; test guaranteed to pass.";

  // Wait for the StateStore to load, and fetch the defaults.
  WaitForStateStore(profile(), extension->id());

  // Ensure the BrowserAction's badge background is still blue.
  ExtensionAction* extension_action =
      ExtensionActionManager::Get(profile())->GetBrowserAction(*extension);
  ASSERT_TRUE(extension_action);
  EXPECT_EQ(SK_ColorBLUE, extension_action->GetBadgeBackgroundColor(0));
}

}  // namespace extensions
