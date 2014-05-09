// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_signature.h"

#include "components/onc/onc_constants.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using base::Value;

namespace chromeos {
namespace onc {
namespace {

const OncValueSignature kBoolSignature = {
  base::Value::TYPE_BOOLEAN, NULL
};
const OncValueSignature kStringSignature = {
  base::Value::TYPE_STRING, NULL
};
const OncValueSignature kIntegerSignature = {
  base::Value::TYPE_INTEGER, NULL
};
const OncValueSignature kStringListSignature = {
  base::Value::TYPE_LIST, NULL, &kStringSignature
};
const OncValueSignature kIntegerListSignature = {
  base::Value::TYPE_LIST, NULL, &kIntegerSignature
};
const OncValueSignature kIPConfigListSignature = {
  base::Value::TYPE_LIST, NULL, &kIPConfigSignature
};
const OncValueSignature kCellularApnListSignature = {
  base::Value::TYPE_LIST, NULL, &kCellularApnSignature
};

const OncFieldSignature issuer_subject_pattern_fields[] = {
    { ::onc::certificate::kCommonName, &kStringSignature},
    { ::onc::certificate::kLocality, &kStringSignature},
    { ::onc::certificate::kOrganization, &kStringSignature},
    { ::onc::certificate::kOrganizationalUnit, &kStringSignature},
    {NULL}};

const OncFieldSignature certificate_pattern_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::certificate::kEnrollmentURI, &kStringListSignature},
    { ::onc::certificate::kIssuer, &kIssuerSubjectPatternSignature},
    { ::onc::certificate::kIssuerCARef, &kStringListSignature},
    // Used internally. Not officially supported.
    { ::onc::certificate::kIssuerCAPEMs, &kStringListSignature},
    { ::onc::certificate::kSubject, &kIssuerSubjectPatternSignature},
    {NULL}};

const OncFieldSignature eap_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::eap::kAnonymousIdentity, &kStringSignature},
    { ::onc::eap::kClientCertPattern, &kCertificatePatternSignature},
    { ::onc::eap::kClientCertRef, &kStringSignature},
    { ::onc::eap::kClientCertType, &kStringSignature},
    { ::onc::eap::kIdentity, &kStringSignature},
    { ::onc::eap::kInner, &kStringSignature},
    { ::onc::eap::kOuter, &kStringSignature},
    { ::onc::eap::kPassword, &kStringSignature},
    { ::onc::eap::kSaveCredentials, &kBoolSignature},
    // Used internally. Not officially supported.
    { ::onc::eap::kServerCAPEMs, &kStringListSignature},
    { ::onc::eap::kServerCARef, &kStringSignature},
    { ::onc::eap::kServerCARefs, &kStringListSignature},
    { ::onc::eap::kUseSystemCAs, &kBoolSignature},
    {NULL}};

const OncFieldSignature ipsec_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::ipsec::kAuthenticationType, &kStringSignature},
    { ::onc::vpn::kClientCertPattern, &kCertificatePatternSignature},
    { ::onc::vpn::kClientCertRef, &kStringSignature},
    { ::onc::vpn::kClientCertType, &kStringSignature},
    { ::onc::ipsec::kGroup, &kStringSignature},
    { ::onc::ipsec::kIKEVersion, &kIntegerSignature},
    { ::onc::ipsec::kPSK, &kStringSignature},
    { ::onc::vpn::kSaveCredentials, &kBoolSignature},
    // Used internally. Not officially supported.
    { ::onc::ipsec::kServerCAPEMs, &kStringListSignature},
    { ::onc::ipsec::kServerCARef, &kStringSignature},
    { ::onc::ipsec::kServerCARefs, &kStringListSignature},
    { ::onc::ipsec::kXAUTH, &kXAUTHSignature},
    // Not yet supported.
    //  { ipsec::kEAP, &kEAPSignature },
    {NULL}};

const OncFieldSignature xauth_fields[] = {
    { ::onc::vpn::kPassword, &kStringSignature},
    { ::onc::vpn::kUsername, &kStringSignature},
    {NULL}};

const OncFieldSignature l2tp_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::vpn::kPassword, &kStringSignature},
    { ::onc::vpn::kSaveCredentials, &kBoolSignature},
    { ::onc::vpn::kUsername, &kStringSignature},
    {NULL}};

