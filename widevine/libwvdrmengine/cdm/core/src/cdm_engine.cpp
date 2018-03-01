// Copyright 2013 Google Inc. All Rights Reserved.

#include "cdm_engine.h"

#include <stdlib.h>

#include <iostream>
#include <sstream>

#include "cdm_session.h"
#include "clock.h"
#include "device_files.h"
#include "license_protocol.pb.h"
#include "log.h"
#include "properties.h"
#include "scoped_ptr.h"
#include "string_conversions.h"
#include "wv_cdm_constants.h"
#include "wv_cdm_event_listener.h"

namespace {
  const uint32_t kUpdateUsageInformationPeriod = 60;  // seconds
  const size_t kUsageReportsPerRequest = 1;
}  // unnamed namespace

namespace wvcdm {

class UsagePropertySet : public CdmClientPropertySet {
 public:
  UsagePropertySet() {}
  virtual ~UsagePropertySet() {}
  void set_security_level(SecurityLevel security_level) {
    if (kLevel3 == security_level)
      security_level_ = QUERY_VALUE_SECURITY_LEVEL_L3;
    else
      security_level_.clear();
  }
  virtual const std::string& security_level() const { return security_level_; }
  virtual bool use_privacy_mode() const { return false; }
  virtual const std::string& service_certificate() const { return empty_; }
  virtual bool is_session_sharing_enabled() const { return false; }
  virtual uint32_t session_sharing_id() const { return 0; }
  virtual void set_session_sharing_id(uint32_t id) {
    id;  // noop to suppress warning
  }
  virtual const std::string& app_id() const { return app_id_; }
  void set_app_id(const std::string& appId) { app_id_ = appId; }

 private:
  std::string app_id_;
  std::string security_level_;
  const std::string empty_;
};

bool CdmEngine::seeded_ = false;

CdmEngine::CdmEngine()
    : cert_provisioning_(NULL),
      cert_provisioning_requested_security_level_(kLevelDefault),
      usage_session_(NULL),
      last_usage_information_update_time_(0),
      timer_event_active_(false) {
  Properties::Init();
  if (!seeded_) {
    Clock clock;
    srand(clock.GetCurrentTime());
    seeded_ = true;
  }
}

CdmEngine::~CdmEngine() {
  AutoLock lock(session_list_lock_);
  CdmSessionMap::iterator i(sessions_.begin());
  for (; i != sessions_.end(); ++i) {
    delete i->second;
  }
  sessions_.clear();
}

CdmResponseType CdmEngine::OpenSession(
    const CdmKeySystem& key_system,
    const CdmClientPropertySet* property_set,
    CdmSessionId* session_id) {
  LOGI("CdmEngine::OpenSession");

  if (!ValidateKeySystem(key_system)) {
    LOGI("CdmEngine::OpenSession: invalid key_system = %s", key_system.c_str());
    return KEY_ERROR;
  }

  if (!session_id) {
    LOGE("CdmEngine::OpenSession: no session ID destination provided");
    return KEY_ERROR;
  }

  scoped_ptr<CdmSession> new_session(new CdmSession(property_set));
  if (new_session->session_id().empty()) {
    LOGE("CdmEngine::OpenSession: failure to generate session ID");
    return UNKNOWN_ERROR;
  }

  CdmResponseType sts = new_session->Init();
  if (sts != NO_ERROR) {
    if (sts == NEED_PROVISIONING) {
      cert_provisioning_requested_security_level_ =
          new_session->GetRequestedSecurityLevel();
    } else {
      LOGE("CdmEngine::OpenSession: bad session init: %d", sts);
    }
    return sts;
  }
  *session_id = new_session->session_id();
  AutoLock lock(session_list_lock_);
  sessions_[*session_id] = new_session.release();
  return NO_ERROR;
}

CdmResponseType CdmEngine::OpenKeySetSession(
    const CdmKeySetId& key_set_id,
    const CdmClientPropertySet* property_set) {
  LOGI("CdmEngine::OpenKeySetSession");

  if (key_set_id.empty()) {
    LOGE("CdmEngine::OpenKeySetSession: invalid key set id");
    return KEY_ERROR;
  }

  CdmSessionId session_id;
  CdmResponseType sts = OpenSession(KEY_SYSTEM, property_set, &session_id);

  if (sts != NO_ERROR)
    return sts;

  release_key_sets_[key_set_id] = session_id;
  return NO_ERROR;
}

CdmResponseType CdmEngine::CloseSession(const CdmSessionId& session_id) {
  LOGI("CdmEngine::CloseSession");
  AutoLock lock(session_list_lock_);
  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::CloseSession: session not found = %s", session_id.c_str());
    return KEY_ERROR;
  }

