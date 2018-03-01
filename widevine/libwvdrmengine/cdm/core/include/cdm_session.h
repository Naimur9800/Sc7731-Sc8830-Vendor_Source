// Copyright 2012 Google Inc. All Rights Reserved.

#ifndef WVCDM_CORE_CDM_SESSION_H_
#define WVCDM_CORE_CDM_SESSION_H_

#include <set>

#include "crypto_session.h"
#include "device_files.h"
#include "initialization_data.h"
#include "license.h"
#include "oemcrypto_adapter.h"
#include "policy_engine.h"
#include "scoped_ptr.h"
#include "wv_cdm_types.h"

namespace wvcdm {

class CdmClientPropertySet;
class WvCdmEventListener;

class CdmSession {
 public:
  explicit CdmSession(const CdmClientPropertySet* cdm_client_property_set);
  virtual ~CdmSession();

  virtual CdmResponseType Init();

  virtual CdmResponseType RestoreOfflineSession(
      const CdmKeySetId& key_set_id, const CdmLicenseType license_type);
  virtual CdmResponseType RestoreUsageSession(
      const CdmKeyMessage& key_request, const CdmKeyResponse& key_response);

  virtual const CdmSessionId& session_id() { return session_id_; }

  virtual CdmResponseType GenerateKeyRequest(
      const InitializationData& init_data, const CdmLicenseType license_type,
      const CdmAppParameterMap& app_parameters, CdmKeyMessage* key_request,
      std::string* server_url);

  // AddKey() - Accept license response and extract key info.
  virtual CdmResponseType AddKey(const CdmKeyResponse& key_response,
                                 CdmKeySetId* key_set_id);

  // CancelKeyRequest() - Cancel session.
  virtual CdmResponseType CancelKeyRequest();

  // Query session status
  virtual CdmResponseType QueryStatus(CdmQueryMap* key_info);

  // Query license information
  virtual CdmResponseType QueryKeyStatus(CdmQueryMap* key_info);

  // Query session control info
  virtual CdmResponseType QueryKeyControlInfo(CdmQueryMap* key_info);

  // Decrypt() - Accept encrypted buffer and return decrypted data.
  virtual CdmResponseType Decrypt(const CdmDecryptionParameters& parameters);

  // License renewal
  // GenerateRenewalRequest() - Construct valid renewal request for the current
  // session keys.
  virtual CdmResponseType GenerateRenewalRequest(CdmKeyMessage* key_request,
                                                 std::string* server_url);

  // RenewKey() - Accept renewal response and update key info.
  virtual CdmResponseType RenewKey(const CdmKeyResponse& key_response);

  // License release
  // GenerateReleaseRequest() - Construct valid release request for the current
  // session keys.
  virtual CdmResponseType GenerateReleaseRequest(CdmKeyMessage* key_request,
                                                 std::string* server_url);

  // ReleaseKey() - Accept response and release key.
  virtual CdmResponseType ReleaseKey(const CdmKeyResponse& key_response);

  virtual bool IsKeyLoaded(const KeyId& key_id);

  virtual bool AttachEventListener(WvCdmEventListener* listener);
  virtual bool DetachEventListener(WvCdmEventListener* listener);

  // Used for notifying the Policy Engine of resolution changes
  virtual void NotifyResolution(uint32_t width, uint32_t height);

  virtual void OnTimerEvent(bool update_usage);
  virtual void OnKeyReleaseEvent(const CdmKeySetId& key_set_id);

  virtual void GetApplicationId(std::string* app_id);
  virtual SecurityLevel GetRequestedSecurityLevel();
  virtual CdmSecurityLevel GetSecurityLevel();

  virtual CdmResponseType DeleteUsageInformation(const std::string& app_id);
  virtual CdmResponseType UpdateUsageInformation();

  virtual bool is_initial_usage_update() { return is_initial_usage_update_; }
  virtual bool is_usage_update_needed() { return is_usage_update_needed_; }
  virtual void reset_usage_flags() {
    is_initial_usage_update_ = false;
    is_usage_update_needed_ = false;
  }

  bool DeleteLicense();

 private:
  // Internal constructor
  void Create(CdmLicense* license_parser, CryptoSession* crypto_session,
              PolicyEngine* policy_engine, DeviceFiles* file_handle,
              const CdmClientPropertySet* cdm_client_property_set);

  // Generate unique ID for each new session.
  CdmSessionId GenerateSessionId();
  bool GenerateKeySetId(CdmKeySetId* key_set_id);

  CdmResponseType StoreLicense();
  bool StoreLicense(DeviceFiles::LicenseState state);

  // instance variables
  bool initialized_;
  CdmSessionId session_id_;
  scoped_ptr<CdmLicense> license_parser_;
  scoped_ptr<CryptoSession> crypto_session_;
  scoped_ptr<PolicyEngine> policy_engine_;
  scoped_ptr<DeviceFiles> file_handle_;
  bool license_received_;
  bool is_offline_;
  bool is_release_;
  CdmSecurityLevel security_level_;
  std::string app_id_;

  // decryption and usage flags
  bool is_initial_decryption_;
  bool has_decrypted_recently_;
  bool is_initial_usage_update_;
  bool is_usage_update_needed_;

  // information useful for offline and usage scenarios
  CdmKeyMessage key_request_;
  CdmKeyResponse key_response_;

  // license type offline related information
  CdmInitData offline_init_data_;
  CdmKeyMessage offline_key_renewal_request_;
  CdmKeyResponse offline_key_renewal_response_;
  std::string offline_release_server_url_;

  // license type release and offline related information
  CdmKeySetId key_set_id_;

  std::set<WvCdmEventListener*> listeners_;

  // For testing only
  // Takes ownership of license_parser, crypto_session, policy_engine
  // and device_files
  CdmSession(CdmLicense* license_parser, CryptoSession* crypto_session,
             PolicyEngine* policy_engine, DeviceFiles* file_handle,
             const CdmClientPropertySet* cdm_client_property_set);
#if defined(UNIT_TEST)
  friend class CdmSessionTest;
#endif

  CORE_DISALLOW_COPY_AND_ASSIGN(CdmSession);
};

}  // namespace wvcdm

#endif  // WVCDM_CORE_CDM_SESSION_H_