const OncFieldSignature openvpn_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::openvpn::kAuth, &kStringSignature},
    { ::onc::openvpn::kAuthNoCache, &kBoolSignature},
    { ::onc::openvpn::kAuthRetry, &kStringSignature},
    { ::onc::openvpn::kCipher, &kStringSignature},
    { ::onc::vpn::kClientCertPattern, &kCertificatePatternSignature},
    { ::onc::vpn::kClientCertRef, &kStringSignature},
    { ::onc::vpn::kClientCertType, &kStringSignature},
    { ::onc::openvpn::kCompLZO, &kStringSignature},
    { ::onc::openvpn::kCompNoAdapt, &kBoolSignature},
    { ::onc::openvpn::kKeyDirection, &kStringSignature},
    { ::onc::openvpn::kNsCertType, &kStringSignature},
    { ::onc::vpn::kPassword, &kStringSignature},
    { ::onc::openvpn::kPort, &kIntegerSignature},
    { ::onc::openvpn::kProto, &kStringSignature},
    { ::onc::openvpn::kPushPeerInfo, &kBoolSignature},
    { ::onc::openvpn::kRemoteCertEKU, &kStringSignature},
    { ::onc::openvpn::kRemoteCertKU, &kStringListSignature},
    { ::onc::openvpn::kRemoteCertTLS, &kStringSignature},
    { ::onc::openvpn::kRenegSec, &kIntegerSignature},
    { ::onc::vpn::kSaveCredentials, &kBoolSignature},
    // Used internally. Not officially supported.
    { ::onc::openvpn::kServerCAPEMs, &kStringListSignature},
    { ::onc::openvpn::kServerCARef, &kStringSignature},
    { ::onc::openvpn::kServerCARefs, &kStringListSignature},
    // Not supported, yet.
    { ::onc::openvpn::kServerCertPEM, &kStringSignature},
    { ::onc::openvpn::kServerCertRef, &kStringSignature},
    { ::onc::openvpn::kServerPollTimeout, &kIntegerSignature},
    { ::onc::openvpn::kShaper, &kIntegerSignature},
    { ::onc::openvpn::kStaticChallenge, &kStringSignature},
    { ::onc::openvpn::kTLSAuthContents, &kStringSignature},
    { ::onc::openvpn::kTLSRemote, &kStringSignature},
    { ::onc::vpn::kUsername, &kStringSignature},
    // Not supported, yet.
    { ::onc::openvpn::kVerb, &kStringSignature},
    { ::onc::openvpn::kVerifyHash, &kStringSignature},
    { ::onc::openvpn::kVerifyX509, &kVerifyX509Signature},
    {NULL}};

const OncFieldSignature verify_x509_fields[] = {
    { ::onc::verify_x509::kName, &kStringSignature},
    { ::onc::verify_x509::kType, &kStringSignature},
    {NULL}};

const OncFieldSignature vpn_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::vpn::kAutoConnect, &kBoolSignature},
    { ::onc::vpn::kHost, &kStringSignature},
    { ::onc::vpn::kIPsec, &kIPsecSignature},
    { ::onc::vpn::kL2TP, &kL2TPSignature},
    { ::onc::vpn::kOpenVPN, &kOpenVPNSignature},
    { ::onc::vpn::kType, &kStringSignature},
    {NULL}};

const OncFieldSignature ethernet_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::ethernet::kAuthentication, &kStringSignature},
    { ::onc::ethernet::kEAP, &kEAPSignature},
    {NULL}};

// Not supported for policy but for reading network state.
const OncFieldSignature ipconfig_fields[] = {
    { ::onc::ipconfig::kGateway, &kStringSignature},
    { ::onc::ipconfig::kIPAddress, &kStringSignature},
    { ::onc::ipconfig::kNameServers, &kStringListSignature},
    { ::onc::ipconfig::kRoutingPrefix, &kIntegerSignature},
    { ::onc::network_config::kSearchDomains, &kStringListSignature},
    { ::onc::ipconfig::kType, &kStringSignature},
    {NULL}};

const OncFieldSignature proxy_location_fields[] = {
    { ::onc::proxy::kHost, &kStringSignature},
    { ::onc::proxy::kPort, &kIntegerSignature}, {NULL}};

