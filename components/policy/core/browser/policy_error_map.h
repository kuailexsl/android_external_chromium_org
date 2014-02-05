// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_BROWSER_POLICY_ERROR_MAP_H_
#define COMPONENTS_POLICY_CORE_BROWSER_POLICY_ERROR_MAP_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_vector.h"
#include "base/strings/string16.h"
#include "components/policy/policy_export.h"

namespace policy {

// Collects error messages and their associated policies.
class POLICY_EXPORT PolicyErrorMap {
 public:
  typedef std::multimap<std::string, base::string16> PolicyMapType;
  typedef PolicyMapType::const_iterator const_iterator;

  class PendingError;

  PolicyErrorMap();
  virtual ~PolicyErrorMap();

  // Returns true when the errors logged are ready to be retrieved. It is always
  // safe to call AddError, but the other methods are only allowed once
  // IsReady is true. IsReady will be true once the UI message loop has started.
  bool IsReady() const;

  // Adds an entry with key |policy| and the error message corresponding to
  // |message_id| in grit/generated_resources.h to the map.
  void AddError(const std::string& policy, int message_id);

  // Adds an entry with key |policy|, subkey |subkey|, and the error message
  // corresponding to |message_id| in grit/generated_resources.h to the map.
  void AddError(const std::string& policy,
                const std::string& subkey,
                int message_id);

  // Adds an entry with key |policy|, list index |index|, and the error message
  // corresponding to |message_id| in grit/generated_resources.h to the map.
  void AddError(const std::string& policy,
                int index,
                int message_id);

  // Adds an entry with key |policy| and the error message corresponding to
  // |message_id| in grit/generated_resources.h to the map and replaces the
  // placeholder within the error message with |replacement_string|.
  void AddError(const std::string& policy,
                int message_id,
                const std::string& replacement_string);

  // Adds an entry with key |policy|, subkey |subkey| and the error message
  // corresponding to |message_id| in grit/generated_resources.h to the map.
  // Replaces the placeholder in the error message with |replacement_string|.
  void AddError(const std::string& policy,
                const std::string& subkey,
                int message_id,
                const std::string& replacement_string);

  // Adds an entry with key |policy|, list index |index| and the error message
  // corresponding to |message_id| in grit/generated_resources.h to the map.
  // Replaces the placeholder in the error message with |replacement_string|.
  void AddError(const std::string& policy,
                int index,
                int message_id,
                const std::string& replacement_string);

  // Adds an entry with key |policy|, the schema validation error location
  // |error_path|, and detailed error |message|.
  void AddError(const std::string& policy,
                const std::string& error_path,
                const std::string& message);

  // Returns all the error messages stored for |policy|, separated by a white
  // space. Returns an empty string if there are no errors for |policy|.
  base::string16 GetErrors(const std::string& policy);

  bool empty();
  size_t size();

  const_iterator begin();
  const_iterator end();

  void Clear();

 private:
  // Maps the error when ready, otherwise adds it to the pending errors list.
  void AddError(PendingError* error);

  // Converts a PendingError into a |map_| entry.
  void Convert(PendingError* error);

  // Converts all pending errors to |map_| entries.
  void CheckReadyAndConvert();

  ScopedVector<PendingError> pending_;
  PolicyMapType map_;

  DISALLOW_COPY_AND_ASSIGN(PolicyErrorMap);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_BROWSER_POLICY_ERROR_MAP_H_
