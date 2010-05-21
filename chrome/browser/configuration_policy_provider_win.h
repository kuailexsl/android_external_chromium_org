// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFIGURATION_POLICY_PROVIDER_WIN_H_
#define CHROME_BROWSER_CONFIGURATION_POLICY_PROVIDER_WIN_H_

#include "chrome/browser/configuration_policy_store.h"
#include "chrome/browser/configuration_policy_provider.h"

// An implementation of |ConfigurationPolicyProvider| using the
// mechanism provided by Windows Groups Policy. Policy decisions are
// stored as values in a special section of the Windows Registry.
// On a managed machine in a domain, this portion of the registry is
// periodically updated by the Windows Group Policy machinery to contain
// the latest version of the policy set by administrators.
class WinConfigurationPolicyProvider : public ConfigurationPolicyProvider {
 public:
  WinConfigurationPolicyProvider();
  virtual ~WinConfigurationPolicyProvider() { }

  // ConfigurationPolicyProvider method overrides:
  virtual bool Provide(ConfigurationPolicyStore* store);

 protected:
  // The sub key path for Chromiums's Group Policy information in the
  // Windows registry.
  static const wchar_t kPolicyRegistrySubKey[];

  // Constants specifying the names of policy-specifying registry values
  // in |kChromiumPolicySubKey|.
  static const wchar_t kHomepageRegistryValueName[];
  static const wchar_t kHomepageIsNewTabPageRegistryValueName[];
  static const wchar_t kCookiesModeRegistryValueName[];

 private:
  // For each Group Policy registry value that maps to a browser policy, there
  // is an entry in |registry_to_policy_map_| of type |RegistryPolicyMapEntry|
  // that specifies the registry value name and the browser policy that it
  // maps to.
  struct RegistryPolicyMapEntry {
    Value::ValueType value_type;
    ConfigurationPolicyStore::PolicyType policy_type;
    const wchar_t* registry_value_name;
  };

  static const RegistryPolicyMapEntry registry_to_policy_map_[];

  // Methods to perfrom type-specific policy lookups in the registry
  // HKLM is checked fist, then HKLM.
  bool GetRegistryPolicyString(const wchar_t* value_name, string16* result);
  bool GetRegistryPolicyBoolean(const wchar_t* value_name, bool* result);
  bool GetRegistryPolicyInteger(const wchar_t* value_name, uint32* result);
};

#endif  // CHROME_BROWSER_CONFIGURATION_POLICY_PROVIDER_WIN_H_