const OncFieldSignature proxy_manual_fields[] = {
    { ::onc::proxy::kFtp, &kProxyLocationSignature},
    { ::onc::proxy::kHttp, &kProxyLocationSignature},
    { ::onc::proxy::kHttps, &kProxyLocationSignature},
    { ::onc::proxy::kSocks, &kProxyLocationSignature},
    {NULL}};

const OncFieldSignature proxy_settings_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::proxy::kExcludeDomains, &kStringListSignature},
    { ::onc::proxy::kManual, &kProxyManualSignature},
    { ::onc::proxy::kPAC, &kStringSignature},
    { ::onc::proxy::kType, &kStringSignature},
    {NULL}};

const OncFieldSignature wifi_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::wifi::kAutoConnect, &kBoolSignature},
    { ::onc::wifi::kEAP, &kEAPSignature},
    { ::onc::wifi::kHiddenSSID, &kBoolSignature},
    { ::onc::wifi::kPassphrase, &kStringSignature},
    { ::onc::wifi::kSSID, &kStringSignature},
    { ::onc::wifi::kSecurity, &kStringSignature},
    {NULL}};

const OncFieldSignature wifi_with_state_fields[] = {
    { ::onc::wifi::kBSSID, &kStringSignature},
    { ::onc::wifi::kFrequency, &kIntegerSignature},
    { ::onc::wifi::kFrequencyList, &kIntegerListSignature},
    { ::onc::wifi::kSignalStrength, &kIntegerSignature},
    {NULL}};

const OncFieldSignature cellular_provider_fields[] = {
    { ::onc::cellular_provider::kCode, &kStringSignature},
    { ::onc::cellular_provider::kCountry, &kStringSignature},
    { ::onc::cellular_provider::kName, &kStringSignature},
    {NULL}};

const OncFieldSignature cellular_apn_fields[] = {
    { ::onc::cellular_apn::kName, &kStringSignature},
    { ::onc::cellular_apn::kUsername, &kStringSignature},
    { ::onc::cellular_apn::kPassword, &kStringSignature},
    {NULL}};

const OncFieldSignature cellular_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::cellular::kAPN, &kCellularApnSignature },
    { ::onc::cellular::kAPNList, &kCellularApnListSignature}, {NULL}};

const OncFieldSignature cellular_with_state_fields[] = {
    { ::onc::cellular::kActivateOverNonCellularNetwork, &kBoolSignature},
    { ::onc::cellular::kActivationState, &kStringSignature},
    { ::onc::cellular::kAllowRoaming, &kStringSignature},
    { ::onc::cellular::kCarrier, &kStringSignature},
    { ::onc::cellular::kESN, &kStringSignature},
    { ::onc::cellular::kFamily, &kStringSignature},
    { ::onc::cellular::kFirmwareRevision, &kStringSignature},
    { ::onc::cellular::kFoundNetworks, &kStringSignature},
    { ::onc::cellular::kHardwareRevision, &kStringSignature},
    { ::onc::cellular::kHomeProvider, &kCellularProviderSignature},
    { ::onc::cellular::kICCID, &kStringSignature},
    { ::onc::cellular::kIMEI, &kStringSignature},
    { ::onc::cellular::kIMSI, &kStringSignature},
    { ::onc::cellular::kManufacturer, &kStringSignature},
    { ::onc::cellular::kMDN, &kStringSignature},
    { ::onc::cellular::kMEID, &kStringSignature},
    { ::onc::cellular::kMIN, &kStringSignature},
    { ::onc::cellular::kModelID, &kStringSignature},
    { ::onc::cellular::kNetworkTechnology, &kStringSignature},
    { ::onc::cellular::kPRLVersion, &kStringSignature},
    { ::onc::cellular::kProviderRequiresRoaming, &kStringSignature},
    { ::onc::cellular::kRoamingState, &kStringSignature},
    { ::onc::cellular::kSelectedNetwork, &kStringSignature},
    { ::onc::cellular::kServingOperator, &kCellularProviderSignature},
    { ::onc::cellular::kSIMLockStatus, &kStringSignature},
    { ::onc::cellular::kSIMPresent, &kStringSignature},
    { ::onc::cellular::kSupportedCarriers, &kStringSignature},
    { ::onc::cellular::kSupportNetworkScan, &kStringSignature},
    {NULL}};

