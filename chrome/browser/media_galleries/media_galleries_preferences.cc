// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media_galleries/media_galleries_preferences.h"

#include "base/base_paths_posix.h"
#include "base/callback.h"
#include "base/i18n/time_formatting.h"
#include "base/path_service.h"
#include "base/prefs/pref_service.h"
#include "base/prefs/scoped_user_pref_update.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/api/media_galleries_private/media_galleries_private_api.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/media_galleries/fileapi/iapps_finder.h"
#include "chrome/browser/media_galleries/fileapi/picasa_finder.h"
#include "chrome/browser/media_galleries/media_file_system_registry.h"
#include "chrome/browser/media_galleries/media_galleries_histograms.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/storage_monitor/media_storage_util.h"
#include "chrome/browser/storage_monitor/storage_monitor.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/permissions/media_galleries_permission.h"
#include "chrome/common/pref_names.h"
#include "components/user_prefs/pref_registry_syncable.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"

using base::DictionaryValue;
using base::ListValue;
using extensions::ExtensionPrefs;

namespace {

// Pref key for the list of media gallery permissions.
const char kMediaGalleriesPermissions[] = "media_galleries_permissions";
// Pref key for Media Gallery ID.
const char kMediaGalleryIdKey[] = "id";
// Pref key for Media Gallery Permission Value.
const char kMediaGalleryHasPermissionKey[] = "has_permission";

const char kMediaGalleriesDeviceIdKey[] = "deviceId";
const char kMediaGalleriesDisplayNameKey[] = "displayName";
const char kMediaGalleriesPathKey[] = "path";
const char kMediaGalleriesPrefIdKey[] = "prefId";
const char kMediaGalleriesTypeKey[] = "type";
const char kMediaGalleriesVolumeLabelKey[] = "volumeLabel";
const char kMediaGalleriesVendorNameKey[] = "vendorName";
const char kMediaGalleriesModelNameKey[] = "modelName";
const char kMediaGalleriesSizeKey[] = "totalSize";
const char kMediaGalleriesLastAttachTimeKey[] = "lastAttachTime";
const char kMediaGalleriesScanAudioCountKey[] = "audioCount";
const char kMediaGalleriesScanImageCountKey[] = "imageCount";
const char kMediaGalleriesScanVideoCountKey[] = "videoCount";
const char kMediaGalleriesPrefsVersionKey[] = "preferencesVersion";

const char kMediaGalleriesTypeAutoDetectedValue[] = "autoDetected";
const char kMediaGalleriesTypeBlackListedValue[] = "blackListed";
const char kMediaGalleriesTypeRemovedScanValue[] = "removedScan";
const char kMediaGalleriesTypeScanResultValue[] = "scanResult";
const char kMediaGalleriesTypeUserAddedValue[] = "userAdded";

const char kIPhotoGalleryName[] = "iPhoto";
const char kITunesGalleryName[] = "iTunes";
const char kPicasaGalleryName[] = "Picasa";

const int kCurrentPrefsVersion = 2;

int NumberExtensionsUsingMediaGalleries(Profile* profile) {
  int count = 0;
  if (!profile)
    return count;
  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  if (!extension_service)
    return count;

  const extensions::ExtensionSet* extensions = extension_service->extensions();
  for (extensions::ExtensionSet::const_iterator i = extensions->begin();
       i != extensions->end(); ++i) {
    if (extensions::PermissionsData::HasAPIPermission(
            *i, extensions::APIPermission::kMediaGalleries) ||
        extensions::PermissionsData::HasAPIPermission(
            *i, extensions::APIPermission::kMediaGalleriesPrivate)) {
      count++;
    }
  }
  return count;
}

bool GetPrefId(const base::DictionaryValue& dict, MediaGalleryPrefId* value) {
  std::string string_id;
  if (!dict.GetString(kMediaGalleriesPrefIdKey, &string_id) ||
      !base::StringToUint64(string_id, value)) {
    return false;
  }

  return true;
}

bool GetType(const base::DictionaryValue& dict,
             MediaGalleryPrefInfo::Type* type) {
  std::string string_type;
  if (!dict.GetString(kMediaGalleriesTypeKey, &string_type))
    return false;

  if (string_type == kMediaGalleriesTypeUserAddedValue) {
    *type = MediaGalleryPrefInfo::kUserAdded;
    return true;
  }
  if (string_type == kMediaGalleriesTypeAutoDetectedValue) {
    *type = MediaGalleryPrefInfo::kAutoDetected;
    return true;
  }
  if (string_type == kMediaGalleriesTypeBlackListedValue) {
    *type = MediaGalleryPrefInfo::kBlackListed;
    return true;
  }
  if (string_type == kMediaGalleriesTypeScanResultValue) {
    *type = MediaGalleryPrefInfo::kScanResult;
    return true;
  }
  if (string_type == kMediaGalleriesTypeRemovedScanValue) {
    *type = MediaGalleryPrefInfo::kRemovedScan;
    return true;
  }

  return false;
}

const char* TypeToStringValue(MediaGalleryPrefInfo::Type type) {
  const char* result = NULL;
  switch (type) {
    case MediaGalleryPrefInfo::kUserAdded:
      result = kMediaGalleriesTypeUserAddedValue;
      break;
    case MediaGalleryPrefInfo::kAutoDetected:
      result = kMediaGalleriesTypeAutoDetectedValue;
      break;
    case MediaGalleryPrefInfo::kBlackListed:
      result = kMediaGalleriesTypeBlackListedValue;
      break;
    case MediaGalleryPrefInfo::kScanResult:
      result = kMediaGalleriesTypeScanResultValue;
      break;
    case MediaGalleryPrefInfo::kRemovedScan:
      result = kMediaGalleriesTypeRemovedScanValue;
      break;
    default:
      NOTREACHED();
      break;
  }
  return result;
}

bool PopulateGalleryPrefInfoFromDictionary(
    const base::DictionaryValue& dict, MediaGalleryPrefInfo* out_gallery_info) {
  MediaGalleryPrefId pref_id;
  base::string16 display_name;
  std::string device_id;
  base::FilePath::StringType path;
  MediaGalleryPrefInfo::Type type = MediaGalleryPrefInfo::kInvalidType;
  base::string16 volume_label;
  base::string16 vendor_name;
  base::string16 model_name;
  double total_size_in_bytes = 0.0;
  double last_attach_time = 0.0;
  bool volume_metadata_valid = false;
  int audio_count = 0;
  int image_count = 0;
  int video_count = 0;
  int prefs_version = 0;

  if (!GetPrefId(dict, &pref_id) ||
      !dict.GetString(kMediaGalleriesDeviceIdKey, &device_id) ||
      !dict.GetString(kMediaGalleriesPathKey, &path) ||
      !GetType(dict, &type)) {
    return false;
  }

  dict.GetString(kMediaGalleriesDisplayNameKey, &display_name);
  dict.GetInteger(kMediaGalleriesPrefsVersionKey, &prefs_version);

  if (dict.GetString(kMediaGalleriesVolumeLabelKey, &volume_label) &&
      dict.GetString(kMediaGalleriesVendorNameKey, &vendor_name) &&
      dict.GetString(kMediaGalleriesModelNameKey, &model_name) &&
      dict.GetDouble(kMediaGalleriesSizeKey, &total_size_in_bytes) &&
      dict.GetDouble(kMediaGalleriesLastAttachTimeKey, &last_attach_time)) {
    volume_metadata_valid = true;
  }

  if (dict.GetInteger(kMediaGalleriesScanAudioCountKey, &audio_count) &&
      dict.GetInteger(kMediaGalleriesScanImageCountKey, &image_count) &&
      dict.GetInteger(kMediaGalleriesScanVideoCountKey, &video_count)) {
    out_gallery_info->audio_count = audio_count;
    out_gallery_info->image_count = image_count;
    out_gallery_info->video_count = video_count;
  } else {
    out_gallery_info->audio_count = 0;
    out_gallery_info->image_count = 0;
    out_gallery_info->video_count = 0;
  }

  out_gallery_info->pref_id = pref_id;
  out_gallery_info->display_name = display_name;
  out_gallery_info->device_id = device_id;
  out_gallery_info->path = base::FilePath(path);
  out_gallery_info->type = type;
  out_gallery_info->volume_label = volume_label;
  out_gallery_info->vendor_name = vendor_name;
  out_gallery_info->model_name = model_name;
  out_gallery_info->total_size_in_bytes = total_size_in_bytes;
  out_gallery_info->last_attach_time =
      base::Time::FromInternalValue(last_attach_time);
  out_gallery_info->volume_metadata_valid = volume_metadata_valid;
  out_gallery_info->prefs_version = prefs_version;

  return true;
}

base::DictionaryValue* CreateGalleryPrefInfoDictionary(
    const MediaGalleryPrefInfo& gallery) {
  base::DictionaryValue* dict = new base::DictionaryValue();
  dict->SetString(kMediaGalleriesPrefIdKey,
                  base::Uint64ToString(gallery.pref_id));
  dict->SetString(kMediaGalleriesDeviceIdKey, gallery.device_id);
  dict->SetString(kMediaGalleriesPathKey, gallery.path.value());
  dict->SetString(kMediaGalleriesTypeKey, TypeToStringValue(gallery.type));

  if (gallery.volume_metadata_valid) {
    dict->SetString(kMediaGalleriesVolumeLabelKey, gallery.volume_label);
    dict->SetString(kMediaGalleriesVendorNameKey, gallery.vendor_name);
    dict->SetString(kMediaGalleriesModelNameKey, gallery.model_name);
    dict->SetDouble(kMediaGalleriesSizeKey, gallery.total_size_in_bytes);
    dict->SetDouble(kMediaGalleriesLastAttachTimeKey,
                    gallery.last_attach_time.ToInternalValue());
  } else {
    dict->SetString(kMediaGalleriesDisplayNameKey, gallery.display_name);
  }

  if (gallery.audio_count || gallery.image_count || gallery.video_count) {
    dict->SetInteger(kMediaGalleriesScanAudioCountKey, gallery.audio_count);
    dict->SetInteger(kMediaGalleriesScanImageCountKey, gallery.image_count);
    dict->SetInteger(kMediaGalleriesScanVideoCountKey, gallery.video_count);
  }

  // Version 0 of the prefs format was that the display_name was always
  // used to show the user-visible name of the gallery. Version 1 means
  // that there is an optional display_name, and when it is present, it
  // overrides the name that would be built from the volume metadata, path,
  // or whatever other data. So if we see a display_name with version 0, it
  // means it may be overwritten simply by getting new volume metadata.
  // A display_name with version 1 should not be overwritten.
  dict->SetInteger(kMediaGalleriesPrefsVersionKey, gallery.prefs_version);

  return dict;
}

bool HasAutoDetectedGalleryPermission(const extensions::Extension& extension) {
  extensions::MediaGalleriesPermission::CheckParam param(
      extensions::MediaGalleriesPermission::kAllAutoDetectedPermission);
  return extensions::PermissionsData::CheckAPIPermissionWithParam(
      &extension, extensions::APIPermission::kMediaGalleries, &param);
}

// Retrieves the MediaGalleryPermission from the given dictionary; DCHECKs on
// failure.
bool GetMediaGalleryPermissionFromDictionary(
    const base::DictionaryValue* dict,
    MediaGalleryPermission* out_permission) {
  std::string string_id;
  if (dict->GetString(kMediaGalleryIdKey, &string_id) &&
      base::StringToUint64(string_id, &out_permission->pref_id) &&
      dict->GetBoolean(kMediaGalleryHasPermissionKey,
                       &out_permission->has_permission)) {
    return true;
  }
  NOTREACHED();
  return false;
}

base::string16 GetDisplayNameForDevice(uint64 storage_size_in_bytes,
                                       const base::string16& name) {
  DCHECK(!name.empty());
  return (storage_size_in_bytes == 0) ?
      name :
      ui::FormatBytes(storage_size_in_bytes) + base::ASCIIToUTF16(" ") + name;
}

// For a device with |device_name| and a relative path |sub_folder|, construct
// a display name. If |sub_folder| is empty, then just return |device_name|.
base::string16 GetDisplayNameForSubFolder(const base::string16& device_name,
                                          const base::FilePath& sub_folder) {
  if (sub_folder.empty())
    return device_name;
  return (sub_folder.BaseName().LossyDisplayName() +
          base::ASCIIToUTF16(" - ") +
          device_name);
}

base::string16 GetFullProductName(const base::string16& vendor_name,
                                  const base::string16& model_name) {
  if (vendor_name.empty() && model_name.empty())
    return base::string16();

  base::string16 product_name;
  if (vendor_name.empty())
    product_name = model_name;
  else if (model_name.empty())
    product_name = vendor_name;
  else if (!vendor_name.empty() && !model_name.empty())
    product_name = vendor_name + base::UTF8ToUTF16(", ") + model_name;

  return product_name;
}

}  // namespace

