// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/google_apis/fake_drive_service.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/stringprintf.h"
#include "chrome/browser/google_apis/gdata_wapi_parser.h"
#include "chrome/browser/google_apis/test_util.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace google_apis {

FakeDriveService::FakeDriveService()
    : resource_id_count_(0) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

FakeDriveService::~FakeDriveService() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

bool FakeDriveService::LoadResourceListForWapi(
    const std::string& relative_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  scoped_ptr<Value> raw_value = test_util::LoadJSONFile(relative_path);
  base::DictionaryValue* as_dict = NULL;
  base::Value* feed = NULL;
  base::DictionaryValue* feed_as_dict = NULL;

  // Extract the "feed" from the raw value and take the ownership.
  // Note that Remove() transfers the ownership to |feed|.
  if (raw_value->GetAsDictionary(&as_dict) &&
      as_dict->Remove("feed", &feed) &&
      feed->GetAsDictionary(&feed_as_dict)) {
    resource_list_value_.reset(feed_as_dict);
  }

  return resource_list_value_;
}

bool FakeDriveService::LoadAccountMetadataForWapi(
    const std::string& relative_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  account_metadata_value_ = test_util::LoadJSONFile(relative_path);
  return account_metadata_value_;
}

bool FakeDriveService::LoadApplicationInfoForDriveApi(
    const std::string& relative_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  app_info_value_ = test_util::LoadJSONFile(relative_path);
  return app_info_value_;
}

void FakeDriveService::Initialize(Profile* profile) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeDriveService::AddObserver(DriveServiceObserver* observer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeDriveService::RemoveObserver(DriveServiceObserver* observer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

bool FakeDriveService::CanStartOperation() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return true;
}

void FakeDriveService::CancelAll() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

bool FakeDriveService::CancelForFilePath(const FilePath& file_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return true;
}

OperationProgressStatusList FakeDriveService::GetProgressStatusList() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return OperationProgressStatusList();
}

bool FakeDriveService::HasAccessToken() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return true;
}

bool FakeDriveService::HasRefreshToken() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return true;
}
void FakeDriveService::GetResourceList(
    const GURL& feed_url,
    int64 start_changestamp,
    const std::string& search_query,
    bool shared_with_me,
    const std::string& directory_resource_id,
    const GetResourceListCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  scoped_ptr<ResourceList> resource_list =
      ResourceList::CreateFrom(*resource_list_value_);
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(callback,
                 HTTP_SUCCESS,
                 base::Passed(&resource_list)));
}

void FakeDriveService::GetResourceEntry(
    const std::string& resource_id,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  base::DictionaryValue* entry = FindEntryByResourceId(resource_id);
  if (entry) {
    scoped_ptr<ResourceEntry> resource_entry =
        ResourceEntry::CreateFrom(*entry);
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, HTTP_SUCCESS, base::Passed(&resource_entry)));
    return;
  }

  scoped_ptr<ResourceEntry> null;
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(callback, HTTP_NOT_FOUND, base::Passed(&null)));
}

void FakeDriveService::GetAccountMetadata(
    const GetAccountMetadataCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  scoped_ptr<AccountMetadataFeed> account_metadata =
      AccountMetadataFeed::CreateFrom(*account_metadata_value_);
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(callback,
                 HTTP_SUCCESS,
                 base::Passed(&account_metadata)));
}

void FakeDriveService::GetApplicationInfo(
    const GetDataCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  scoped_ptr<base::Value> copied_app_info_value(
      app_info_value_->DeepCopy());
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(callback,
                 HTTP_SUCCESS,
                 base::Passed(&copied_app_info_value)));

}

void FakeDriveService::DeleteResource(
    const GURL& edit_url,
    const EntryActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  base::DictionaryValue* resource_list_dict = NULL;
  base::ListValue* entries = NULL;
  // Go through entries and remove the one that matches |edit_url|.
  if (resource_list_value_->GetAsDictionary(&resource_list_dict) &&
      resource_list_dict->GetList("entry", &entries)) {
    for (size_t i = 0; i < entries->GetSize(); ++i) {
      base::DictionaryValue* entry = NULL;
      base::ListValue* links = NULL;
      if (entries->GetDictionary(i, &entry) &&
          entry->GetList("link", &links)) {
        for (size_t j = 0; j < links->GetSize(); ++j) {
          base::DictionaryValue* link = NULL;
          std::string rel;
          std::string href;
          if (links->GetDictionary(j, &link) &&
              link->GetString("rel", &rel) &&
              link->GetString("href", &href) &&
              rel == "edit" &&
              GURL(href) == edit_url) {
            entries->Remove(i, NULL);
            MessageLoop::current()->PostTask(
                FROM_HERE, base::Bind(callback, HTTP_SUCCESS));
            return;
          }
        }
      }
    }
  }

  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, HTTP_NOT_FOUND));
}

