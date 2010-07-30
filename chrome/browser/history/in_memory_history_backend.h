// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Contains the history backend wrapper around the in-memory URL database. This
// object maintains an in-memory cache of the subset of history required to do
// in-line autocomplete.
//
// It is created on the history thread and passed to the main thread where
// operations can be completed synchronously. It listenes for notifications
// from the "regular" history backend and keeps itself in sync.

#ifndef CHROME_BROWSER_HISTORY_IN_MEMORY_HISTORY_BACKEND_H_
#define CHROME_BROWSER_HISTORY_IN_MEMORY_HISTORY_BACKEND_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/scoped_ptr.h"
#include "chrome/common/notification_registrar.h"

class FilePath;
class HistoryDatabase;
class Profile;

namespace history {

class InMemoryDatabase;
class InMemoryURLIndex;
class URLDatabase;
struct URLsDeletedDetails;
struct URLsModifiedDetails;

class InMemoryHistoryBackend : public NotificationObserver {
 public:
  InMemoryHistoryBackend();
  ~InMemoryHistoryBackend();

  // Initializes with data from the given history database.
  bool Init(const FilePath& history_filename, URLDatabase* db);

  // Does initialization work when this object is attached to the history
  // system on the main thread. The argument is the profile with which the
  // attached history service is under.
  void AttachToHistoryService(Profile* profile);

  // Returns the underlying database associated with this backend. The current
  // autocomplete code was written fro this, but it should probably be removed
  // so that it can deal directly with this object, rather than the DB.
  InMemoryDatabase* db() const {
    return db_.get();
  }

  // Returns the in memory index owned by this backend.  This index is only
  // loaded when the --enable-in-memory-url-index flag is used.
  InMemoryURLIndex* index() const {
    return index_.get();
  }

  // Notification callback.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  FRIEND_TEST_ALL_PREFIXES(HistoryBackendTest, DeleteAll);

  // Handler for NOTIFY_HISTORY_TYPED_URLS_MODIFIED.
  void OnTypedURLsModified(const URLsModifiedDetails& details);

  // Handler for NOTIFY_HISTORY_URLS_DELETED.
  void OnURLsDeleted(const URLsDeletedDetails& details);

  NotificationRegistrar registrar_;

  scoped_ptr<InMemoryDatabase> db_;

  scoped_ptr<InMemoryURLIndex> index_;

  // The profile that this object is attached. May be NULL before
  // initialization.
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryHistoryBackend);
};

}  // namespace history

#endif  // CHROME_BROWSER_HISTORY_IN_MEMORY_HISTORY_BACKEND_H_