  CdmSession* session = iter->second;
  sessions_.erase(session_id);
  // If the event timer is active, it will delete this session when it's done.
  if (timer_event_active_) dead_sessions_.push_back(session);
  else delete session;
  return NO_ERROR;
}

CdmResponseType CdmEngine::CloseKeySetSession(const CdmKeySetId& key_set_id) {
  LOGI("CdmEngine::CloseKeySetSession");

  CdmReleaseKeySetMap::iterator iter = release_key_sets_.find(key_set_id);
  if (iter == release_key_sets_.end()) {
    LOGE("CdmEngine::CloseKeySetSession: key set id not found = %s",
        key_set_id.c_str());
    return KEY_ERROR;
  }

  CdmResponseType sts = CloseSession(iter->second);
  release_key_sets_.erase(iter);
  return sts;
}

CdmResponseType CdmEngine::GenerateKeyRequest(
    const CdmSessionId& session_id,
    const CdmKeySetId& key_set_id,
    const InitializationData& init_data,
    const CdmLicenseType license_type,
    CdmAppParameterMap& app_parameters,
    CdmKeyMessage* key_request,
    std::string* server_url) {
  LOGI("CdmEngine::GenerateKeyRequest");

  CdmSessionId id = session_id;
  CdmResponseType sts;

  if (license_type == kLicenseTypeRelease) {
    if (key_set_id.empty()) {
      LOGE("CdmEngine::GenerateKeyRequest: invalid key set ID");
      return UNKNOWN_ERROR;
    }

    if (!session_id.empty()) {
      LOGE("CdmEngine::GenerateKeyRequest: invalid session ID = %s",
          session_id.c_str());
      return UNKNOWN_ERROR;
    }

    CdmReleaseKeySetMap::iterator iter = release_key_sets_.find(key_set_id);
    if (iter == release_key_sets_.end()) {
      LOGE("CdmEngine::GenerateKeyRequest: key set ID not found = %s",
          key_set_id.c_str());
      return UNKNOWN_ERROR;
    }

    id = iter->second;
  }

  CdmSessionMap::iterator iter = sessions_.find(id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::GenerateKeyRequest: session_id not found = %s",
        id.c_str());
    return KEY_ERROR;
  }

  if (!key_request) {
    LOGE("CdmEngine::GenerateKeyRequest: no key request destination provided");
    return KEY_ERROR;
  }

  key_request->clear();

  if (license_type == kLicenseTypeRelease) {
    sts = iter->second->RestoreOfflineSession(key_set_id, kLicenseTypeRelease);
    if (sts != KEY_ADDED) {
      LOGE("CdmEngine::GenerateKeyRequest: key release restoration failed,"
          "sts = %d", sts);
      return sts;
    }
  }

  sts = iter->second->GenerateKeyRequest(init_data, license_type,
                                         app_parameters, key_request,
                                         server_url);

  if (KEY_MESSAGE != sts) {
    if (sts == NEED_PROVISIONING) {
      cert_provisioning_requested_security_level_ =
          iter->second->GetRequestedSecurityLevel();
    }
    LOGE("CdmEngine::GenerateKeyRequest: key request generation failed, "
        "sts = %d", sts);
    return sts;
  }

  if (license_type == kLicenseTypeRelease) {
    OnKeyReleaseEvent(key_set_id);
  }

  return KEY_MESSAGE;
}

