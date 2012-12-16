// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/syncable/local_file_change_tracker.h"

#include <deque>
#include <set>

#include "base/basictypes.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/stl_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/blob/mock_blob_url_request_context.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_system_task_runners.h"
#include "webkit/fileapi/syncable/canned_syncable_file_system.h"
#include "webkit/fileapi/syncable/local_file_sync_context.h"
#include "webkit/fileapi/syncable/sync_status_code.h"
#include "webkit/fileapi/syncable/syncable_file_system_util.h"
#include "webkit/quota/quota_manager.h"

using webkit_blob::MockBlobURLRequestContext;
using webkit_blob::ScopedTextBlob;

namespace fileapi {

class LocalFileChangeTrackerTest : public testing::Test {
 public:
  LocalFileChangeTrackerTest()
      : message_loop_(MessageLoop::TYPE_IO),
        file_system_(GURL("http://example.com"), "test",
                     base::MessageLoopProxy::current(),
                     base::MessageLoopProxy::current()) {}

  virtual void SetUp() OVERRIDE {
    file_system_.SetUp();

    sync_context_ = new LocalFileSyncContext(base::MessageLoopProxy::current(),
                                             base::MessageLoopProxy::current());
    ASSERT_EQ(SYNC_STATUS_OK,
              file_system_.MaybeInitializeFileSystemContext(sync_context_));
  }

  virtual void TearDown() OVERRIDE {
    if (sync_context_)
      sync_context_->ShutdownOnUIThread();
    sync_context_ = NULL;

    message_loop_.RunUntilIdle();
    file_system_.TearDown();
    // Make sure we don't leave the external filesystem.
    // (CannedSyncableFileSystem::TearDown does not do this as there may be
    // multiple syncable file systems registered for the name)
    RevokeSyncableFileSystem("test");
  }

 protected:
  FileSystemURL URL(const std::string& path) {
    return file_system_.URL(path);
  }

  FileSystemContext* file_system_context() {
    return file_system_.file_system_context();
  }

  LocalFileChangeTracker* change_tracker() {
    return file_system_context()->change_tracker();
  }

  void VerifyAndClearChange(const FileSystemURL& url,
                            const FileChange& expected_change) {
    SCOPED_TRACE(testing::Message() << url.DebugString() <<
                 " expecting:" << expected_change.DebugString());
    // Get the changes for URL and verify.
    FileChangeList changes;
    change_tracker()->GetChangesForURL(url, &changes);
    ASSERT_EQ(1U, changes.size());
    SCOPED_TRACE(testing::Message() << url.DebugString() <<
                 " actual:" << changes.DebugString());
    EXPECT_EQ(expected_change, changes.list()[0]);

    // Clear the URL from the change tracker.
    change_tracker()->ClearChangesForURL(url);
  }

  void DropChangesInTracker() {
    change_tracker()->DropAllChanges();
  }

  void RestoreChangesFromTrackerDB() {
    change_tracker()->CollectLastDirtyChanges(file_system_context());
  }

  MessageLoop message_loop_;

  CannedSyncableFileSystem file_system_;
  scoped_refptr<LocalFileSyncContext> sync_context_;

  DISALLOW_COPY_AND_ASSIGN(LocalFileChangeTrackerTest);
};

TEST_F(LocalFileChangeTrackerTest, GetChanges) {
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system_.OpenFileSystem());

  // Test URLs (no parent/child relationships, as we test such cases
  // mainly in LocalFileSyncStatusTest).
  const char kPath0[] = "test/dir a/dir";
  const char kPath1[] = "test/dir b";
  const char kPath2[] = "test/foo.txt";
  const char kPath3[] = "test/bar";
  const char kPath4[] = "temporary/dir a";
  const char kPath5[] = "temporary/foo";

  change_tracker()->OnCreateDirectory(URL(kPath0));
  change_tracker()->OnRemoveDirectory(URL(kPath0));  // Offset the create.
  change_tracker()->OnRemoveDirectory(URL(kPath1));
  change_tracker()->OnCreateDirectory(URL(kPath2));
  change_tracker()->OnRemoveFile(URL(kPath3));
  change_tracker()->OnModifyFile(URL(kPath4));
  change_tracker()->OnCreateFile(URL(kPath5));
  change_tracker()->OnRemoveFile(URL(kPath5));  // Recorded as 'delete'.

