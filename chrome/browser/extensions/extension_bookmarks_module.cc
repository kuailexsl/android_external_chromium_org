// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_bookmarks_module.h"

#include "base/json/json_writer.h"
#include "base/string_util.h"
#include "chrome/browser/bookmarks/bookmark_codec.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/extensions/extension_bookmarks_module_constants.h"
#include "chrome/browser/extensions/extension_message_service.h"
#include "chrome/browser/profile.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"

namespace keys = extension_bookmarks_module_constants;

// Helper functions.
class ExtensionBookmarks {
 public:
  // Convert |node| into a JSON value.
  static DictionaryValue* GetNodeDictionary(const BookmarkNode* node,
                                            bool recurse) {
    DictionaryValue* dict = new DictionaryValue();
    dict->SetString(keys::kIdKey, Int64ToString(node->id()));

    const BookmarkNode* parent = node->GetParent();
    if (parent) {
      dict->SetString(keys::kParentIdKey, Int64ToString(parent->id()));
      dict->SetInteger(keys::kIndexKey, parent->IndexOfChild(node));
    }

    if (!node->is_folder()) {
      dict->SetString(keys::kUrlKey, node->GetURL().spec());
    } else {
      // Javascript Date wants milliseconds since the epoch, ToDoubleT is
      // seconds.
      base::Time t = node->date_group_modified();
      if (!t.is_null())
        dict->SetReal(keys::kDateGroupModifiedKey, floor(t.ToDoubleT() * 1000));
    }

    dict->SetString(keys::kTitleKey, node->GetTitle());
    if (!node->date_added().is_null()) {
      // Javascript Date wants milliseconds since the epoch, ToDoubleT is
      // seconds.
      dict->SetReal(keys::kDateAddedKey,
                    floor(node->date_added().ToDoubleT() * 1000));
    }

    if (recurse && node->is_folder()) {
      int childCount = node->GetChildCount();
      ListValue* children = new ListValue();
      for (int i = 0; i < childCount; ++i) {
        const BookmarkNode* child = node->GetChild(i);
        DictionaryValue* dict = GetNodeDictionary(child, true);
        children->Append(dict);
      }
      dict->Set(keys::kChildrenKey, children);
    }
    return dict;
  }

  // Add a JSON representation of |node| to the JSON |list|.
  static void AddNode(const BookmarkNode* node, ListValue* list, bool recurse) {
    DictionaryValue* dict = GetNodeDictionary(node, recurse);
    list->Append(dict);
  }

  static bool RemoveNode(BookmarkModel* model, int64 id, bool recursive,
                         std::string* error) {
    const BookmarkNode* node = model->GetNodeByID(id);
    if (!node) {
      *error = keys::kNoNodeError;
      return false;
    }
    if (node == model->root_node() ||
        node == model->other_node() ||
        node == model->GetBookmarkBarNode()) {
      *error = keys::kModifySpecialError;
      return false;
    }
    if (node->is_folder() && node->GetChildCount() > 0 && !recursive) {
      *error = keys::kFolderNotEmptyError;
      return false;
    }

    const BookmarkNode* parent = node->GetParent();
    int index = parent->IndexOfChild(node);
    model->Remove(parent, index);
    return true;
  }

 private:
  ExtensionBookmarks();
};

void BookmarksFunction::Run() {
  // TODO(erikkay) temporary hack until adding an event listener can notify the
  // browser.
  BookmarkModel* model = profile()->GetBookmarkModel();
  if (!model->IsLoaded()) {
    // Bookmarks are not ready yet.  We'll wait.
    registrar_.Add(this, NotificationType::BOOKMARK_MODEL_LOADED,
                   NotificationService::AllSources());
    AddRef();  // Balanced in Observe().
    return;
  }

  ExtensionBookmarkEventRouter* event_router =
      ExtensionBookmarkEventRouter::GetSingleton();
  event_router->Observe(model);
  SendResponse(RunImpl());
}

bool BookmarksFunction::GetBookmarkIdAsInt64(
    const std::string& id_string, int64* id) {
  if (StringToInt64(id_string, id))
    return true;

  error_ = keys::kInvalidIdError;
  return false;
}

void BookmarksFunction::Observe(NotificationType type,
                                const NotificationSource& source,
                                const NotificationDetails& details) {
  DCHECK(type == NotificationType::BOOKMARK_MODEL_LOADED);
  DCHECK(profile()->GetBookmarkModel()->IsLoaded());
  Run();
  Release();  // Balanced in Run().
}

// static
ExtensionBookmarkEventRouter* ExtensionBookmarkEventRouter::GetSingleton() {
  return Singleton<ExtensionBookmarkEventRouter>::get();
}