CdmResponseType CdmEngine::AddKey(
    const CdmSessionId& session_id,
    const CdmKeyResponse& key_data,
    CdmKeySetId* key_set_id) {
  LOGI("CdmEngine::AddKey");

  CdmSessionId id = session_id;
  bool license_type_release = session_id.empty();

  if (license_type_release) {
    if (!key_set_id) {
      LOGE("CdmEngine::AddKey: no key set id provided");
      return KEY_ERROR;
    }

    if (key_set_id->empty()) {
      LOGE("CdmEngine::AddKey: invalid key set id");
      return KEY_ERROR;
    }

    CdmReleaseKeySetMap::iterator iter = release_key_sets_.find(*key_set_id);
    if (iter == release_key_sets_.end()) {
      LOGE("CdmEngine::AddKey: key set id not found = %s", key_set_id->c_str());
      return KEY_ERROR;
    }

    id = iter->second;
  }

  CdmSessionMap::iterator iter = sessions_.find(id);

  if (iter == sessions_.end()) {
    LOGE("CdmEngine::AddKey: session id not found = %s", id.c_str());
    return KEY_ERROR;
  }

  if (key_data.empty()) {
    LOGE("CdmEngine::AddKey: no key_data");
    return KEY_ERROR;
  }

  CdmResponseType sts = iter->second->AddKey(key_data, key_set_id);

  if (KEY_ADDED != sts) {
    LOGE("CdmEngine::AddKey: keys not added, result = %d", sts);
    return sts;
  }

  return KEY_ADDED;
}

CdmResponseType CdmEngine::RestoreKey(
    const CdmSessionId& session_id,
    const CdmKeySetId& key_set_id) {
  LOGI("CdmEngine::RestoreKey");

  if (key_set_id.empty()) {
    LOGE("CdmEngine::RestoreKey: invalid key set id");
    return KEY_ERROR;
  }

  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::RestoreKey: session_id not found = %s ",
        session_id.c_str());
    return UNKNOWN_ERROR;
  }

  CdmResponseType sts =
      iter->second->RestoreOfflineSession(key_set_id, kLicenseTypeOffline);
  if (sts == NEED_PROVISIONING) {
    cert_provisioning_requested_security_level_ =
        iter->second->GetRequestedSecurityLevel();
  }
  if (KEY_ADDED != sts) {
    LOGE("CdmEngine::RestoreKey: restore offline session error = %d", sts);
  }
  return sts;
}

CdmResponseType CdmEngine::CancelKeyRequest(const CdmSessionId& session_id) {
  LOGI("CdmEngine::CancelKeyRequest");

  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::CancelKeyRequest: session_id not found = %s",
         session_id.c_str());
    return KEY_ERROR;
  }

  iter->second->CancelKeyRequest();
  return NO_ERROR;
}

CdmResponseType CdmEngine::GenerateRenewalRequest(
    const CdmSessionId& session_id,
    CdmKeyMessage* key_request,
    std::string* server_url) {
  LOGI("CdmEngine::GenerateRenewalRequest");

  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::GenerateRenewalRequest: session_id not found = %s",
         session_id.c_str());
    return KEY_ERROR;
  }

  if (!key_request) {
    LOGE("CdmEngine::GenerateRenewalRequest: no key request destination");
    return KEY_ERROR;
  }

  key_request->clear();

  CdmResponseType sts = iter->second->GenerateRenewalRequest(key_request,
                                                             server_url);

  if (KEY_MESSAGE != sts) {
    LOGE("CdmEngine::GenerateRenewalRequest: key request gen. failed, sts=%d",
         sts);
    return sts;
  }

  return KEY_MESSAGE;
}

CdmResponseType CdmEngine::RenewKey(
    const CdmSessionId& session_id,
    const CdmKeyResponse& key_data) {
  LOGI("CdmEngine::RenewKey");

  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::RenewKey: session_id not found = %s", session_id.c_str());
    return KEY_ERROR;
  }

  if (key_data.empty()) {
    LOGE("CdmEngine::RenewKey: no key_data");
    return KEY_ERROR;
  }

  CdmResponseType sts = iter->second->RenewKey(key_data);
  if (KEY_ADDED != sts) {
    LOGE("CdmEngine::RenewKey: keys not added, sts=%d", sts);
    return sts;
  }

  return KEY_ADDED;
}

