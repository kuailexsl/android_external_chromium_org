// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_IMPL_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_IMPL_H_

#include "content/browser/download/download_file.h"

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time.h"
#include "base/timer.h"
#include "content/browser/download/base_file.h"
#include "content/browser/download/byte_stream.h"
#include "content/public/browser/download_save_info.h"
#include "net/base/net_log.h"

namespace content {
class ByteStreamReader;
class DownloadDestinationObserver;
class DownloadManager;
class PowerSaveBlocker;
struct DownloadCreateInfo;

class CONTENT_EXPORT DownloadFileImpl : virtual public DownloadFile {
 public:
  // Takes ownership of the object pointed to by |request_handle|.
  // |bound_net_log| will be used for logging the download file's events.
  // May be constructed on any thread.  All methods besides the constructor
  // (including destruction) must occur on the FILE thread.
  //
  // Note that the DownloadFileImpl automatically reads from the passed in
  // stream, and sends updates and status of those reads to the
  // DownloadDestinationObserver.
  DownloadFileImpl(
    scoped_ptr<DownloadSaveInfo> save_info,
    const FilePath& default_downloads_directory,
    const GURL& url,
    const GURL& referrer_url,
    int64 received_bytes,
    bool calculate_hash,
    scoped_ptr<ByteStreamReader> stream,
    const net::BoundNetLog& bound_net_log,
    scoped_ptr<PowerSaveBlocker> power_save_blocker,
    base::WeakPtr<DownloadDestinationObserver> observer);

  virtual ~DownloadFileImpl();

  // DownloadFile functions.
  virtual void Initialize(const InitializeCallback& callback) OVERRIDE;
  virtual void Rename(const FilePath& full_path,
                      bool overwrite_existing_file,
                      const RenameCompletionCallback& callback) OVERRIDE;
  virtual void Detach(const DetachCompletionCallback& callback) OVERRIDE;
  virtual void Cancel() OVERRIDE;
  virtual FilePath FullPath() const OVERRIDE;
  virtual bool InProgress() const OVERRIDE;
  virtual int64 BytesSoFar() const OVERRIDE;
  virtual int64 CurrentSpeed() const OVERRIDE;
  virtual bool GetHash(std::string* hash) OVERRIDE;
  virtual std::string GetHashState() OVERRIDE;

 protected:
  // For test class overrides.
  virtual DownloadInterruptReason AppendDataToFile(
      const char* data, size_t data_len);

 private:
  // Send an update on our progress.
  void SendUpdate();

  // Called when there's some activity on stream_reader_ that needs to be
  // handled.
  void StreamActive();

  // The base file instance.
  BaseFile file_;

  // The default directory for creating the download file.
  FilePath default_download_directory_;

  // The stream through which data comes.
  // TODO(rdsmith): Move this into BaseFile; requires using the same
  // stream semantics in SavePackage.  Alternatively, replace SaveFile
  // with DownloadFile and get rid of BaseFile.
  scoped_ptr<ByteStreamReader> stream_reader_;

  // Used to trigger progress updates.
  scoped_ptr<base::RepeatingTimer<DownloadFileImpl> > update_timer_;

  // Statistics
  size_t bytes_seen_;
  base::TimeDelta disk_writes_time_;
  base::TimeTicks download_start_;

  net::BoundNetLog bound_net_log_;

  base::WeakPtr<DownloadDestinationObserver> observer_;

  base::WeakPtrFactory<DownloadFileImpl> weak_factory_;

  // RAII handle to keep the system from sleeping while we're downloading.
  scoped_ptr<PowerSaveBlocker> power_save_blocker_;

  DISALLOW_COPY_AND_ASSIGN(DownloadFileImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_IMPL_H_
