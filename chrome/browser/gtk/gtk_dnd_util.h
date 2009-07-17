// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_GTK_DND_UTIL_H_
#define CHROME_BROWSER_GTK_GTK_DND_UTIL_H_

#include <gtk/gtk.h>

class GtkDndUtil {
 public:
  // Registry of all internal int codes for drag and drop.
  enum {
    // Intra-application types.
    CHROME_TAB = 1 << 0,
    CHROME_BOOKMARK_ITEM = 1 << 1,
    CHROME_WEBDROP_FILE_CONTENTS = 1 << 2,
    CHROME_NAMED_URL = 1 << 3,

    // Standard types.
    TEXT_PLAIN = 1 << 4,
    TEXT_URI_LIST = 1 << 5,
    TEXT_HTML = 1 << 6,
  };

  // Get the atom for a given target (of the above enum type). Will return NULL
  // for non-custom targets, such as CHROME_TEXT_PLAIN.
  static GdkAtom GetAtomForTarget(int target);

  // Creates a target list from the given mask. The mask should be an OR of
  // CHROME_* values. The target list is returned with ref count 1; the caller
  // is responsible for unreffing it when it is no longer needed.
  // Since the MIME type for WEBDROP_FILE_CONTENTS depends on the file's
  // contents, that flag is ignored by this function. It is the responsibility
  // of the client code to do the right thing.
  static GtkTargetList* GetTargetListFromCodeMask(int code_mask);

  // Set the drag target list for |dest| or |source| with the target list that
  // corresponds to |code_mask|.
  static void SetDestTargetListFromCodeMask(GtkWidget* dest, int code_mask);
  static void SetSourceTargetListFromCodeMask(GtkWidget* source, int code_mask);

 private:
  GtkDndUtil();
};

#endif  // CHROME_BROWSER_GTK_GTK_DND_UTIL_H_