CdmResponseType CdmEngine::QueryStatus(CdmQueryMap* key_info) {
  LOGI("CdmEngine::QueryStatus");
  CryptoSession crypto_session;
  switch (crypto_session.GetSecurityLevel()) {
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

  std::string deviceId;
  bool success = crypto_session.GetDeviceUniqueId(&deviceId);
  if (success) {
    (*key_info)[QUERY_KEY_DEVICE_ID] = deviceId;
  }

  uint32_t system_id;
  success = crypto_session.GetSystemId(&system_id);
  if (success) {
    std::ostringstream system_id_stream;
    system_id_stream << system_id;
    (*key_info)[QUERY_KEY_SYSTEM_ID] = system_id_stream.str();
  }

  std::string provisioning_id;
  success = crypto_session.GetProvisioningId(&provisioning_id);
  if (success) {
    (*key_info)[QUERY_KEY_PROVISIONING_ID] = provisioning_id;
  }

  CryptoSession::OemCryptoHdcpVersion current_hdcp;
  CryptoSession::OemCryptoHdcpVersion max_hdcp;
  success = crypto_session.GetHdcpCapabilities(&current_hdcp, &max_hdcp);
  if (success) {
    (*key_info)[QUERY_KEY_CURRENT_HDCP_LEVEL] = MapHdcpVersion(current_hdcp);
    (*key_info)[QUERY_KEY_MAX_HDCP_LEVEL] = MapHdcpVersion(max_hdcp);
  }

  bool supports_usage_reporting;
  success = crypto_session.UsageInformationSupport(&supports_usage_reporting);
  if (success) {
    (*key_info)[QUERY_KEY_USAGE_SUPPORT] =
        supports_usage_reporting ? QUERY_VALUE_TRUE : QUERY_VALUE_FALSE;
  }

  return NO_ERROR;
}

CdmResponseType CdmEngine::QuerySessionStatus(const CdmSessionId& session_id,
                                              CdmQueryMap* key_info) {
  LOGI("CdmEngine::QuerySessionStatus");
  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::QuerySessionStatus: session_id not found = %s",
         session_id.c_str());
    return KEY_ERROR;
  }
  return iter->second->QueryStatus(key_info);
}

CdmResponseType CdmEngine::QueryKeyStatus(
    const CdmSessionId& session_id,
    CdmQueryMap* key_info) {
  LOGI("CdmEngine::QueryKeyStatus");
  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::QueryKeyStatus: session_id not found = %s",
         session_id.c_str());
    return KEY_ERROR;
  }
  return iter->second->QueryKeyStatus(key_info);
}

CdmResponseType CdmEngine::QueryKeyControlInfo(
    const CdmSessionId& session_id,
    CdmQueryMap* key_info) {
  LOGI("CdmEngine::QueryKeyControlInfo");
  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::QueryKeyControlInfo: session_id not found = %s",
         session_id.c_str());
    return KEY_ERROR;
  }
  return iter->second->QueryKeyControlInfo(key_info);
}

/*
 * Composes a device provisioning request and output the request in JSON format
 * in *request. It also returns the default url for the provisioning server
 * in *default_url.
 *
 * Returns NO_ERROR for success and UNKNOWN_ERROR if fails.
 */
CdmResponseType CdmEngine::GetProvisioningRequest(
    CdmCertificateType cert_type,
    const std::string& cert_authority,
    CdmProvisioningRequest* request,
    std::string* default_url) {
  if (!request || !default_url) {
    LOGE("CdmEngine::GetProvisioningRequest: invalid input parameters");
    return UNKNOWN_ERROR;
  }
  if (NULL == cert_provisioning_.get()) {
    cert_provisioning_.reset(new CertificateProvisioning());
  }
  CdmResponseType ret = cert_provisioning_->GetProvisioningRequest(
      cert_provisioning_requested_security_level_,
      cert_type,
      cert_authority,
      request,
      default_url);
  if (ret != NO_ERROR) {
    cert_provisioning_.reset(NULL);  // Release resources.
  }
  return ret;
}

