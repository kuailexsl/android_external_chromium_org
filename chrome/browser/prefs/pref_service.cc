// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/pref_service.h"

#include <algorithm>

#include "base/bind.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/prefs/default_pref_store.h"
#include "base/stl_util.h"
#include "base/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/value_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/prefs/pref_notifier_impl.h"
#include "chrome/browser/prefs/pref_registry.h"
#include "chrome/browser/prefs/pref_value_store.h"

using content::BrowserContext;

namespace {

class ReadErrorHandler : public PersistentPrefStore::ReadErrorDelegate {
 public:
  ReadErrorHandler(base::Callback<void(PersistentPrefStore::PrefReadError)> cb)
      : callback_(cb) {}

  virtual void OnError(PersistentPrefStore::PrefReadError error) OVERRIDE {
    callback_.Run(error);
  }

 private:
  base::Callback<void(PersistentPrefStore::PrefReadError)> callback_;
};

}  // namespace

PrefService::PrefService(
    PrefNotifierImpl* pref_notifier,
    PrefValueStore* pref_value_store,
    PersistentPrefStore* user_prefs,
    PrefRegistry* pref_registry,
    base::Callback<void(PersistentPrefStore::PrefReadError)>
        read_error_callback,
    bool async)
    : pref_notifier_(pref_notifier),
      pref_value_store_(pref_value_store),
      pref_registry_(pref_registry),
      user_pref_store_(user_prefs),
      read_error_callback_(read_error_callback) {
  pref_notifier_->SetPrefService(this);

  pref_registry_->SetRegistrationCallback(
      base::Bind(&PrefService::AddRegisteredPreference,
                 base::Unretained(this)));
  pref_registry_->SetUnregistrationCallback(
      base::Bind(&PrefService::RemoveRegisteredPreference,
                 base::Unretained(this)));
  AddInitialPreferences();

  InitFromStorage(async);
}

PrefService::~PrefService() {
  DCHECK(CalledOnValidThread());

  // Remove our callbacks, setting NULL ones.
  pref_registry_->SetRegistrationCallback(PrefRegistry::RegistrationCallback());
  pref_registry_->SetUnregistrationCallback(
      PrefRegistry::UnregistrationCallback());

  // Reset pointers so accesses after destruction reliably crash.
  pref_value_store_.reset();
  pref_registry_ = NULL;
  user_pref_store_ = NULL;
  pref_notifier_.reset();
}

void PrefService::InitFromStorage(bool async) {
  if (!async) {
    read_error_callback_.Run(user_pref_store_->ReadPrefs());
  } else {
    // Guarantee that initialization happens after this function returned.
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&PersistentPrefStore::ReadPrefsAsync,
                   user_pref_store_.get(),
                   new ReadErrorHandler(read_error_callback_)));
  }
}

bool PrefService::ReloadPersistentPrefs() {
  return user_pref_store_->ReadPrefs() ==
             PersistentPrefStore::PREF_READ_ERROR_NONE;
}

void PrefService::CommitPendingWrite() {
  DCHECK(CalledOnValidThread());
  user_pref_store_->CommitPendingWrite();
}

bool PrefService::GetBoolean(const char* path) const {
  DCHECK(CalledOnValidThread());

  bool result = false;

  const base::Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return result;
  }
  bool rv = value->GetAsBoolean(&result);
  DCHECK(rv);
  return result;
}

int PrefService::GetInteger(const char* path) const {
  DCHECK(CalledOnValidThread());

  int result = 0;

  const base::Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return result;
  }
  bool rv = value->GetAsInteger(&result);
  DCHECK(rv);
  return result;
}

double PrefService::GetDouble(const char* path) const {
  DCHECK(CalledOnValidThread());

  double result = 0.0;

  const base::Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return result;
  }
  bool rv = value->GetAsDouble(&result);
  DCHECK(rv);
  return result;
}

std::string PrefService::GetString(const char* path) const {
  DCHECK(CalledOnValidThread());

  std::string result;

  const base::Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return result;
  }
  bool rv = value->GetAsString(&result);
  DCHECK(rv);
  return result;
}

base::FilePath PrefService::GetFilePath(const char* path) const {
  DCHECK(CalledOnValidThread());

  base::FilePath result;

  const base::Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return base::FilePath(result);
  }
  bool rv = base::GetValueAsFilePath(*value, &result);
  DCHECK(rv);
  return result;
}