MediaGalleryPrefInfo::MediaGalleryPrefInfo()
    : pref_id(kInvalidMediaGalleryPrefId),
      type(kInvalidType),
      total_size_in_bytes(0),
      volume_metadata_valid(false),
      audio_count(0),
      image_count(0),
      video_count(0),
      prefs_version(0) {
}

MediaGalleryPrefInfo::~MediaGalleryPrefInfo() {}

base::FilePath MediaGalleryPrefInfo::AbsolutePath() const {
  base::FilePath base_path = MediaStorageUtil::FindDevicePathById(device_id);
  DCHECK(!path.IsAbsolute());
  return base_path.empty() ? base_path : base_path.Append(path);
}

bool MediaGalleryPrefInfo::IsBlackListedType() const {
  return type == kBlackListed || type == kRemovedScan;
}

base::string16 MediaGalleryPrefInfo::GetGalleryDisplayName() const {
  if (!StorageInfo::IsRemovableDevice(device_id)) {
    // For fixed storage, the default name is the fully qualified directory
    // name, or in the case of a root directory, the root directory name.
    // Exception: ChromeOS -- the full pathname isn't visible there, so only
    // the directory name is used.
    base::FilePath path = AbsolutePath();
    if (!display_name.empty())
      return display_name;

#if defined(OS_CHROMEOS)
    // See chrome/browser/chromeos/fileapi/file_system_backend.cc
    base::FilePath download_path;
    if (PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS_SAFE, &download_path)) {
      base::FilePath relative;
      if (download_path.AppendRelativePath(path, &relative))
        return relative.LossyDisplayName();
    }
    return path.BaseName().LossyDisplayName();
#else
    return path.LossyDisplayName();
#endif
  }

  base::string16 name = display_name;
  if (name.empty())
    name = volume_label;
  if (name.empty())
    name = GetFullProductName(vendor_name, model_name);
  if (name.empty())
    name = l10n_util::GetStringUTF16(IDS_MEDIA_GALLERIES_UNLABELED_DEVICE);

  name = GetDisplayNameForDevice(total_size_in_bytes, name);

  if (!path.empty())
    name = GetDisplayNameForSubFolder(name, path);

  return name;
}