  FileSystemURLSet urls;
  file_system_.GetChangedURLsInTracker(&urls);

  EXPECT_EQ(5U, urls.size());
  EXPECT_TRUE(ContainsKey(urls, URL(kPath1)));
  EXPECT_TRUE(ContainsKey(urls, URL(kPath2)));
  EXPECT_TRUE(ContainsKey(urls, URL(kPath3)));
  EXPECT_TRUE(ContainsKey(urls, URL(kPath4)));
  EXPECT_TRUE(ContainsKey(urls, URL(kPath5)));

  // Changes for kPath0 must have been offset and removed.
  EXPECT_FALSE(ContainsKey(urls, URL(kPath0)));

  // GetNextChangedURLs only returns up to max_urls (i.e. 3) urls.
  std::deque<FileSystemURL> urls_to_process;
  change_tracker()->GetNextChangedURLs(&urls_to_process, 3);
  ASSERT_EQ(3U, urls_to_process.size());

  // Let it return all.
  urls_to_process.clear();
  change_tracker()->GetNextChangedURLs(&urls_to_process, 0);
  ASSERT_EQ(5U, urls_to_process.size());

  // The changes must be in the last-modified-time order.
  EXPECT_EQ(URL(kPath1), urls_to_process[0]);
  EXPECT_EQ(URL(kPath2), urls_to_process[1]);
  EXPECT_EQ(URL(kPath3), urls_to_process[2]);
  EXPECT_EQ(URL(kPath4), urls_to_process[3]);
  EXPECT_EQ(URL(kPath5), urls_to_process[4]);

  // Modify kPath4 again.
  change_tracker()->OnModifyFile(URL(kPath4));

  // Now the order must be changed.
  urls_to_process.clear();
  change_tracker()->GetNextChangedURLs(&urls_to_process, 0);
  ASSERT_EQ(5U, urls_to_process.size());
  EXPECT_EQ(URL(kPath1), urls_to_process[0]);
  EXPECT_EQ(URL(kPath2), urls_to_process[1]);
  EXPECT_EQ(URL(kPath3), urls_to_process[2]);
  EXPECT_EQ(URL(kPath5), urls_to_process[3]);
  EXPECT_EQ(URL(kPath4), urls_to_process[4]);

  VerifyAndClearChange(URL(kPath1),
               FileChange(FileChange::FILE_CHANGE_DELETE,
                          SYNC_FILE_TYPE_DIRECTORY));
  VerifyAndClearChange(URL(kPath2),
               FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                          SYNC_FILE_TYPE_DIRECTORY));
  VerifyAndClearChange(URL(kPath3),
               FileChange(FileChange::FILE_CHANGE_DELETE,
                          SYNC_FILE_TYPE_FILE));
  VerifyAndClearChange(URL(kPath4),
               FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                          SYNC_FILE_TYPE_FILE));
  VerifyAndClearChange(URL(kPath5),
               FileChange(FileChange::FILE_CHANGE_DELETE,
                          SYNC_FILE_TYPE_FILE));
}

