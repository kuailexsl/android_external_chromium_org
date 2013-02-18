// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_FILE_SYSTEM_SYNC_FILE_SYSTEM_SERVICE_H_
#define CHROME_BROWSER_SYNC_FILE_SYSTEM_SYNC_FILE_SYSTEM_SERVICE_H_

#include <set>
#include <string>

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/api/sync/profile_sync_service_observer.h"
#include "chrome/browser/profiles/profile_keyed_service.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "chrome/browser/sync_file_system/file_status_observer.h"
#include "chrome/browser/sync_file_system/local_file_sync_service.h"
#include "chrome/browser/sync_file_system/remote_file_sync_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "googleurl/src/gurl.h"
#include "webkit/fileapi/syncable/sync_callbacks.h"

class ProfileSyncServiceBase;

namespace fileapi {
class FileSystemContext;
}

namespace sync_file_system {

class SyncEventObserver;

class SyncFileSystemService
    : public ProfileKeyedService,
      public ProfileSyncServiceObserver,
      public LocalFileSyncService::Observer,
      public RemoteFileSyncService::Observer,
      public FileStatusObserver,
      public content::NotificationObserver,
      public base::SupportsWeakPtr<SyncFileSystemService> {
 public:
  // ProfileKeyedService overrides.
  virtual void Shutdown() OVERRIDE;

  void InitializeForApp(
      fileapi::FileSystemContext* file_system_context,
      const std::string& service_name,
      const GURL& app_origin,
      const fileapi::SyncStatusCallback& callback);

  // Returns the file |url|'s sync status.
  void GetFileSyncStatus(
      const fileapi::FileSystemURL& url,
      const fileapi::SyncFileStatusCallback& callback);

  void AddSyncEventObserver(SyncEventObserver* observer);
  void RemoveSyncEventObserver(SyncEventObserver* observer);

 private:
  friend class SyncFileSystemServiceFactory;
  friend class SyncFileSystemServiceTest;
  friend struct base::DefaultDeleter<SyncFileSystemService>;

  explicit SyncFileSystemService(Profile* profile);
  virtual ~SyncFileSystemService();

  void Initialize(scoped_ptr<LocalFileSyncService> local_file_service,
                  scoped_ptr<RemoteFileSyncService> remote_file_service);

  void DidGetConflictFileInfo(const fileapi::ConflictFileInfoCallback& callback,
                              const fileapi::FileSystemURL& url,
                              const fileapi::SyncFileMetadata* local_metadata,
                              const fileapi::SyncFileMetadata* remote_metadata,
                              fileapi::SyncStatusCode status);

  // Callbacks for InitializeForApp.
  void DidInitializeFileSystem(const GURL& app_origin,
                               const fileapi::SyncStatusCallback& callback,
                               fileapi::SyncStatusCode status);
  void DidRegisterOrigin(const GURL& app_origin,
                         const fileapi::SyncStatusCallback& callback,
                         fileapi::SyncStatusCode status);

  // Overrides sync_enabled_ setting. This should be called only by tests.
  void SetSyncEnabledForTesting(bool enabled);

  // Called when following observer methods are called:
  // - OnLocalChangeAvailable()
  // - OnRemoteChangeAvailable()
  // - OnRemoteServiceStateUpdated()
  void MaybeStartSync();

  // Called from MaybeStartSync().
  void MaybeStartRemoteSync();
  void MaybeStartLocalSync();

  // Callbacks for remote/local sync.
  void DidProcessRemoteChange(fileapi::SyncStatusCode status,
                              const fileapi::FileSystemURL& url);
  void DidProcessLocalChange(fileapi::SyncStatusCode status,
                             const fileapi::FileSystemURL& url);

  void DidGetLocalChangeStatus(const fileapi::SyncFileStatusCallback& callback,
                               fileapi::SyncStatusCode status,
                               bool has_pending_local_changes);

  void OnSyncEnabledForRemoteSync();

  // RemoteFileSyncService::Observer overrides.
  virtual void OnLocalChangeAvailable(int64 pending_changes) OVERRIDE;

  // LocalFileSyncService::Observer overrides.
  virtual void OnRemoteChangeQueueUpdated(int64 pending_changes) OVERRIDE;
  virtual void OnRemoteServiceStateUpdated(
      RemoteServiceState state,
      const std::string& description) OVERRIDE;

  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // ProfileSyncServiceObserver:
  virtual void OnStateChanged() OVERRIDE;

  // SyncFileStatusObserver:
  virtual void OnFileStatusChanged(
      const fileapi::FileSystemURL& url,
      fileapi::SyncFileStatus sync_status,
      fileapi::SyncAction action_taken,
      fileapi::SyncDirection direction) OVERRIDE;

  // Check the profile's sync preference settings and call
  // remote_file_service_->SetSyncEnabled() to update the status.
  // |profile_sync_service| must be non-null.
  void UpdateSyncEnabledStatus(ProfileSyncServiceBase* profile_sync_service);

  Profile* profile_;
  content::NotificationRegistrar registrar_;

  int64 pending_local_changes_;
  int64 pending_remote_changes_;

  scoped_ptr<LocalFileSyncService> local_file_service_;
  scoped_ptr<RemoteFileSyncService> remote_file_service_;

  bool local_sync_running_;
  bool remote_sync_running_;

  // If a remote sync is returned with SYNC_STATUS_FILE_BUSY we mark this
  // true and register the busy file URL to wait for a sync enabled event
  // for the URL. When this flag is set to true it won't be worth trying
  // another remote sync.
  bool is_waiting_remote_sync_enabled_;

  // Indicates if sync is currently enabled or not.
  bool sync_enabled_;

  ObserverList<SyncEventObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(SyncFileSystemService);
};

class SyncFileSystemServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static SyncFileSystemService* GetForProfile(Profile* profile);
  static SyncFileSystemService* FindForProfile(Profile* profile);
  static SyncFileSystemServiceFactory* GetInstance();

  // This overrides the remote service for testing.
  // For testing this must be called before GetForProfile is called.
  // Otherwise a new DriveFileSyncService is created for the new service.
  // Since we use scoped_ptr it's one-off and the instance is passed
  // to the newly created SyncFileSystemService.
  void set_mock_remote_file_service(
      scoped_ptr<RemoteFileSyncService> mock_remote_service);

 private:
  friend struct DefaultSingletonTraits<SyncFileSystemServiceFactory>;
  SyncFileSystemServiceFactory();
  virtual ~SyncFileSystemServiceFactory();

  // ProfileKeyedServiceFactory overrides.
  virtual ProfileKeyedService* BuildServiceInstanceFor(
      Profile* profile) const OVERRIDE;

  mutable scoped_ptr<RemoteFileSyncService> mock_remote_file_service_;
};

}  // namespace sync_file_system

#endif  // CHROME_BROWSER_SYNC_FILE_SYSTEM_SYNC_FILE_SYSTEM_SERVICE_H_
