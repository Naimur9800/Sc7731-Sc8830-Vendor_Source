// Copyright 2012 Google Inc. All Rights Reserved.

#include "cdm_session.h"

#include <iostream>
#include <sstream>
#include <stdlib.h>

#include "cdm_engine.h"
#include "clock.h"
#include "crypto_session.h"
#include "device_files.h"
#include "file_store.h"
#include "log.h"
#include "properties.h"
#include "string_conversions.h"
#include "wv_cdm_constants.h"
#include "wv_cdm_event_listener.h"

namespace {
const size_t kKeySetIdLength = 14;
}  // namespace

namespace wvcdm {

typedef std::set<WvCdmEventListener*>::iterator CdmEventListenerIter;

CdmSession::CdmSession(const CdmClientPropertySet* cdm_client_property_set) {
  CryptoSession* crypto_session = new CryptoSession();
  Create(new CdmLicense(), crypto_session, new PolicyEngine(crypto_session),
         new DeviceFiles(), cdm_client_property_set);
}

CdmSession::CdmSession(
    CdmLicense* license_parser,
    CryptoSession* crypto_session,
    PolicyEngine* policy_engine,
    DeviceFiles* file_handle,
    const CdmClientPropertySet* cdm_client_property_set) {
  Create(license_parser, crypto_session, policy_engine, file_handle,
         cdm_client_property_set);
}

void CdmSession::Create(
    CdmLicense* license_parser,
    CryptoSession* crypto_session,
    PolicyEngine* policy_engine,
    DeviceFiles* file_handle,
    const CdmClientPropertySet* cdm_client_property_set) {
  // Just return on failures. Failures will be signaled in Init.
  if (NULL == license_parser) {
    LOGE("CdmSession::Create: License parser not provided");
    return;
  }
  if (NULL == crypto_session) {
    LOGE("CdmSession::Create: Crypto session not provided");
    return;
  }
  if (NULL == policy_engine) {
    LOGE("CdmSession::Create: Policy engine not provided");
    return;
  }
  if (NULL == file_handle) {
    LOGE("CdmSession::Create: Device files not provided");
    return;
  }
  initialized_ = false;
  session_id_ = GenerateSessionId();
  license_parser_.reset(license_parser);
  crypto_session_.reset(crypto_session);
  file_handle_.reset(file_handle);
  policy_engine_.reset(policy_engine);
  license_received_ = false;
  is_offline_ = false;
  is_release_ = false;
  is_initial_usage_update_ = true;
  is_usage_update_needed_ = false;
  is_initial_decryption_ = true;
  has_decrypted_recently_ = false;
  if (cdm_client_property_set) {
    Properties::AddSessionPropertySet(session_id_, cdm_client_property_set);
  }
  security_level_ = GetRequestedSecurityLevel() == kLevel3
                        ? kSecurityLevelL3 : GetSecurityLevel();
  app_id_.clear();
}

CdmSession::~CdmSession() { Properties::RemoveSessionPropertySet(session_id_); }

CdmResponseType CdmSession::Init() {
  if (session_id_.empty()) {
    LOGE("CdmSession::Init: Failed, session not properly constructed");
    return UNKNOWN_ERROR;
  }
  if (initialized_) {
    LOGE("CdmSession::Init: Failed due to previous initialization");
    return UNKNOWN_ERROR;
  }
  CdmResponseType sts = crypto_session_->Open(GetRequestedSecurityLevel());
  if (NO_ERROR != sts) return sts;

  std::string token;
  if (Properties::use_certificates_as_identification()) {
    std::string wrapped_key;
    if (!file_handle_->Init(security_level_) ||
        !file_handle_->RetrieveCertificate(&token, &wrapped_key) ||
        !crypto_session_->LoadCertificatePrivateKey(wrapped_key)) {
      return NEED_PROVISIONING;
    }
  } else {
    if (!crypto_session_->GetToken(&token)) return UNKNOWN_ERROR;
  }

  if (!license_parser_->Init(token, crypto_session_.get(),
                             policy_engine_.get()))
    return UNKNOWN_ERROR;

  license_received_ = false;
  is_initial_decryption_ = true;
  initialized_ = true;
  return NO_ERROR;
}

CdmResponseType CdmSession::RestoreOfflineSession(
    const CdmKeySetId& key_set_id, const CdmLicenseType license_type) {
  key_set_id_ = key_set_id;

  // Retrieve license information from persistent store
  if (!file_handle_->Reset(security_level_))
    return UNKNOWN_ERROR;

  DeviceFiles::LicenseState license_state;
  int64_t playback_start_time;
  int64_t last_playback_time;

  if (!file_handle_->RetrieveLicense(key_set_id, &license_state,
                                     &offline_init_data_,
                                     &key_request_, &key_response_,
                                     &offline_key_renewal_request_,
                                     &offline_key_renewal_response_,
                                     &offline_release_server_url_,
                                     &playback_start_time,
                                     &last_playback_time)) {
    LOGE("CdmSession::Init failed to retrieve license. key set id = %s",
         key_set_id.c_str());
    return UNKNOWN_ERROR;
  }

  // Do not restore a released offline license, unless a release retry
  if (!(license_type == kLicenseTypeRelease ||
       license_state == DeviceFiles::kLicenseStateActive)) {
    LOGE("CdmSession::Init invalid offline license state = %d, type = %d",
        license_state,
        license_type);
    return UNKNOWN_ERROR;
  }

  if (license_type == kLicenseTypeRelease) {
    if (!license_parser_->RestoreLicenseForRelease(key_request_,
                                                   key_response_)) {
      return UNKNOWN_ERROR;
    }
  } else {
    if (!license_parser_->RestoreOfflineLicense(key_request_, key_response_,
                                                offline_key_renewal_response_,
                                                playback_start_time,
                                                last_playback_time)) {
      return UNKNOWN_ERROR;
    }
  }

  license_received_ = true;
  is_offline_ = true;
  is_release_ = license_type == kLicenseTypeRelease;
  return KEY_ADDED;
}

CdmResponseType CdmSession::RestoreUsageSession(
    const CdmKeyMessage& key_request,
    const CdmKeyResponse& key_response) {
  key_request_ = key_request;
  key_response_ = key_response;

  if (!license_parser_->RestoreLicenseForRelease(key_request_, key_response_)) {
    return UNKNOWN_ERROR;
  }

  license_received_ = true;
  is_offline_ = false;
  is_release_ = true;
  return KEY_ADDED;
}

CdmResponseType CdmSession::GenerateKeyRequest(
    const InitializationData& init_data, const CdmLicenseType license_type,
    const CdmAppParameterMap& app_parameters, CdmKeyMessage* key_request,
    std::string* server_url) {
  if (crypto_session_.get() == NULL) {
    LOGW("CdmSession::GenerateKeyRequest: Invalid crypto session");
    return UNKNOWN_ERROR;
  }

  if (!crypto_session_->IsOpen()) {
    LOGW("CdmSession::GenerateKeyRequest: Crypto session not open");
    return UNKNOWN_ERROR;
  }

  switch (license_type) {
    case kLicenseTypeStreaming: is_offline_ = false; break;
    case kLicenseTypeOffline: is_offline_ = true; break;
    case kLicenseTypeRelease: is_release_ = true; break;
    default:
      LOGE("CdmSession::GenerateKeyRequest: unrecognized license type: %ld",
           license_type);
      return UNKNOWN_ERROR;
  }

  if (is_release_) {
    return GenerateReleaseRequest(key_request, server_url);
  } else if (license_received_) {  // renewal
    return GenerateRenewalRequest(key_request, server_url);
  } else {
    if (!init_data.is_supported()) {
      LOGW("CdmSession::GenerateKeyRequest: unsupported init data type (%s)",
           init_data.type().c_str());
      return KEY_ERROR;
    }
    if (init_data.IsEmpty() && !license_parser_->HasInitData()) {
      LOGW("CdmSession::GenerateKeyRequest: init data absent");
      return KEY_ERROR;
    }

    if (!license_parser_->PrepareKeyRequest(init_data, license_type,
                                            app_parameters, session_id_,
                                            key_request, server_url)) {
      return KEY_ERROR;
    }

    key_request_ = *key_request;
    if (is_offline_) {
      offline_init_data_ = init_data.data();
      offline_release_server_url_ = *server_url;
    }

    return KEY_MESSAGE;
  }
}

// AddKey() - Accept license response and extract key info.
CdmResponseType CdmSession::AddKey(const CdmKeyResponse& key_response,
                                   CdmKeySetId* key_set_id) {
  if (crypto_session_.get() == NULL) {
    LOGW("CdmSession::AddKey: Invalid crypto session");
    return UNKNOWN_ERROR;
  }

  if (!crypto_session_->IsOpen()) {
    LOGW("CdmSession::AddKey: Crypto session not open");
    return UNKNOWN_ERROR;
  }

  if (is_release_) {
    CdmResponseType sts = ReleaseKey(key_response);
    return (NO_ERROR == sts) ? KEY_ADDED : sts;
  } else if (license_received_) {  // renewal
    return RenewKey(key_response);
  } else {
    CdmResponseType sts = license_parser_->HandleKeyResponse(key_response);

    if (sts != KEY_ADDED) return sts;

    license_received_ = true;
    key_response_ = key_response;

    if (is_offline_ || !license_parser_->provider_session_token().empty()) {
      sts = StoreLicense();
      if (sts != NO_ERROR) return sts;
    }

    *key_set_id = key_set_id_;
    return KEY_ADDED;
  }
}

CdmResponseType CdmSession::QueryStatus(CdmQueryMap* key_info) {
  if (crypto_session_.get() == NULL) {
    LOGE("CdmSession::QueryStatus: Invalid crypto session");
    return UNKNOWN_ERROR;
  }

  if (!crypto_session_->IsOpen()) {
    LOGE("CdmSession::QueryStatus: Crypto session not open");
    return UNKNOWN_ERROR;
  }

  switch (crypto_session_->GetSecurityLevel()) {
    case kSecurityLevelL1:
      (*key_info)[QUERY_KEY_SECURITY_LEVEL] = QUERY_VALUE_SECURITY_LEVEL_L1;
      break;
    case kSecurityLevelL2:
      (*key_info)[QUERY_KEY_SECURITY_LEVEL] = QUERY_VALUE_SECURITY_LEVEL_L2;
      break;
    case kSecurityLevelL3:
      (*key_info)[QUERY_KEY_SECURITY_LEVEL] = QUERY_VALUE_SECURITY_LEVEL_L3;
      break;
    case kSecurityLevelUninitialized:
    case kSecurityLevelUnknown:
      (*key_info)[QUERY_KEY_SECURITY_LEVEL] =
          QUERY_VALUE_SECURITY_LEVEL_UNKNOWN;
      break;
    default:
      return KEY_ERROR;
  }
  return NO_ERROR;
}

CdmResponseType CdmSession::QueryKeyStatus(CdmQueryMap* key_info) {
  return policy_engine_->Query(key_info);
}

CdmResponseType CdmSession::QueryKeyControlInfo(CdmQueryMap* key_info) {
  if (crypto_session_.get() == NULL) {
    LOGW("CdmSession::QueryKeyControlInfo: Invalid crypto session");
    return UNKNOWN_ERROR;
  }

  if (!crypto_session_->IsOpen()) {
    LOGW("CdmSession::QueryKeyControlInfo: Crypto session not open");
    return UNKNOWN_ERROR;
  }

  std::stringstream ss;
  ss << crypto_session_->oec_session_id();
  (*key_info)[QUERY_KEY_OEMCRYPTO_SESSION_ID] = ss.str();
  return NO_ERROR;
}

// CancelKeyRequest() - Cancel session.
CdmResponseType CdmSession::CancelKeyRequest() {
  crypto_session_->Close();
  return NO_ERROR;
}

// Decrypt() - Accept encrypted buffer and return decrypted data.
CdmResponseType CdmSession::Decrypt(const CdmDecryptionParameters& params) {
  if (crypto_session_.get() == NULL || !crypto_session_->IsOpen())
    return UNKNOWN_ERROR;

  if (params.is_encrypted && !policy_engine_->CanDecrypt(*params.key_id)) {
    return policy_engine_->IsLicenseForFuture() ? KEY_ERROR : NEED_KEY;
  }

  CdmResponseType status = crypto_session_->Decrypt(params);

  switch (status) {
    case NO_ERROR:
      if (is_initial_decryption_) {
        policy_engine_->BeginDecryption();
        is_initial_decryption_ = false;
      }
      has_decrypted_recently_ = true;
      if (!is_usage_update_needed_) {
        is_usage_update_needed_ =
          !license_parser_->provider_session_token().empty();
      }
      break;
    case UNKNOWN_ERROR: {
      Clock clock;
      int64_t current_time = clock.GetCurrentTime();
      if (policy_engine_->IsLicenseDurationExpired(current_time) ||
          policy_engine_->IsPlaybackDurationExpired(current_time)) {
        return NEED_KEY;
      }
      break;
    }
    default:  //Ignore
      break;
  }

  return status;
}

// License renewal
// GenerateRenewalRequest() - Construct valid renewal request for the current
// session keys.
CdmResponseType CdmSession::GenerateRenewalRequest(CdmKeyMessage* key_request,
                                                   std::string* server_url) {
  CdmResponseType status =
      license_parser_->PrepareKeyUpdateRequest(true, key_request, server_url);

  if (KEY_MESSAGE != status)
    return status;

  if (is_offline_) {
    offline_key_renewal_request_ = *key_request;
  }
  return KEY_MESSAGE;
}

// RenewKey() - Accept renewal response and update key info.
CdmResponseType CdmSession::RenewKey(const CdmKeyResponse& key_response) {
  CdmResponseType sts =
      license_parser_->HandleKeyUpdateResponse(true, key_response);
  if (sts != KEY_ADDED) return sts;

  if (is_offline_) {
    offline_key_renewal_response_ = key_response;
    if (!StoreLicense(DeviceFiles::kLicenseStateActive)) return UNKNOWN_ERROR;
  }
  return KEY_ADDED;
}

CdmResponseType CdmSession::GenerateReleaseRequest(CdmKeyMessage* key_request,
                                                   std::string* server_url) {
  is_release_ = true;
  CdmResponseType status =
      license_parser_->PrepareKeyUpdateRequest(false, key_request, server_url);

  if (KEY_MESSAGE != status)
    return status;

  if (is_offline_) {  // Mark license as being released
    if (!StoreLicense(DeviceFiles::kLicenseStateReleasing))
      return UNKNOWN_ERROR;
  }
  return KEY_MESSAGE;
}

// ReleaseKey() - Accept release response and  release license.
CdmResponseType CdmSession::ReleaseKey(const CdmKeyResponse& key_response) {
  CdmResponseType sts = license_parser_->HandleKeyUpdateResponse(false,
                                                                key_response);
  if (KEY_ADDED != sts)
    return sts;

  if (is_offline_ || !license_parser_->provider_session_token().empty()) {
    DeleteLicense();
  }
  return NO_ERROR;
}

bool CdmSession::IsKeyLoaded(const KeyId& key_id) {
  return license_parser_->IsKeyLoaded(key_id);
}

CdmSessionId CdmSession::GenerateSessionId() {
  static int session_num = 1;
  return SESSION_ID_PREFIX + IntToString(++session_num);
}

bool CdmSession::GenerateKeySetId(CdmKeySetId* key_set_id) {
  if (!key_set_id) {
    LOGW("CdmSession::GenerateKeySetId: key set id destination not provided");
    return false;
  }

  std::vector<uint8_t> random_data(
      (kKeySetIdLength - sizeof(KEY_SET_ID_PREFIX)) / 2, 0);

  if (!file_handle_->Reset(security_level_))
    return false;

  while (key_set_id->empty()) {
    if (!crypto_session_->GetRandom(random_data.size(), &random_data[0]))
      return false;

    *key_set_id = KEY_SET_ID_PREFIX + b2a_hex(random_data);

    // key set collision
    if (file_handle_->LicenseExists(*key_set_id)) {
      key_set_id->clear();
    }
  }
  return true;
}

CdmResponseType CdmSession::StoreLicense() {
  if (is_offline_) {
    if (!GenerateKeySetId(&key_set_id_)) {
      LOGE("CdmSession::StoreLicense: Unable to generate key set Id");
      return UNKNOWN_ERROR;
    }

    if (!StoreLicense(DeviceFiles::kLicenseStateActive)) {
      LOGE("CdmSession::StoreLicense: Unable to store license");
      CdmResponseType sts = Init();
      if (sts != NO_ERROR) {
        LOGW("CdmSession::StoreLicense: Reinitialization failed");
        return sts;
      }

      key_set_id_.clear();
      return UNKNOWN_ERROR;
    }
    return NO_ERROR;
  }

  std::string provider_session_token =
      license_parser_->provider_session_token();
  if (provider_session_token.empty()) {
    LOGE("CdmSession::StoreLicense: No provider session token and not offline");
    return UNKNOWN_ERROR;
  }

  if (!file_handle_->Reset(security_level_)) {
    LOGE("CdmSession::StoreLicense: Unable to initialize device files");
    return UNKNOWN_ERROR;
  }

  std::string app_id;
  GetApplicationId(&app_id);
  if (!file_handle_->StoreUsageInfo(provider_session_token, key_request_,
                                    key_response_, app_id)) {
    LOGE("CdmSession::StoreLicense: Unable to store usage info");
    return UNKNOWN_ERROR;
  }
  return NO_ERROR;
}

bool CdmSession::StoreLicense(DeviceFiles::LicenseState state) {
  if (!file_handle_->Reset(security_level_))
    return false;

  return file_handle_->StoreLicense(
      key_set_id_, state, offline_init_data_, key_request_,
      key_response_, offline_key_renewal_request_,
      offline_key_renewal_response_, offline_release_server_url_,
      policy_engine_->GetPlaybackStartTime(),
      policy_engine_->GetLastPlaybackTime());
}

bool CdmSession::DeleteLicense() {
  if (!is_offline_ && license_parser_->provider_session_token().empty())
    return false;

  if (!file_handle_->Reset(security_level_)) {
    LOGE("CdmSession::DeleteLicense: Unable to initialize device files");
    return false;
  }

  if (is_offline_) {
    return file_handle_->DeleteLicense(key_set_id_);
  } else {
    std::string app_id;
    GetApplicationId(&app_id);
    return file_handle_->DeleteUsageInfo(
        app_id, license_parser_->provider_session_token());
  }
}

bool CdmSession::AttachEventListener(WvCdmEventListener* listener) {
  std::pair<CdmEventListenerIter, bool> result = listeners_.insert(listener);
  return result.second;
}

bool CdmSession::DetachEventListener(WvCdmEventListener* listener) {
  return (listeners_.erase(listener) == 1);
}

void CdmSession::NotifyResolution(uint32_t width, uint32_t height) {
  policy_engine_->NotifyResolution(width, height);
}

void CdmSession::OnTimerEvent(bool update_usage) {
  bool event_occurred = false;
  CdmEventType event;

  if (update_usage && has_decrypted_recently_) {
    policy_engine_->DecryptionEvent();
    has_decrypted_recently_ = false;
    if (is_offline_ && !is_release_) {
      StoreLicense(DeviceFiles::kLicenseStateActive);
    }
  }
  policy_engine_->OnTimerEvent(&event_occurred, &event);

  if (event_occurred) {
    for (CdmEventListenerIter iter = listeners_.begin();
         iter != listeners_.end(); ++iter) {
      (*iter)->OnEvent(session_id_, event);
    }
  }
}

void CdmSession::OnKeyReleaseEvent(const CdmKeySetId& key_set_id) {
  if (key_set_id_ == key_set_id) {
    for (CdmEventListenerIter iter = listeners_.begin();
         iter != listeners_.end(); ++iter) {
      (*iter)->OnEvent(session_id_, LICENSE_EXPIRED_EVENT);
    }
  }
}

void CdmSession::GetApplicationId(std::string* app_id) {
  if (app_id && !Properties::GetApplicationId(session_id_, app_id)) {
    *app_id = "";
  }
}

SecurityLevel CdmSession::GetRequestedSecurityLevel() {
  std::string security_level;
  if (Properties::GetSecurityLevel(session_id_, &security_level) &&
      security_level == QUERY_VALUE_SECURITY_LEVEL_L3) {
    return kLevel3;
  }

  return kLevelDefault;
}

CdmSecurityLevel CdmSession::GetSecurityLevel() {
  if (NULL == crypto_session_.get())
    return kSecurityLevelUninitialized;

  return crypto_session_.get()->GetSecurityLevel();
}

CdmResponseType CdmSession::DeleteUsageInformation(const std::string& app_id) {
  if (!file_handle_->Reset(security_level_)) {
    LOGE("CdmSession::StoreLicense: Unable to initialize device files");
    return UNKNOWN_ERROR;
  }

  if (file_handle_->DeleteAllUsageInfoForApp(app_id)) {
    return NO_ERROR;
  } else {
    LOGE("CdmSession::DeleteUsageInformation: failed");
    return UNKNOWN_ERROR;
  }
}

CdmResponseType CdmSession::UpdateUsageInformation() {
  return crypto_session_->UpdateUsageInformation();
}

}  // namespace wvcdm
