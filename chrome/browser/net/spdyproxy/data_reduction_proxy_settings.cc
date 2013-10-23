// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/spdyproxy/data_reduction_proxy_settings.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/prefs/pref_member.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefs/proxy_prefs.h"
#include "chrome/browser/prefs/scoped_user_pref_update.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "net/base/host_port_pair.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

using base::FieldTrialList;
using base::StringPrintf;

namespace {

// Key of the UMA DataReductionProxy.StartupState histogram.
const char kUMAProxyStartupStateHistogram[] =
    "DataReductionProxy.StartupState";
// Values of the UMA DataReductionProxy.StartupState histogram.
enum ProxyStartupState {
  PROXY_NOT_AVAILABLE = 0,
  PROXY_DISABLED,
  PROXY_ENABLED,
  PROXY_STARTUP_STATE_COUNT,
};

const char kEnabled[] = "Enabled";

int64 GetInt64PrefValue(const ListValue& list_value, size_t index) {
  int64 val = 0;
  std::string pref_value;
  bool rv = list_value.GetString(index, &pref_value);
  DCHECK(rv);
  if (rv) {
    rv = base::StringToInt64(pref_value, &val);
    DCHECK(rv);
  }
  return val;
}

}  // namespace

DataReductionProxySettings::DataReductionProxySettings()
    : has_turned_on_(false),
      has_turned_off_(false),
      disabled_by_carrier_(false),
      enabled_by_user_(false) {
}

DataReductionProxySettings::~DataReductionProxySettings() {
  if (IsDataReductionProxyAllowed())
    spdy_proxy_auth_enabled_.Destroy();
}

void DataReductionProxySettings::InitPrefMembers() {
  spdy_proxy_auth_enabled_.Init(
      prefs::kSpdyProxyAuthEnabled,
      GetOriginalProfilePrefs(),
      base::Bind(&DataReductionProxySettings::OnProxyEnabledPrefChange,
                 base::Unretained(this)));
}

void DataReductionProxySettings::InitDataReductionProxySettings() {
  InitPrefMembers();

  // Disable the proxy if it is not allowed to be used.
  if (!IsDataReductionProxyAllowed())
    return;

  AddDefaultProxyBypassRules();
  net::NetworkChangeNotifier::AddIPAddressObserver(this);

  const CommandLine& command_line = *CommandLine::ForCurrentProcess();

  // Setting the kEnableSpdyProxyAuth switch has the same effect as enabling
  // the feature via settings, in that once set, the preference will be sticky
  // across instances of Chrome. Disabling the feature can only be done through
  // the settings menu.
  RecordDataReductionInit();
  if (spdy_proxy_auth_enabled_.GetValue() ||
      command_line.HasSwitch(switches::kEnableSpdyProxyAuth)) {
    MaybeActivateDataReductionProxy(true);
  } else {
    // This is logged so we can use this information in user feedback.
    LogProxyState(false /* enabled */, true /* at startup */);
  }
}

void DataReductionProxySettings::AddHostPatternToBypass(
    const std::string& pattern) {
  bypass_rules_.push_back(pattern);
}

void DataReductionProxySettings::AddURLPatternToBypass(
    const std::string& pattern) {
  size_t pos = pattern.find("/");
  if (pattern.find("/", pos + 1) == pos + 1)
    pos = pattern.find("/", pos + 2);

  std::string host_pattern;
  if (pos != std::string::npos)
    host_pattern = pattern.substr(0, pos);
  else
    host_pattern = pattern;

  AddHostPatternToBypass(host_pattern);
}

bool DataReductionProxySettings::IsDataReductionProxyAllowed() {
  return IsProxyOriginSetOnCommandLine() ||
      (FieldTrialList::FindFullName("DataCompressionProxyRollout") == kEnabled);
}

bool DataReductionProxySettings::IsDataReductionProxyPromoAllowed() {
  return IsProxyOriginSetOnCommandLine() ||
      (IsDataReductionProxyAllowed() &&
        FieldTrialList::FindFullName("DataCompressionProxyPromoVisibility") ==
            kEnabled);
}

std::string DataReductionProxySettings::GetDataReductionProxyOrigin() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kSpdyProxyAuthOrigin))
    return command_line.GetSwitchValueASCII(switches::kSpdyProxyAuthOrigin);