TEST_F(LocalFileChangeTrackerTest, RestoreCreateAndModifyChanges) {
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system_.OpenFileSystem());

  FileSystemURLSet urls;

  const char kPath0[] = "file a";
  const char kPath1[] = "dir a";
  const char kPath2[] = "dir a/dir";
  const char kPath3[] = "dir a/file a";
  const char kPath4[] = "dir a/file b";

  const char kPath0Copy[] = "file b";      // To be copied from kPath0
  const char kPath1Copy[] = "dir b";       // To be copied from kPath1
  const char kPath2Copy[] = "dir b/dir";   // To be copied from kPath2
  const char kPath3Copy[] = "dir b/file a";  // To be copied from kPath3
  const char kPath4Copy[] = "dir b/file b";  // To be copied from kPath4

  file_system_.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(0U, urls.size());

  const GURL blob_url("blob:test");
  const std::string kData("Lorem ipsum.");
  MockBlobURLRequestContext url_request_context(file_system_context());
  ScopedTextBlob blob(url_request_context, blob_url, kData);

  // Creates files and nested directories.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath0)));       // Creates a file.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath1)));  // Creates a dir.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath2)));  // Creates another dir.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath3)));       // Creates a file.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.TruncateFile(URL(kPath3), 1));  // Modifies the file.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath4)));    // Creates another file.
  EXPECT_EQ(static_cast<int64>(kData.size()),
            file_system_.Write(&url_request_context,
                               URL(kPath4), blob_url));  // Modifies the file.

  // Copies the file and the parent directory.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Copy(URL(kPath0), URL(kPath0Copy)));  // Copy the file.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Copy(URL(kPath1), URL(kPath1Copy)));  // Copy the dir.

  file_system_.GetChangedURLsInTracker(&urls);
  EXPECT_EQ(10U, urls.size());

  DropChangesInTracker();

  // Make sure we have no in-memory changes in the tracker.
  urls.clear();
  file_system_.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(0U, urls.size());

  RestoreChangesFromTrackerDB();

  // Make sure the changes are restored from the DB.
  urls.clear();
  file_system_.GetChangedURLsInTracker(&urls);
  EXPECT_EQ(10U, urls.size());

  VerifyAndClearChange(URL(kPath0),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));
  VerifyAndClearChange(URL(kPath1),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_DIRECTORY));
  VerifyAndClearChange(URL(kPath2),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_DIRECTORY));
  VerifyAndClearChange(URL(kPath3),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));
  VerifyAndClearChange(URL(kPath4),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));

  VerifyAndClearChange(URL(kPath0Copy),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));
  VerifyAndClearChange(URL(kPath1Copy),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_DIRECTORY));
  VerifyAndClearChange(URL(kPath2Copy),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_DIRECTORY));
  VerifyAndClearChange(URL(kPath3Copy),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));
  VerifyAndClearChange(URL(kPath4Copy),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));
}

TEST_F(LocalFileChangeTrackerTest, RestoreRemoveChanges) {
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system_.OpenFileSystem());

  FileSystemURLSet urls;

  const char kPath0[] = "file";
  const char kPath1[] = "dir a";
  const char kPath2[] = "dir b";
  const char kPath3[] = "dir b/file";
  const char kPath4[] = "dir b/dir c";
  const char kPath5[] = "dir b/dir c/file";

  file_system_.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(0U, urls.size());

  // Creates and removes a same file.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath0)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Remove(URL(kPath0), false /* recursive */));

  // Creates and removes a same directory.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath1)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Remove(URL(kPath1), false /* recursive */));

  // Creates files and nested directories, then removes the parent directory.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath2)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath3)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath4)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath5)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Remove(URL(kPath2), true /* recursive */));

  urls.clear();
  file_system_.GetChangedURLsInTracker(&urls);
  EXPECT_EQ(3U, urls.size());

  DropChangesInTracker();

  // Make sure we have no in-memory changes in the tracker.
  urls.clear();
  file_system_.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(0U, urls.size());

  RestoreChangesFromTrackerDB();

  // Make sure the changes are restored from the DB.
  urls.clear();
  file_system_.GetChangedURLsInTracker(&urls);
  // Since directories to have been reverted (kPath1, kPath2, kPath4) are
  // treated as FILE_CHANGE_DELETE, the number of changes should be 6.
  EXPECT_EQ(6U, urls.size());

  VerifyAndClearChange(URL(kPath0),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath1),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath2),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath3),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath4),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath5),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
}

TEST_F(LocalFileChangeTrackerTest, RestoreMoveChanges) {
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system_.OpenFileSystem());

  FileSystemURLSet urls;

  const char kPath0[] = "file a";
  const char kPath1[] = "dir a";
  const char kPath2[] = "dir a/file";
  const char kPath3[] = "dir a/dir";
  const char kPath4[] = "dir a/dir/file";

  const char kPath5[] = "file b";          // To be moved from kPath0.
  const char kPath6[] = "dir b";           // To be moved from kPath1.
  const char kPath7[] = "dir b/file";      // To be moved from kPath2.
  const char kPath8[] = "dir b/dir";       // To be moved from kPath3.
  const char kPath9[] = "dir b/dir/file";  // To be moved from kPath4.

  file_system_.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(0U, urls.size());

  // Creates and moves a same file.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath0)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Move(URL(kPath0), URL(kPath5)));

  // Creates files and nested directories, then moves the parent directory.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath1)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath2)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath3)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath4)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Move(URL(kPath1), URL(kPath6)));

  urls.clear();
  file_system_.GetChangedURLsInTracker(&urls);
  // Since kPath1 and kPath3 were reverted, the number of changes should be 8.
  EXPECT_EQ(8U, urls.size());

  DropChangesInTracker();

  // Make sure we have no in-memory changes in the tracker.
  urls.clear();
  file_system_.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(0U, urls.size());

  RestoreChangesFromTrackerDB();

  // Make sure the changes are restored from the DB.
  urls.clear();
  file_system_.GetChangedURLsInTracker(&urls);
  // Since directories to have been reverted (kPath1 and kPath3) are treated as
  // FILE_CHANGE_DELETE, the number of changes should be 10.
  EXPECT_EQ(10U, urls.size());

  VerifyAndClearChange(URL(kPath0),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath1),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath2),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath3),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath4),
                       FileChange(FileChange::FILE_CHANGE_DELETE,
                                  SYNC_FILE_TYPE_UNKNOWN));
  VerifyAndClearChange(URL(kPath5),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));
  VerifyAndClearChange(URL(kPath6),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_DIRECTORY));
  VerifyAndClearChange(URL(kPath7),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));
  VerifyAndClearChange(URL(kPath8),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_DIRECTORY));
  VerifyAndClearChange(URL(kPath9),
                       FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                                  SYNC_FILE_TYPE_FILE));
}

