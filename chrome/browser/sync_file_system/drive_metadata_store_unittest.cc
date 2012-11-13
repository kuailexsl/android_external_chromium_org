// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/drive_metadata_store.h"

#include "base/file_path.h"
#include "base/message_loop.h"
#include "base/scoped_temp_dir.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/threading/thread.h"
#include "chrome/browser/sync_file_system/sync_file_system.pb.h"
#include "content/public/browser/browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/fileapi/isolated_context.h"
#include "webkit/fileapi/syncable/syncable_file_system_util.h"

#define FPL FILE_PATH_LITERAL

using content::BrowserThread;
using fileapi::SyncStatusCode;

namespace sync_file_system {

namespace {

const char kOrigin[] = "http://www.example.com";
const char kServiceName[] = "drive";

typedef DriveMetadataStore::ResourceIDMap ResourceIDMap;

fileapi::FileSystemURL URL(const FilePath& path) {
  return fileapi::CreateSyncableFileSystemURL(
      GURL(kOrigin), kServiceName, path);
}

std::string GetResourceID(const ResourceIDMap& sync_origins,
                          const GURL& origin) {
  ResourceIDMap::const_iterator itr = sync_origins.find(origin);
  if (itr == sync_origins.end())
    return std::string();
  return itr->second;
}

DriveMetadata CreateMetadata(const std::string& resource_id,
                             const std::string& md5_checksum,
                             bool conflicted) {
  DriveMetadata metadata;
  metadata.set_resource_id(resource_id);
  metadata.set_md5_checksum(md5_checksum);
  metadata.set_conflicted(conflicted);
  return metadata;
}

}  // namespace

class DriveMetadataStoreTest : public testing::Test {
 public:
  DriveMetadataStoreTest()
      : created_(false) {}

  virtual ~DriveMetadataStoreTest() {}

  virtual void SetUp() OVERRIDE {
    file_thread_.reset(new base::Thread("Thread_File"));
    file_thread_->Start();

    ui_task_runner_ = base::MessageLoopProxy::current();
    file_task_runner_ = file_thread_->message_loop_proxy();

    ASSERT_TRUE(base_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(fileapi::RegisterSyncableFileSystem(kServiceName));
  }

  virtual void TearDown() OVERRIDE {
    EXPECT_TRUE(fileapi::RevokeSyncableFileSystem(kServiceName));

    DropDatabase();
    file_thread_->Stop();
    message_loop_.RunAllPending();
  }

 protected:
  void InitializeDatabase() {
    EXPECT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());

    bool done = false;
    SyncStatusCode status = fileapi::SYNC_STATUS_UNKNOWN;
    bool created = false;

    drive_metadata_store_.reset(
        new DriveMetadataStore(base_dir_.path(), file_task_runner_));
    drive_metadata_store_->Initialize(
        base::Bind(&DriveMetadataStoreTest::DidInitializeDatabase,
                   base::Unretained(this), &done, &status, &created));
    message_loop_.Run();

    EXPECT_TRUE(done);
    EXPECT_EQ(fileapi::SYNC_STATUS_OK, status);

    if (created) {
      EXPECT_FALSE(created_);
      created_ = created;
      return;
    }
    EXPECT_TRUE(created_);
  }

  void DropDatabase() {
    EXPECT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    drive_metadata_store_.reset();
  }

  void DropSyncRootDirectoryInStore() {
    EXPECT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    drive_metadata_store_->sync_root_directory_resource_id_.clear();
  }

  void RestoreSyncRootDirectoryFromDB() {
    EXPECT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    drive_metadata_store_->RestoreSyncRootDirectory(
        base::Bind(&DriveMetadataStoreTest::DidRestoreSyncRootDirectory,
                   base::Unretained(this)));
    message_loop_.Run();
  }

  void DropSyncOriginsInStore() {
    EXPECT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    drive_metadata_store_->batch_sync_origins_.clear();
    drive_metadata_store_->incremental_sync_origins_.clear();
    EXPECT_TRUE(drive_metadata_store_->batch_sync_origins().empty());
    EXPECT_TRUE(drive_metadata_store_->incremental_sync_origins().empty());
  }

