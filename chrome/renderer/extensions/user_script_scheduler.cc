// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/user_script_scheduler.h"

#include "base/bind.h"
#include "base/message_loop.h"
#include "chrome/common/extensions/extension_error_utils.h"
#include "chrome/common/extensions/extension_manifest_constants.h"
#include "chrome/common/extensions/extension_messages.h"
#include "chrome/renderer/extensions/extension_dispatcher.h"
#include "chrome/renderer/extensions/extension_groups.h"
#include "chrome/renderer/extensions/extension_helper.h"
#include "chrome/renderer/extensions/user_script_slave.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebString.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"

namespace {
// The length of time to wait after the DOM is complete to try and run user
// scripts.
const int kUserScriptIdleTimeoutMs = 200;
}

using WebKit::WebDocument;
using WebKit::WebFrame;
using WebKit::WebString;
using WebKit::WebView;
using extensions::Extension;

UserScriptScheduler::UserScriptScheduler(
    WebFrame* frame, ExtensionDispatcher* extension_dispatcher)
    : ALLOW_THIS_IN_INITIALIZER_LIST(weak_factory_(this)),
      frame_(frame),
      current_location_(UserScript::UNDEFINED),
      has_run_idle_(false),
      extension_dispatcher_(extension_dispatcher) {
  for (int i = UserScript::UNDEFINED; i < UserScript::RUN_LOCATION_LAST; ++i) {
    pending_execution_map_[static_cast<UserScript::RunLocation>(i)] =
      std::queue<linked_ptr<ExtensionMsg_ExecuteCode_Params> >();
  }
}

UserScriptScheduler::~UserScriptScheduler() {
}

void UserScriptScheduler::ExecuteCode(
    const ExtensionMsg_ExecuteCode_Params& params) {
  UserScript::RunLocation run_at =
    static_cast<UserScript::RunLocation>(params.run_at);
  if (current_location_ < run_at) {
    pending_execution_map_[run_at].push(
        linked_ptr<ExtensionMsg_ExecuteCode_Params>(
            new ExtensionMsg_ExecuteCode_Params(params)));
    return;
  }

  ExecuteCodeImpl(params);
}

void UserScriptScheduler::DidCreateDocumentElement() {
  current_location_ = UserScript::DOCUMENT_START;
  MaybeRun();
}

void UserScriptScheduler::DidFinishDocumentLoad() {
  current_location_ = UserScript::DOCUMENT_END;
  MaybeRun();
  // Schedule a run for DOCUMENT_IDLE
  MessageLoop::current()->PostDelayedTask(
      FROM_HERE, base::Bind(&UserScriptScheduler::IdleTimeout,
                            weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(kUserScriptIdleTimeoutMs));
}

void UserScriptScheduler::DidFinishLoad() {
  current_location_ = UserScript::DOCUMENT_IDLE;
  // Ensure that running scripts does not keep any progress UI running.
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&UserScriptScheduler::MaybeRun,
                            weak_factory_.GetWeakPtr()));
}

void UserScriptScheduler::DidStartProvisionalLoad() {
  // The frame is navigating, so reset the state since we'll want to inject
  // scripts once the load finishes.
  current_location_ = UserScript::UNDEFINED;
  has_run_idle_ = false;
  weak_factory_.InvalidateWeakPtrs();
  std::map<UserScript::RunLocation, ExecutionQueue>::iterator itr =
    pending_execution_map_.begin();
  for (itr = pending_execution_map_.begin();
       itr != pending_execution_map_.end(); ++itr) {
    while (!itr->second.empty())
      itr->second.pop();
  }
}

void UserScriptScheduler::IdleTimeout() {
  current_location_ = UserScript::DOCUMENT_IDLE;
  MaybeRun();
}

