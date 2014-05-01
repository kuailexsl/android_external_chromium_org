// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_GARBAGE_COLLECTOR_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_GARBAGE_COLLECTOR_H_

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/extensions/install_observer.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}

class ExtensionService;

namespace extensions {

// The class responsible for cleaning up the cruft left behind on the file
// system by uninstalled (or failed install) extensions.
// The class is owned by ExtensionService, but is mostly independent. Tasks to
// garbage collect extensions and isolated storage are posted once the
// ExtensionSystem signals ready.
class ExtensionGarbageCollector : public KeyedService, public InstallObserver {
 public:
  explicit ExtensionGarbageCollector(content::BrowserContext* context);
  virtual ~ExtensionGarbageCollector();

  static ExtensionGarbageCollector* Get(content::BrowserContext* context);

#if defined(OS_CHROMEOS)
  // Enable or disable garbage collection. See |disable_garbage_collection_|.
  void disable_garbage_collection() { disable_garbage_collection_ = true; }
  void enable_garbage_collection() { disable_garbage_collection_ = false; }
#endif

  // Manually trigger GarbageCollectExtensions() for testing.
  void GarbageCollectExtensionsForTest();

  // Overriddes for KeyedService:
  virtual void Shutdown() OVERRIDE;

  // Overriddes for InstallObserver
  virtual void OnBeginCrxInstall(const std::string& extension_id) OVERRIDE;
  virtual void OnFinishCrxInstall(const std::string& extension_id,
                                  bool success) OVERRIDE;

 private:
  // Cleans up the extension install directory. It can end up with garbage in it
  // if extensions can't initially be removed when they are uninstalled (eg if a
  // file is in use).
  // Obsolete version directories are removed, as are directories that aren't
  // found in the ExtensionPrefs.
  // The "Temp" directory that is used during extension installation will get
  // removed iff there are no pending installations.
  void GarbageCollectExtensions();

  // Garbage collects apps/extensions isolated storage if it is uninstalled.
  // There is an exception for ephemeral apps because they can outlive their
  // cache lifetimes.
  void GarbageCollectIsolatedStorageIfNeeded();

  // The BrowserContext associated with the GarbageCollector.
  content::BrowserContext* context_;

#if defined(OS_CHROMEOS)
  // TODO(rkc): HACK alert - this is only in place to allow the
  // kiosk_mode_screensaver to prevent its extension from getting garbage
  // collected. Remove this once KioskModeScreensaver is removed.
  // See crbug.com/280363
  bool disable_garbage_collection_;
#endif

  // The number of currently ongoing CRX installations. This is used to prevent
  // garbage collection from running while a CRX is being installed.
  int crx_installs_in_progress_;

  // Generate weak pointers for safely posting to the file thread for garbage
  // collection.
  base::WeakPtrFactory<ExtensionGarbageCollector> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionGarbageCollector);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_GARBAGE_COLLECTOR_H_
