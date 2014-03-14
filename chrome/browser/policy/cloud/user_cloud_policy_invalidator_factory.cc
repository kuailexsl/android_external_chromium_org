// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/cloud/user_cloud_policy_invalidator_factory.h"

#include "base/command_line.h"
#include "chrome/browser/invalidation/invalidation_service_factory.h"
#include "chrome/browser/policy/cloud/user_cloud_policy_invalidator.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/policy/core/common/policy_switches.h"
#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_factory_chromeos.h"
#else
#include "chrome/browser/policy/cloud/user_cloud_policy_manager_factory.h"
#include "components/policy/core/common/cloud/user_cloud_policy_manager.h"
#endif

namespace policy {

// static
UserCloudPolicyInvalidatorFactory*
    UserCloudPolicyInvalidatorFactory::GetInstance() {
  return Singleton<UserCloudPolicyInvalidatorFactory>::get();
}

UserCloudPolicyInvalidatorFactory::UserCloudPolicyInvalidatorFactory()
    : BrowserContextKeyedServiceFactory(
          "UserCloudPolicyInvalidator",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(invalidation::InvalidationServiceFactory::GetInstance());
#if defined(OS_CHROMEOS)
  DependsOn(UserCloudPolicyManagerFactoryChromeOS::GetInstance());
#else
  DependsOn(UserCloudPolicyManagerFactory::GetInstance());
#endif
}

UserCloudPolicyInvalidatorFactory::~UserCloudPolicyInvalidatorFactory() {}

KeyedService* UserCloudPolicyInvalidatorFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (CommandLine::ForCurrentProcess()->HasSwitch(
        switches::kDisableCloudPolicyPush)) {
    return NULL;
  }

  Profile* profile = static_cast<Profile*>(context);
#if defined(OS_CHROMEOS)
  CloudPolicyManager* policy_manager =
      UserCloudPolicyManagerFactoryChromeOS::GetForProfile(profile);
#else
  CloudPolicyManager* policy_manager =
      UserCloudPolicyManagerFactory::GetForBrowserContext(context);
#endif
  if (!policy_manager)
    return NULL;

  return new UserCloudPolicyInvalidator(profile, policy_manager);
}

bool UserCloudPolicyInvalidatorFactory::
ServiceIsCreatedWithBrowserContext() const {
  // Must be automatically created to enable user policy invalidations.
  return true;
}

bool UserCloudPolicyInvalidatorFactory::ServiceIsNULLWhileTesting() const {
  // Not used in tests.
  return true;
}

}  // namespace policy