ExtensionBookmarkEventRouter::ExtensionBookmarkEventRouter() {
}

ExtensionBookmarkEventRouter::~ExtensionBookmarkEventRouter() {
}

void ExtensionBookmarkEventRouter::Observe(BookmarkModel* model) {
  if (models_.find(model) == models_.end()) {
    model->AddObserver(this);
    models_.insert(model);
  }
}

void ExtensionBookmarkEventRouter::DispatchEvent(Profile *profile,
                                                 const char* event_name,
                                                 const std::string json_args) {
  if (profile->GetExtensionMessageService()) {
    profile->GetExtensionMessageService()->
        DispatchEventToRenderers(event_name, json_args);
  }
}

void ExtensionBookmarkEventRouter::Loaded(BookmarkModel* model) {
  // TODO(erikkay): Perhaps we should send this event down to the extension
  // so they know when it's safe to use the API?
}

void ExtensionBookmarkEventRouter::BookmarkNodeMoved(
    BookmarkModel* model,
    const BookmarkNode* old_parent,
    int old_index,
    const BookmarkNode* new_parent,
    int new_index) {
  ListValue args;
  const BookmarkNode* node = new_parent->GetChild(new_index);
  args.Append(new StringValue(Int64ToString(node->id())));
  DictionaryValue* object_args = new DictionaryValue();
  object_args->SetString(keys::kParentIdKey, Int64ToString(new_parent->id()));
  object_args->SetInteger(keys::kIndexKey, new_index);
  object_args->SetString(keys::kOldParentIdKey,
                         Int64ToString(old_parent->id()));
  object_args->SetInteger(keys::kOldIndexKey, old_index);
  args.Append(object_args);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), keys::kOnBookmarkMoved, json_args);
}

void ExtensionBookmarkEventRouter::BookmarkNodeAdded(BookmarkModel* model,
                                                     const BookmarkNode* parent,
                                                     int index) {
  ListValue args;
  const BookmarkNode* node = parent->GetChild(index);
  args.Append(new StringValue(Int64ToString(node->id())));
  DictionaryValue* obj = ExtensionBookmarks::GetNodeDictionary(node, false);
  args.Append(obj);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), keys::kOnBookmarkCreated, json_args);
}

void ExtensionBookmarkEventRouter::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int index,
    const BookmarkNode* node) {
  ListValue args;
  args.Append(new StringValue(Int64ToString(node->id())));
  DictionaryValue* object_args = new DictionaryValue();
  object_args->SetString(keys::kParentIdKey, Int64ToString(parent->id()));
  object_args->SetInteger(keys::kIndexKey, index);
  args.Append(object_args);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), keys::kOnBookmarkRemoved, json_args);
}

void ExtensionBookmarkEventRouter::BookmarkNodeChanged(
    BookmarkModel* model, const BookmarkNode* node) {
  ListValue args;
  args.Append(new StringValue(Int64ToString(node->id())));

  // TODO(erikkay) The only two things that BookmarkModel sends this
  // notification for are title and favicon.  Since we're currently ignoring
  // favicon and since the notification doesn't say which one anyway, for now
  // we only include title.  The ideal thing would be to change BookmarkModel
  // to indicate what changed.
  DictionaryValue* object_args = new DictionaryValue();
  object_args->SetString(keys::kTitleKey, node->GetTitle());
  args.Append(object_args);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(), keys::kOnBookmarkChanged, json_args);
}

void ExtensionBookmarkEventRouter::BookmarkNodeFavIconLoaded(
    BookmarkModel* model, const BookmarkNode* node) {
  // TODO(erikkay) anything we should do here?
}

void ExtensionBookmarkEventRouter::BookmarkNodeChildrenReordered(
    BookmarkModel* model, const BookmarkNode* node) {
  ListValue args;
  args.Append(new StringValue(Int64ToString(node->id())));
  int childCount = node->GetChildCount();
  ListValue* children = new ListValue();
  for (int i = 0; i < childCount; ++i) {
    const BookmarkNode* child = node->GetChild(i);
    Value* child_id = new StringValue(Int64ToString(child->id()));
    children->Append(child_id);
  }
  DictionaryValue* reorder_info = new DictionaryValue();
  reorder_info->Set(keys::kChildIdsKey, children);
  args.Append(reorder_info);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(model->profile(),
                keys::kOnBookmarkChildrenReordered,
                json_args);
}