void UserScriptScheduler::MaybeRun() {
  if (current_location_ == UserScript::UNDEFINED)
    return;

  if (!has_run_idle_ && current_location_ == UserScript::DOCUMENT_IDLE) {
    has_run_idle_ = true;
    extension_dispatcher_->user_script_slave()->InjectScripts(
        frame_, UserScript::DOCUMENT_IDLE);
  }

  // Run all tasks from the current time and earlier.
  for (int i = UserScript::DOCUMENT_START;
       i <= current_location_; ++i) {
    UserScript::RunLocation run_time = static_cast<UserScript::RunLocation>(i);
    while (!pending_execution_map_[run_time].empty()) {
      linked_ptr<ExtensionMsg_ExecuteCode_Params>& params =
        pending_execution_map_[run_time].front();
      ExecuteCodeImpl(*params);
      pending_execution_map_[run_time].pop();
    }
  }
}

void UserScriptScheduler::ExecuteCodeImpl(
    const ExtensionMsg_ExecuteCode_Params& params) {
  const Extension* extension = extension_dispatcher_->extensions()->GetByID(
      params.extension_id);
  content::RenderView* render_view =
      content::RenderView::FromWebView(frame_->view());

  // Since extension info is sent separately from user script info, they can
  // be out of sync. We just ignore this situation.
  if (!extension) {
    render_view->Send(new ExtensionHostMsg_ExecuteCodeFinished(
        render_view->GetRoutingID(), params.request_id, true, ""));
    return;
  }

  std::vector<WebFrame*> frame_vector;
  frame_vector.push_back(frame_);
  if (params.all_frames)
    GetAllChildFrames(frame_, &frame_vector);

  for (std::vector<WebFrame*>::iterator frame_it = frame_vector.begin();
       frame_it != frame_vector.end(); ++frame_it) {
    WebFrame* frame = *frame_it;
    if (params.is_javascript) {
      // We recheck access here in the renderer for extra safety against races
      // with navigation.
      //
      // But different frames can have different URLs, and the extension might
      // only have access to a subset of them. For the top frame, we can
      // immediately send an error and stop because the browser process
      // considers that an error too.
      //
      // For child frames, we just skip ones the extension doesn't have access
      // to and carry on.
      if (!extension->CanExecuteScriptOnPage(frame->document().url(),
                                             NULL, NULL)) {
        if (frame->parent()) {
          continue;
        } else {
          render_view->Send(new ExtensionHostMsg_ExecuteCodeFinished(
              render_view->GetRoutingID(), params.request_id, false,
              ExtensionErrorUtils::FormatErrorMessage(
                  extension_manifest_errors::kCannotAccessPage,
                  frame->document().url().spec())));
          return;
        }
      }

      WebScriptSource source(WebString::fromUTF8(params.code));
      if (params.in_main_world) {
        frame->executeScript(source);
      } else {
        std::vector<WebScriptSource> sources;
        sources.push_back(source);
        frame->executeScriptInIsolatedWorld(
            extension_dispatcher_->user_script_slave()->
                GetIsolatedWorldIdForExtension(extension, frame),
            &sources.front(), sources.size(), EXTENSION_GROUP_CONTENT_SCRIPTS);
      }
    } else {
      frame->document().insertUserStyleSheet(
          WebString::fromUTF8(params.code),
          // Author level is consistent with WebView::addUserStyleSheet.
          WebDocument::UserStyleAuthorLevel);
    }
  }

  render_view->Send(new ExtensionHostMsg_ExecuteCodeFinished(
      render_view->GetRoutingID(), params.request_id, true, ""));
}

bool UserScriptScheduler::GetAllChildFrames(
    WebFrame* parent_frame,
    std::vector<WebFrame*>* frames_vector) const {
  if (!parent_frame)
    return false;

  for (WebFrame* child_frame = parent_frame->firstChild(); child_frame;
       child_frame = child_frame->nextSibling()) {
    frames_vector->push_back(child_frame);
    GetAllChildFrames(child_frame, frames_vector);
  }
  return true;
}
