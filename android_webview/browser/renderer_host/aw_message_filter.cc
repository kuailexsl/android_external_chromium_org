/*
 *  Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *      * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "android_webview/browser/renderer_host/aw_message_filter.h"
#include "android_webview/common/render_view_messages.h"
#include "android_webview/common/aw_resource.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;


namespace android_webview {


AwMessageFilter::AwMessageFilter(int render_process_id)
    : BrowserMessageFilter(AndroidWebViewMsgStart){}
    AwMessageFilter::~AwMessageFilter() {
}

bool AwMessageFilter::OnMessageReceived(const IPC::Message& message) {

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AwMessageFilter, message)
    IPC_MESSAGE_HANDLER(AwViewHostMsg_GetBrowserResource, OnGetBrowserResource)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}


void AwMessageFilter::OnGetBrowserResource(std::string resource, std::string* result) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (resource.compare(ERROR_NETWORK) == 0)
    *result = AwResource::GetLoadErrorPageContent();
  else if (resource.compare(ERROR_NO_DOMAIN) == 0)
    *result = AwResource::GetNoDomainPageContent();
  else
    *result = std::string("Unknown error");
}

}
