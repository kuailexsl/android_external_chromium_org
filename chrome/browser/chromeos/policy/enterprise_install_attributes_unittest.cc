// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/enterprise_install_attributes.h"

#include "base/bind.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_ptr.h"
#include "base/run_loop.h"
#include "chrome/browser/policy/proto/chromeos/install_attributes.pb.h"
#include "chromeos/cryptohome/cryptohome_library.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

void CopyLockResult(base::RunLoop* loop,
                    EnterpriseInstallAttributes::LockResult* out,
                    EnterpriseInstallAttributes::LockResult result) {
  *out = result;
  loop->Quit();
}

}  // namespace

static const char kTestUser[] = "test@example.com";
static const char kTestDomain[] = "example.com";
static const char kTestDeviceId[] = "133750519";

class EnterpriseInstallAttributesTest : public testing::Test {
 protected:
  EnterpriseInstallAttributesTest()
      : cryptohome_(chromeos::CryptohomeLibrary::GetTestImpl()),
        stub_cryptohome_client_(chromeos::CryptohomeClient::Create(
            chromeos::STUB_DBUS_CLIENT_IMPLEMENTATION, NULL)),
        install_attributes_(cryptohome_.get(), stub_cryptohome_client_.get()) {}

  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  base::FilePath GetTempPath() const {
    return temp_dir_.path().Append("install_attrs_test");
  }

  void SetAttribute(
      cryptohome::SerializedInstallAttributes* install_attrs_proto,
      const std::string& name,
      const std::string& value) {
    cryptohome::SerializedInstallAttributes::Attribute* attribute;
    attribute = install_attrs_proto->add_attributes();
    attribute->set_name(name);
    attribute->set_value(value);
  }

  MessageLoopForUI message_loop_;
  base::ScopedTempDir temp_dir_;
  scoped_ptr<chromeos::CryptohomeLibrary> cryptohome_;
  scoped_ptr<chromeos::CryptohomeClient> stub_cryptohome_client_;
  EnterpriseInstallAttributes install_attributes_;

  EnterpriseInstallAttributes::LockResult LockDeviceAndWaitForResult(
      const std::string& user,
      DeviceMode device_mode,
      const std::string& device_id) {
    base::RunLoop loop;
    EnterpriseInstallAttributes::LockResult result;
    install_attributes_.LockDevice(user, device_mode, device_id,
                                   base::Bind(&CopyLockResult, &loop, &result));
    loop.Run();
    return result;
  }
};

TEST_F(EnterpriseInstallAttributesTest, Lock) {
  EXPECT_EQ(EnterpriseInstallAttributes::LOCK_SUCCESS,
            LockDeviceAndWaitForResult(
                kTestUser,
                DEVICE_MODE_ENTERPRISE,
                kTestDeviceId));

  EXPECT_EQ(EnterpriseInstallAttributes::LOCK_SUCCESS,
            LockDeviceAndWaitForResult(
                kTestUser,
                DEVICE_MODE_ENTERPRISE,
                kTestDeviceId));
  // Another user from the same domain should also succeed.
  EXPECT_EQ(EnterpriseInstallAttributes::LOCK_SUCCESS,
            LockDeviceAndWaitForResult(
                "test1@example.com",
                DEVICE_MODE_ENTERPRISE,
                kTestDeviceId));
  // But another domain should fail.
  EXPECT_EQ(EnterpriseInstallAttributes::LOCK_WRONG_USER,
            LockDeviceAndWaitForResult(
                "test@bluebears.com",
                DEVICE_MODE_ENTERPRISE,
                kTestDeviceId));
}

TEST_F(EnterpriseInstallAttributesTest, IsEnterpriseDevice) {
  install_attributes_.ReadCacheFile(GetTempPath());
  EXPECT_FALSE(install_attributes_.IsEnterpriseDevice());
  ASSERT_EQ(EnterpriseInstallAttributes::LOCK_SUCCESS,
            LockDeviceAndWaitForResult(
                kTestUser,
                DEVICE_MODE_ENTERPRISE,
                kTestDeviceId));
  EXPECT_TRUE(install_attributes_.IsEnterpriseDevice());
}

TEST_F(EnterpriseInstallAttributesTest, GetDomain) {
  install_attributes_.ReadCacheFile(GetTempPath());
  EXPECT_EQ(std::string(), install_attributes_.GetDomain());
  ASSERT_EQ(EnterpriseInstallAttributes::LOCK_SUCCESS,
            LockDeviceAndWaitForResult(
                kTestUser,
                DEVICE_MODE_ENTERPRISE,
                kTestDeviceId));
  EXPECT_EQ(kTestDomain, install_attributes_.GetDomain());
}

