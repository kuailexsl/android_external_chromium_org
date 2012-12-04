// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_NET_NET_UTIL_H_
#define CHROME_TEST_CHROMEDRIVER_NET_NET_UTIL_H_

#include <string>

class GURL;
class URLRequestContextGetter;

// Synchronously fetches data from a GET HTTP request to the given URL.
// Returns true if response is 200 OK and sets response body to |response|.
bool FetchUrl(const GURL& url,
              URLRequestContextGetter* getter,
              std::string* response);

#endif  // CHROME_TEST_CHROMEDRIVER_NET_NET_UTIL_H_
