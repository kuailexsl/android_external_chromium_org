// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/logging_native_handler.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace extensions {

LoggingNativeHandler::LoggingNativeHandler(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {
  RouteFunction("DCHECK",
      base::Bind(&LoggingNativeHandler::Dcheck, base::Unretained(this)));
  RouteFunction("CHECK",
      base::Bind(&LoggingNativeHandler::Check, base::Unretained(this)));
  RouteFunction("DCHECK_IS_ON",
      base::Bind(&LoggingNativeHandler::DcheckIsOn, base::Unretained(this)));
  RouteFunction("LOG",
      base::Bind(&LoggingNativeHandler::Log, base::Unretained(this)));
  RouteFunction("WARNING",
      base::Bind(&LoggingNativeHandler::Warning, base::Unretained(this)));
}

LoggingNativeHandler::~LoggingNativeHandler() {}

void LoggingNativeHandler::Check(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  bool check_value;
  std::string error_message;
  ParseArgs(args, &check_value, &error_message);
  CHECK(check_value) << error_message;
}

void LoggingNativeHandler::Dcheck(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  bool check_value;
  std::string error_message;
  ParseArgs(args, &check_value, &error_message);
  DCHECK(check_value) << error_message;
}

void LoggingNativeHandler::DcheckIsOn(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  args.GetReturnValue().Set(DCHECK_IS_ON);
}

void LoggingNativeHandler::Log(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(1, args.Length());
  LOG(INFO) << *v8::String::Utf8Value(args[0]);
}

void LoggingNativeHandler::Warning(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(1, args.Length());
  LOG(WARNING) << *v8::String::Utf8Value(args[0]);
}

void LoggingNativeHandler::ParseArgs(
    const v8::FunctionCallbackInfo<v8::Value>& args,
    bool* check_value,
    std::string* error_message) {
  CHECK_LE(args.Length(), 2);
  *check_value = args[0]->BooleanValue();
  if (args.Length() == 2) {
    *error_message = "Error: " + std::string(
        *v8::String::Utf8Value(args[1]));
  }

  v8::Handle<v8::StackTrace> stack_trace =
      v8::StackTrace::CurrentStackTrace(args.GetIsolate(), 10);
  if (stack_trace.IsEmpty() || stack_trace->GetFrameCount() <= 0) {
    *error_message += "\n    <no stack trace>";
  } else {
    for (size_t i = 0; i < (size_t) stack_trace->GetFrameCount(); ++i) {
      v8::Handle<v8::StackFrame> frame = stack_trace->GetFrame(i);
      CHECK(!frame.IsEmpty());
      *error_message += base::StringPrintf("\n    at %s (%s:%d:%d)",
          ToStringOrDefault(frame->GetFunctionName(), "<anonymous>").c_str(),
          ToStringOrDefault(frame->GetScriptName(), "<anonymous>").c_str(),
          frame->GetLineNumber(),
          frame->GetColumn());
    }
  }
}

std::string LoggingNativeHandler::ToStringOrDefault(
    const v8::Handle<v8::String>& v8_string,
    const std::string& dflt) {
  if (v8_string.IsEmpty())
    return dflt;
  std::string ascii_value = *v8::String::Utf8Value(v8_string);
  return ascii_value.empty() ? dflt : ascii_value;
}

}  // namespace extensions
