// Copyright 2013 Google Inc. All Rights Reserved.

#include "wv_content_decryption_module.h"

#include <iostream>

#include "cdm_client_property_set.h"
#include "cdm_engine.h"
#include "initialization_data.h"
#include "log.h"
#include "properties.h"
#include "wv_cdm_constants.h"
#include "wv_cdm_event_listener.h"

namespace {
const int kCdmPolicyTimerDurationSeconds = 1;
}

namespace wvcdm {

Lock WvContentDecryptionModule::session_sharing_id_generation_lock_;

WvContentDecryptionModule::WvContentDecryptionModule()
    : cdm_engine_(new CdmEngine()) {}

WvContentDecryptionModule::~WvContentDecryptionModule() {
  DisablePolicyTimer(true);
}

bool WvContentDecryptionModule::IsSupported(const std::string& init_data_type) {
  return InitializationData(init_data_type).is_supported();
}

bool WvContentDecryptionModule::IsCenc(const std::string& init_data_type) {
  return InitializationData(init_data_type).is_cenc();
}

bool WvContentDecryptionModule::IsWebm(const std::string& init_data_type) {
  return InitializationData(init_data_type).is_webm();
}

CdmResponseType WvContentDecryptionModule::OpenSession(
    const CdmKeySystem& key_system,
    CdmClientPropertySet* property_set,
    CdmSessionId* session_id) {
  if (property_set && property_set->is_session_sharing_enabled()) {
    AutoLock auto_lock(session_sharing_id_generation_lock_);
    if (property_set->session_sharing_id() == 0)
      property_set->set_session_sharing_id(GenerateSessionSharingId());
  }

  return cdm_engine_->OpenSession(key_system, property_set, session_id);
}

CdmResponseType WvContentDecryptionModule::CloseSession(
    const CdmSessionId& session_id) {
  CdmResponseType sts = cdm_engine_->CloseSession(session_id);
  DisablePolicyTimer(false);
  return sts;
}

CdmResponseType WvContentDecryptionModule::GenerateKeyRequest(
    const CdmSessionId& session_id,
    const CdmKeySetId& key_set_id,
    const std::string& init_data_type,
    const CdmInitData& init_data,
    const CdmLicenseType license_type,
    CdmAppParameterMap& app_parameters,
    CdmClientPropertySet* property_set,
    CdmKeyMessage* key_request,
    std::string* server_url) {
  CdmResponseType sts;
  if (license_type == kLicenseTypeRelease) {
      sts = cdm_engine_->OpenKeySetSession(key_set_id, property_set);
      if (sts != NO_ERROR)
        return sts;
  }
  InitializationData initialization_data(init_data_type, init_data);
  sts = cdm_engine_->GenerateKeyRequest(session_id, key_set_id,
                                        initialization_data, license_type,
                                        app_parameters, key_request,
                                        server_url);

  switch(license_type) {
    case kLicenseTypeRelease:
      if (sts != KEY_MESSAGE)
        cdm_engine_->CloseKeySetSession(key_set_id);
      break;
    default:
      if (sts == KEY_MESSAGE)
        EnablePolicyTimer();
      break;
  }
  return sts;
}

CdmResponseType WvContentDecryptionModule::AddKey(
    const CdmSessionId& session_id,
    const CdmKeyResponse& key_data,
    CdmKeySetId* key_set_id) {
  CdmResponseType sts = cdm_engine_->AddKey(session_id, key_data, key_set_id);
  if (sts == KEY_ADDED && session_id.empty())   // license type release
    cdm_engine_->CloseKeySetSession(*key_set_id);
  return sts;
}

CdmResponseType WvContentDecryptionModule::RestoreKey(
    const CdmSessionId& session_id,
    const CdmKeySetId& key_set_id) {
  CdmResponseType sts = cdm_engine_->RestoreKey(session_id, key_set_id);
  if (sts == KEY_ADDED)
    EnablePolicyTimer();
  return sts;
}

CdmResponseType WvContentDecryptionModule::CancelKeyRequest(
    const CdmSessionId& session_id) {
  return cdm_engine_->CancelKeyRequest(session_id);
}

CdmResponseType WvContentDecryptionModule::QueryStatus(CdmQueryMap* key_info) {
  return cdm_engine_->QueryStatus(key_info);
}

CdmResponseType WvContentDecryptionModule::QuerySessionStatus(
    const CdmSessionId& session_id, CdmQueryMap* key_info) {
  return cdm_engine_->QuerySessionStatus(session_id, key_info);
}

CdmResponseType WvContentDecryptionModule::QueryKeyStatus(
    const CdmSessionId& session_id, CdmQueryMap* key_info) {
  return cdm_engine_->QueryKeyStatus(session_id, key_info);
}

CdmResponseType WvContentDecryptionModule::QueryKeyControlInfo(
    const CdmSessionId& session_id, CdmQueryMap* key_info) {
  return cdm_engine_->QueryKeyControlInfo(session_id, key_info);
}

CdmResponseType WvContentDecryptionModule::GetProvisioningRequest(
    CdmCertificateType cert_type,
    const std::string& cert_authority,
    CdmProvisioningRequest* request,
    std::string* default_url) {
  return cdm_engine_->GetProvisioningRequest(cert_type, cert_authority,
                                             request, default_url);
}

CdmResponseType WvContentDecryptionModule::HandleProvisioningResponse(
    CdmProvisioningResponse& response,
    std::string* cert,
    std::string* wrapped_key) {
  return cdm_engine_->HandleProvisioningResponse(response, cert, wrapped_key);
}

CdmResponseType WvContentDecryptionModule::Unprovision(
    CdmSecurityLevel level) {
  return cdm_engine_->Unprovision(level);
}

CdmResponseType WvContentDecryptionModule::GetUsageInfo(
    const std::string& app_id, CdmUsageInfo* usage_info) {
  return cdm_engine_->GetUsageInfo(app_id, usage_info);
}

CdmResponseType WvContentDecryptionModule::GetUsageInfo(
    const std::string& app_id,
    const CdmSecureStopId& ssid,
    CdmUsageInfo* usage_info) {
  return cdm_engine_->GetUsageInfo(app_id, ssid, usage_info);
}

CdmResponseType WvContentDecryptionModule::ReleaseAllUsageInfo(
    const std::string& app_id) {
  return cdm_engine_->ReleaseAllUsageInfo(app_id);
}

CdmResponseType WvContentDecryptionModule::ReleaseUsageInfo(
    const CdmUsageInfoReleaseMessage& message) {
  return cdm_engine_->ReleaseUsageInfo(message);
}

CdmResponseType WvContentDecryptionModule::Decrypt(
    const CdmSessionId& session_id,
    bool validate_key_id,
    const CdmDecryptionParameters& parameters) {
  CdmSessionId local_session_id = session_id;
  if (validate_key_id &&
      Properties::GetSessionSharingId(session_id) != 0) {
    bool status = cdm_engine_->FindSessionForKey(*parameters.key_id,
                                                 &local_session_id);
    if (!status) {
      LOGE("WvContentDecryptionModule::Decrypt: unable to find session");
      return NEED_KEY;
    }
  }
  return cdm_engine_->Decrypt(local_session_id, parameters);
}

bool WvContentDecryptionModule::AttachEventListener(
    const CdmSessionId& session_id, WvCdmEventListener* listener) {
  return cdm_engine_->AttachEventListener(session_id, listener);
}

bool WvContentDecryptionModule::DetachEventListener(
    const CdmSessionId& session_id, WvCdmEventListener* listener) {
  return cdm_engine_->DetachEventListener(session_id, listener);
}

void WvContentDecryptionModule::NotifyResolution(const CdmSessionId& session_id,
                                                 uint32_t width,
                                                 uint32_t height) {
  cdm_engine_->NotifyResolution(session_id, width, height);
}

void WvContentDecryptionModule::EnablePolicyTimer() {
  if (!policy_timer_.IsRunning())
    policy_timer_.Start(this, kCdmPolicyTimerDurationSeconds);
}

void WvContentDecryptionModule::DisablePolicyTimer(bool force) {
  if ((cdm_engine_->SessionSize() == 0 || force) && policy_timer_.IsRunning())
    policy_timer_.Stop();
}

void WvContentDecryptionModule::OnTimerEvent() {
  cdm_engine_->OnTimerEvent();
}

uint32_t WvContentDecryptionModule::GenerateSessionSharingId() {
  static int next_session_sharing_id = 0;
  return ++next_session_sharing_id;
}



}  // namespace wvcdm