base::string16 MediaGalleryPrefInfo::GetGalleryTooltip() const {
  return AbsolutePath().LossyDisplayName();
}

base::string16 MediaGalleryPrefInfo::GetGalleryAdditionalDetails() const {
  base::string16 attached;
  if (StorageInfo::IsRemovableDevice(device_id)) {
    if (MediaStorageUtil::IsRemovableStorageAttached(device_id)) {
      attached = l10n_util::GetStringUTF16(
          IDS_MEDIA_GALLERIES_DIALOG_DEVICE_ATTACHED);
    } else if (!last_attach_time.is_null()) {
      attached = l10n_util::GetStringFUTF16(
          IDS_MEDIA_GALLERIES_LAST_ATTACHED,
          base::TimeFormatShortDateNumeric(last_attach_time));
    } else {
      attached = l10n_util::GetStringUTF16(
          IDS_MEDIA_GALLERIES_DIALOG_DEVICE_NOT_ATTACHED);
    }
  }

  return attached;
}

bool MediaGalleryPrefInfo::IsGalleryAvailable() const {
  return !StorageInfo::IsRemovableDevice(device_id) ||
         MediaStorageUtil::IsRemovableStorageAttached(device_id);
}

MediaGalleriesPreferences::GalleryChangeObserver::~GalleryChangeObserver() {}

MediaGalleriesPreferences::MediaGalleriesPreferences(Profile* profile)
    : initialized_(false),
      pre_initialization_callbacks_waiting_(0),
      profile_(profile),
      extension_prefs_for_testing_(NULL),
      weak_factory_(this) {
}

MediaGalleriesPreferences::~MediaGalleriesPreferences() {
  if (StorageMonitor::GetInstance())
    StorageMonitor::GetInstance()->RemoveObserver(this);
}

void MediaGalleriesPreferences::EnsureInitialized(base::Closure callback) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  if (IsInitialized()) {
    if (!callback.is_null())
      callback.Run();
    return;
  }

  on_initialize_callbacks_.push_back(callback);
  if (on_initialize_callbacks_.size() > 1)
    return;

  // This counter must match the number of async methods dispatched below.
  // It cannot be incremented inline with each callback, as some may return
  // synchronously, decrement the counter to 0, and prematurely trigger
  // FinishInitialization.
  pre_initialization_callbacks_waiting_ = 4;

  // Check whether we should be initializing -- are there any extensions that
  // are using media galleries?
  media_galleries::UsageCount(media_galleries::PREFS_INITIALIZED);
  if (NumberExtensionsUsingMediaGalleries(profile_) == 0) {
    media_galleries::UsageCount(media_galleries::PREFS_INITIALIZED_ERROR);
  }

  // We determine the freshness of the profile here, before any of the finders
  // return and add media galleries to it.
  StorageMonitor::GetInstance()->EnsureInitialized(
      base::Bind(&MediaGalleriesPreferences::OnStorageMonitorInit,
                 weak_factory_.GetWeakPtr(),
                 !APIHasBeenUsed(profile_) /* add_default_galleries */));

  // Look for optional default galleries every time.
  iapps::FindITunesLibrary(
      base::Bind(&MediaGalleriesPreferences::OnFinderDeviceID,
                 weak_factory_.GetWeakPtr()));

  picasa::FindPicasaDatabase(
      base::Bind(&MediaGalleriesPreferences::OnFinderDeviceID,
                 weak_factory_.GetWeakPtr()));

  iapps::FindIPhotoLibrary(
      base::Bind(&MediaGalleriesPreferences::OnFinderDeviceID,
                 weak_factory_.GetWeakPtr()));
}