/*
 * The response message consists of a device certificate and the device RSA key.
 * The device RSA key is stored in the T.E.E. The device certificate is stored
 * in the device.
 *
 * Returns NO_ERROR for success and UNKNOWN_ERROR if fails.
 */
CdmResponseType CdmEngine::HandleProvisioningResponse(
    CdmProvisioningResponse& response,
    std::string* cert,
    std::string* wrapped_key) {
  if (response.empty()) {
    LOGE("CdmEngine::HandleProvisioningResponse: Empty provisioning response.");
    cert_provisioning_.reset(NULL);
    return UNKNOWN_ERROR;
  }
  if (NULL == cert) {
    LOGE("CdmEngine::HandleProvisioningResponse: invalid certificate "
         "destination");
    cert_provisioning_.reset(NULL);
    return UNKNOWN_ERROR;
  }
  if (NULL == wrapped_key) {
    LOGE("CdmEngine::HandleProvisioningResponse: invalid wrapped key "
         "destination");
    cert_provisioning_.reset(NULL);
    return UNKNOWN_ERROR;
  }
  if (NULL == cert_provisioning_.get()) {
    LOGE("CdmEngine::HandleProvisioningResponse: provisioning object missing.");
    return UNKNOWN_ERROR;
  }
  CdmResponseType ret =
      cert_provisioning_->HandleProvisioningResponse(response, cert,
                                                     wrapped_key);
  cert_provisioning_.reset(NULL);  // Release resources.
  return ret;
}

CdmResponseType CdmEngine::Unprovision(CdmSecurityLevel security_level) {
  DeviceFiles handle;
  if (!handle.Init(security_level)) {
    LOGE("CdmEngine::Unprovision: unable to initialize device files");
    return UNKNOWN_ERROR;
  }

  if (!handle.DeleteAllFiles()) {
    LOGE("CdmEngine::Unprovision: unable to delete files");
    return UNKNOWN_ERROR;
  }

  scoped_ptr<CryptoSession> crypto_session(new CryptoSession());
  CdmResponseType status = crypto_session->Open(
      security_level == kSecurityLevelL3 ? kLevel3 : kLevelDefault);
  if (NO_ERROR != status) {
    LOGE("CdmEngine::Unprovision: error opening crypto session: %d", status);
    return UNKNOWN_ERROR;
  }
  status = crypto_session->DeleteAllUsageReports();
  if (status != NO_ERROR) {
    LOGE("CdmEngine::Unprovision: error deleteing usage reports: %d", status);
  }
  return status;
}

CdmResponseType CdmEngine::GetUsageInfo(const std::string& app_id,
                                        const CdmSecureStopId& ssid,
                                        CdmUsageInfo* usage_info) {
  if (NULL == usage_property_set_.get()) {
    usage_property_set_.reset(new UsagePropertySet());
  }
  usage_property_set_->set_security_level(kLevelDefault);
  usage_property_set_->set_app_id(app_id);
  usage_session_.reset(new CdmSession(usage_property_set_.get()));
  CdmResponseType status = usage_session_->Init();
  if (NO_ERROR != status) {
    LOGE("CdmEngine::GetUsageInfo: session init error");
    return status;
  }
  DeviceFiles handle;
  if (!handle.Init(usage_session_->GetSecurityLevel())) {
    LOGE("CdmEngine::GetUsageInfo: device file init error");
    return UNKNOWN_ERROR;
  }

  CdmKeyMessage license_request;
  CdmKeyResponse license_response;
  if (!handle.RetrieveUsageInfo(app_id, ssid, &license_request,
                                &license_response)) {
    usage_property_set_->set_security_level(kLevel3);
    usage_property_set_->set_app_id(app_id);
    usage_session_.reset(new CdmSession(usage_property_set_.get()));
    status = usage_session_->Init();
    if (NO_ERROR != status) {
      LOGE("CdmEngine::GetUsageInfo: session init error");
      return status;
    }
    if (!handle.Reset(usage_session_->GetSecurityLevel())) {
      LOGE("CdmEngine::GetUsageInfo: device file init error");
      return UNKNOWN_ERROR;
    }
    if (!handle.RetrieveUsageInfo(app_id, ssid, &license_request,
                                  &license_response)) {
      // No entry found for that ssid.
      return UNKNOWN_ERROR;
    }
  }

  std::string server_url;
  usage_info->resize(1);
  status =
      usage_session_->RestoreUsageSession(license_request, license_response);
  if (KEY_ADDED != status) {
    LOGE("CdmEngine::GetUsageInfo: restore usage session error %d", status);
    usage_info->clear();
    return status;
  }

  status =
      usage_session_->GenerateReleaseRequest(&(*usage_info)[0], &server_url);

  if (KEY_MESSAGE != status) {
    LOGE("CdmEngine::GetUsageInfo: generate release request error: %d", status);
    usage_info->clear();
    return status;
  }
  return KEY_MESSAGE;
}

