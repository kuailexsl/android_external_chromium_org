// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/tracked/tracked_atomic_preference.h"

#include "base/values.h"
#include "chrome/browser/prefs/pref_hash_store_transaction.h"

TrackedAtomicPreference::TrackedAtomicPreference(
    const std::string& pref_path,
    size_t reporting_id,
    size_t reporting_ids_count,
    PrefHashFilter::EnforcementLevel enforcement_level)
    : pref_path_(pref_path),
      helper_(pref_path, reporting_id, reporting_ids_count, enforcement_level) {
}

void TrackedAtomicPreference::OnNewValue(
    const base::Value* value,
    PrefHashStoreTransaction* transaction) const {
  transaction->StoreHash(pref_path_, value);
}

void TrackedAtomicPreference::EnforceAndReport(
    base::DictionaryValue* pref_store_contents,
    PrefHashStoreTransaction* transaction) const {
  const base::Value* value = NULL;
  pref_store_contents->Get(pref_path_, &value);
  PrefHashStoreTransaction::ValueState value_state =
      transaction->CheckValue(pref_path_, value);

  helper_.ReportValidationResult(value_state);

  TrackedPreferenceHelper::ResetAction reset_action =
      helper_.GetAction(value_state);
  helper_.ReportAction(reset_action);

  if (reset_action == TrackedPreferenceHelper::DO_RESET)
    pref_store_contents->RemovePath(pref_path_, NULL);

  if (value_state != PrefHashStoreTransaction::UNCHANGED) {
    // Store the hash for the new value (whether it was reset or not).
    const base::Value* new_value = NULL;
    pref_store_contents->Get(pref_path_, &new_value);
    transaction->StoreHash(pref_path_, new_value);
  }
}