bool MediaGalleriesPreferences::IsInitialized() const { return initialized_; }

Profile* MediaGalleriesPreferences::profile() { return profile_; }

void MediaGalleriesPreferences::OnInitializationCallbackReturned() {
  DCHECK(!IsInitialized());
  DCHECK_GT(pre_initialization_callbacks_waiting_, 0);
  if (--pre_initialization_callbacks_waiting_ == 0)
    FinishInitialization();
}

void MediaGalleriesPreferences::FinishInitialization() {
  DCHECK(!IsInitialized());

  initialized_ = true;

  StorageMonitor* monitor = StorageMonitor::GetInstance();
  DCHECK(monitor->IsInitialized());

  InitFromPrefs();

  StorageMonitor::GetInstance()->AddObserver(this);

  std::vector<StorageInfo> existing_devices =
      monitor->GetAllAvailableStorages();
  for (size_t i = 0; i < existing_devices.size(); i++) {
    if (!(StorageInfo::IsMediaDevice(existing_devices[i].device_id()) &&
          StorageInfo::IsRemovableDevice(existing_devices[i].device_id())))
      continue;
    AddGallery(existing_devices[i].device_id(),
               base::FilePath(),
               MediaGalleryPrefInfo::kAutoDetected,
               existing_devices[i].storage_label(),
               existing_devices[i].vendor_name(),
               existing_devices[i].model_name(),
               existing_devices[i].total_size_in_bytes(),
               base::Time::Now(), 0, 0, 0);
  }

  for (std::vector<base::Closure>::iterator iter =
           on_initialize_callbacks_.begin();
       iter != on_initialize_callbacks_.end();
       ++iter) {
    iter->Run();
  }
  on_initialize_callbacks_.clear();
}

void MediaGalleriesPreferences::AddDefaultGalleries() {
  const int kDirectoryKeys[] = {
    chrome::DIR_USER_MUSIC,
    chrome::DIR_USER_PICTURES,
    chrome::DIR_USER_VIDEOS,
  };

  for (size_t i = 0; i < arraysize(kDirectoryKeys); ++i) {
    base::FilePath path;
    if (!PathService::Get(kDirectoryKeys[i], &path))
      continue;

    base::FilePath relative_path;
    StorageInfo info;
    if (MediaStorageUtil::GetDeviceInfoFromPath(path, &info, &relative_path)) {
      AddGalleryInternal(info.device_id(), info.name(), relative_path,
                         MediaGalleryPrefInfo::kAutoDetected,
                         info.storage_label(), info.vendor_name(),
                         info.model_name(), info.total_size_in_bytes(),
                         base::Time(), true, 0, 0, 0, kCurrentPrefsVersion);
    }
  }
}

bool MediaGalleriesPreferences::UpdateDeviceIDForSingletonType(
    const std::string& device_id) {
  StorageInfo::Type singleton_type;
  if (!StorageInfo::CrackDeviceId(device_id, &singleton_type, NULL))
    return false;

  PrefService* prefs = profile_->GetPrefs();
  scoped_ptr<ListPrefUpdate> update(new ListPrefUpdate(
      prefs, prefs::kMediaGalleriesRememberedGalleries));
  base::ListValue* list = update->Get();
  for (base::ListValue::iterator iter = list->begin();
       iter != list->end(); ++iter) {
    // All of these calls should succeed, but preferences file can be corrupt.
    base::DictionaryValue* dict;
    if (!(*iter)->GetAsDictionary(&dict))
      continue;
    std::string this_device_id;
    if (!dict->GetString(kMediaGalleriesDeviceIdKey, &this_device_id))
      continue;
    if (this_device_id == device_id)
      return true;  // No update is necessary.
    StorageInfo::Type device_type;
    if (!StorageInfo::CrackDeviceId(this_device_id, &device_type, NULL))
      continue;

    if (device_type == singleton_type) {
      dict->SetString(kMediaGalleriesDeviceIdKey, device_id);
      update.reset();  // commits the update.
      InitFromPrefs();
      MediaGalleryPrefId pref_id;
      if (GetPrefId(*dict, &pref_id)) {
        FOR_EACH_OBSERVER(GalleryChangeObserver,
                          gallery_change_observers_,
                          OnGalleryInfoUpdated(this, pref_id));
      }
      return true;
    }
  }
  return false;
}

void MediaGalleriesPreferences::OnStorageMonitorInit(
    bool add_default_galleries) {
  if (add_default_galleries)
    AddDefaultGalleries();
  OnInitializationCallbackReturned();
}

void MediaGalleriesPreferences::OnFinderDeviceID(const std::string& device_id) {
  if (!device_id.empty() && !UpdateDeviceIDForSingletonType(device_id)) {
    std::string gallery_name;
    if (StorageInfo::IsIPhotoDevice(device_id))
      gallery_name = kIPhotoGalleryName;
    else if (StorageInfo::IsITunesDevice(device_id))
      gallery_name = kITunesGalleryName;
    else if (StorageInfo::IsPicasaDevice(device_id))
      gallery_name = kPicasaGalleryName;
    else
      NOTREACHED();

    AddGalleryInternal(device_id, base::ASCIIToUTF16(gallery_name),
                       base::FilePath(), MediaGalleryPrefInfo::kAutoDetected,
                       base::string16(), base::string16(), base::string16(), 0,
                       base::Time(), false, 0, 0, 0, kCurrentPrefsVersion);
  }

  OnInitializationCallbackReturned();
}

