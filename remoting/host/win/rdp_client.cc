// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/win/rdp_client.h"

#include <windows.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/win/registry.h"
#include "net/base/ip_endpoint.h"
#include "remoting/base/typed_buffer.h"
#include "remoting/host/win/rdp_client_window.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"

namespace remoting {

namespace {

// 127.0.0.1 is explicitly blocked by the RDP ActiveX control, so we use
// 127.0.0.2 instead.
const unsigned char kRdpLoopbackAddress[] = { 127, 0, 0, 2 };

const int kDefaultRdpPort = 3389;

// The port number used by RDP is stored in the registry.
const wchar_t kRdpPortKeyName[] = L"SYSTEM\\CurrentControlSet\\Control\\"
    L"Terminal Server\\WinStations\\RDP-Tcp";
const wchar_t kRdpPortValueName[] = L"PortNumber";

}  // namespace

// The core of RdpClient is ref-counted since it services calls and notifies
// events on the caller task runner, but runs the ActiveX control on the UI
// task runner.
class RdpClient::Core
    : public base::RefCountedThreadSafe<Core>,
      public RdpClientWindow::EventHandler {
 public:
  Core(
      scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      RdpClient::EventHandler* event_handler);

  // Initiates a loopback RDP connection.
  void Connect(const webrtc::DesktopSize& screen_size,
               const std::string& terminal_id);

  // Initiates a graceful shutdown of the RDP connection.
  void Disconnect();

  // Sends Secure Attention Sequence to the session.
  void InjectSas();

  // RdpClientWindow::EventHandler interface.
  virtual void OnConnected() OVERRIDE;
  virtual void OnDisconnected() OVERRIDE;

 private:
  friend class base::RefCountedThreadSafe<Core>;
  virtual ~Core();

  // Helpers for the event handler's methods that make sure that OnRdpClosed()
  // is the last notification delivered and is delevered only once.
  void NotifyConnected();
  void NotifyClosed();

  // Task runner on which the caller expects |event_handler_| to be notified.
  scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner_;

  // Task runner on which |rdp_client_window_| is running.
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  // Event handler receiving notification about connection state. The pointer is
  // cleared when Disconnect() methods is called, stopping any further updates.
  RdpClient::EventHandler* event_handler_;

  // Hosts the RDP ActiveX control.
  scoped_ptr<RdpClientWindow> rdp_client_window_;

  // A self-reference to keep the object alive during connection shutdown.
  scoped_refptr<Core> self_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

RdpClient::RdpClient(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    const webrtc::DesktopSize& screen_size,
    const std::string& terminal_id,
    EventHandler* event_handler) {
  DCHECK(caller_task_runner->BelongsToCurrentThread());

  core_ = new Core(caller_task_runner, ui_task_runner, event_handler);
  core_->Connect(screen_size, terminal_id);
}

RdpClient::~RdpClient() {
  DCHECK(CalledOnValidThread());

  core_->Disconnect();
}

void RdpClient::InjectSas() {
  DCHECK(CalledOnValidThread());

  core_->InjectSas();
}

RdpClient::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    RdpClient::EventHandler* event_handler)
    : caller_task_runner_(caller_task_runner),
      ui_task_runner_(ui_task_runner),
      event_handler_(event_handler) {
}

void RdpClient::Core::Connect(const webrtc::DesktopSize& screen_size,
                              const std::string& terminal_id) {
  if (!ui_task_runner_->BelongsToCurrentThread()) {
    ui_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::Connect, this, screen_size, terminal_id));
    return;
  }

  DCHECK_EQ(base::MessageLoop::current()->type(), base::MessageLoop::TYPE_UI);
  DCHECK(!rdp_client_window_);
  DCHECK(!self_);

  // Read the port number used by RDP.
  DWORD server_port;
  base::win::RegKey key(HKEY_LOCAL_MACHINE, kRdpPortKeyName, KEY_READ);
  if (!key.Valid() ||
      (key.ReadValueDW(kRdpPortValueName, &server_port) != ERROR_SUCCESS)) {
    server_port = kDefaultRdpPort;
  }

  net::IPAddressNumber server_address(
      kRdpLoopbackAddress,
      kRdpLoopbackAddress + arraysize(kRdpLoopbackAddress));
  net::IPEndPoint server_endpoint(server_address, server_port);

  // Create the ActiveX control window.
  rdp_client_window_.reset(new RdpClientWindow(server_endpoint, terminal_id,
                                               this));
  if (!rdp_client_window_->Connect(screen_size)) {
    rdp_client_window_.reset();

    // Notify the caller that connection attempt failed.
    NotifyClosed();
  }
}

void RdpClient::Core::Disconnect() {
  if (!ui_task_runner_->BelongsToCurrentThread()) {
    ui_task_runner_->PostTask(FROM_HERE, base::Bind(&Core::Disconnect, this));
    return;
  }

  // The caller does not expect any notifications to be delivered after this
  // point.
  event_handler_ = NULL;

  // Gracefully shutdown the RDP connection.
  if (rdp_client_window_) {
    self_ = this;
    rdp_client_window_->Disconnect();
  }
}

void RdpClient::Core::InjectSas() {
  if (!ui_task_runner_->BelongsToCurrentThread()) {
    ui_task_runner_->PostTask(FROM_HERE, base::Bind(&Core::InjectSas, this));
    return;
  }

  if (rdp_client_window_)
    rdp_client_window_->InjectSas();
}

void RdpClient::Core::OnConnected() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  DCHECK(rdp_client_window_);

  NotifyConnected();
}

void RdpClient::Core::OnDisconnected() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  DCHECK(rdp_client_window_);

  NotifyClosed();

  // Delay window destruction until no ActiveX control's code is on the stack.
  ui_task_runner_->DeleteSoon(FROM_HERE, rdp_client_window_.release());
  self_ = NULL;
}

RdpClient::Core::~Core() {
  DCHECK(!event_handler_);
  DCHECK(!rdp_client_window_);
}

void RdpClient::Core::NotifyConnected() {
  if (!caller_task_runner_->BelongsToCurrentThread()) {
    caller_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::NotifyConnected, this));
    return;
  }

  if (event_handler_)
    event_handler_->OnRdpConnected();
}

void RdpClient::Core::NotifyClosed() {
  if (!caller_task_runner_->BelongsToCurrentThread()) {
    caller_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::NotifyClosed, this));
    return;
  }

  if (event_handler_) {
    RdpClient::EventHandler* event_handler = event_handler_;
    event_handler_ = NULL;
    event_handler->OnRdpClosed();
  }
}

}  // namespace remoting