const OncFieldSignature network_configuration_fields[] = {
    { ::onc::kRecommended, &kRecommendedSignature},
    { ::onc::network_config::kEthernet, &kEthernetSignature},
    { ::onc::network_config::kGUID, &kStringSignature},
    // Not supported for policy but for reading network state.
    { ::onc::network_config::kIPConfigs, &kIPConfigListSignature},
    { ::onc::network_config::kName, &kStringSignature},
    // Not supported, yet.
    { ::onc::network_config::kNameServers, &kStringListSignature},
    { ::onc::network_config::kProxySettings, &kProxySettingsSignature},
    { ::onc::kRemove, &kBoolSignature},
    // Not supported, yet.
    { ::onc::network_config::kSearchDomains, &kStringListSignature},
    { ::onc::network_config::kType, &kStringSignature},
    { ::onc::network_config::kVPN, &kVPNSignature},
    { ::onc::network_config::kWiFi, &kWiFiSignature},
    { ::onc::network_config::kCellular, &kCellularSignature},
    {NULL}};

const OncFieldSignature network_with_state_fields[] = {
    { ::onc::network_config::kCellular, &kCellularWithStateSignature},
    { ::onc::network_config::kConnectionState, &kStringSignature},
    { ::onc::network_config::kConnectable, &kBoolSignature},
    { ::onc::network_config::kErrorState, &kStringSignature},
    { ::onc::network_config::kWiFi, &kWiFiWithStateSignature},
    {NULL}};

const OncFieldSignature global_network_configuration_fields[] = {
    { ::onc::global_network_config::kAllowOnlyPolicyNetworksToAutoconnect,
      &kBoolSignature},
    {NULL}};

const OncFieldSignature certificate_fields[] = {
    { ::onc::certificate::kGUID, &kStringSignature},
    { ::onc::certificate::kPKCS12, &kStringSignature},
    { ::onc::kRemove, &kBoolSignature},
    { ::onc::certificate::kTrustBits, &kStringListSignature},
    { ::onc::certificate::kType, &kStringSignature},
    { ::onc::certificate::kX509, &kStringSignature},
    {NULL}};

const OncFieldSignature toplevel_configuration_fields[] = {
    { ::onc::toplevel_config::kCertificates, &kCertificateListSignature},
    { ::onc::toplevel_config::kNetworkConfigurations,
      &kNetworkConfigurationListSignature},
    { ::onc::toplevel_config::kGlobalNetworkConfiguration,
      &kGlobalNetworkConfigurationSignature},
    { ::onc::toplevel_config::kType, &kStringSignature},
    { ::onc::encrypted::kCipher, &kStringSignature},
    { ::onc::encrypted::kCiphertext, &kStringSignature},
    { ::onc::encrypted::kHMAC, &kStringSignature},
    { ::onc::encrypted::kHMACMethod, &kStringSignature},
    { ::onc::encrypted::kIV, &kStringSignature},
    { ::onc::encrypted::kIterations, &kIntegerSignature},
    { ::onc::encrypted::kSalt, &kStringSignature},
    { ::onc::encrypted::kStretch, &kStringSignature}, {NULL}};

}  // namespace