void MediaGalleriesPreferences::InitFromPrefs() {
  known_galleries_.clear();
  device_map_.clear();

  PrefService* prefs = profile_->GetPrefs();
  const base::ListValue* list = prefs->GetList(
      prefs::kMediaGalleriesRememberedGalleries);
  if (list) {
    for (base::ListValue::const_iterator it = list->begin();
         it != list->end(); ++it) {
      const base::DictionaryValue* dict = NULL;
      if (!(*it)->GetAsDictionary(&dict))
        continue;

      MediaGalleryPrefInfo gallery_info;
      if (!PopulateGalleryPrefInfoFromDictionary(*dict, &gallery_info))
        continue;

      known_galleries_[gallery_info.pref_id] = gallery_info;
      device_map_[gallery_info.device_id].insert(gallery_info.pref_id);
    }
  }
}

void MediaGalleriesPreferences::AddGalleryChangeObserver(
    GalleryChangeObserver* observer) {
  DCHECK(IsInitialized());
  gallery_change_observers_.AddObserver(observer);
}

void MediaGalleriesPreferences::RemoveGalleryChangeObserver(
    GalleryChangeObserver* observer) {
  DCHECK(IsInitialized());
  gallery_change_observers_.RemoveObserver(observer);
}

void MediaGalleriesPreferences::OnRemovableStorageAttached(
    const StorageInfo& info) {
  DCHECK(IsInitialized());
  if (!StorageInfo::IsMediaDevice(info.device_id()))
    return;

  AddGallery(info.device_id(), base::FilePath(),
             MediaGalleryPrefInfo::kAutoDetected, info.storage_label(),
             info.vendor_name(), info.model_name(), info.total_size_in_bytes(),
             base::Time::Now(), 0, 0, 0);
}

bool MediaGalleriesPreferences::LookUpGalleryByPath(
    const base::FilePath& path,
    MediaGalleryPrefInfo* gallery_info) const {
  DCHECK(IsInitialized());
  StorageInfo info;
  base::FilePath relative_path;
  if (!MediaStorageUtil::GetDeviceInfoFromPath(path, &info, &relative_path)) {
    if (gallery_info)
      *gallery_info = MediaGalleryPrefInfo();
    return false;
  }

  relative_path = relative_path.NormalizePathSeparators();
  MediaGalleryPrefIdSet galleries_on_device =
      LookUpGalleriesByDeviceId(info.device_id());
  for (MediaGalleryPrefIdSet::const_iterator it = galleries_on_device.begin();
       it != galleries_on_device.end();
       ++it) {
    const MediaGalleryPrefInfo& gallery = known_galleries_.find(*it)->second;
    if (gallery.path != relative_path)
      continue;

    if (gallery_info)
      *gallery_info = gallery;
    return true;
  }

  // This method is called by controller::FilesSelected when the user
  // adds a new gallery. Control reaches here when the selected gallery is
  // on a volume we know about, but have no gallery already for. Returns
  // hypothetical data to the caller about what the prefs will look like
  // if the gallery is added.
  // TODO(gbillock): split this out into another function so it doesn't
  // conflate LookUp.
  if (gallery_info) {
    gallery_info->pref_id = kInvalidMediaGalleryPrefId;
    gallery_info->device_id = info.device_id();
    gallery_info->path = relative_path;
    gallery_info->type = MediaGalleryPrefInfo::kInvalidType;
    gallery_info->volume_label = info.storage_label();
    gallery_info->vendor_name = info.vendor_name();
    gallery_info->model_name = info.model_name();
    gallery_info->total_size_in_bytes = info.total_size_in_bytes();
    gallery_info->last_attach_time = base::Time::Now();
    gallery_info->volume_metadata_valid = true;
    gallery_info->prefs_version = kCurrentPrefsVersion;
  }
  return false;
}

MediaGalleryPrefIdSet MediaGalleriesPreferences::LookUpGalleriesByDeviceId(
    const std::string& device_id) const {
  DeviceIdPrefIdsMap::const_iterator found = device_map_.find(device_id);
  if (found == device_map_.end())
    return MediaGalleryPrefIdSet();
  return found->second;
}

base::FilePath MediaGalleriesPreferences::LookUpGalleryPathForExtension(
    MediaGalleryPrefId gallery_id,
    const extensions::Extension* extension,
    bool include_unpermitted_galleries) {
  DCHECK(IsInitialized());
  DCHECK(extension);
  if (!include_unpermitted_galleries &&
      !ContainsKey(GalleriesForExtension(*extension), gallery_id))
    return base::FilePath();

  MediaGalleriesPrefInfoMap::const_iterator it =
      known_galleries_.find(gallery_id);
  if (it == known_galleries_.end())
    return base::FilePath();
  return MediaStorageUtil::FindDevicePathById(it->second.device_id);
}

MediaGalleryPrefId MediaGalleriesPreferences::AddGallery(
    const std::string& device_id,
    const base::FilePath& relative_path,
    MediaGalleryPrefInfo::Type type,
    const base::string16& volume_label,
    const base::string16& vendor_name,
    const base::string16& model_name,
    uint64 total_size_in_bytes,
    base::Time last_attach_time,
    int audio_count,
    int image_count,
    int video_count) {
  DCHECK(IsInitialized());
  return AddGalleryInternal(device_id, base::string16(), relative_path,
                            type, volume_label, vendor_name, model_name,
                            total_size_in_bytes, last_attach_time, true,
                            audio_count, image_count, video_count,
                            kCurrentPrefsVersion);
}