bool PrefService::HasPrefPath(const char* path) const {
  const Preference* pref = FindPreference(path);
  return pref && !pref->IsDefaultValue();
}

DictionaryValue* PrefService::GetPreferenceValues() const {
  DCHECK(CalledOnValidThread());
  DictionaryValue* out = new DictionaryValue;
  PrefRegistry::const_iterator i = pref_registry_->begin();
  for (; i != pref_registry_->end(); ++i) {
    const Value* value = GetPreferenceValue(i->first);
    DCHECK(value);
    out->Set(i->first, value->DeepCopy());
  }
  return out;
}

const PrefService::Preference* PrefService::FindPreference(
    const char* pref_name) const {
  DCHECK(CalledOnValidThread());
  PreferenceMap::iterator it = prefs_map_.find(pref_name);
  if (it != prefs_map_.end())
    return &(it->second);
  const base::Value* default_value = NULL;
  if (!pref_registry_->defaults()->GetValue(pref_name, &default_value))
    return NULL;
  it = prefs_map_.insert(
      std::make_pair(pref_name, Preference(
          this, pref_name, default_value->GetType()))).first;
  return &(it->second);
}

bool PrefService::ReadOnly() const {
  return user_pref_store_->ReadOnly();
}

PrefService::PrefInitializationStatus PrefService::GetInitializationStatus()
    const {
  if (!user_pref_store_->IsInitializationComplete())
    return INITIALIZATION_STATUS_WAITING;

  switch (user_pref_store_->GetReadError()) {
    case PersistentPrefStore::PREF_READ_ERROR_NONE:
      return INITIALIZATION_STATUS_SUCCESS;
    case PersistentPrefStore::PREF_READ_ERROR_NO_FILE:
      return INITIALIZATION_STATUS_CREATED_NEW_PROFILE;
    default:
      return INITIALIZATION_STATUS_ERROR;
  }
}

bool PrefService::IsManagedPreference(const char* pref_name) const {
  const Preference* pref = FindPreference(pref_name);
  return pref && pref->IsManaged();
}

bool PrefService::IsUserModifiablePreference(const char* pref_name) const {
  const Preference* pref = FindPreference(pref_name);
  return pref && pref->IsUserModifiable();
}

const DictionaryValue* PrefService::GetDictionary(const char* path) const {
  DCHECK(CalledOnValidThread());

  const Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return NULL;
  }
  if (value->GetType() != Value::TYPE_DICTIONARY) {
    NOTREACHED();
    return NULL;
  }
  return static_cast<const DictionaryValue*>(value);
}

const base::Value* PrefService::GetUserPrefValue(const char* path) const {
  DCHECK(CalledOnValidThread());

  const Preference* pref = FindPreference(path);
  if (!pref) {
    NOTREACHED() << "Trying to get an unregistered pref: " << path;
    return NULL;
  }

  // Look for an existing preference in the user store. If it doesn't
  // exist, return NULL.
  base::Value* value = NULL;
  if (!user_pref_store_->GetMutableValue(path, &value))
    return NULL;

  if (!value->IsType(pref->GetType())) {
    NOTREACHED() << "Pref value type doesn't match registered type.";
    return NULL;
  }

  return value;
}

const base::Value* PrefService::GetDefaultPrefValue(const char* path) const {
  DCHECK(CalledOnValidThread());
  // Lookup the preference in the default store.
  const base::Value* value = NULL;
  if (!pref_registry_->defaults()->GetValue(path, &value)) {
    NOTREACHED() << "Default value missing for pref: " << path;
    return NULL;
  }
  return value;
}

const ListValue* PrefService::GetList(const char* path) const {
  DCHECK(CalledOnValidThread());

  const Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return NULL;
  }
  if (value->GetType() != Value::TYPE_LIST) {
    NOTREACHED();
    return NULL;
  }
  return static_cast<const ListValue*>(value);
}

void PrefService::AddPrefObserver(const char* path, PrefObserver* obs) {
  pref_notifier_->AddPrefObserver(path, obs);
}

void PrefService::RemovePrefObserver(const char* path, PrefObserver* obs) {
  pref_notifier_->RemovePrefObserver(path, obs);
}

void PrefService::AddPrefInitObserver(base::Callback<void(bool)> obs) {
  pref_notifier_->AddInitObserver(obs);
}

PrefRegistry* PrefService::DeprecatedGetPrefRegistry() {
  return pref_registry_.get();
}

