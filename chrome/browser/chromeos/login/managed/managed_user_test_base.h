// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_MANAGED_MANAGED_USER_TEST_BASE_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_MANAGED_MANAGED_USER_TEST_BASE_H_

#include <string>

#include "base/compiler_specific.h"
#include "chrome/browser/chromeos/login/login_manager_test.h"
#include "chrome/browser/chromeos/net/network_portal_detector_test_impl.h"
#include "chrome/browser/managed_mode/managed_user_registration_utility_stub.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/cryptohome/mock_async_method_caller.h"
#include "chromeos/cryptohome/mock_homedir_methods.h"
#include "sync/api/fake_sync_change_processor.h"
#include "sync/api/sync_change.h"
#include "sync/api/sync_error_factory_mock.h"
#include "sync/protocol/sync.pb.h"

namespace chromeos {

namespace testing {

const char kStubEthernetServicePath[] = "eth0";

const char kTestManager[] = "test-manager@gmail.com";
const char kTestOtherUser[] = "test-user@gmail.com";

const char kTestManagerPassword[] = "password";
const char kTestSupervisedUserDisplayName[] = "John Doe";
const char kTestSupervisedUserPassword[] = "simplepassword";

class ManagedUsersSyncTestAdapter {
 public:
  explicit ManagedUsersSyncTestAdapter(Profile* profile);

  bool HasChanges() { return !processor_->changes().empty(); }

  scoped_ptr< ::sync_pb::ManagedUserSpecifics> GetFirstChange();

  void AddChange(const ::sync_pb::ManagedUserSpecifics& proto, bool update);

  syncer::FakeSyncChangeProcessor* processor_;
  ManagedUserSyncService* service_;
  int next_sync_data_id_;
};

class ManagedUsersSharedSettingsSyncTestAdapter {
 public:
  explicit ManagedUsersSharedSettingsSyncTestAdapter(Profile* profile);

  bool HasChanges() { return !processor_->changes().empty(); }

  scoped_ptr< ::sync_pb::ManagedUserSharedSettingSpecifics> GetFirstChange();

  void AddChange(const ::sync_pb::ManagedUserSharedSettingSpecifics& proto,
                 bool update);

  void AddChange(const std::string& mu_id,
                 const std::string& key,
                 const base::Value& value,
                 bool acknowledged,
                 bool update);

  syncer::FakeSyncChangeProcessor* processor_;
  ManagedUserSharedSettingsService* service_;
  int next_sync_data_id_;
};

class ManagedUserTestBase : public chromeos::LoginManagerTest {
 public:
  ManagedUserTestBase();
  virtual ~ManagedUserTestBase();

  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE;
  virtual void CleanUpOnMainThread() OVERRIDE;

 protected:
  virtual void TearDown() OVERRIDE;

  virtual void TearDownInProcessBrowserTestFixture() OVERRIDE;

  void JSEval(const std::string& script);

  void JSExpectAsync(const std::string& function);

  void JSSetTextField(const std::string& element_selector,
                      const std::string& value);

  void PrepareUsers();
  void StartFlowLoginAsManager();
  void FillNewUserData(const std::string& display_name);
  void StartUserCreation(const std::string& button_id,
                         const std::string& expected_display_name);
  void SigninAsSupervisedUser(bool check_homedir_calls,
                              int user_index,
                              const std::string& expected_display_name);
  void SigninAsManager(int user_index);
  void RemoveSupervisedUser(unsigned long original_user_count,
                            int user_index,
                            const std::string& expected_display_name);

  cryptohome::MockAsyncMethodCaller* mock_async_method_caller_;
  cryptohome::MockHomedirMethods* mock_homedir_methods_;
  NetworkPortalDetectorTestImpl* network_portal_detector_;
  ManagedUserRegistrationUtilityStub* registration_utility_stub_;
  scoped_ptr<ScopedTestingManagedUserRegistrationUtility> scoped_utility_;
  scoped_ptr<ManagedUsersSharedSettingsSyncTestAdapter>
      shared_settings_adapter_;
  scoped_ptr<ManagedUsersSyncTestAdapter> managed_users_adapter_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ManagedUserTestBase);
};

}  // namespace testing

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_MANAGED_MANAGED_USER_TEST_BASE_H_