void FakeDriveService::DownloadHostedDocument(
    const FilePath& virtual_path,
    const FilePath& local_cache_path,
    const GURL& content_url,
    DocumentExportFormat format,
    const DownloadActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());
}

void FakeDriveService::DownloadFile(
    const FilePath& virtual_path,
    const FilePath& local_cache_path,
    const GURL& content_url,
    const DownloadActionCallback& download_action_callback,
    const GetContentCallback& get_content_callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!download_action_callback.is_null());

  base::DictionaryValue* entry = FindEntryByContentUrl(content_url);
  if (!entry) {
    base::MessageLoopProxy::current()->PostTask(
        FROM_HERE,
        base::Bind(download_action_callback, HTTP_NOT_FOUND, FilePath()));
    return;
  }

  // Write the content URL as the content of the file.
  if (static_cast<int>(content_url.spec().size()) !=
      file_util::WriteFile(local_cache_path,
                           content_url.spec().data(),
                           content_url.spec().size())) {
    base::MessageLoopProxy::current()->PostTask(
        FROM_HERE,
        base::Bind(download_action_callback, GDATA_FILE_ERROR, FilePath()));
    return;
  }

  base::MessageLoopProxy::current()->PostTask(
      FROM_HERE,
      base::Bind(download_action_callback, HTTP_SUCCESS, local_cache_path));
}

void FakeDriveService::CopyHostedDocument(
    const std::string& resource_id,
    const FilePath::StringType& new_name,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  base::DictionaryValue* resource_list_dict = NULL;
  base::ListValue* entries = NULL;
  // Go through entries and copy the one that matches |resource_id|.
  if (resource_list_value_->GetAsDictionary(&resource_list_dict) &&
      resource_list_dict->GetList("entry", &entries)) {
    for (size_t i = 0; i < entries->GetSize(); ++i) {
      base::DictionaryValue* entry = NULL;
      base::DictionaryValue* resource_id_dict = NULL;
      base::ListValue* categories = NULL;
      std::string current_resource_id;
      if (entries->GetDictionary(i, &entry) &&
          entry->GetDictionary("gd$resourceId", &resource_id_dict) &&
          resource_id_dict->GetString("$t", &current_resource_id) &&
          resource_id == current_resource_id &&
          entry->GetList("category", &categories)) {
        // Check that the resource is a hosted document. We consider it a
        // hosted document if the kind is neither "folder" nor "file".
        for (size_t k = 0; k < categories->GetSize(); ++k) {
          base::DictionaryValue* category = NULL;
          std::string scheme, term;
          if (categories->GetDictionary(k, &category) &&
              category->GetString("scheme", &scheme) &&
              category->GetString("term", &term) &&
              scheme == "http://schemas.google.com/g/2005#kind" &&
              term != "http://schemas.google.com/docs/2007#file" &&
              term != "http://schemas.google.com/docs/2007#folder") {
            // Make a copy and set the new resource ID and the new title.
            scoped_ptr<DictionaryValue> copied_entry(entry->DeepCopy());
            copied_entry->SetString("gd$resourceId.$t",
                                    resource_id + "_copied");
            copied_entry->SetString("title.$t",
                                    FilePath(new_name).AsUTF8Unsafe());
            // Parse the new entry.
            scoped_ptr<ResourceEntry> resource_entry =
                ResourceEntry::CreateFrom(*copied_entry);
            // Add it to the resource list.
            entries->Append(copied_entry.release());

            MessageLoop::current()->PostTask(
                FROM_HERE,
                base::Bind(callback,
                           HTTP_SUCCESS,
                           base::Passed(&resource_entry)));
            return;
          }
        }
      }
    }
  }

  scoped_ptr<ResourceEntry> null;
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(callback, HTTP_NOT_FOUND, base::Passed(&null)));
}