#if defined(SPDY_PROXY_AUTH_ORIGIN)
  return SPDY_PROXY_AUTH_ORIGIN;
#else
  return std::string();
#endif
}

std::string DataReductionProxySettings::GetDataReductionProxyAuth() {
  if (!IsDataReductionProxyAllowed())
    return std::string();
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kSpdyProxyAuthOrigin)) {
    // If an origin is provided via a switch, then only consider the value
    // that is provided by a switch. Do not use the preprocessor constant.
    // Don't expose SPDY_PROXY_AUTH_VALUE to a proxy passed in via the command
    // line.
    if (command_line.HasSwitch(switches::kSpdyProxyAuthValue))
      return command_line.GetSwitchValueASCII(switches::kSpdyProxyAuthValue);
    return std::string();
  }
#if defined(SPDY_PROXY_AUTH_VALUE)
  return SPDY_PROXY_AUTH_VALUE;
#else
  return std::string();
#endif
}

bool DataReductionProxySettings::IsDataReductionProxyEnabled() {
  return spdy_proxy_auth_enabled_.GetValue();
}

bool DataReductionProxySettings::IsDataReductionProxyManaged() {
  return spdy_proxy_auth_enabled_.IsManaged();
}

void DataReductionProxySettings::SetDataReductionProxyEnabled(bool enabled) {
  // Prevent configuring the proxy when it is not allowed to be used.
  if (!IsDataReductionProxyAllowed())
    return;

  spdy_proxy_auth_enabled_.SetValue(enabled);
  OnProxyEnabledPrefChange();
}

int64 DataReductionProxySettings::GetDataReductionLastUpdateTime() {
  PrefService* local_state = GetLocalStatePrefs();
  int64 last_update_internal =
      local_state->GetInt64(prefs::kDailyHttpContentLengthLastUpdateDate);
  base::Time last_update = base::Time::FromInternalValue(last_update_internal);
  return static_cast<int64>(last_update.ToJsTime());
}

std::vector<long long>
DataReductionProxySettings::GetDailyOriginalContentLengths() {
  return GetDailyContentLengths(prefs::kDailyHttpOriginalContentLength);
}

std::vector<long long>
DataReductionProxySettings::GetDailyReceivedContentLengths() {
  return GetDailyContentLengths(prefs::kDailyHttpReceivedContentLength);
}

void DataReductionProxySettings::OnURLFetchComplete(
    const net::URLFetcher* source) {
  net::URLRequestStatus status = source->GetStatus();
  if (status.status() == net::URLRequestStatus::FAILED &&
      status.error() == net::ERR_INTERNET_DISCONNECTED) {
    return;
  }

  std::string response;
  source->GetResponseAsString(&response);

  if ("OK" == response.substr(0, 2)) {
    DVLOG(1) << "The data reduction proxy is not blocked.";

    if (enabled_by_user_ && disabled_by_carrier_) {
      // The user enabled the proxy, but sometime previously in the session,
      // the network operator had blocked the proxy. Now that the network
      // operator is unblocking it, configure it to the user's desires.
      SetProxyConfigs(true, false);
    }
    disabled_by_carrier_ = false;
    return;
  }
  DVLOG(1) << "The data reduction proxy is blocked.";

  if (enabled_by_user_ && !disabled_by_carrier_) {
    // Disable the proxy.
    SetProxyConfigs(false, false);
  }
  disabled_by_carrier_ = true;
}

std::string DataReductionProxySettings::GetDataReductionProxyOriginHostPort() {
  std::string spdy_proxy = GetDataReductionProxyOrigin();
  if (spdy_proxy.empty()) {
    DLOG(ERROR) << "A SPDY proxy has not been set.";
    return spdy_proxy;
  }
  // Remove a trailing slash from the proxy string if one exists as well as
  // leading HTTPS scheme.
  return net::HostPortPair::FromURL(GURL(spdy_proxy)).ToString();
}

void DataReductionProxySettings::OnIPAddressChanged() {
  if (enabled_by_user_) {
    DCHECK(IsDataReductionProxyAllowed());
    ProbeWhetherDataReductionProxyIsAvailable();
  }
}