const OncValueSignature kRecommendedSignature = {
  base::Value::TYPE_LIST, NULL, &kStringSignature
};
const OncValueSignature kEAPSignature = {
  base::Value::TYPE_DICTIONARY, eap_fields, NULL
};
const OncValueSignature kIssuerSubjectPatternSignature = {
  base::Value::TYPE_DICTIONARY, issuer_subject_pattern_fields, NULL
};
const OncValueSignature kCertificatePatternSignature = {
  base::Value::TYPE_DICTIONARY, certificate_pattern_fields, NULL
};
const OncValueSignature kIPsecSignature = {
  base::Value::TYPE_DICTIONARY, ipsec_fields, NULL
};
const OncValueSignature kXAUTHSignature = {
  base::Value::TYPE_DICTIONARY, xauth_fields, NULL
};
const OncValueSignature kL2TPSignature = {
  base::Value::TYPE_DICTIONARY, l2tp_fields, NULL
};
const OncValueSignature kOpenVPNSignature = {
  base::Value::TYPE_DICTIONARY, openvpn_fields, NULL
};
const OncValueSignature kVerifyX509Signature = {
  base::Value::TYPE_DICTIONARY, verify_x509_fields, NULL
};
const OncValueSignature kVPNSignature = {
  base::Value::TYPE_DICTIONARY, vpn_fields, NULL
};
const OncValueSignature kEthernetSignature = {
  base::Value::TYPE_DICTIONARY, ethernet_fields, NULL
};
const OncValueSignature kIPConfigSignature = {
  base::Value::TYPE_DICTIONARY, ipconfig_fields, NULL
};
const OncValueSignature kProxyLocationSignature = {
  base::Value::TYPE_DICTIONARY, proxy_location_fields, NULL
};
const OncValueSignature kProxyManualSignature = {
  base::Value::TYPE_DICTIONARY, proxy_manual_fields, NULL
};
const OncValueSignature kProxySettingsSignature = {
  base::Value::TYPE_DICTIONARY, proxy_settings_fields, NULL
};
const OncValueSignature kWiFiSignature = {
  base::Value::TYPE_DICTIONARY, wifi_fields, NULL
};
const OncValueSignature kCertificateSignature = {
  base::Value::TYPE_DICTIONARY, certificate_fields, NULL
};
const OncValueSignature kNetworkConfigurationSignature = {
  base::Value::TYPE_DICTIONARY, network_configuration_fields, NULL
};
const OncValueSignature kGlobalNetworkConfigurationSignature = {
  base::Value::TYPE_DICTIONARY, global_network_configuration_fields, NULL
};
const OncValueSignature kCertificateListSignature = {
  base::Value::TYPE_LIST, NULL, &kCertificateSignature
};
const OncValueSignature kNetworkConfigurationListSignature = {
  base::Value::TYPE_LIST, NULL, &kNetworkConfigurationSignature
};
const OncValueSignature kToplevelConfigurationSignature = {
  base::Value::TYPE_DICTIONARY, toplevel_configuration_fields, NULL
};

// Derived "ONC with State" signatures.
const OncValueSignature kNetworkWithStateSignature = {
  base::Value::TYPE_DICTIONARY, network_with_state_fields, NULL,
  &kNetworkConfigurationSignature
};
const OncValueSignature kWiFiWithStateSignature = {
  base::Value::TYPE_DICTIONARY, wifi_with_state_fields, NULL, &kWiFiSignature
};
const OncValueSignature kCellularSignature = {
  base::Value::TYPE_DICTIONARY, cellular_fields, NULL
};
const OncValueSignature kCellularWithStateSignature = {
  base::Value::TYPE_DICTIONARY, cellular_with_state_fields, NULL,
  &kCellularSignature
};
const OncValueSignature kCellularProviderSignature = {
  base::Value::TYPE_DICTIONARY, cellular_provider_fields, NULL
};
const OncValueSignature kCellularApnSignature = {
  base::Value::TYPE_DICTIONARY, cellular_apn_fields, NULL
};

const OncFieldSignature* GetFieldSignature(const OncValueSignature& signature,
                                           const std::string& onc_field_name) {
  if (!signature.fields)
    return NULL;
  for (const OncFieldSignature* field_signature = signature.fields;
       field_signature->onc_field_name != NULL; ++field_signature) {
    if (onc_field_name == field_signature->onc_field_name)
      return field_signature;
  }
  if (signature.base_signature)
    return GetFieldSignature(*signature.base_signature, onc_field_name);
  return NULL;
}

namespace {

struct CredentialEntry {
  const OncValueSignature* value_signature;
  const char* field_name;
};

const CredentialEntry credentials[] = {
    {&kEAPSignature, ::onc::eap::kPassword},
    {&kIPsecSignature, ::onc::ipsec::kPSK},
    {&kXAUTHSignature, ::onc::vpn::kPassword},
    {&kL2TPSignature, ::onc::vpn::kPassword},
    {&kOpenVPNSignature, ::onc::vpn::kPassword},
    {&kOpenVPNSignature, ::onc::openvpn::kTLSAuthContents},
    {&kWiFiSignature, ::onc::wifi::kPassphrase},
    {&kCellularApnSignature, ::onc::cellular_apn::kPassword},
    {NULL}};

}  // namespace

bool FieldIsCredential(const OncValueSignature& signature,
                       const std::string& onc_field_name) {
  for (const CredentialEntry* entry = credentials;
       entry->value_signature != NULL; ++entry) {
    if (&signature == entry->value_signature &&
        onc_field_name == entry->field_name) {
      return true;
    }
  }
  return false;
}

}  // namespace onc
}  // namespace chromeos