TEST_F(LocalFileChangeTrackerTest, NextChangedURLsWithRecursiveCopy) {
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system_.OpenFileSystem());

  FileSystemURLSet urls;

  const char kPath0[] = "dir a";
  const char kPath1[] = "dir a/file";
  const char kPath2[] = "dir a/dir";

  const char kPath0Copy[] = "dir b";
  const char kPath1Copy[] = "dir b/file";
  const char kPath2Copy[] = "dir b/dir";

  // Creates kPath0,1,2 and then copies them all.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath0)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath1)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath2)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Copy(URL(kPath0), URL(kPath0Copy)));

  std::deque<FileSystemURL> urls_to_process;
  change_tracker()->GetNextChangedURLs(&urls_to_process, 0);
  ASSERT_EQ(6U, urls_to_process.size());

  // Creation must have occured first.
  EXPECT_EQ(URL(kPath0), urls_to_process[0]);
  EXPECT_EQ(URL(kPath1), urls_to_process[1]);
  EXPECT_EQ(URL(kPath2), urls_to_process[2]);

  // Then recursive copy took place. The exact order cannot be determined
  // but the parent directory must have been created first.
  EXPECT_EQ(URL(kPath0Copy), urls_to_process[3]);
  EXPECT_TRUE(URL(kPath1Copy) == urls_to_process[4] ||
              URL(kPath2Copy) == urls_to_process[4]);
  EXPECT_TRUE(URL(kPath1Copy) == urls_to_process[5] ||
              URL(kPath2Copy) == urls_to_process[5]);
}

TEST_F(LocalFileChangeTrackerTest, NextChangedURLsWithRecursiveRemove) {
  EXPECT_EQ(base::PLATFORM_FILE_OK, file_system_.OpenFileSystem());

  FileSystemURLSet urls;

  const char kPath0[] = "dir a";
  const char kPath1[] = "dir a/file1";
  const char kPath2[] = "dir a/file2";

  // Creates kPath0,1,2 and then removes them all.
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateDirectory(URL(kPath0)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath1)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.CreateFile(URL(kPath2)));
  EXPECT_EQ(base::PLATFORM_FILE_OK,
            file_system_.Remove(URL(kPath0), true /* recursive */));

  std::deque<FileSystemURL> urls_to_process;
  change_tracker()->GetNextChangedURLs(&urls_to_process, 0);

  // This is actually not really desirable, but since the directory
  // creation and deletion have been offset now we only have two
  // file deletion changes.
  //
  // NOTE: This will cause 2 local sync for deleting nonexistent files
  // on the remote side.
  //
  // TODO(kinuko): For micro optimization we could probably restore the ADD
  // change type (other than ADD_OR_UPDATE) and offset file ADD+DELETE
  // changes too.
  ASSERT_EQ(2U, urls_to_process.size());

  // The exact order of recursive removal cannot be determined.
  EXPECT_TRUE(URL(kPath1) == urls_to_process[0] ||
              URL(kPath2) == urls_to_process[0]);
  EXPECT_TRUE(URL(kPath1) == urls_to_process[1] ||
              URL(kPath2) == urls_to_process[1]);
}

}  // namespace fileapi