void DataReductionProxySettings::OnProxyEnabledPrefChange() {
  if (!IsDataReductionProxyAllowed())
    return;
  MaybeActivateDataReductionProxy(false);
}

void DataReductionProxySettings::AddDefaultProxyBypassRules() {
  // localhost
  AddHostPatternToBypass("<local>");
  // RFC1918 private addresses.
  AddHostPatternToBypass("10.0.0.0/8");
  AddHostPatternToBypass("172.16.0.0/12");
  AddHostPatternToBypass("192.168.0.0/16");
   // RFC4193 private addresses.
  AddHostPatternToBypass("fc00::/7");
}

void DataReductionProxySettings::LogProxyState(bool enabled, bool at_startup) {
  // This must stay a LOG(WARNING); the output is used in processing customer
  // feedback.
  const char kAtStartup[] = "at startup";
  const char kByUser[] = "by user action";
  const char kOn[] = "ON";
  const char kOff[] = "OFF";

  LOG(WARNING) << "SPDY proxy " << (enabled ? kOn : kOff)
               << " " << (at_startup ? kAtStartup : kByUser);
}

PrefService* DataReductionProxySettings::GetOriginalProfilePrefs() {
  return g_browser_process->profile_manager()->GetLastUsedProfile()->
      GetOriginalProfile()->GetPrefs();
}

PrefService* DataReductionProxySettings::GetLocalStatePrefs() {
  return g_browser_process->local_state();
}

bool DataReductionProxySettings::IsProxyOriginSetOnCommandLine() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  return command_line.HasSwitch(switches::kSpdyProxyAuthOrigin);
}

void DataReductionProxySettings::ResetDataReductionStatistics() {
  PrefService* prefs = GetLocalStatePrefs();
  if (!prefs)
    return;
  ListPrefUpdate original_update(prefs, prefs::kDailyHttpOriginalContentLength);
  ListPrefUpdate received_update(prefs, prefs::kDailyHttpReceivedContentLength);
  original_update->Clear();
  received_update->Clear();
  for (size_t i = 0; i < spdyproxy::kNumDaysInHistory; ++i) {
    original_update->AppendString(base::Int64ToString(0));
    received_update->AppendString(base::Int64ToString(0));
  }
}

void DataReductionProxySettings::MaybeActivateDataReductionProxy(
    bool at_startup) {
  PrefService* prefs = GetOriginalProfilePrefs();

  // TODO(marq): Consider moving this so stats are wiped the first time the
  // proxy settings are actually (not maybe) turned on.
  if (spdy_proxy_auth_enabled_.GetValue() &&
      !prefs->GetBoolean(prefs::kSpdyProxyAuthWasEnabledBefore)) {
    prefs->SetBoolean(prefs::kSpdyProxyAuthWasEnabledBefore, true);
    ResetDataReductionStatistics();
  }

  std::string spdy_proxy_origin = GetDataReductionProxyOriginHostPort();

  // Configure use of the data reduction proxy if it is enabled and the proxy
  // origin is non-empty.
  enabled_by_user_=
      spdy_proxy_auth_enabled_.GetValue() && !spdy_proxy_origin.empty();
  SetProxyConfigs(enabled_by_user_ && !disabled_by_carrier_, at_startup);

  // Check if the proxy has been disabled explicitly by the carrier.
  if (enabled_by_user_)
    ProbeWhetherDataReductionProxyIsAvailable();
}

void DataReductionProxySettings::SetProxyConfigs(bool enabled,
                                                 bool at_startup) {
  LogProxyState(enabled, at_startup);
  PrefService* prefs = GetOriginalProfilePrefs();
  DCHECK(prefs);
  DictionaryPrefUpdate update(prefs, prefs::kProxy);
  base::DictionaryValue* dict = update.Get();
  if (enabled) {
    std::string proxy_server_config =
        "http=" + GetDataReductionProxyOrigin() + ",direct://;";
    dict->SetString("server", proxy_server_config);
    dict->SetString("mode",
                    ProxyModeToString(ProxyPrefs::MODE_FIXED_SERVERS));
    dict->SetString("bypass_list", JoinString(bypass_rules_, ", "));
  } else {
    dict->SetString("mode", ProxyModeToString(ProxyPrefs::MODE_SYSTEM));
    dict->SetString("server", "");
    dict->SetString("bypass_list", "");
  }
}

