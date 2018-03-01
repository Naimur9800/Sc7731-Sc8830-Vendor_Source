// Copyright 2013 Google Inc. All Rights Reserved.

#include "config_test_env.h"

namespace {
const std::string kWidevineKeySystem = "com.widevine.alpha";

// Content Protection license server data
const std::string kCpLicenseServer =
    "http://widevine-proxy.appspot.com/proxy";
const std::string kCpClientAuth = "";
const std::string kCpKeyId =
    "00000042"  // blob size
    "70737368"  // "pssh"
    "00000000"  // flags
    "edef8ba979d64acea3c827dcd51d21ed"   // Widevine system id
    "00000022"  // pssh data size
    // pssh data:
    "08011a0d7769646576696e655f746573"
    "74220f73747265616d696e675f636c69"
    "7031";
const std::string kCpOfflineKeyId =
    "00000040"  // blob size
    "70737368"  // "pssh"
    "00000000"  // flags
    "edef8ba979d64acea3c827dcd51d21ed"   // Widevine system id
    "00000020"  // pssh data size
    // pssh data:
    "08011a0d7769646576696e655f746573"
    "74220d6f66666c696e655f636c697032";

// Google Play license server data
const std::string kGpLicenseServer =
    "https://jmt17.google.com/video/license/GetCencLicense";
// Test client authorization string.
// NOTE: Append a userdata attribute to place a unique marker that the
// server team can use to track down specific requests during debugging
// e.g., "<existing-client-auth-string>&userdata=<your-ldap>.<your-tag>"
//       "<existing-client-auth-string>&userdata=jbmr2.dev"
const std::string kGpClientAuth =
    "?source=YOUTUBE&video_id=EGHC6OHNbOo&oauth=ya.gtsqawidevine";
const std::string kGpKeyId =
    "00000034"  // blob size
    "70737368"  // "pssh"
    "00000000"  // flags
    "edef8ba979d64acea3c827dcd51d21ed"  // Widevine system id
    "00000014"  // pssh data size
    // pssh data:
    "08011210e02562e04cd55351b14b3d74"
    "8d36ed8e";
const std::string kGpOfflineKeyId = kGpKeyId;

const std::string kGpClientOfflineQueryParameters =
    "&offline=true";
const std::string kGpClientOfflineRenewalQueryParameters =
    "&offline=true&renewal=true";
const std::string kGpClientOfflineReleaseQueryParameters =
    "&offline=true&release=true";

// An invalid key id, expected to fail
const std::string kWrongKeyId =
    "00000034"  // blob size
    "70737368"  // "pssh"
    "00000000"  // flags
    "edef8ba979d64acea3c827dcd51d21ed"  // Widevine system id
    "00000014"  // pssh data size
    // pssh data:
    "0901121094889920e8d6520098577df8"
    "f2dd5546";

// URL of provisioning server (returned by GetProvisioningRequest())
const std::string kProductionProvisioningServerUrl =
    "https://www.googleapis.com/"
    "certificateprovisioning/v1/devicecertificates/create"
    "?key=AIzaSyB-5OLKTx2iU5mko18DfdwK5611JIjbUhE";

const wvcdm::ConfigTestEnv::LicenseServerConfiguration license_servers[] = {
  { wvcdm::kGooglePlayServer, kGpLicenseServer,
    kGpClientAuth, kGpKeyId, kGpOfflineKeyId },
  { wvcdm::kContentProtectionServer, kCpLicenseServer,
    kCpClientAuth, kCpKeyId, kCpOfflineKeyId },
};

}  // namespace

namespace wvcdm {

ConfigTestEnv::ConfigTestEnv(LicenseServerId server_id) {
  Init(server_id);
}

ConfigTestEnv::ConfigTestEnv(LicenseServerId server_id, bool streaming) {
  Init(server_id);
  if (!streaming)
    key_id_ = license_servers[server_id].offline_key_id;
}

ConfigTestEnv::ConfigTestEnv(LicenseServerId server_id, bool streaming,
                             bool renew, bool release) {
  Init(server_id);
  if (!streaming) {
    key_id_ = license_servers[server_id].offline_key_id;

    if (wvcdm::kGooglePlayServer == server_id) {
      if (renew) {
        client_auth_.append(kGpClientOfflineRenewalQueryParameters);
      } else if (release) {
        client_auth_.append(kGpClientOfflineReleaseQueryParameters);
      } else {
        client_auth_.append(kGpClientOfflineQueryParameters);
      }
    }
  }
}

void ConfigTestEnv::Init(LicenseServerId server_id) {
  client_auth_ = license_servers[server_id].client_tag;
  key_id_ = license_servers[server_id].key_id;
  key_system_ = kWidevineKeySystem;
  license_server_ = license_servers[server_id].url;
  provisioning_server_url_ = kProductionProvisioningServerUrl;
  wrong_key_id_= kWrongKeyId;
}

}  // namespace wvcdm