CdmResponseType CdmEngine::GetUsageInfo(const std::string& app_id,
                                        CdmUsageInfo* usage_info) {
  // Return a random usage report from a random security level
  SecurityLevel security_level = ((rand() % 2) == 0) ? kLevelDefault : kLevel3;
  CdmResponseType status = UNKNOWN_ERROR;
  do {
    status = GetUsageInfo(app_id, security_level, usage_info);

    if (KEY_MESSAGE == status && !usage_info->empty())
      return status;
  } while (KEY_CANCELED == status);


  security_level = (kLevel3 == security_level) ? kLevelDefault : kLevel3;
  do {
    status = GetUsageInfo(app_id, security_level, usage_info);
    if (NEED_PROVISIONING == status)
      return NO_ERROR;  // Valid scenario that one of the security
                        // levels has not been provisioned
  } while (KEY_CANCELED == status);
  return status;
}

CdmResponseType CdmEngine::GetUsageInfo(const std::string& app_id,
                                        SecurityLevel requested_security_level,
                                        CdmUsageInfo* usage_info) {
  if (NULL == usage_property_set_.get()) {
    usage_property_set_.reset(new UsagePropertySet());
  }
  usage_property_set_->set_security_level(requested_security_level);
  usage_property_set_->set_app_id(app_id);

  usage_session_.reset(new CdmSession(usage_property_set_.get()));

  CdmResponseType status = usage_session_->Init();
  if (NO_ERROR != status) {
    LOGE("CdmEngine::GetUsageInfo: session init error");
    return status;
  }

  DeviceFiles handle;
  if (!handle.Init(usage_session_->GetSecurityLevel())) {
    LOGE("CdmEngine::GetUsageInfo: unable to initialize device files");
    return UNKNOWN_ERROR;
  }

  std::vector<std::pair<CdmKeyMessage, CdmKeyResponse> > license_info;
  if (!handle.RetrieveUsageInfo(app_id, &license_info)) {
    LOGE("CdmEngine::GetUsageInfo: unable to read usage information");
    return UNKNOWN_ERROR;
  }

  if (0 == license_info.size()) {
    usage_info->resize(0);
    return NO_ERROR;
  }

  std::string server_url;
  usage_info->resize(kUsageReportsPerRequest);

  uint32_t index = rand() % license_info.size();
  status = usage_session_->RestoreUsageSession(license_info[index].first,
                                               license_info[index].second);
  if (KEY_ADDED != status) {
    LOGE("CdmEngine::GetUsageInfo: restore usage session (%u) error %d",
         index, status);
    usage_info->clear();
    return status;
  }

  status = usage_session_->GenerateReleaseRequest(&(*usage_info)[0], &server_url);

  switch (status) {
    case KEY_MESSAGE:
      break;
    case KEY_CANCELED:                  // usage information not present in
      usage_session_->DeleteLicense();  // OEMCrypto, delete and try again
      usage_info->clear();
      break;
    default:
      LOGE("CdmEngine::GetUsageInfo: generate release request error: %d",
           status);
      usage_info->clear();
      break;
  }
  return status;
}

