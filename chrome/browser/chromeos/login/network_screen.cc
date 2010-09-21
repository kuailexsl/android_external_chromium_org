// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/network_screen.h"

#include "app/l10n_util.h"
#include "app/resource_bundle.h"
#include "base/logging.h"
#include "base/string16.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "chrome/browser/chromeos/login/help_app_launcher.h"
#include "chrome/browser/chromeos/login/network_selection_view.h"
#include "chrome/browser/chromeos/login/screen_observer.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "views/widget/widget.h"
#include "views/window/window.h"


namespace {

// Time in seconds for connection timeout.
const int kConnectionTimeoutSec = 15;

// Considering 10px shadow from each side & welcome title height at 30px.
const int kWelcomeScreenWidth = 580;
const int kWelcomeScreenHeight = 335;

}  // namespace

namespace chromeos {

///////////////////////////////////////////////////////////////////////////////
// NetworkScreen, public:

NetworkScreen::NetworkScreen(WizardScreenDelegate* delegate)
    : ViewScreen<NetworkSelectionView>(delegate,
                                       kWelcomeScreenWidth,
                                       kWelcomeScreenHeight),
      is_network_subscribed_(false),
      continue_pressed_(false),
      bubble_(NULL) {
}

NetworkScreen::~NetworkScreen() {
  connection_timer_.Stop();
  UnsubscribeNetworkNotification();
}

////////////////////////////////////////////////////////////////////////////////
// NetworkScreen, NetworkScreenDelegate implementation:

void NetworkScreen::ClearErrors() {
  // bubble_ will be set to NULL in callback.
  if (bubble_)
    bubble_->Close();
}

///////////////////////////////////////////////////////////////////////////////
// views::ButtonListener implementation:

void NetworkScreen::ButtonPressed(views::Button* sender,
                                  const views::Event& event) {
  ClearErrors();
  NetworkLibrary* network = CrosLibrary::Get()->GetNetworkLibrary();
  if (network && network->Connected()) {
    NotifyOnConnection();
  } else {
    continue_pressed_ = true;
    WaitForConnection(network_id_);
  }
}

////////////////////////////////////////////////////////////////////////////////
// NetworkLibrary::Observer implementation:

void NetworkScreen::NetworkChanged(NetworkLibrary* network_lib) {
  UpdateStatus(network_lib);
}

///////////////////////////////////////////////////////////////////////////////
// NetworkScreen, ViewScreen implementation:

void NetworkScreen::CreateView() {
  language_switch_menu_.InitLanguageMenu();
  ViewScreen<NetworkSelectionView>::CreateView();
}

NetworkSelectionView* NetworkScreen::AllocateView() {
  return new NetworkSelectionView(this);
}

///////////////////////////////////////////////////////////////////////////////
// NetworkScreen, views::InfoBubbleDelegate implementation:

void NetworkScreen::OnHelpLinkActivated() {
  if (!help_app_.get())
    help_app_.reset(new HelpAppLauncher(view()->GetNativeWindow()));
  help_app_->ShowHelpTopic(HelpAppLauncher::HELP_CONNECTIVITY);
}

////////////////////////////////////////////////////////////////////////////////
// NetworkScreen, public:

void NetworkScreen::Refresh() {
  if (CrosLibrary::Get()->EnsureLoaded()) {
    SubscribeNetworkNotification();
    NetworkChanged(chromeos::CrosLibrary::Get()->GetNetworkLibrary());
  }
}

////////////////////////////////////////////////////////////////////////////////
// NetworkScreen, private:

void NetworkScreen::SubscribeNetworkNotification() {
  if (!is_network_subscribed_) {
    is_network_subscribed_ = true;
    chromeos::CrosLibrary::Get()->GetNetworkLibrary()->AddObserver(this);
  }
}

void NetworkScreen::UnsubscribeNetworkNotification() {
  if (is_network_subscribed_) {
    is_network_subscribed_ = false;
    chromeos::CrosLibrary::Get()->GetNetworkLibrary()->RemoveObserver(this);
  }
}

void NetworkScreen::NotifyOnConnection() {
  // TODO(nkostylev): Check network connectivity.
  UnsubscribeNetworkNotification();
  connection_timer_.Stop();
  delegate()->GetObserver(this)->OnExit(ScreenObserver::NETWORK_CONNECTED);
}

void NetworkScreen::OnConnectionTimeout() {
  StopWaitingForConnection(network_id_);
  // Show error bubble.
  ClearErrors();
  views::View* network_control = view()->GetNetworkControlView();
  bubble_ = MessageBubble::Show(
      network_control->GetWidget(),
      network_control->GetScreenBounds(),
      BubbleBorder::LEFT_TOP,
      ResourceBundle::GetSharedInstance().GetBitmapNamed(IDR_WARNING),
      l10n_util::GetStringF(IDS_NETWORK_SELECTION_ERROR,
                            l10n_util::GetString(IDS_PRODUCT_OS_NAME),
                            UTF16ToWide(network_id_)),
      l10n_util::GetString(IDS_NETWORK_SELECTION_ERROR_HELP),
      this);
}

void NetworkScreen::UpdateStatus(NetworkLibrary* network) {
  if (!view() || !network)
    return;

  if (network->ethernet_connected()) {
    StopWaitingForConnection(
        l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_ETHERNET));
  } else if (network->wifi_connected()) {
    StopWaitingForConnection(ASCIIToUTF16(network->wifi_name()));
  } else if (network->cellular_connected()) {
    StopWaitingForConnection(ASCIIToUTF16(network->cellular_name()));
  } else if (network->ethernet_connecting()) {
    WaitForConnection(
        l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_ETHERNET));
  } else if (network->wifi_connecting()) {
    WaitForConnection(ASCIIToUTF16(network->wifi_name()));
  } else if (network->cellular_connecting()) {
    WaitForConnection(ASCIIToUTF16(network->cellular_name()));
  } else {
    view()->EnableContinue(network->Connected());
  }
}

void NetworkScreen::StopWaitingForConnection(const string16& network_id) {
  NetworkLibrary* network = CrosLibrary::Get()->GetNetworkLibrary();
  bool is_connected = network && network->Connected();
  if (is_connected && continue_pressed_) {
    NotifyOnConnection();
    return;
  }

  continue_pressed_ = false;
  connection_timer_.Stop();

  network_id_ = network_id;
  view()->ShowConnectingStatus(false, network_id_);
  view()->EnableContinue(is_connected);
}

void NetworkScreen::WaitForConnection(const string16& network_id) {
  connection_timer_.Stop();
  connection_timer_.Start(base::TimeDelta::FromSeconds(kConnectionTimeoutSec),
                          this,
                          &NetworkScreen::OnConnectionTimeout);

  network_id_ = network_id;
  view()->ShowConnectingStatus(true, network_id_);

  view()->EnableContinue(false);
}

}  // namespace chromeos