MediaGalleryPrefId MediaGalleriesPreferences::AddGalleryInternal(
    const std::string& device_id, const base::string16& display_name,
    const base::FilePath& relative_path, MediaGalleryPrefInfo::Type type,
    const base::string16& volume_label, const base::string16& vendor_name,
    const base::string16& model_name, uint64 total_size_in_bytes,
    base::Time last_attach_time, bool volume_metadata_valid,
    int audio_count, int image_count, int video_count, int prefs_version) {
  DCHECK(type == MediaGalleryPrefInfo::kUserAdded ||
         type == MediaGalleryPrefInfo::kAutoDetected ||
         type == MediaGalleryPrefInfo::kScanResult);
  base::FilePath normalized_relative_path =
      relative_path.NormalizePathSeparators();
  MediaGalleryPrefIdSet galleries_on_device =
    LookUpGalleriesByDeviceId(device_id);
  for (MediaGalleryPrefIdSet::const_iterator pref_id_it =
           galleries_on_device.begin();
       pref_id_it != galleries_on_device.end();
       ++pref_id_it) {
    const MediaGalleryPrefInfo& existing =
        known_galleries_.find(*pref_id_it)->second;
    if (existing.path != normalized_relative_path)
      continue;

    bool update_gallery_type = false;
    MediaGalleryPrefInfo::Type new_type = existing.type;
    if (type == MediaGalleryPrefInfo::kUserAdded) {
      if (existing.type == MediaGalleryPrefInfo::kBlackListed) {
        new_type = MediaGalleryPrefInfo::kAutoDetected;
        update_gallery_type = true;
      }
      if (existing.type == MediaGalleryPrefInfo::kRemovedScan) {
        new_type = MediaGalleryPrefInfo::kUserAdded;
        update_gallery_type = true;
      }
    }

    // Status quo: In M27 and M28, galleries added manually use version 0,
    // and galleries added automatically (including default galleries) use
    // version 1. The name override is used by default galleries as well
    // as all device attach events.
    // We want to upgrade the name if the existing version is < 2. Leave it
    // alone if the existing display name is set with version == 2 and the
    // proposed new name is empty.
    bool update_gallery_name = existing.display_name != display_name;
    if (existing.prefs_version == 2 && !existing.display_name.empty() &&
        display_name.empty()) {
      update_gallery_name = false;
    }
    bool update_gallery_metadata = volume_metadata_valid &&
        ((existing.volume_label != volume_label) ||
         (existing.vendor_name != vendor_name) ||
         (existing.model_name != model_name) ||
         (existing.total_size_in_bytes != total_size_in_bytes) ||
         (existing.last_attach_time != last_attach_time));

    bool update_scan_counts =
      new_type != MediaGalleryPrefInfo::kRemovedScan &&
      new_type != MediaGalleryPrefInfo::kBlackListed &&
      (audio_count > 0 || image_count > 0 || video_count > 0);

    if (!update_gallery_name && !update_gallery_type &&
        !update_gallery_metadata && !update_scan_counts)
      return *pref_id_it;

    PrefService* prefs = profile_->GetPrefs();
    scoped_ptr<ListPrefUpdate> update(
        new ListPrefUpdate(prefs, prefs::kMediaGalleriesRememberedGalleries));
    base::ListValue* list = update->Get();

    for (base::ListValue::const_iterator list_iter = list->begin();
         list_iter != list->end();
         ++list_iter) {
      base::DictionaryValue* dict;
      MediaGalleryPrefId iter_id;
      if ((*list_iter)->GetAsDictionary(&dict) &&
          GetPrefId(*dict, &iter_id) &&
          *pref_id_it == iter_id) {
        if (update_gallery_type)
          dict->SetString(kMediaGalleriesTypeKey, TypeToStringValue(new_type));
        if (update_gallery_name)
          dict->SetString(kMediaGalleriesDisplayNameKey, display_name);
        if (update_gallery_metadata) {
          dict->SetString(kMediaGalleriesVolumeLabelKey, volume_label);
          dict->SetString(kMediaGalleriesVendorNameKey, vendor_name);
          dict->SetString(kMediaGalleriesModelNameKey, model_name);
          dict->SetDouble(kMediaGalleriesSizeKey, total_size_in_bytes);
          dict->SetDouble(kMediaGalleriesLastAttachTimeKey,
                          last_attach_time.ToInternalValue());
        }
        if (update_scan_counts) {
          dict->SetInteger(kMediaGalleriesScanAudioCountKey, audio_count);
          dict->SetInteger(kMediaGalleriesScanImageCountKey, image_count);
          dict->SetInteger(kMediaGalleriesScanVideoCountKey, video_count);
        }
        dict->SetInteger(kMediaGalleriesPrefsVersionKey, prefs_version);
        break;
      }
    }

    // Commits the prefs update.
    update.reset();

    InitFromPrefs();
    FOR_EACH_OBSERVER(GalleryChangeObserver, gallery_change_observers_,
                      OnGalleryInfoUpdated(this, *pref_id_it));
    return *pref_id_it;
  }

  PrefService* prefs = profile_->GetPrefs();

  MediaGalleryPrefInfo gallery_info;
  gallery_info.pref_id = prefs->GetUint64(prefs::kMediaGalleriesUniqueId);
  prefs->SetUint64(prefs::kMediaGalleriesUniqueId, gallery_info.pref_id + 1);
  gallery_info.display_name = display_name;
  gallery_info.device_id = device_id;
  gallery_info.path = normalized_relative_path;
  gallery_info.type = type;
  gallery_info.volume_label = volume_label;
  gallery_info.vendor_name = vendor_name;
  gallery_info.model_name = model_name;
  gallery_info.total_size_in_bytes = total_size_in_bytes;
  gallery_info.last_attach_time = last_attach_time;
  gallery_info.volume_metadata_valid = volume_metadata_valid;
  gallery_info.audio_count = audio_count;
  gallery_info.image_count = image_count;
  gallery_info.video_count = video_count;
  gallery_info.prefs_version = prefs_version;

  {
    ListPrefUpdate update(prefs, prefs::kMediaGalleriesRememberedGalleries);
    base::ListValue* list = update.Get();
    list->Append(CreateGalleryPrefInfoDictionary(gallery_info));
  }
  InitFromPrefs();
  FOR_EACH_OBSERVER(GalleryChangeObserver,
                    gallery_change_observers_,
                    OnGalleryAdded(this, gallery_info.pref_id));

  return gallery_info.pref_id;
}

MediaGalleryPrefId MediaGalleriesPreferences::AddGalleryByPath(
    const base::FilePath& path, MediaGalleryPrefInfo::Type type) {
  DCHECK(IsInitialized());
  MediaGalleryPrefInfo gallery_info;
  if (LookUpGalleryByPath(path, &gallery_info) &&
      !gallery_info.IsBlackListedType()) {
    return gallery_info.pref_id;
  }
  return AddGalleryInternal(gallery_info.device_id,
                            gallery_info.display_name,
                            gallery_info.path,
                            type,
                            gallery_info.volume_label,
                            gallery_info.vendor_name,
                            gallery_info.model_name,
                            gallery_info.total_size_in_bytes,
                            gallery_info.last_attach_time,
                            gallery_info.volume_metadata_valid,
                            0, 0, 0,
                            kCurrentPrefsVersion);
}