CdmResponseType CdmEngine::ReleaseAllUsageInfo(const std::string& app_id) {
  CdmResponseType status = NO_ERROR;
  DeviceFiles handle[kSecurityLevelUnknown - kSecurityLevelL1];
  for (int i = 0, j = kSecurityLevelL1; j < kSecurityLevelUnknown; ++i, ++j) {
    if (handle[i].Init(static_cast<CdmSecurityLevel>(j))) {
      if (!handle[i].DeleteAllUsageInfoForApp(app_id)) {
        LOGE("CdmEngine::ReleaseAllUsageInfo: failed to delete L%d secure stops", j);
        status = UNKNOWN_ERROR;
      }
    } else {
      LOGE("CdmEngine::ReleaseAllUsageInfo: failed to initialize L%d device files", j);
      status = UNKNOWN_ERROR;
    }
  }
  return status;
}

CdmResponseType CdmEngine::ReleaseUsageInfo(
    const CdmUsageInfoReleaseMessage& message) {
  if (NULL == usage_session_.get()) {
    LOGE("CdmEngine::ReleaseUsageInfo: cdm session not initialized");
    return UNKNOWN_ERROR;
  }

  CdmResponseType status = usage_session_->ReleaseKey(message);
  usage_session_.reset(NULL);
  if (NO_ERROR != status) {
    LOGE("CdmEngine::ReleaseUsageInfo: release key error: %d", status);
  }
  return status;
}

CdmResponseType CdmEngine::Decrypt(
    const CdmSessionId& session_id,
    const CdmDecryptionParameters& parameters) {
  if (parameters.key_id == NULL) {
    LOGE("CdmEngine::Decrypt: no key_id");
    return KEY_ERROR;
  }

  if (parameters.encrypt_buffer == NULL) {
    LOGE("CdmEngine::Decrypt: no src encrypt buffer");
    return KEY_ERROR;
  }

  if (parameters.iv == NULL) {
    LOGE("CdmEngine::Decrypt: no iv");
    return KEY_ERROR;
  }

  if (parameters.decrypt_buffer == NULL) {
    if (!parameters.is_secure &&
        !Properties::Properties::oem_crypto_use_fifo()) {
      LOGE("CdmEngine::Decrypt: no dest decrypt buffer");
      return KEY_ERROR;
    } // else we must be level 1 direct and we don't need to return a buffer.
  }

  CdmSessionMap::iterator iter;
  if (session_id.empty()) {
    // Loop through the sessions to find the session containing the key_id.
    for (iter = sessions_.begin(); iter != sessions_.end(); ++iter) {
      if (iter->second->IsKeyLoaded(*parameters.key_id)) {
        break;
      }
    }
  } else {
    iter = sessions_.find(session_id);
  }
  if (iter == sessions_.end()) {
    LOGE("CdmEngine::Decrypt: session not found: id=%s, id size=%d",
         session_id.c_str(),
         session_id.size());
    return KEY_ERROR;
  }

  return iter->second->Decrypt(parameters);
}

bool CdmEngine::IsKeyLoaded(const KeyId& key_id) {
  for (CdmSessionMap::iterator iter = sessions_.begin();
       iter != sessions_.end(); ++iter) {
    if (iter->second->IsKeyLoaded(key_id)) {
      return true;
    }
  }
  return false;
}

bool CdmEngine::FindSessionForKey(
    const KeyId& key_id,
    CdmSessionId* session_id) {
  if (NULL == session_id) {
    LOGE("CdmEngine::FindSessionForKey: session id not provided");
    return false;
  }

  CdmSessionMap::iterator iter = sessions_.find(*session_id);
  if (iter != sessions_.end()) {
    if (iter->second->IsKeyLoaded(key_id)) {
      return true;
    }
  }

  uint32_t session_sharing_id = Properties::GetSessionSharingId(*session_id);

  for (iter = sessions_.begin(); iter != sessions_.end(); ++iter) {
    CdmSessionId local_session_id = iter->second->session_id();
    if (Properties::GetSessionSharingId(local_session_id) ==
        session_sharing_id) {
      if (iter->second->IsKeyLoaded(key_id)) {
        *session_id = local_session_id;
        return true;
      }
    }
  }
  return false;
}

bool CdmEngine::AttachEventListener(
    const CdmSessionId& session_id,
    WvCdmEventListener* listener) {
  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    return false;
  }

  return iter->second->AttachEventListener(listener);
}

