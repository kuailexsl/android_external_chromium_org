// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GOOGLE_APIS_AUTH_SERVICE_OBSERVER_H_
#define CHROME_BROWSER_GOOGLE_APIS_AUTH_SERVICE_OBSERVER_H_

namespace gdata {

// Interface for classes that need to observe events from AuthService.
// All events are notified on UI thread.
class AuthServiceObserver {
 public:
  // Triggered when a new OAuth2 refresh token is received from TokenService.
  virtual void OnOAuth2RefreshTokenChanged() = 0;

 protected:
  virtual ~AuthServiceObserver() {}
};

}  // namespace gdata

#endif  // CHROME_BROWSER_GOOGLE_APIS_AUTH_SERVICE_OBSERVER_H_