void MediaGalleriesPreferences::ForgetGalleryById(MediaGalleryPrefId id) {
  EraseOrBlacklistGalleryById(id, false);
}

void MediaGalleriesPreferences::EraseGalleryById(MediaGalleryPrefId id) {
  EraseOrBlacklistGalleryById(id, true);
}

void MediaGalleriesPreferences::EraseOrBlacklistGalleryById(
    MediaGalleryPrefId id, bool erase) {
  DCHECK(IsInitialized());
  PrefService* prefs = profile_->GetPrefs();
  scoped_ptr<ListPrefUpdate> update(new ListPrefUpdate(
      prefs, prefs::kMediaGalleriesRememberedGalleries));
  base::ListValue* list = update->Get();

  if (!ContainsKey(known_galleries_, id))
    return;

  for (base::ListValue::iterator iter = list->begin();
       iter != list->end(); ++iter) {
    base::DictionaryValue* dict;
    MediaGalleryPrefId iter_id;
    if ((*iter)->GetAsDictionary(&dict) && GetPrefId(*dict, &iter_id) &&
        id == iter_id) {
      RemoveGalleryPermissionsFromPrefs(id);
      MediaGalleryPrefInfo::Type type;
      if (!erase && GetType(*dict, &type) &&
          (type == MediaGalleryPrefInfo::kAutoDetected ||
           type == MediaGalleryPrefInfo::kScanResult)) {
        if (type == MediaGalleryPrefInfo::kAutoDetected) {
          dict->SetString(kMediaGalleriesTypeKey,
                          kMediaGalleriesTypeBlackListedValue);
        } else {
          dict->SetString(kMediaGalleriesTypeKey,
                          kMediaGalleriesTypeRemovedScanValue);
          dict->SetInteger(kMediaGalleriesScanAudioCountKey, 0);
          dict->SetInteger(kMediaGalleriesScanImageCountKey, 0);
          dict->SetInteger(kMediaGalleriesScanVideoCountKey, 0);
        }
      } else {
        list->Erase(iter, NULL);
      }
      update.reset(NULL);  // commits the update.

      InitFromPrefs();
      FOR_EACH_OBSERVER(GalleryChangeObserver,
                        gallery_change_observers_,
                        OnGalleryRemoved(this, id));
      return;
    }
  }
}

bool MediaGalleriesPreferences::NonAutoGalleryHasPermission(
    MediaGalleryPrefId id) const {
  DCHECK(IsInitialized());
  DCHECK(!ContainsKey(known_galleries_, id) ||
         known_galleries_.find(id)->second.type !=
             MediaGalleryPrefInfo::kAutoDetected);
  ExtensionPrefs* prefs = GetExtensionPrefs();
  const base::DictionaryValue* extensions =
      prefs->pref_service()->GetDictionary(extensions::pref_names::kExtensions);
  if (!extensions)
    return true;

  for (base::DictionaryValue::Iterator iter(*extensions); !iter.IsAtEnd();
       iter.Advance()) {
    if (!extensions::Extension::IdIsValid(iter.key())) {
      NOTREACHED();
      continue;
    }
    std::vector<MediaGalleryPermission> permissions =
        GetGalleryPermissionsFromPrefs(iter.key());
    for (std::vector<MediaGalleryPermission>::const_iterator it =
             permissions.begin(); it != permissions.end(); ++it) {
      if (it->pref_id == id) {
        if (it->has_permission)
          return true;
        break;
      }
    }
  }
  return false;
}

MediaGalleryPrefIdSet MediaGalleriesPreferences::GalleriesForExtension(
    const extensions::Extension& extension) const {
  DCHECK(IsInitialized());
  MediaGalleryPrefIdSet result;

  if (HasAutoDetectedGalleryPermission(extension)) {
    for (MediaGalleriesPrefInfoMap::const_iterator it =
             known_galleries_.begin(); it != known_galleries_.end(); ++it) {
      if (it->second.type == MediaGalleryPrefInfo::kAutoDetected)
        result.insert(it->second.pref_id);
    }
  }

  std::vector<MediaGalleryPermission> stored_permissions =
      GetGalleryPermissionsFromPrefs(extension.id());
  for (std::vector<MediaGalleryPermission>::const_iterator it =
           stored_permissions.begin(); it != stored_permissions.end(); ++it) {
    if (!it->has_permission) {
      result.erase(it->pref_id);
    } else {
      MediaGalleriesPrefInfoMap::const_iterator gallery =
          known_galleries_.find(it->pref_id);
      DCHECK(gallery != known_galleries_.end());
      if (!gallery->second.IsBlackListedType()) {
        result.insert(it->pref_id);
      } else {
        NOTREACHED() << gallery->second.device_id;
      }
    }
  }
  return result;
}

bool MediaGalleriesPreferences::SetGalleryPermissionForExtension(
    const extensions::Extension& extension,
    MediaGalleryPrefId pref_id,
    bool has_permission) {
  DCHECK(IsInitialized());
  // The gallery may not exist anymore if the user opened a second config
  // surface concurrently and removed it. Drop the permission update if so.
  MediaGalleriesPrefInfoMap::const_iterator gallery_info =
      known_galleries_.find(pref_id);
  if (gallery_info == known_galleries_.end())
    return false;

  bool default_permission = false;
  if (gallery_info->second.type == MediaGalleryPrefInfo::kAutoDetected)
    default_permission = HasAutoDetectedGalleryPermission(extension);
  // When the permission matches the default, we don't need to remember it.
  if (has_permission == default_permission) {
    if (!UnsetGalleryPermissionInPrefs(extension.id(), pref_id))
      // If permission wasn't set, assume nothing has changed.
      return false;
  } else {
    if (!SetGalleryPermissionInPrefs(extension.id(), pref_id, has_permission))
      return false;
  }
  if (has_permission)
    FOR_EACH_OBSERVER(GalleryChangeObserver,
                      gallery_change_observers_,
                      OnPermissionAdded(this, extension.id(), pref_id));
  else
    FOR_EACH_OBSERVER(GalleryChangeObserver,
                      gallery_change_observers_,
                      OnPermissionRemoved(this, extension.id(), pref_id));
  return true;
}