void PrefService::AddInitialPreferences() {
  for (PrefRegistry::const_iterator it = pref_registry_->begin();
       it != pref_registry_->end();
       ++it) {
    AddRegisteredPreference(it->first.c_str(), it->second);
  }
}

// TODO(joi): Once MarkNeedsEmptyValue is gone, we can probably
// completely get rid of this method. There will be one difference in
// semantics; currently all registered preferences are stored right
// away in the prefs_map_, if we remove this they would be stored only
// opportunistically.
void PrefService::AddRegisteredPreference(const char* path,
                                          Value* default_value) {
  DCHECK(CalledOnValidThread());

  // For ListValue and DictionaryValue with non empty default, empty value
  // for |path| needs to be persisted in |user_pref_store_|. So that
  // non empty default is not used when user sets an empty ListValue or
  // DictionaryValue.
  bool needs_empty_value = false;
  base::Value::Type orig_type = default_value->GetType();
  if (orig_type == base::Value::TYPE_LIST) {
    const base::ListValue* list = NULL;
    if (default_value->GetAsList(&list) && !list->empty())
      needs_empty_value = true;
  } else if (orig_type == base::Value::TYPE_DICTIONARY) {
    const base::DictionaryValue* dict = NULL;
    if (default_value->GetAsDictionary(&dict) && !dict->empty())
      needs_empty_value = true;
  }
  if (needs_empty_value)
    user_pref_store_->MarkNeedsEmptyValue(path);
}

// TODO(joi): We can get rid of this once the ability to unregister
// prefs has been removed.
void PrefService::RemoveRegisteredPreference(const char* path) {
  DCHECK(CalledOnValidThread());

  prefs_map_.erase(path);
}

void PrefService::ClearPref(const char* path) {
  DCHECK(CalledOnValidThread());

  const Preference* pref = FindPreference(path);
  if (!pref) {
    NOTREACHED() << "Trying to clear an unregistered pref: " << path;
    return;
  }
  user_pref_store_->RemoveValue(path);
}

void PrefService::Set(const char* path, const Value& value) {
  SetUserPrefValue(path, value.DeepCopy());
}

void PrefService::SetBoolean(const char* path, bool value) {
  SetUserPrefValue(path, Value::CreateBooleanValue(value));
}

void PrefService::SetInteger(const char* path, int value) {
  SetUserPrefValue(path, Value::CreateIntegerValue(value));
}

void PrefService::SetDouble(const char* path, double value) {
  SetUserPrefValue(path, Value::CreateDoubleValue(value));
}

void PrefService::SetString(const char* path, const std::string& value) {
  SetUserPrefValue(path, Value::CreateStringValue(value));
}

void PrefService::SetFilePath(const char* path, const base::FilePath& value) {
  SetUserPrefValue(path, base::CreateFilePathValue(value));
}

void PrefService::SetInt64(const char* path, int64 value) {
  SetUserPrefValue(path, Value::CreateStringValue(base::Int64ToString(value)));
}

int64 PrefService::GetInt64(const char* path) const {
  DCHECK(CalledOnValidThread());

  const Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return 0;
  }
  std::string result("0");
  bool rv = value->GetAsString(&result);
  DCHECK(rv);

  int64 val;
  base::StringToInt64(result, &val);
  return val;
}

void PrefService::SetUint64(const char* path, uint64 value) {
  SetUserPrefValue(path, Value::CreateStringValue(base::Uint64ToString(value)));
}

uint64 PrefService::GetUint64(const char* path) const {
  DCHECK(CalledOnValidThread());

  const Value* value = GetPreferenceValue(path);
  if (!value) {
    NOTREACHED() << "Trying to read an unregistered pref: " << path;
    return 0;
  }
  std::string result("0");
  bool rv = value->GetAsString(&result);
  DCHECK(rv);

  uint64 val;
  base::StringToUint64(result, &val);
  return val;
}

Value* PrefService::GetMutableUserPref(const char* path,
                                       base::Value::Type type) {
  CHECK(type == Value::TYPE_DICTIONARY || type == Value::TYPE_LIST);
  DCHECK(CalledOnValidThread());

  const Preference* pref = FindPreference(path);
  if (!pref) {
    NOTREACHED() << "Trying to get an unregistered pref: " << path;
    return NULL;
  }
  if (pref->GetType() != type) {
    NOTREACHED() << "Wrong type for GetMutableValue: " << path;
    return NULL;
  }

  // Look for an existing preference in the user store. If it doesn't
  // exist or isn't the correct type, create a new user preference.
  Value* value = NULL;
  if (!user_pref_store_->GetMutableValue(path, &value) ||
      !value->IsType(type)) {
    if (type == Value::TYPE_DICTIONARY) {
      value = new DictionaryValue;
    } else if (type == Value::TYPE_LIST) {
      value = new ListValue;
    } else {
      NOTREACHED();
    }
    user_pref_store_->SetValueSilently(path, value);
  }
  return value;
}

