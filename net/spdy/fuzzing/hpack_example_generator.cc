// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/files/file.h"
#include "base/strings/string_number_conversions.h"
#include "net/spdy/fuzzing/hpack_fuzz_util.h"
#include "net/spdy/hpack_constants.h"
#include "net/spdy/hpack_encoder.h"

namespace {

// Target file for generated HPACK header sets.
const char kFileToWrite[] = "file-to-write";

// Number of header sets to generate.
const char kExampleCount[] = "example-count";

}  // namespace

using net::HpackFuzzUtil;
using std::map;
using std::string;

// Generates a configurable number of header sets (using HpackFuzzUtil), and
// sequentially encodes each header set with an HpackEncoder. Encoded header
// sets are written to the output file in length-prefixed blocks.
int main(int argc, char** argv) {
  base::AtExitManager exit_manager;

  CommandLine::Init(argc, argv);
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();

  if (!command_line.HasSwitch(kFileToWrite) ||
      !command_line.HasSwitch(kExampleCount)) {
    LOG(ERROR) << "Usage: " << argv[0]
               << " --" << kFileToWrite << "=/path/to/file.out"
               << " --" << kExampleCount << "=1000";
    return -1;
  }
  string file_to_write = command_line.GetSwitchValueASCII(kFileToWrite);

  int example_count = 0;
  base::StringToInt(command_line.GetSwitchValueASCII(kExampleCount),
                    &example_count);

  DVLOG(1) << "Writing output to " << file_to_write;
  base::File file_out(base::FilePath(file_to_write),
                      base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  CHECK(file_out.IsValid()) << file_out.error_details();

  HpackFuzzUtil::GeneratorContext context;
  HpackFuzzUtil::InitializeGeneratorContext(&context);
  net::HpackEncoder encoder(net::ObtainHpackHuffmanTable());

  for (int i = 0; i != example_count; ++i) {
    map<string, string> headers =
        HpackFuzzUtil::NextGeneratedHeaderSet(&context);

    string buffer;
    CHECK(encoder.EncodeHeaderSet(headers, &buffer));

    string prefix = HpackFuzzUtil::HeaderBlockPrefix(buffer.size());

    CHECK_LT(0, file_out.WriteAtCurrentPos(prefix.data(), prefix.size()));
    CHECK_LT(0, file_out.WriteAtCurrentPos(buffer.data(), buffer.size()));
  }
  CHECK(file_out.Flush());
  DVLOG(1) << "Generated " << example_count << " blocks.";
  return 0;
}
