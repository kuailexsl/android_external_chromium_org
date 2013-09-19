// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_FILEAPI_FILE_SYSTEM_DISPATCHER_H_
#define CONTENT_CHILD_FILEAPI_FILE_SYSTEM_DISPATCHER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/id_map.h"
#include "base/process/process.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_platform_file.h"
#include "webkit/common/fileapi/file_system_types.h"
#include "webkit/common/quota/quota_types.h"

namespace base {
class FilePath;
struct PlatformFileInfo;
}

namespace fileapi {
struct DirectoryEntry;
}

class GURL;

namespace content {

// Dispatches and sends file system related messages sent to/from a child
// process from/to the main browser process.  There is one instance
// per child process.  Messages are dispatched on the main child thread.
class FileSystemDispatcher : public IPC::Listener {
 public:
  typedef base::Callback<void(base::PlatformFileError error)> StatusCallback;
  typedef base::Callback<void(
      const base::PlatformFileInfo& file_info)> MetadataCallback;
  typedef base::Callback<void(
      const base::PlatformFileInfo& file_info,
      const base::FilePath& platform_path,
      int request_id)> CreateSnapshotFileCallback;
  typedef base::Callback<void(
      const std::vector<fileapi::DirectoryEntry>& entries,
      bool has_more)> ReadDirectoryCallback;
  typedef base::Callback<void(
      const std::string& name,
      const GURL& root)> OpenFileSystemCallback;
  typedef base::Callback<void(
      int64 bytes,
      bool complete)> WriteCallback;
  typedef base::Callback<void(
      base::PlatformFile file,
      int file_open_id,
      quota::QuotaLimitType quota_policy)> OpenFileCallback;

  FileSystemDispatcher();
  virtual ~FileSystemDispatcher();

  // IPC::Listener implementation.
  virtual bool OnMessageReceived(const IPC::Message& msg) OVERRIDE;

  void OpenFileSystem(const GURL& origin_url,
                      fileapi::FileSystemType type,
                      long long size,
                      bool create,
                      const OpenFileSystemCallback& success_callback,
                      const StatusCallback& error_callback);
  void DeleteFileSystem(const GURL& origin_url,
                        fileapi::FileSystemType type,
                        const StatusCallback& callback);
  void Move(const GURL& src_path,
            const GURL& dest_path,
            const StatusCallback& callback);
  void Copy(const GURL& src_path,
            const GURL& dest_path,
            const StatusCallback& callback);
  void Remove(const GURL& path,
              bool recursive,
              const StatusCallback& callback);
  void ReadMetadata(const GURL& path,
                    const MetadataCallback& success_callback,
                    const StatusCallback& error_callback);
  void CreateFile(const GURL& path,
                  bool exclusive,
                  const StatusCallback& callback);
  void CreateDirectory(const GURL& path,
                       bool exclusive,
                       bool recursive,
                       const StatusCallback& callback);
  void Exists(const GURL& path,
              bool for_directory,
              const StatusCallback& callback);
  void ReadDirectory(const GURL& path,
                     const ReadDirectoryCallback& success_callback,
                     const StatusCallback& error_callback);
  void Truncate(const GURL& path,
                int64 offset,
                int* request_id_out,
                const StatusCallback& callback);
  void WriteDeprecated(
      const GURL& path,
      const GURL& blob_url,
      int64 offset,
      int* request_id_out,
      const WriteCallback& success_callback,
      const StatusCallback& error_callback);
  void Write(const GURL& path,
             const std::string& blob_id,
             int64 offset,
             int* request_id_out,
             const WriteCallback& success_callback,
             const StatusCallback& error_callback);
  void Cancel(int request_id_to_cancel,
              const StatusCallback& callback);
  void TouchFile(const GURL& file_path,
                 const base::Time& last_access_time,
                 const base::Time& last_modified_time,
                 const StatusCallback& callback);

  // This returns a raw open PlatformFile, unlike the above, which are
  // self-contained operations.
  void OpenPepperFile(const GURL& file_path,
                      int pp_open_flags,
                      const OpenFileCallback& success_callback,
                      const StatusCallback& error_callback);
  // This must be paired with OpenFile, and called after finished using the
  // raw PlatformFile returned from OpenFile.
  void NotifyCloseFile(int file_open_id);

  // The caller must send FileSystemHostMsg_DidReceiveSnapshot message
  // with |request_id| passed to |success_callback| after the snapshot file
  // is successfully received.
  void CreateSnapshotFile(const GURL& file_path,
                          const CreateSnapshotFileCallback& success_callback,
                          const StatusCallback& error_callback);

 private:
  class CallbackDispatcher;

  // Message handlers.
  void OnDidOpenFileSystem(int request_id,
                           const std::string& name,
                           const GURL& root);
  void OnDidSucceed(int request_id);
  void OnDidReadMetadata(int request_id,
                         const base::PlatformFileInfo& file_info);
  void OnDidCreateSnapshotFile(int request_id,
                               const base::PlatformFileInfo& file_info,
                               const base::FilePath& platform_path);
  void OnDidReadDirectory(int request_id,
                          const std::vector<fileapi::DirectoryEntry>& entries,
                          bool has_more);
  void OnDidFail(int request_id, base::PlatformFileError error_code);
  void OnDidWrite(int request_id, int64 bytes, bool complete);
  void OnDidOpenFile(
      int request_id,
      IPC::PlatformFileForTransit file,
      int file_open_id,
      quota::QuotaLimitType quota_policy);

  IDMap<CallbackDispatcher, IDMapOwnPointer> dispatchers_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemDispatcher);
};

}  // namespace content

#endif  // CONTENT_CHILD_FILEAPI_FILE_SYSTEM_DISPATCHER_H_
