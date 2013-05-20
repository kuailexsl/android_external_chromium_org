// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_PATH_H_
#define CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_PATH_H_

#include <vector>

#include "base/logging.h"
#include "base/string16.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebIDBKeyPath.h"

namespace content {

class CONTENT_EXPORT IndexedDBKeyPath {
 public:
  IndexedDBKeyPath();  // Defaults to WebKit::WebIDBKeyPath::NullType.
  explicit IndexedDBKeyPath(const string16&);
  explicit IndexedDBKeyPath(const std::vector<string16>&);
  explicit IndexedDBKeyPath(const WebKit::WebIDBKeyPath&);
  ~IndexedDBKeyPath();

  bool IsNull() const { return type_ == WebKit::WebIDBKeyPath::NullType; }
  bool IsValid() const;
  bool operator==(const IndexedDBKeyPath& other) const;

  WebKit::WebIDBKeyPath::Type type() const { return type_; }
  const std::vector<string16>& array() const;
  const string16& string() const;
  operator WebKit::WebIDBKeyPath() const;

 private:
  WebKit::WebIDBKeyPath::Type type_;
  string16 string_;
  std::vector<string16> array_;
};

}  // namespace content

#endif  // CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_PATH_H_
