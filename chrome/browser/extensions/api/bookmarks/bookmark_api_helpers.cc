// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/bookmarks/bookmark_api_helpers.h"

#include <math.h>  // For floor()
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/extensions/api/bookmarks/bookmark_api_constants.h"
#include "chrome/common/extensions/api/bookmarks.h"

namespace extensions {

namespace keys = bookmark_api_constants;
using api::bookmarks::BookmarkTreeNode;

namespace bookmark_api_helpers {

namespace {

void AddNodeHelper(const BookmarkNode* node,
                   std::vector<linked_ptr<BookmarkTreeNode> >* nodes,
                   bool recurse,
                   bool only_folders) {
  if (node->IsVisible()) {
    linked_ptr<BookmarkTreeNode> new_node(GetBookmarkTreeNode(node,
                                                              recurse,
                                                              only_folders));
    nodes->push_back(new_node);
  }
}

}  // namespace

BookmarkTreeNode* GetBookmarkTreeNode(const BookmarkNode* node,
                                      bool recurse,
                                      bool only_folders) {
  BookmarkTreeNode* bookmark_tree_node = new BookmarkTreeNode;

  bookmark_tree_node->id = base::Int64ToString(node->id());

  const BookmarkNode* parent = node->parent();
  if (parent) {
    bookmark_tree_node->parent_id.reset(new std::string(
        base::Int64ToString(parent->id())));
    bookmark_tree_node->index.reset(new int(parent->GetIndexOf(node)));
  }

  if (!node->is_folder()) {
    bookmark_tree_node->url.reset(new std::string(node->url().spec()));
  } else {
    // Javascript Date wants milliseconds since the epoch, ToDoubleT is seconds.
    base::Time t = node->date_folder_modified();
    if (!t.is_null()) {
      bookmark_tree_node->date_group_modified.reset(
          new double(floor(t.ToDoubleT() * 1000)));
    }
  }

  bookmark_tree_node->title = UTF16ToUTF8(node->GetTitle());
  if (!node->date_added().is_null()) {
    // Javascript Date wants milliseconds since the epoch, ToDoubleT is seconds.
    bookmark_tree_node->date_added.reset(
        new double(floor(node->date_added().ToDoubleT() * 1000)));
  }

  if (recurse && node->is_folder()) {
    std::vector<linked_ptr<BookmarkTreeNode> > children;
    for (int i = 0; i < node->child_count(); ++i) {
      const BookmarkNode* child = node->GetChild(i);
      if (child->IsVisible() && (!only_folders || child->is_folder())) {
        linked_ptr<BookmarkTreeNode> child_node(
            GetBookmarkTreeNode(child, true, only_folders));
        children.push_back(child_node);
      }
    }
    bookmark_tree_node->children.reset(
        new std::vector<linked_ptr<BookmarkTreeNode> >(children));
  }
  return bookmark_tree_node;
}

void AddNode(const BookmarkNode* node,
             std::vector<linked_ptr<BookmarkTreeNode> >* nodes,
             bool recurse) {
  return AddNodeHelper(node, nodes, recurse, false);
}

void AddNodeFoldersOnly(const BookmarkNode* node,
                        std::vector<linked_ptr<BookmarkTreeNode> >* nodes,
                        bool recurse) {
  return AddNodeHelper(node, nodes, recurse, true);
}

bool RemoveNode(BookmarkModel* model,
                int64 id,
                bool recursive,
                std::string* error) {
  const BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    *error = keys::kNoNodeError;
    return false;
  }
  if (model->is_permanent_node(node)) {
    *error = keys::kModifySpecialError;
    return false;
  }
  if (node->is_folder() && !node->empty() && !recursive) {
    *error = keys::kFolderNotEmptyError;
    return false;
  }

  const BookmarkNode* parent = node->parent();
  model->Remove(parent, parent->GetIndexOf(node));
  return true;
}

}  // namespace bookmark_api_helpers
}  // namespace extensions
