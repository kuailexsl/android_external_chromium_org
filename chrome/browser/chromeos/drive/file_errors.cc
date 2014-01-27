// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/file_errors.h"

#include "base/logging.h"

namespace drive {

std::string FileErrorToString(FileError error) {
  switch (error) {
    case FILE_ERROR_OK:
      return "FILE_ERROR_OK";

    case FILE_ERROR_FAILED:
      return "FILE_ERROR_FAILED";

    case FILE_ERROR_IN_USE:
      return "FILE_ERROR_IN_USE";

    case FILE_ERROR_EXISTS:
      return "FILE_ERROR_EXISTS";

    case FILE_ERROR_NOT_FOUND:
      return "FILE_ERROR_NOT_FOUND";

    case FILE_ERROR_ACCESS_DENIED:
      return "FILE_ERROR_ACCESS_DENIED";

    case FILE_ERROR_TOO_MANY_OPENED:
      return "FILE_ERROR_TOO_MANY_OPENED";

    case FILE_ERROR_NO_MEMORY:
      return "FILE_ERROR_NO_MEMORY";

    case FILE_ERROR_NO_SERVER_SPACE:
      return "FILE_ERROR_NO_SERVER_SPACE";

    case FILE_ERROR_NOT_A_DIRECTORY:
      return "FILE_ERROR_NOT_A_DIRECTORY";

    case FILE_ERROR_INVALID_OPERATION:
      return "FILE_ERROR_INVALID_OPERATION";

    case FILE_ERROR_SECURITY:
      return "FILE_ERROR_SECURITY";

    case FILE_ERROR_ABORT:
      return "FILE_ERROR_ABORT";

    case FILE_ERROR_NOT_A_FILE:
      return "FILE_ERROR_NOT_A_FILE";

    case FILE_ERROR_NOT_EMPTY:
      return "FILE_ERROR_NOT_EMPTY";

    case FILE_ERROR_INVALID_URL:
      return "FILE_ERROR_INVALID_URL";

    case FILE_ERROR_NO_CONNECTION:
      return "FILE_ERROR_NO_CONNECTION";

    case FILE_ERROR_NO_LOCAL_SPACE:
      return "FILE_ERROR_NO_LOCAL_SPACE";

    case FILE_ERROR_SERVICE_UNAVAILABLE:
      return "FILE_ERROR_SERVICE_UNAVAILABLE";
  }

  NOTREACHED();
  return "";
}

base::File::Error FileErrorToBaseFileError(FileError error) {
  switch (error) {
    case FILE_ERROR_OK:
      return base::File::FILE_OK;

    case FILE_ERROR_FAILED:
      return base::File::FILE_ERROR_FAILED;

    case FILE_ERROR_IN_USE:
      return base::File::FILE_ERROR_IN_USE;

    case FILE_ERROR_EXISTS:
      return base::File::FILE_ERROR_EXISTS;

    case FILE_ERROR_NOT_FOUND:
      return base::File::FILE_ERROR_NOT_FOUND;

    case FILE_ERROR_ACCESS_DENIED:
      return base::File::FILE_ERROR_ACCESS_DENIED;

    case FILE_ERROR_TOO_MANY_OPENED:
      return base::File::FILE_ERROR_TOO_MANY_OPENED;

    case FILE_ERROR_NO_MEMORY:
      return base::File::FILE_ERROR_NO_MEMORY;

    case FILE_ERROR_NO_SERVER_SPACE:
      return base::File::FILE_ERROR_NO_SPACE;

    case FILE_ERROR_NOT_A_DIRECTORY:
      return base::File::FILE_ERROR_NOT_A_DIRECTORY;

    case FILE_ERROR_INVALID_OPERATION:
      return base::File::FILE_ERROR_INVALID_OPERATION;

    case FILE_ERROR_SECURITY:
      return base::File::FILE_ERROR_SECURITY;

    case FILE_ERROR_ABORT:
      return base::File::FILE_ERROR_ABORT;

    case FILE_ERROR_NOT_A_FILE:
      return base::File::FILE_ERROR_NOT_A_FILE;

    case FILE_ERROR_NOT_EMPTY:
      return base::File::FILE_ERROR_NOT_EMPTY;

    case FILE_ERROR_INVALID_URL:
      return base::File::FILE_ERROR_INVALID_URL;

    case FILE_ERROR_NO_CONNECTION:
      return base::File::FILE_ERROR_FAILED;

    case FILE_ERROR_NO_LOCAL_SPACE:
      return base::File::FILE_ERROR_FAILED;

    case FILE_ERROR_SERVICE_UNAVAILABLE:
      return base::File::FILE_ERROR_FAILED;
  }

  NOTREACHED();
  return base::File::FILE_ERROR_FAILED;
}

FileError GDataToFileError(google_apis::GDataErrorCode status) {
  switch (status) {
    case google_apis::HTTP_SUCCESS:
    case google_apis::HTTP_CREATED:
    case google_apis::HTTP_NO_CONTENT:
      return FILE_ERROR_OK;
    case google_apis::HTTP_UNAUTHORIZED:
    case google_apis::HTTP_FORBIDDEN:
      return FILE_ERROR_ACCESS_DENIED;
    case google_apis::HTTP_NOT_FOUND:
    case google_apis::HTTP_GONE:
      return FILE_ERROR_NOT_FOUND;
    case google_apis::HTTP_INTERNAL_SERVER_ERROR:
    case google_apis::HTTP_SERVICE_UNAVAILABLE:
      return FILE_ERROR_SERVICE_UNAVAILABLE;
    case google_apis::HTTP_NOT_IMPLEMENTED:
      return FILE_ERROR_INVALID_OPERATION;
    case google_apis::GDATA_CANCELLED:
      return FILE_ERROR_ABORT;
    case google_apis::GDATA_NO_CONNECTION:
      return FILE_ERROR_NO_CONNECTION;
    case google_apis::GDATA_NO_SPACE:
      return FILE_ERROR_NO_SERVER_SPACE;
    default:
      return FILE_ERROR_FAILED;
  }
}

}  // namespace drive
