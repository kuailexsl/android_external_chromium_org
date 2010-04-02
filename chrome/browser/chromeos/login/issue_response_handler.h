// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_ISSUE_RESPONSE_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_ISSUE_RESPONSE_HANDLER_H_

#include <string>

#include "base/logging.h"
#include "chrome/browser/chromeos/login/auth_response_handler.h"

class URLRequestContextGetter;

class IssueResponseHandler : public AuthResponseHandler {
 public:
  explicit IssueResponseHandler(URLRequestContextGetter* getter)
      : getter_(getter) {}
  virtual ~IssueResponseHandler() {}

  // Overridden from AuthResponseHandler.
  virtual bool CanHandle(const GURL& url);

  // Overridden from AuthResponseHandler.
  // Takes in a response from IssueAuthToken, formats into an appropriate query
  // to sent to TokenAuth, and issues said query.  |catcher| will receive
  // the response to the fetch.
  virtual URLFetcher* Handle(const std::string& to_process,
                             URLFetcher::Delegate* catcher);

  // exposed for testing
  std::string token_url() { return token_url_; }
 private:
  std::string token_url_;
  URLRequestContextGetter* getter_;
};

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_ISSUE_RESPONSE_HANDLER_H_