TEST_F(EnterpriseInstallAttributesTest, GetRegistrationUser) {
  install_attributes_.ReadCacheFile(GetTempPath());
  EXPECT_EQ(std::string(), install_attributes_.GetRegistrationUser());
  ASSERT_EQ(EnterpriseInstallAttributes::LOCK_SUCCESS,
            LockDeviceAndWaitForResult(
                kTestUser,
                DEVICE_MODE_ENTERPRISE,
                kTestDeviceId));
  EXPECT_EQ(kTestUser, install_attributes_.GetRegistrationUser());
}

TEST_F(EnterpriseInstallAttributesTest, GetDeviceId) {
  install_attributes_.ReadCacheFile(GetTempPath());
  EXPECT_EQ(std::string(), install_attributes_.GetDeviceId());
  ASSERT_EQ(EnterpriseInstallAttributes::LOCK_SUCCESS,
            LockDeviceAndWaitForResult(
                kTestUser,
                DEVICE_MODE_ENTERPRISE,
                kTestDeviceId));
  EXPECT_EQ(kTestDeviceId, install_attributes_.GetDeviceId());
}

TEST_F(EnterpriseInstallAttributesTest, GetMode) {
  install_attributes_.ReadCacheFile(GetTempPath());
  EXPECT_EQ(DEVICE_MODE_PENDING, install_attributes_.GetMode());
  ASSERT_EQ(EnterpriseInstallAttributes::LOCK_SUCCESS,
            LockDeviceAndWaitForResult(
                kTestUser,
                DEVICE_MODE_KIOSK,
                kTestDeviceId));
  EXPECT_EQ(DEVICE_MODE_KIOSK,
            install_attributes_.GetMode());
}

TEST_F(EnterpriseInstallAttributesTest, ConsumerDevice) {
  install_attributes_.ReadCacheFile(GetTempPath());
  EXPECT_EQ(DEVICE_MODE_PENDING, install_attributes_.GetMode());
  // Lock the attributes empty.
  ASSERT_TRUE(cryptohome_->InstallAttributesFinalize());
  base::RunLoop loop;
  install_attributes_.ReadImmutableAttributes(base::Bind(loop.QuitClosure()));
  loop.Run();

  ASSERT_FALSE(cryptohome_->InstallAttributesIsFirstInstall());
  EXPECT_EQ(DEVICE_MODE_CONSUMER, install_attributes_.GetMode());
}

TEST_F(EnterpriseInstallAttributesTest, DeviceLockedFromOlderVersion) {
  install_attributes_.ReadCacheFile(GetTempPath());
  EXPECT_EQ(DEVICE_MODE_PENDING, install_attributes_.GetMode());
  // Lock the attributes as if it was done from older Chrome version.
  ASSERT_TRUE(cryptohome_->InstallAttributesSet(
      EnterpriseInstallAttributes::kAttrEnterpriseOwned, "true"));
  ASSERT_TRUE(cryptohome_->InstallAttributesSet(
      EnterpriseInstallAttributes::kAttrEnterpriseUser, kTestUser));
  ASSERT_TRUE(cryptohome_->InstallAttributesFinalize());
  base::RunLoop loop;
  install_attributes_.ReadImmutableAttributes(base::Bind(loop.QuitClosure()));
  loop.Run();

  ASSERT_FALSE(cryptohome_->InstallAttributesIsFirstInstall());
  EXPECT_EQ(DEVICE_MODE_ENTERPRISE, install_attributes_.GetMode());
  EXPECT_EQ(kTestDomain, install_attributes_.GetDomain());
  EXPECT_EQ(kTestUser, install_attributes_.GetRegistrationUser());
  EXPECT_EQ("", install_attributes_.GetDeviceId());
}

TEST_F(EnterpriseInstallAttributesTest, ReadCacheFile) {
  cryptohome::SerializedInstallAttributes install_attrs_proto;
  SetAttribute(&install_attrs_proto,
               EnterpriseInstallAttributes::kAttrEnterpriseOwned, "true");
  SetAttribute(&install_attrs_proto,
               EnterpriseInstallAttributes::kAttrEnterpriseUser, kTestUser);
  const std::string blob(install_attrs_proto.SerializeAsString());
  ASSERT_EQ(static_cast<int>(blob.size()),
            file_util::WriteFile(GetTempPath(), blob.c_str(), blob.size()));
  install_attributes_.ReadCacheFile(GetTempPath());
  EXPECT_EQ(DEVICE_MODE_ENTERPRISE, install_attributes_.GetMode());
  EXPECT_EQ(kTestDomain, install_attributes_.GetDomain());
  EXPECT_EQ(kTestUser, install_attributes_.GetRegistrationUser());
  EXPECT_EQ("", install_attributes_.GetDeviceId());
}

}  // namespace policy