bool GetBookmarksFunction::RunImpl() {
  BookmarkModel* model = profile()->GetBookmarkModel();
  scoped_ptr<ListValue> json(new ListValue());
  if (args_->IsType(Value::TYPE_LIST)) {
    ListValue* ids = static_cast<ListValue*>(args_);
    size_t count = ids->GetSize();
    EXTENSION_FUNCTION_VALIDATE(count > 0);
    for (size_t i = 0; i < count; ++i) {
      int64 id;
      std::string id_string;
      EXTENSION_FUNCTION_VALIDATE(ids->GetString(i, &id_string));
      if (!GetBookmarkIdAsInt64(id_string, &id))
        return false;
      const BookmarkNode* node = model->GetNodeByID(id);
      if (!node) {
        error_ = keys::kNoNodeError;
        return false;
      } else {
        ExtensionBookmarks::AddNode(node, json.get(), false);
      }
    }
  } else {
    int64 id;
    std::string id_string;
    EXTENSION_FUNCTION_VALIDATE(args_->GetAsString(&id_string));
    if (!GetBookmarkIdAsInt64(id_string, &id))
      return false;
    const BookmarkNode* node = model->GetNodeByID(id);
    if (!node) {
      error_ = keys::kNoNodeError;
      return false;
    }
    ExtensionBookmarks::AddNode(node, json.get(), false);
  }

  result_.reset(json.release());
  return true;
}

bool GetBookmarkChildrenFunction::RunImpl() {
  BookmarkModel* model = profile()->GetBookmarkModel();
  int64 id;
  std::string id_string;
  EXTENSION_FUNCTION_VALIDATE(args_->GetAsString(&id_string));
  if (!GetBookmarkIdAsInt64(id_string, &id))
    return false;
  scoped_ptr<ListValue> json(new ListValue());
  const BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    error_ = keys::kNoNodeError;
    return false;
  }
  int child_count = node->GetChildCount();
  for (int i = 0; i < child_count; ++i) {
    const BookmarkNode* child = node->GetChild(i);
    ExtensionBookmarks::AddNode(child, json.get(), false);
  }

  result_.reset(json.release());
  return true;
}

bool GetBookmarkTreeFunction::RunImpl() {
  BookmarkModel* model = profile()->GetBookmarkModel();
  scoped_ptr<ListValue> json(new ListValue());
  const BookmarkNode* node = model->root_node();
  ExtensionBookmarks::AddNode(node, json.get(), true);
  result_.reset(json.release());
  return true;
}

bool SearchBookmarksFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_STRING));

  std::wstring query;
  EXTENSION_FUNCTION_VALIDATE(args_->GetAsString(&query));

  BookmarkModel* model = profile()->GetBookmarkModel();
  ListValue* json = new ListValue();
  std::wstring lang = profile()->GetPrefs()->GetString(prefs::kAcceptLanguages);
  std::vector<const BookmarkNode*> nodes;
  bookmark_utils::GetBookmarksContainingText(model, query, 50, lang, &nodes);
  std::vector<const BookmarkNode*>::iterator i = nodes.begin();
  for (; i != nodes.end(); ++i) {
    const BookmarkNode* node = *i;
    ExtensionBookmarks::AddNode(node, json, false);
  }

  result_.reset(json);
  return true;
}

bool RemoveBookmarkFunction::RunImpl() {
  bool recursive = false;
  if (name() == RemoveTreeBookmarkFunction::function_name())
    recursive = true;

  BookmarkModel* model = profile()->GetBookmarkModel();
  int64 id;
  std::string id_string;
  if (args_->IsType(Value::TYPE_STRING) &&
      args_->GetAsString(&id_string) &&
      StringToInt64(id_string, &id)) {
    return ExtensionBookmarks::RemoveNode(model, id, recursive, &error_);
  } else {
    EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_LIST));
    ListValue* ids = static_cast<ListValue*>(args_);
    size_t count = ids->GetSize();
    EXTENSION_FUNCTION_VALIDATE(count > 0);
    for (size_t i = 0; i < count; ++i) {
      EXTENSION_FUNCTION_VALIDATE(ids->GetString(i, &id_string));
      if (!GetBookmarkIdAsInt64(id_string, &id))
        return false;
      if (!ExtensionBookmarks::RemoveNode(model, id, recursive, &error_))
        return false;
    }
    return true;
  }
}

