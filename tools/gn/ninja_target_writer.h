// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_NINJA_TARGET_WRITER_H_
#define TOOLS_GN_NINJA_TARGET_WRITER_H_

#include <iosfwd>

#include "base/basictypes.h"
#include "tools/gn/ninja_helper.h"
#include "tools/gn/path_output.h"

class Settings;
class Target;

// Generates one target's ".ninja" file. The toplevel "build.ninja" file is
// generated by the NinjaBuildWriter.
class NinjaTargetWriter {
 public:
  NinjaTargetWriter(const Target* target, std::ostream& out);
  virtual ~NinjaTargetWriter();

  static void RunAndWriteFile(const Target* target);

  virtual void Run() = 0;

 protected:
  void WriteEnvironment();

  const Settings* settings_;  // Non-owning.
  const Target* target_;  // Non-owning.
  std::ostream& out_;
  PathOutput path_output_;

  NinjaHelper helper_;

 private:
  void WriteCopyRules();

  DISALLOW_COPY_AND_ASSIGN(NinjaTargetWriter);
};

#endif  // TOOLS_GN_NINJA_TARGET_WRITER_H_