void FakeDriveService::RenameResource(
    const GURL& edit_url,
    const FilePath::StringType& new_name,
    const EntryActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  base::DictionaryValue* entry = FindEntryByEditUrl(edit_url);
  if (entry) {
    entry->SetString("title.$t",
                     FilePath(new_name).AsUTF8Unsafe());
    MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, HTTP_SUCCESS));
    return;
  }

  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, HTTP_NOT_FOUND));
}

void FakeDriveService::AddResourceToDirectory(
    const GURL& parent_content_url,
    const GURL& edit_url,
    const EntryActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  base::DictionaryValue* entry = FindEntryByEditUrl(edit_url);
  if (entry) {
    base::ListValue* links = NULL;
    if (entry->GetList("link", &links)) {
      bool parent_link_found = false;
      for (size_t i = 0; i < links->GetSize(); ++i) {
        base::DictionaryValue* link = NULL;
        std::string rel;
        if (links->GetDictionary(i, &link) &&
            link->GetString("rel", &rel) &&
            rel == "http://schemas.google.com/docs/2007#parent") {
          link->SetString("href", parent_content_url.spec());
          parent_link_found = true;
        }
      }
      // The parent link does not exist if a resource is in the root
      // directory.
      if (!parent_link_found) {
        base::DictionaryValue* link = new base::DictionaryValue;
        link->SetString("rel", "http://schemas.google.com/docs/2007#parent");
        link->SetString("href", parent_content_url.spec());
        links->Append(link);
      }

      MessageLoop::current()->PostTask(
          FROM_HERE, base::Bind(callback, HTTP_SUCCESS));
      return;
    }
  }

  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, HTTP_NOT_FOUND));
}

void FakeDriveService::RemoveResourceFromDirectory(
    const GURL& parent_content_url,
    const std::string& resource_id,
    const EntryActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  base::DictionaryValue* entry = FindEntryByResourceId(resource_id);
  if (entry) {
    base::ListValue* links = NULL;
    if (entry->GetList("link", &links)) {
      for (size_t i = 0; i < links->GetSize(); ++i) {
        base::DictionaryValue* link = NULL;
        std::string rel;
        std::string href;
        if (links->GetDictionary(i, &link) &&
            link->GetString("rel", &rel) &&
            link->GetString("href", &href) &&
            rel == "http://schemas.google.com/docs/2007#parent" &&
            GURL(href) == parent_content_url) {
          links->Remove(i, NULL);
          MessageLoop::current()->PostTask(
              FROM_HERE, base::Bind(callback, HTTP_SUCCESS));
          return;
        }
      }
    }
  }

  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, HTTP_NOT_FOUND));
}

void FakeDriveService::AddNewDirectory(
    const GURL& parent_content_url,
    const FilePath::StringType& directory_name,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  // If the parent content URL is not empty, the parent should exist.
  if (!parent_content_url.is_empty()) {
    base::DictionaryValue* parent_entry =
        FindEntryByContentUrl(parent_content_url);
    if (!parent_entry) {
      scoped_ptr<ResourceEntry> null;
      MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(callback, HTTP_NOT_FOUND, base::Passed(&null)));
      return;
    }
  }

  const std::string new_resource_id = GetNewResourceId();

  scoped_ptr<base::DictionaryValue> new_entry(new base::DictionaryValue);
  // Set the resource ID and the title
  new_entry->SetString("gd$resourceId.$t", new_resource_id);
  new_entry->SetString("title.$t", FilePath(directory_name).AsUTF8Unsafe());

  // Add "category" which sets the resource type to folder.
  base::ListValue* categories = new base::ListValue;
  base::DictionaryValue* category = new base::DictionaryValue;
  category->SetString("label", "folder");
  category->SetString("scheme", "http://schemas.google.com/g/2005#kind");
  category->SetString("term", "http://schemas.google.com/docs/2007#folder");
  categories->Append(category);
  new_entry->Set("category", categories);

  // Add "content" which sets the content URL.
  base::DictionaryValue* content = new base::DictionaryValue;
  content->SetString("src", "https://xxx/content/" + new_resource_id);
  new_entry->Set("content", content);

  // Add "link" which sets the parent URL and the edit URL.
  base::ListValue* links = new base::ListValue;
  if (!parent_content_url.is_empty()) {
    base::DictionaryValue* parent_link = new base::DictionaryValue;
    parent_link->SetString("href", parent_content_url.spec());
    parent_link->SetString("rel",
                           "http://schemas.google.com/docs/2007#parent");
    links->Append(parent_link);
  }
  base::DictionaryValue* edit_link = new base::DictionaryValue;
  edit_link->SetString("href", "https://xxx/edit/" + new_resource_id);
  edit_link->SetString("rel", "edit");
  links->Append(edit_link);
  new_entry->Set("link", links);

  // Add the new entry to the resource list.
  base::DictionaryValue* resource_list_dict = NULL;
  base::ListValue* entries = NULL;
  if (resource_list_value_->GetAsDictionary(&resource_list_dict) &&
      resource_list_dict->GetList("entry", &entries)) {
    // Parse the entry before releasing it.
    scoped_ptr<ResourceEntry> parsed_entry(
        ResourceEntry::CreateFrom(*new_entry));

    entries->Append(new_entry.release());

    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, HTTP_SUCCESS, base::Passed(&parsed_entry)));
    return;
  }

  scoped_ptr<ResourceEntry> null;
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(callback, HTTP_NOT_FOUND, base::Passed(&null)));
}

