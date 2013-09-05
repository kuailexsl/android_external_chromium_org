// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_MANIFEST_HIGHLIGHTER_H_
#define EXTENSIONS_BROWSER_MANIFEST_HIGHLIGHTER_H_

#include <string>

#include "base/basictypes.h"

namespace extensions {

// Use the ManifestHighlighter class to find the bounds of a feature in the
// manifest. The manifest is parsed for the feature upon construction of the
// object.
// A feature can be at any level in the hierarchy. The "start" of a feature is
// the first character of the feature name, or the beginning quote of the name,
// if present. The "end" of a feature is wherever the next item at the same
// level starts.
// For instance, the bounds for the 'permissions' feature at the top level could
// be '"permissions": { "tabs", "history", "downloads" }', but the feature for
// 'tabs' within 'permissions' would just be '"tabs"'.
// We can't use the JSONParser to do this, because we want to display the actual
// manifest, and once we parse it into Values, we lose any formatting the user
// may have had.
// If a feature cannot be found, the feature will have zero-length.
class ManifestHighlighter {
 public:
  ManifestHighlighter(const std::string& manifest,
                      const std::string& key,
                      const std::string& specific /* optional */);
  ~ManifestHighlighter();

  // Get the portion of the manifest which should not be highlighted and is
  // before the feature.
  std::string GetBeforeFeature() const;

  // Get the feature portion of the manifest, which should be highlighted.
  std::string GetFeature() const;

  // Get the portion of the manifest which should not be highlighted and is
  // after the feature.
  std::string GetAfterFeature() const;

 private:
  // Called from the constructor; determine the start and end bounds of a
  // feature, using both the key and specific information.
  void Parse(const std::string& key, const std::string& specific);

  // Find the bounds of any feature, either a full key or a specific item within
  // the key. |enforce_at_top_level| means that the feature we find must be at
  // the same level as |start_| (i.e., ignore nested elements).
  // Returns true on success.
  bool FindBounds(const std::string& feature, bool enforce_at_top_level);

  // Finds the end of the feature.
  void FindBoundsEnd(const std::string& feature, size_t local_start);

  // The manifest we are parsing.
  std::string manifest_;

  // The start of the feature.
  size_t start_;

  // The end of the feature.
  size_t end_;

  DISALLOW_COPY_AND_ASSIGN(ManifestHighlighter);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_MANIFEST_HIGHLIGHTER_H_
