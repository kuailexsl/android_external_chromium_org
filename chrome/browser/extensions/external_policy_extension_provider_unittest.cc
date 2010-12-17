// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/logging.h"
#include "base/values.h"
#include "base/version.h"
#include "chrome/browser/browser_thread.h"
#include "chrome/browser/extensions/external_policy_extension_provider.h"
#include "chrome/common/extensions/extension.h"
#include "testing/gtest/include/gtest/gtest.h"

class ExternalPolicyExtensionProviderTest : public testing::Test {
 public:
  ExternalPolicyExtensionProviderTest()
      : loop_(MessageLoop::TYPE_IO),
        ui_thread_(BrowserThread::UI, &loop_),
        file_thread_(BrowserThread::FILE, &loop_) {
  }

  virtual ~ExternalPolicyExtensionProviderTest() {
  }

 private:
  MessageLoop loop_;
  BrowserThread ui_thread_;
  BrowserThread file_thread_;
};

class MockExternalPolicyExtensionProviderVisitor
    : public ExternalExtensionProvider::Visitor {
 public:
  MockExternalPolicyExtensionProviderVisitor() {
  }

  // Initialize a provider with |policy_forcelist|, and check that it parses
  // exactly those extensions, that are specified in |policy_validlist|.
  void Visit(ListValue* policy_forcelist,
             ListValue* policy_validlist,
             const std::set<std::string>& ignore_list) {
    provider_.reset(new ExternalPolicyExtensionProvider(policy_forcelist));

    // Extensions will be removed from this list as they visited,
    // so it should be emptied by the end.
    remaining_extensions = policy_validlist;
    provider_->VisitRegisteredExtension(this);
    EXPECT_EQ(0u, remaining_extensions->GetSize());
  }

  virtual void OnExternalExtensionFileFound(const std::string& id,
                                            const Version* version,
                                            const FilePath& path,
                                            Extension::Location unused) {
    ADD_FAILURE() << "There should be no external extensions from files.";
  }

  virtual void OnExternalExtensionUpdateUrlFound(
      const std::string& id, const GURL& update_url,
      Extension::Location location) {
    // Extension has the correct location.
    EXPECT_EQ(Extension::EXTERNAL_POLICY_DOWNLOAD, location);

    // Provider returns the correct location when asked.
    Extension::Location location1;
    scoped_ptr<Version> version1;
    provider_->GetExtensionDetails(id, &location1, &version1);
    EXPECT_EQ(Extension::EXTERNAL_POLICY_DOWNLOAD, location1);
    EXPECT_FALSE(version1.get());

    // Remove the extension from our list.
    StringValue ext_str(id + ";" + update_url.spec());
    EXPECT_NE(-1, remaining_extensions->Remove(ext_str));
 }

 private:
  ListValue* remaining_extensions;

  scoped_ptr<ExternalPolicyExtensionProvider> provider_;

  DISALLOW_COPY_AND_ASSIGN(MockExternalPolicyExtensionProviderVisitor);
};

TEST_F(ExternalPolicyExtensionProviderTest, PolicyIsParsed) {
  ListValue forced_extensions;
  forced_extensions.Append(Value::CreateStringValue(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;http://www.example.com/crx?a=5;b=6"));
  forced_extensions.Append(Value::CreateStringValue(
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb;"
      "https://clients2.google.com/service/update2/crx"));

  MockExternalPolicyExtensionProviderVisitor mv;
  std::set<std::string> empty;
  mv.Visit(&forced_extensions, &forced_extensions, empty);
}

TEST_F(ExternalPolicyExtensionProviderTest, InvalidPolicyIsNotParsed) {
  ListValue forced_extensions, valid_extensions;
  StringValue valid(
      "cccccccccccccccccccccccccccccccc;http://www.example.com/crx");
  valid_extensions.Append(valid.DeepCopy());
  forced_extensions.Append(valid.DeepCopy());
  // Add invalid strings:
  forced_extensions.Append(Value::CreateStringValue(""));
  forced_extensions.Append(Value::CreateStringValue(";"));
  forced_extensions.Append(Value::CreateStringValue(";;"));
  forced_extensions.Append(Value::CreateStringValue(
      ";http://www.example.com/crx"));
  forced_extensions.Append(Value::CreateStringValue(
      ";aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  forced_extensions.Append(Value::CreateStringValue(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;"));
  forced_extensions.Append(Value::CreateStringValue(
      "http://www.example.com/crx;aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  forced_extensions.Append(Value::CreateStringValue(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;http#//www.example.com/crx"));
  forced_extensions.Append(Value::CreateStringValue(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
  forced_extensions.Append(Value::CreateStringValue(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaahttp#//www.example.com/crx"));

  MockExternalPolicyExtensionProviderVisitor mv;
  std::set<std::string> empty;
  mv.Visit(&forced_extensions, &valid_extensions, empty);
}