const MediaGalleriesPrefInfoMap& MediaGalleriesPreferences::known_galleries()
    const {
  DCHECK(IsInitialized());
  return known_galleries_;
}

base::Time MediaGalleriesPreferences::GetLastScanCompletionTime() const {
  int64 last_scan_time_internal =
      profile_->GetPrefs()->GetInt64(prefs::kMediaGalleriesLastScanTime);
  return base::Time::FromInternalValue(last_scan_time_internal);
}

void MediaGalleriesPreferences::SetLastScanCompletionTime(
    const base::Time& time) {
  profile_->GetPrefs()->SetInt64(prefs::kMediaGalleriesLastScanTime,
                                 time.ToInternalValue());
}

void MediaGalleriesPreferences::Shutdown() {
  weak_factory_.InvalidateWeakPtrs();
  profile_ = NULL;
}

// static
bool MediaGalleriesPreferences::APIHasBeenUsed(Profile* profile) {
  MediaGalleryPrefId current_id =
      profile->GetPrefs()->GetUint64(prefs::kMediaGalleriesUniqueId);
  return current_id != kInvalidMediaGalleryPrefId + 1;
}

// static
void MediaGalleriesPreferences::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(prefs::kMediaGalleriesRememberedGalleries,
                             user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterUint64Pref(
      prefs::kMediaGalleriesUniqueId,
      kInvalidMediaGalleryPrefId + 1,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterInt64Pref(
      prefs::kMediaGalleriesLastScanTime,
      base::Time().ToInternalValue(),
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
}

bool MediaGalleriesPreferences::SetGalleryPermissionInPrefs(
    const std::string& extension_id,
    MediaGalleryPrefId gallery_id,
    bool has_access) {
  DCHECK(IsInitialized());
  ExtensionPrefs::ScopedListUpdate update(GetExtensionPrefs(),
                                          extension_id,
                                          kMediaGalleriesPermissions);
  base::ListValue* permissions = update.Get();
  if (!permissions) {
    permissions = update.Create();
  } else {
    // If the gallery is already in the list, update the permission...
    for (base::ListValue::iterator iter = permissions->begin();
         iter != permissions->end(); ++iter) {
      base::DictionaryValue* dict = NULL;
      if (!(*iter)->GetAsDictionary(&dict))
        continue;
      MediaGalleryPermission perm;
      if (!GetMediaGalleryPermissionFromDictionary(dict, &perm))
        continue;
      if (perm.pref_id == gallery_id) {
        if (has_access != perm.has_permission) {
          dict->SetBoolean(kMediaGalleryHasPermissionKey, has_access);
          return true;
        } else {
          return false;
        }
      }
    }
  }
  // ...Otherwise, add a new entry for the gallery.
  base::DictionaryValue* dict = new base::DictionaryValue;
  dict->SetString(kMediaGalleryIdKey, base::Uint64ToString(gallery_id));
  dict->SetBoolean(kMediaGalleryHasPermissionKey, has_access);
  permissions->Append(dict);
  return true;
}

bool MediaGalleriesPreferences::UnsetGalleryPermissionInPrefs(
    const std::string& extension_id,
    MediaGalleryPrefId gallery_id) {
  DCHECK(IsInitialized());
  ExtensionPrefs::ScopedListUpdate update(GetExtensionPrefs(),
                                          extension_id,
                                          kMediaGalleriesPermissions);
  base::ListValue* permissions = update.Get();
  if (!permissions)
    return false;

  for (base::ListValue::iterator iter = permissions->begin();
       iter != permissions->end(); ++iter) {
    const base::DictionaryValue* dict = NULL;
    if (!(*iter)->GetAsDictionary(&dict))
      continue;
    MediaGalleryPermission perm;
    if (!GetMediaGalleryPermissionFromDictionary(dict, &perm))
      continue;
    if (perm.pref_id == gallery_id) {
      permissions->Erase(iter, NULL);
      return true;
    }
  }
  return false;
}

std::vector<MediaGalleryPermission>
MediaGalleriesPreferences::GetGalleryPermissionsFromPrefs(
    const std::string& extension_id) const {
  DCHECK(IsInitialized());
  std::vector<MediaGalleryPermission> result;
  const base::ListValue* permissions;
  if (!GetExtensionPrefs()->ReadPrefAsList(extension_id,
                                           kMediaGalleriesPermissions,
                                           &permissions)) {
    return result;
  }

  for (base::ListValue::const_iterator iter = permissions->begin();
       iter != permissions->end(); ++iter) {
    base::DictionaryValue* dict = NULL;
    if (!(*iter)->GetAsDictionary(&dict))
      continue;
    MediaGalleryPermission perm;
    if (!GetMediaGalleryPermissionFromDictionary(dict, &perm))
      continue;
    result.push_back(perm);
  }

  return result;
}

void MediaGalleriesPreferences::RemoveGalleryPermissionsFromPrefs(
    MediaGalleryPrefId gallery_id) {
  DCHECK(IsInitialized());
  ExtensionPrefs* prefs = GetExtensionPrefs();
  const base::DictionaryValue* extensions =
      prefs->pref_service()->GetDictionary(extensions::pref_names::kExtensions);
  if (!extensions)
    return;

  for (base::DictionaryValue::Iterator iter(*extensions); !iter.IsAtEnd();
       iter.Advance()) {
    if (!extensions::Extension::IdIsValid(iter.key())) {
      NOTREACHED();
      continue;
    }
    UnsetGalleryPermissionInPrefs(iter.key(), gallery_id);
  }
}

ExtensionPrefs* MediaGalleriesPreferences::GetExtensionPrefs() const {
  DCHECK(IsInitialized());
  if (extension_prefs_for_testing_)
    return extension_prefs_for_testing_;
  return extensions::ExtensionPrefs::Get(profile_);
}

void MediaGalleriesPreferences::SetExtensionPrefsForTesting(
    extensions::ExtensionPrefs* extension_prefs) {
  DCHECK(IsInitialized());
  extension_prefs_for_testing_ = extension_prefs;
}
