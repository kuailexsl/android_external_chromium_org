// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_CONTENT_PASSWORD_MANAGER_DRIVER_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_CONTENT_PASSWORD_MANAGER_DRIVER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "chrome/browser/password_manager/password_manager_driver.h"

namespace content {
class WebContents;
}

class ContentPasswordManagerDriver : public PasswordManagerDriver {
 public:
  explicit ContentPasswordManagerDriver(content::WebContents* web_contents);
  virtual ~ContentPasswordManagerDriver();

  // PasswordManagerDriver implementation.
  virtual void FillPasswordForm(
      const autofill::PasswordFormFillData& form_data) OVERRIDE;
  virtual bool DidLastPageLoadEncounterSSLErrors() OVERRIDE;

 private:
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(ContentPasswordManagerDriver);
};

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_CONTENT_PASSWORD_MANAGER_DRIVER_H_
