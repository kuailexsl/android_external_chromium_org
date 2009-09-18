// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Handle passing definitions for NaCl
#ifndef CHROME_COMMON_NACL_TYPES_H_
#define CHROME_COMMON_NACL_TYPES_H_

namespace nacl {
#if defined(OS_WIN)
typedef HANDLE FileDescriptor;
#define NATIVE_HANDLE(desc) (static_cast<HANDLE>(desc))
#elif defined(OS_POSIX)
typedef base::FileDescriptor FileDescriptor;
#define NATIVE_HANDLE(desc) ((desc).fd)
#endif
}

#endif  // CHROME_COMMON_NACL_TYPES_H_