void FakeDriveService::InitiateUpload(
    const InitiateUploadParams& params,
    const InitiateUploadCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());
}

void FakeDriveService::ResumeUpload(const ResumeUploadParams& params,
                                    const ResumeUploadCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());
}

void FakeDriveService::AuthorizeApp(const GURL& edit_url,
                                    const std::string& app_id,
                                    const AuthorizeAppCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());
}

base::DictionaryValue* FakeDriveService::FindEntryByResourceId(
    const std::string& resource_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  base::DictionaryValue* resource_list_dict = NULL;
  base::ListValue* entries = NULL;
  // Go through entries and return the one that matches |resource_id|.
  if (resource_list_value_->GetAsDictionary(&resource_list_dict) &&
      resource_list_dict->GetList("entry", &entries)) {
    for (size_t i = 0; i < entries->GetSize(); ++i) {
      base::DictionaryValue* entry = NULL;
      base::DictionaryValue* resource_id_dict = NULL;
      std::string current_resource_id;
      if (entries->GetDictionary(i, &entry) &&
          entry->GetDictionary("gd$resourceId", &resource_id_dict) &&
          resource_id_dict->GetString("$t", &current_resource_id) &&
          resource_id == current_resource_id) {
        return entry;
      }
    }
  }

  return NULL;
}

base::DictionaryValue* FakeDriveService::FindEntryByEditUrl(
    const GURL& edit_url) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  base::DictionaryValue* resource_list_dict = NULL;
  base::ListValue* entries = NULL;
  // Go through entries and return the one that matches |edit_url|.
  if (resource_list_value_->GetAsDictionary(&resource_list_dict) &&
      resource_list_dict->GetList("entry", &entries)) {
    for (size_t i = 0; i < entries->GetSize(); ++i) {
      base::DictionaryValue* entry = NULL;
      base::ListValue* links = NULL;
      if (entries->GetDictionary(i, &entry) &&
          entry->GetList("link", &links)) {
        for (size_t j = 0; j < links->GetSize(); ++j) {
          base::DictionaryValue* link = NULL;
          std::string rel;
          std::string href;
          if (links->GetDictionary(j, &link) &&
              link->GetString("rel", &rel) &&
              link->GetString("href", &href) &&
              rel == "edit" &&
              GURL(href) == edit_url) {
            return entry;
          }
        }
      }
    }
  }

  return NULL;
}

base::DictionaryValue* FakeDriveService::FindEntryByContentUrl(
    const GURL& content_url) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  base::DictionaryValue* resource_list_dict = NULL;
  base::ListValue* entries = NULL;
  // Go through entries and return the one that matches |content_url|.
  if (resource_list_value_->GetAsDictionary(&resource_list_dict) &&
      resource_list_dict->GetList("entry", &entries)) {
    for (size_t i = 0; i < entries->GetSize(); ++i) {
      base::DictionaryValue* entry = NULL;
      base::DictionaryValue* content = NULL;
      std::string current_content_url;
      if (entries->GetDictionary(i, &entry) &&
          entry->GetDictionary("content", &content) &&
          content->GetString("src", &current_content_url) &&
          content_url == GURL(current_content_url)) {
        return entry;
      }
    }
  }

  return NULL;
}

std::string FakeDriveService::GetNewResourceId() {
  ++resource_id_count_;
  return base::StringPrintf("resource_id_%d", resource_id_count_);
}

}  // namespace google_apis