// Metrics methods
void DataReductionProxySettings::RecordDataReductionInit() {
  ProxyStartupState state = PROXY_NOT_AVAILABLE;
  if (IsDataReductionProxyAllowed())
    state = IsDataReductionProxyEnabled() ? PROXY_ENABLED : PROXY_DISABLED;
  UMA_HISTOGRAM_ENUMERATION(kUMAProxyStartupStateHistogram,
                            state,
                            PROXY_STARTUP_STATE_COUNT);
}

DataReductionProxySettings::ContentLengthList
DataReductionProxySettings::GetDailyContentLengths(const char* pref_name) {
  DataReductionProxySettings::ContentLengthList content_lengths;
  const ListValue* list_value = GetLocalStatePrefs()->GetList(pref_name);
  if (list_value->GetSize() == spdyproxy::kNumDaysInHistory) {
    for (size_t i = 0; i < spdyproxy::kNumDaysInHistory; ++i) {
      content_lengths.push_back(GetInt64PrefValue(*list_value, i));
    }
  }
  return content_lengths;
 }

void DataReductionProxySettings::GetContentLengths(
    unsigned int days,
    int64* original_content_length,
    int64* received_content_length,
    int64* last_update_time) {
  DCHECK_LE(days, spdyproxy::kNumDaysInHistory);
  PrefService* local_state = GetLocalStatePrefs();
  if (!local_state) {
    *original_content_length = 0L;
    *received_content_length = 0L;
    *last_update_time = 0L;
    return;
  }

  const ListValue* original_list =
      local_state->GetList(prefs::kDailyHttpOriginalContentLength);
  const ListValue* received_list =
      local_state->GetList(prefs::kDailyHttpReceivedContentLength);

  if (original_list->GetSize() != spdyproxy::kNumDaysInHistory ||
      received_list->GetSize() != spdyproxy::kNumDaysInHistory) {
    *original_content_length = 0L;
    *received_content_length = 0L;
    *last_update_time = 0L;
    return;
  }

  int64 orig = 0L;
  int64 recv = 0L;
  // Include days from the end of the list going backwards.
  for (size_t i = spdyproxy::kNumDaysInHistory - days;
       i < spdyproxy::kNumDaysInHistory; ++i) {
    orig += GetInt64PrefValue(*original_list, i);
    recv += GetInt64PrefValue(*received_list, i);
  }
  *original_content_length = orig;
  *received_content_length = recv;
  *last_update_time =
      local_state->GetInt64(prefs::kDailyHttpContentLengthLastUpdateDate);
}

std::string DataReductionProxySettings::GetProxyCheckURL() {
  if (!IsDataReductionProxyAllowed())
    return std::string();
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDataReductionProxyProbeURL)) {
    return command_line.GetSwitchValueASCII(
        switches::kDataReductionProxyProbeURL);
  }
#if defined(DATA_REDUCTION_PROXY_PROBE_URL)
  return DATA_REDUCTION_PROXY_PROBE_URL;
#else
  return std::string();
#endif
}

net::URLFetcher* DataReductionProxySettings::GetURLFetcher() {
  std::string url = GetProxyCheckURL();
  if (url.empty())
    return NULL;
  net::URLFetcher* fetcher = net::URLFetcher::Create(GURL(url),
                                                     net::URLFetcher::GET,
                                                     this);
  fetcher->SetLoadFlags(net::LOAD_DISABLE_CACHE);
  fetcher->SetLoadFlags(net::LOAD_BYPASS_PROXY);
  Profile* profile = g_browser_process->profile_manager()->
      GetDefaultProfile();
  fetcher->SetRequestContext(profile->GetRequestContext());
  // Configure max retries to be at most kMaxRetries times for 5xx errors.
  static const int kMaxRetries = 5;
  fetcher->SetMaxRetriesOn5xx(kMaxRetries);
  return fetcher;
}

void
DataReductionProxySettings::ProbeWhetherDataReductionProxyIsAvailable() {
  net::URLFetcher* fetcher = GetURLFetcher();
  if (!fetcher)
    return;
  fetcher_.reset(fetcher);
  fetcher_->Start();
}