bool CreateBookmarkFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_DICTIONARY));
  DictionaryValue* json = static_cast<DictionaryValue*>(args_);

  BookmarkModel* model = profile()->GetBookmarkModel();
  int64 parentId;
  if (!json->HasKey(keys::kParentIdKey)) {
    // Optional, default to "other bookmarks".
    parentId = model->other_node()->id();
  } else {
    std::string parentId_string;
    EXTENSION_FUNCTION_VALIDATE(json->GetString(keys::kParentIdKey,
                                                &parentId_string));
    if (!GetBookmarkIdAsInt64(parentId_string, &parentId))
      return false;
  }
  const BookmarkNode* parent = model->GetNodeByID(parentId);
  if (!parent) {
    error_ = keys::kNoParentError;
    return false;
  }
  if (parent->GetParent() == NULL) {  // Can't create children of the root.
    error_ = keys::kNoParentError;
    return false;
  }

  int index;
  if (!json->HasKey(keys::kIndexKey)) {  // Optional (defaults to end).
    index = parent->GetChildCount();
  } else {
    EXTENSION_FUNCTION_VALIDATE(json->GetInteger(keys::kIndexKey, &index));
    if (index > parent->GetChildCount() || index < 0) {
      error_ = keys::kInvalidIndexError;
      return false;
    }
  }

  std::wstring title;
  json->GetString(keys::kTitleKey, &title);  // Optional.
  std::string url_string;
  json->GetString(keys::kUrlKey, &url_string);  // Optional.
  GURL url(url_string);
  if (!url.is_empty() && !url.is_valid()) {
    error_ = keys::kInvalidUrlError;
    return false;
  }

  const BookmarkNode* node;
  if (url_string.length())
    node = model->AddURL(parent, index, title, url);
  else
    node = model->AddGroup(parent, index, title);
  DCHECK(node);
  if (!node) {
    error_ = keys::kNoNodeError;
    return false;
  }

  DictionaryValue* ret = ExtensionBookmarks::GetNodeDictionary(node, false);
  result_.reset(ret);

  return true;
}

bool MoveBookmarkFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_LIST));
  const ListValue* args = static_cast<const ListValue*>(args_);
  int64 id;
  std::string id_string;
  EXTENSION_FUNCTION_VALIDATE(args->GetString(0, &id_string));
  if (!GetBookmarkIdAsInt64(id_string, &id))
    return false;
  DictionaryValue* destination;
  EXTENSION_FUNCTION_VALIDATE(args->GetDictionary(1, &destination));

  BookmarkModel* model = profile()->GetBookmarkModel();
  const BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    error_ = keys::kNoNodeError;
    return false;
  }
  if (node == model->root_node() ||
      node == model->other_node() ||
      node == model->GetBookmarkBarNode()) {
    error_ = keys::kModifySpecialError;
    return false;
  }

  const BookmarkNode* parent;
  if (!destination->HasKey(keys::kParentIdKey)) {
    // Optional, defaults to current parent.
    parent = node->GetParent();
  } else {
    std::string parentId_string;
    EXTENSION_FUNCTION_VALIDATE(destination->GetString(keys::kParentIdKey,
        &parentId_string));
    int64 parentId;
    if (!GetBookmarkIdAsInt64(parentId_string, &parentId))
      return false;

    parent = model->GetNodeByID(parentId);
  }
  if (!parent) {
    error_ = keys::kNoParentError;
    // TODO(erikkay) return an error message.
    return false;
  }
  if (parent == model->root_node()) {
    error_ = keys::kModifySpecialError;
    return false;
  }

  int index;
  if (destination->HasKey(keys::kIndexKey)) {  // Optional (defaults to end).
    EXTENSION_FUNCTION_VALIDATE(destination->GetInteger(keys::kIndexKey,
                                                        &index));
    if (index > parent->GetChildCount() || index < 0) {
      error_ = keys::kInvalidIndexError;
      return false;
    }
  } else {
    index = parent->GetChildCount();
  }

  model->Move(node, parent, index);

  DictionaryValue* ret = ExtensionBookmarks::GetNodeDictionary(node, false);
  result_.reset(ret);

  return true;
}

bool UpdateBookmarkFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_LIST));
  const ListValue* args = static_cast<const ListValue*>(args_);
  int64 id;
  std::string id_string;
  EXTENSION_FUNCTION_VALIDATE(args->GetString(0, &id_string));
  if (!GetBookmarkIdAsInt64(id_string, &id))
    return false;
  DictionaryValue* updates;
  EXTENSION_FUNCTION_VALIDATE(args->GetDictionary(1, &updates));
  std::wstring title;
  updates->GetString(keys::kTitleKey, &title);  // Optional (empty is clear).

  BookmarkModel* model = profile()->GetBookmarkModel();
  const BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    error_ = keys::kNoNodeError;
    return false;
  }
  if (node == model->root_node() ||
      node == model->other_node() ||
      node == model->GetBookmarkBarNode()) {
    error_ = keys::kModifySpecialError;
    return false;
  }
  model->SetTitle(node, title);

  DictionaryValue* ret = ExtensionBookmarks::GetNodeDictionary(node, false);
  result_.reset(ret);

  return true;
}