bool CdmEngine::DetachEventListener(
    const CdmSessionId& session_id,
    WvCdmEventListener* listener) {
  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter == sessions_.end()) {
    return false;
  }

  return iter->second->DetachEventListener(listener);
}

void CdmEngine::NotifyResolution(const CdmSessionId& session_id, uint32_t width,
                                 uint32_t height) {
  CdmSessionMap::iterator iter = sessions_.find(session_id);
  if (iter != sessions_.end()) {
    iter->second->NotifyResolution(width, height);
  }
}

bool CdmEngine::ValidateKeySystem(const CdmKeySystem& key_system) {
  return (key_system.find("widevine") != std::string::npos);
}

void CdmEngine::OnTimerEvent() {
  Clock clock;
  uint64_t current_time = clock.GetCurrentTime();
  bool usage_update_period_expired = false;

  if (current_time - last_usage_information_update_time_ >
      kUpdateUsageInformationPeriod) {
    usage_update_period_expired = true;
    last_usage_information_update_time_ = current_time;
  }

  bool is_initial_usage_update = false;
  bool is_usage_update_needed = false;

  // Make a copy of all currently open sessions.  While this event handler is
  // active, after we release session_list_lock_, CloseSession and OpenSession
  // might modify sessions_, but they will not delete sessions.
  session_list_lock_.Acquire();
  const size_t session_count = sessions_.size();
  CdmSession* event_sessions[session_count];
  timer_event_active_ = true;
  size_t index = 0;
  for (CdmSessionMap::iterator iter = sessions_.begin();
       iter != sessions_.end() && index < session_count; ++iter) {
    is_initial_usage_update = is_initial_usage_update ||
        iter->second->is_initial_usage_update();
    is_usage_update_needed = is_usage_update_needed ||
        iter->second->is_usage_update_needed();
    event_sessions[index++] = iter->second;
  }
  session_list_lock_.Release();


  for (size_t i=0; i < session_count; i++) {
    event_sessions[i]->OnTimerEvent(usage_update_period_expired);
  }

  if (is_usage_update_needed &&
      (usage_update_period_expired || is_initial_usage_update)) {
    bool has_usage_been_updated = false;
    for (size_t i=0; i < session_count; i++) {
      event_sessions[i]->reset_usage_flags();
      if (!has_usage_been_updated) {
        // usage is updated for all sessions so this needs to be
        // called only once per update usage information period
        CdmResponseType status = event_sessions[i]->UpdateUsageInformation();
        if (NO_ERROR != status) {
          LOGW("Update usage information failed: %d", status);
        } else {
          has_usage_been_updated = true;
        }
      }
    }
  }
  {
    // We are done with the event handler.  Now we look for any sessions that
    // were closed.
    AutoLock lock(session_list_lock_);
    for (size_t i=0; i < dead_sessions_.size(); i++) {
      delete dead_sessions_[i];
    }
    dead_sessions_.clear();
    timer_event_active_ = false;
  }
}

void CdmEngine::OnKeyReleaseEvent(const CdmKeySetId& key_set_id) {

  for (CdmSessionMap::iterator iter = sessions_.begin();
       iter != sessions_.end(); ++iter) {
    iter->second->OnKeyReleaseEvent(key_set_id);
  }
}

std::string CdmEngine::MapHdcpVersion(
    CryptoSession::OemCryptoHdcpVersion version) {
  switch (version) {
    case CryptoSession::kOemCryptoNoHdcpDeviceAttached:
      return QUERY_VALUE_DISCONNECTED;
    case CryptoSession::kOemCryptoHdcpNotSupported:
      return QUERY_VALUE_UNPROTECTED;
    case CryptoSession::kOemCryptoHdcpVersion1:
      return QUERY_VALUE_HDCP_V1;
    case CryptoSession::kOemCryptoHdcpVersion2:
      return QUERY_VALUE_HDCP_V2_0;
    case CryptoSession::kOemCryptoHdcpVersion2_1:
      return QUERY_VALUE_HDCP_V2_1;
    case CryptoSession::kOemCryptoHdcpVersion2_2:
      return QUERY_VALUE_HDCP_V2_2;
  }
  return "";
}

}  // namespace wvcdm
