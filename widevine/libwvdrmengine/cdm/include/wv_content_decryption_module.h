// Copyright 2013 Google Inc. All Rights Reserved.

#ifndef CDM_BASE_WV_CONTENT_DECRYPTION_MODULE_H_
#define CDM_BASE_WV_CONTENT_DECRYPTION_MODULE_H_

#include <UniquePtr.h>

#include "lock.h"
#include "timer.h"
#include "wv_cdm_types.h"

namespace wvcdm {

class CdmClientPropertySet;
class CdmEngine;
class WvCdmEventListener;

class WvContentDecryptionModule : public TimerHandler {
 public:
  WvContentDecryptionModule();
  virtual ~WvContentDecryptionModule();

  // Static methods
  static bool IsSupported(const std::string& init_data_type);
  static bool IsCenc(const std::string& init_data_type);
  static bool IsWebm(const std::string& init_data_type);

  // Session related methods
  virtual CdmResponseType OpenSession(
      const CdmKeySystem& key_system,
      CdmClientPropertySet* property_set,
      CdmSessionId* session_id);
  virtual CdmResponseType CloseSession(const CdmSessionId& session_id);

  // Construct a valid license request.
  virtual CdmResponseType GenerateKeyRequest(const CdmSessionId& session_id,
                                             const CdmKeySetId& key_set_id,
                                             const std::string& init_data_type,
                                             const CdmInitData& init_data,
                                             const CdmLicenseType license_type,
                                             CdmAppParameterMap& app_parameters,
                                             CdmClientPropertySet* property_set,
                                             CdmKeyMessage* key_request,
                                             std::string* server_url);

  // Accept license response and extract key info.
  virtual CdmResponseType AddKey(const CdmSessionId& session_id,
                                 const CdmKeyResponse& key_data,
                                 CdmKeySetId* key_set_id);

  // Setup keys for offline usage which were retrived in an earlier key request
  virtual CdmResponseType RestoreKey(const CdmSessionId& session_id,
                                     const CdmKeySetId& key_set_id);

  // Cancel session
  virtual CdmResponseType CancelKeyRequest(const CdmSessionId& session_id);

  // Query system information
  virtual CdmResponseType QueryStatus(CdmQueryMap* key_info);

  // Query session information
  virtual CdmResponseType QuerySessionStatus(const CdmSessionId& session_id,
                                             CdmQueryMap* key_info);

  // Query license information
  virtual CdmResponseType QueryKeyStatus(const CdmSessionId& session_id,
                                         CdmQueryMap* key_info);

  // Query session control information
  virtual CdmResponseType QueryKeyControlInfo(const CdmSessionId& session_id,
                                              CdmQueryMap* key_info);

  // Provisioning related methods
  virtual CdmResponseType GetProvisioningRequest(
      CdmCertificateType cert_type,
      const std::string& cert_authority,
      CdmProvisioningRequest* request,
      std::string* default_url);

  virtual CdmResponseType HandleProvisioningResponse(
      CdmProvisioningResponse& response,
      std::string* cert,
      std::string* wrapped_key);

  virtual CdmResponseType Unprovision(CdmSecurityLevel level);

  // Secure stop related methods
  virtual CdmResponseType GetUsageInfo(const std::string& app_id,
                                       CdmUsageInfo* usage_info);
  virtual CdmResponseType GetUsageInfo(const std::string& app_id,
                                       const CdmSecureStopId& ssid,
                                       CdmUsageInfo* usage_info);
  virtual CdmResponseType ReleaseAllUsageInfo(const std::string& app_id);
  virtual CdmResponseType ReleaseUsageInfo(
      const CdmUsageInfoReleaseMessage& message);

  // Accept encrypted buffer and decrypt data.
  // Decryption parameters that need to be specified are
  // is_encrypted, is_secure, key_id, encrypt_buffer, encrypt_length,
  // iv, block_offset, decrypt_buffer, decrypt_buffer_length,
  // decrypt_buffer_offset and subsample_flags
  virtual CdmResponseType Decrypt(const CdmSessionId& session_id,
                                  bool validate_key_id,
                                  const CdmDecryptionParameters& parameters);

  // Event listener related methods
  virtual bool AttachEventListener(const CdmSessionId& session_id,
                                   WvCdmEventListener* listener);
  virtual bool DetachEventListener(const CdmSessionId& session_id,
                                   WvCdmEventListener* listener);

  virtual void NotifyResolution(const CdmSessionId& session_id, uint32_t width,
                                uint32_t height);

 private:
  uint32_t GenerateSessionSharingId();

  // timer related methods to drive policy decisions
  void EnablePolicyTimer();
  void DisablePolicyTimer(bool force);
  void OnTimerEvent();

  static Lock session_sharing_id_generation_lock_;
  Timer policy_timer_;

  // instance variables
  UniquePtr<CdmEngine> cdm_engine_;

  CORE_DISALLOW_COPY_AND_ASSIGN(WvContentDecryptionModule);
};

}  // namespace wvcdm

#endif  // CDM_BASE_WV_CONTENT_DECRYPTION_MODULE_H_