void PrefService::ReportUserPrefChanged(const std::string& key) {
  user_pref_store_->ReportValueChanged(key);
}

void PrefService::SetUserPrefValue(const char* path, Value* new_value) {
  scoped_ptr<Value> owned_value(new_value);
  DCHECK(CalledOnValidThread());

  const Preference* pref = FindPreference(path);
  if (!pref) {
    NOTREACHED() << "Trying to write an unregistered pref: " << path;
    return;
  }
  if (pref->GetType() != new_value->GetType()) {
    NOTREACHED() << "Trying to set pref " << path
                 << " of type " << pref->GetType()
                 << " to value of type " << new_value->GetType();
    return;
  }

  user_pref_store_->SetValue(path, owned_value.release());
}

void PrefService::UpdateCommandLinePrefStore(PrefStore* command_line_store) {
  pref_value_store_->UpdateCommandLinePrefStore(command_line_store);
}

///////////////////////////////////////////////////////////////////////////////
// PrefService::Preference

PrefService::Preference::Preference(const PrefService* service,
                                    const char* name,
                                    base::Value::Type type)
      : name_(name),
        type_(type),
        pref_service_(service) {
  DCHECK(name);
  DCHECK(service);
}

const std::string PrefService::Preference::name() const {
  return name_;
}

base::Value::Type PrefService::Preference::GetType() const {
  return type_;
}

const Value* PrefService::Preference::GetValue() const {
  const Value* result= pref_service_->GetPreferenceValue(name_);
  DCHECK(result) << "Must register pref before getting its value";
  return result;
}

const Value* PrefService::Preference::GetRecommendedValue() const {
  DCHECK(pref_service_->FindPreference(name_.c_str())) <<
      "Must register pref before getting its value";

  const Value* found_value = NULL;
  if (pref_value_store()->GetRecommendedValue(name_, type_, &found_value)) {
    DCHECK(found_value->IsType(type_));
    return found_value;
  }

  // The pref has no recommended value.
  return NULL;
}

bool PrefService::Preference::IsManaged() const {
  return pref_value_store()->PrefValueInManagedStore(name_.c_str());
}

bool PrefService::Preference::IsRecommended() const {
  return pref_value_store()->PrefValueFromRecommendedStore(name_.c_str());
}

bool PrefService::Preference::HasExtensionSetting() const {
  return pref_value_store()->PrefValueInExtensionStore(name_.c_str());
}

bool PrefService::Preference::HasUserSetting() const {
  return pref_value_store()->PrefValueInUserStore(name_.c_str());
}

bool PrefService::Preference::IsExtensionControlled() const {
  return pref_value_store()->PrefValueFromExtensionStore(name_.c_str());
}

bool PrefService::Preference::IsUserControlled() const {
  return pref_value_store()->PrefValueFromUserStore(name_.c_str());
}

bool PrefService::Preference::IsDefaultValue() const {
  return pref_value_store()->PrefValueFromDefaultStore(name_.c_str());
}

bool PrefService::Preference::IsUserModifiable() const {
  return pref_value_store()->PrefValueUserModifiable(name_.c_str());
}

bool PrefService::Preference::IsExtensionModifiable() const {
  return pref_value_store()->PrefValueExtensionModifiable(name_.c_str());
}

const base::Value* PrefService::GetPreferenceValue(
    const std::string& path) const {
  DCHECK(CalledOnValidThread());
  const Value* default_value = NULL;
  if (pref_registry_->defaults()->GetValue(path, &default_value)) {
    const Value* found_value = NULL;
    base::Value::Type default_type = default_value->GetType();
    if (pref_value_store_->GetValue(path, default_type, &found_value)) {
      DCHECK(found_value->IsType(default_type));
      return found_value;
    } else {
      // Every registered preference has at least a default value.
      NOTREACHED() << "no valid value found for registered pref " << path;
    }
  }

  return NULL;
}