  void RestoreSyncOriginsFromDB() {
    EXPECT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    drive_metadata_store_->RestoreSyncOrigins(
        base::Bind(&DriveMetadataStoreTest::DidRestoreSyncOrigins,
                   base::Unretained(this)));
    message_loop_.Run();
  }

  DriveMetadataStore* drive_metadata_store() {
    return drive_metadata_store_.get();
  }

 private:
  void DidInitializeDatabase(bool* done_out,
                             SyncStatusCode* status_out,
                             bool* created_out,
                             SyncStatusCode status,
                             bool created) {
    *done_out = true;
    *status_out = status;
    *created_out = created;
    message_loop_.Quit();
  }

  void DidRestoreSyncRootDirectory(SyncStatusCode status) {
    EXPECT_EQ(fileapi::SYNC_STATUS_OK, status);
    message_loop_.Quit();
  }

  void DidRestoreSyncOrigins(SyncStatusCode status) {
    EXPECT_EQ(fileapi::SYNC_STATUS_OK, status);
    message_loop_.Quit();
  }

  ScopedTempDir base_dir_;

  MessageLoop message_loop_;
  scoped_ptr<base::Thread> file_thread_;

  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  scoped_ptr<DriveMetadataStore> drive_metadata_store_;

  bool created_;

  DISALLOW_COPY_AND_ASSIGN(DriveMetadataStoreTest);
};

TEST_F(DriveMetadataStoreTest, InitializationTest) {
  InitializeDatabase();
}

TEST_F(DriveMetadataStoreTest, ReadWriteTest) {
  InitializeDatabase();

  const fileapi::FileSystemURL url = URL(FilePath());
  DriveMetadata metadata;
  EXPECT_EQ(fileapi::SYNC_DATABASE_ERROR_NOT_FOUND,
            drive_metadata_store()->ReadEntry(url, &metadata));

  metadata = CreateMetadata("1234567890", "09876543210", true);
  EXPECT_EQ(fileapi::SYNC_STATUS_OK,
            drive_metadata_store()->UpdateEntry(url, metadata));
  drive_metadata_store()->SetLargestChangeStamp(1);

  DropDatabase();
  InitializeDatabase();

  EXPECT_EQ(1, drive_metadata_store()->GetLargestChangeStamp());

  DriveMetadata metadata2;
  EXPECT_EQ(fileapi::SYNC_STATUS_OK,
            drive_metadata_store()->ReadEntry(url, &metadata2));
  EXPECT_EQ(metadata.resource_id(), metadata2.resource_id());
  EXPECT_EQ(metadata.md5_checksum(), metadata2.md5_checksum());
  EXPECT_EQ(metadata.conflicted(), metadata2.conflicted());

  EXPECT_EQ(fileapi::SYNC_STATUS_OK, drive_metadata_store()->DeleteEntry(url));
  EXPECT_EQ(fileapi::SYNC_DATABASE_ERROR_NOT_FOUND,
            drive_metadata_store()->ReadEntry(url, &metadata));
  EXPECT_EQ(fileapi::SYNC_DATABASE_ERROR_NOT_FOUND,
            drive_metadata_store()->DeleteEntry(url));
}

TEST_F(DriveMetadataStoreTest, GetConflictURLsTest) {
  InitializeDatabase();

  fileapi::FileSystemURLSet urls;
  EXPECT_EQ(fileapi::SYNC_STATUS_OK,
            drive_metadata_store()->GetConflictURLs(&urls));
  EXPECT_EQ(0U, urls.size());

  const FilePath path1(FPL("file1"));
  const FilePath path2(FPL("file2"));
  const FilePath path3(FPL("file3"));

  // Populate metadata in DriveMetadataStore. The metadata identified by "file2"
  // and "file3" are marked as conflicted.
  DriveMetadataStore* store = drive_metadata_store();
  EXPECT_EQ(fileapi::SYNC_STATUS_OK,
            store->UpdateEntry(URL(path1), CreateMetadata("1", "1", false)));
  EXPECT_EQ(fileapi::SYNC_STATUS_OK,
            store->UpdateEntry(URL(path2), CreateMetadata("2", "2", true)));
  EXPECT_EQ(fileapi::SYNC_STATUS_OK,
            store->UpdateEntry(URL(path3), CreateMetadata("3", "3", true)));

  EXPECT_EQ(fileapi::SYNC_STATUS_OK, store->GetConflictURLs(&urls));
  EXPECT_EQ(2U, urls.size());
  EXPECT_FALSE(ContainsKey(urls, URL(path1)));
  EXPECT_TRUE(ContainsKey(urls, URL(path2)));
  EXPECT_TRUE(ContainsKey(urls, URL(path3)));
}

TEST_F(DriveMetadataStoreTest, StoreSyncRootDirectory) {
  const std::string kResourceID("hoge");

  InitializeDatabase();

  EXPECT_TRUE(drive_metadata_store()->sync_root_directory().empty());

  drive_metadata_store()->SetSyncRootDirectory(kResourceID);
  EXPECT_EQ(kResourceID, drive_metadata_store()->sync_root_directory());

  DropSyncRootDirectoryInStore();
  EXPECT_TRUE(drive_metadata_store()->sync_root_directory().empty());

  RestoreSyncRootDirectoryFromDB();
  EXPECT_EQ(kResourceID, drive_metadata_store()->sync_root_directory());
}

TEST_F(DriveMetadataStoreTest, StoreSyncOrigin) {
  const GURL kOrigin1("http://www1.example.com");
  const GURL kOrigin2("http://www2.example.com");
  const std::string kResourceID1("hoge");
  const std::string kResourceID2("fuga");

  InitializeDatabase();
  DriveMetadataStore* store = drive_metadata_store();

  // Make sure origins have not been marked yet.
  EXPECT_FALSE(store->IsBatchSyncOrigin(kOrigin1));
  EXPECT_FALSE(store->IsBatchSyncOrigin(kOrigin2));
  EXPECT_FALSE(store->IsIncrementalSyncOrigin(kOrigin1));
  EXPECT_FALSE(store->IsIncrementalSyncOrigin(kOrigin2));

  // Mark origins as batch sync origins.
  store->AddBatchSyncOrigin(kOrigin1, kResourceID1);
  store->AddBatchSyncOrigin(kOrigin2, kResourceID2);
  EXPECT_TRUE(store->IsBatchSyncOrigin(kOrigin1));
  EXPECT_TRUE(store->IsBatchSyncOrigin(kOrigin2));
  EXPECT_EQ(kResourceID1, GetResourceID(store->batch_sync_origins(), kOrigin1));
  EXPECT_EQ(kResourceID2, GetResourceID(store->batch_sync_origins(), kOrigin2));

  // Mark |kOrigin1| as an incremental sync origin. |kOrigin2| should have still
  // been marked as a batch sync origin.
  store->MoveBatchSyncOriginToIncremental(kOrigin1);
  EXPECT_FALSE(store->IsBatchSyncOrigin(kOrigin1));
  EXPECT_TRUE(store->IsBatchSyncOrigin(kOrigin2));
  EXPECT_TRUE(store->IsIncrementalSyncOrigin(kOrigin1));
  EXPECT_FALSE(store->IsIncrementalSyncOrigin(kOrigin2));
  EXPECT_EQ(kResourceID1,
            GetResourceID(store->incremental_sync_origins(), kOrigin1));
  EXPECT_EQ(kResourceID2, GetResourceID(store->batch_sync_origins(), kOrigin2));

  DropSyncOriginsInStore();

  // Make sure origins have been dropped.
  EXPECT_FALSE(store->IsBatchSyncOrigin(kOrigin1));
  EXPECT_FALSE(store->IsBatchSyncOrigin(kOrigin2));
  EXPECT_FALSE(store->IsIncrementalSyncOrigin(kOrigin1));
  EXPECT_FALSE(store->IsIncrementalSyncOrigin(kOrigin2));

  RestoreSyncOriginsFromDB();

  // Make sure origins have been restored.
  EXPECT_FALSE(store->IsBatchSyncOrigin(kOrigin1));
  EXPECT_TRUE(store->IsBatchSyncOrigin(kOrigin2));
  EXPECT_TRUE(store->IsIncrementalSyncOrigin(kOrigin1));
  EXPECT_FALSE(store->IsIncrementalSyncOrigin(kOrigin2));
  EXPECT_EQ(kResourceID1,
            GetResourceID(store->incremental_sync_origins(), kOrigin1));
  EXPECT_EQ(kResourceID2, GetResourceID(store->batch_sync_origins(), kOrigin2));
}

}  // namespace sync_file_system
