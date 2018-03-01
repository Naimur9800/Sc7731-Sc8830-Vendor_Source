// Copyright 2013 Google Inc. All Rights Reserved.

#ifndef WVCDM_CORE_CDM_ENGINE_H_
#define WVCDM_CORE_CDM_ENGINE_H_

#include <string>

#include "cdm_session.h"
#include "certificate_provisioning.h"
#include "crypto_session.h"
#include "initialization_data.h"
#include "lock.h"
#include "oemcrypto_adapter.h"
#include "scoped_ptr.h"
#include "wv_cdm_types.h"

namespace wvcdm {

class CdmClientPropertySet;
class CryptoEngine;
class UsagePropertySet;
class WvCdmEventListener;

typedef std::map<CdmSessionId, CdmSession*> CdmSessionMap;
typedef std::map<CdmKeySetId, CdmSessionId> CdmReleaseKeySetMap;

class CdmEngine {
 public:
  CdmEngine();
  virtual ~CdmEngine();

  // Session related methods
  virtual CdmResponseType OpenSession(const CdmKeySystem& key_system,
                                      const CdmClientPropertySet* property_set,
                                      CdmSessionId* session_id);
  virtual CdmResponseType CloseSession(const CdmSessionId& session_id);

  virtual CdmResponseType OpenKeySetSession(
      const CdmKeySetId& key_set_id,
      const CdmClientPropertySet* property_set);
  virtual CdmResponseType CloseKeySetSession(const CdmKeySetId& key_set_id);

  // License related methods
  // Construct a valid license request
  virtual CdmResponseType GenerateKeyRequest(
      const CdmSessionId& session_id, const CdmKeySetId& key_set_id,
      const InitializationData& init_data, const CdmLicenseType license_type,
      CdmAppParameterMap& app_parameters, CdmKeyMessage* key_request,
      std::string* server_url);

  // Accept license response and extract key info.
  virtual CdmResponseType AddKey(const CdmSessionId& session_id,
                                 const CdmKeyResponse& key_data,
                                 CdmKeySetId* key_set_id);

  virtual CdmResponseType RestoreKey(const CdmSessionId& session_id,
                                     const CdmKeySetId& key_set_id);

  virtual CdmResponseType CancelKeyRequest(const CdmSessionId& session_id);

  // Construct valid renewal request for the current session keys.
  virtual CdmResponseType GenerateRenewalRequest(const CdmSessionId& session_id,
                                                 CdmKeyMessage* key_request,
                                                 std::string* server_url);

  // Accept renewal response and update key info.
  virtual CdmResponseType RenewKey(const CdmSessionId& session_id,
                                   const CdmKeyResponse& key_data);

  // Query system information
  virtual CdmResponseType QueryStatus(CdmQueryMap* info);

  // Query session information
  virtual CdmResponseType QuerySessionStatus(const CdmSessionId& session_id,
                                             CdmQueryMap* key_info);

  // Query license information
  virtual CdmResponseType QueryKeyStatus(const CdmSessionId& session_id,
                                         CdmQueryMap* key_info);

  // Query seesion control information
  virtual CdmResponseType QueryKeyControlInfo(const CdmSessionId& session_id,
                                              CdmQueryMap* key_info);

  // Provisioning related methods
  virtual CdmResponseType GetProvisioningRequest(
      CdmCertificateType cert_type, const std::string& cert_authority,
      CdmProvisioningRequest* request, std::string* default_url);

  virtual CdmResponseType HandleProvisioningResponse(
      CdmProvisioningResponse& response, std::string* cert,
      std::string* wrapped_key);

  virtual CdmResponseType Unprovision(CdmSecurityLevel security_level);

  // Usage related methods for streaming licenses
  // Retrieve a random usage info from the list of all usage infos for this app
  // id.
  virtual CdmResponseType GetUsageInfo(const std::string& app_id,
                                       CdmUsageInfo* usage_info);
  // Retrieve the usage info for the specified pst.
  // Returns UNKNOWN_ERROR if no usage info was found.
  virtual CdmResponseType GetUsageInfo(const std::string& app_id,
                                       const CdmSecureStopId& ssid,
                                       CdmUsageInfo* usage_info);
  virtual CdmResponseType ReleaseAllUsageInfo(const std::string& app_id);
  virtual CdmResponseType ReleaseUsageInfo(
      const CdmUsageInfoReleaseMessage& message);

  // Decryption and key related methods
  // Accept encrypted buffer and return decrypted data.
  virtual CdmResponseType Decrypt(const CdmSessionId& session_id,
                                  const CdmDecryptionParameters& parameters);

  virtual size_t SessionSize() const { return sessions_.size(); }

  // Is the key known to any session?
  virtual bool IsKeyLoaded(const KeyId& key_id);
  virtual bool FindSessionForKey(const KeyId& key_id, CdmSessionId* sessionId);

  // Event listener related methods
  virtual bool AttachEventListener(const CdmSessionId& session_id,
                                   WvCdmEventListener* listener);
  virtual bool DetachEventListener(const CdmSessionId& session_id,
                                   WvCdmEventListener* listener);

  // Used for notifying the Max-Res Engine of resolution changes
  virtual void NotifyResolution(const CdmSessionId& session_id, uint32_t width,
                                uint32_t height);

  // Timer expiration method
  virtual void OnTimerEvent();

 private:
  // private methods
  bool ValidateKeySystem(const CdmKeySystem& key_system);
  CdmResponseType GetUsageInfo(const std::string& app_id,
                               SecurityLevel requested_security_level,
                               CdmUsageInfo* usage_info);

  void OnKeyReleaseEvent(const CdmKeySetId& key_set_id);

  std::string MapHdcpVersion(CryptoSession::OemCryptoHdcpVersion version);

  // instance variables
  CdmSessionMap sessions_;
  CdmReleaseKeySetMap release_key_sets_;
  scoped_ptr<CertificateProvisioning> cert_provisioning_;
  SecurityLevel cert_provisioning_requested_security_level_;

  static bool seeded_;

  // usage related variables
  scoped_ptr<CdmSession> usage_session_;
  scoped_ptr<UsagePropertySet> usage_property_set_;
  int64_t last_usage_information_update_time_;

  Lock session_list_lock_;
  // If the timer is currently active -- do not delete any sessions.
  bool timer_event_active_;
  // Sessions to be deleted after the timer has finished.
  std::vector<CdmSession*> dead_sessions_;

  CORE_DISALLOW_COPY_AND_ASSIGN(CdmEngine);
};

}  // namespace wvcdm

#endif  // WVCDM_CORE_CDM_ENGINE_H_
