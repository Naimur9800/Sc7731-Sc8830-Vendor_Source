// Copyright 2013 Google Inc. All Rights Reserved.

#include "log.h"
#include "properties_configuration.h"
#include "wv_cdm_constants.h"

namespace {
const char* kSecurityLevelDirs[] = {"L1/", "L3/"};
}  // namespace

namespace wvcdm {
bool Properties::oem_crypto_use_secure_buffers_;
bool Properties::oem_crypto_use_fifo_;
bool Properties::oem_crypto_use_userspace_buffers_;
bool Properties::oem_crypto_require_usage_tables_;
bool Properties::use_certificates_as_identification_;
bool Properties::security_level_path_backward_compatibility_support_;
scoped_ptr<CdmClientPropertySetMap> Properties::session_property_set_;

void Properties::Init() {
  oem_crypto_use_secure_buffers_ = kPropertyOemCryptoUseSecureBuffers;
  oem_crypto_use_fifo_ = kPropertyOemCryptoUseFifo;
  oem_crypto_use_userspace_buffers_ = kPropertyOemCryptoUseUserSpaceBuffers;
  oem_crypto_require_usage_tables_ = kPropertyOemCryptoRequireUsageTable;
  use_certificates_as_identification_ =
      kPropertyUseCertificatesAsIdentification;
  security_level_path_backward_compatibility_support_ =
      kSecurityLevelPathBackwardCompatibilitySupport;
  session_property_set_.reset(new CdmClientPropertySetMap());
}

bool Properties::AddSessionPropertySet(
    const CdmSessionId& session_id, const CdmClientPropertySet* property_set) {
  if (NULL == session_property_set_.get()) {
    return false;
  }
  std::pair<CdmClientPropertySetMap::iterator, bool> result =
      session_property_set_->insert(
          std::pair<const CdmSessionId, const CdmClientPropertySet*>(
              session_id, property_set));
  return result.second;
}

bool Properties::RemoveSessionPropertySet(const CdmSessionId& session_id) {
  if (NULL == session_property_set_.get()) {
    return false;
  }
  return (1 == session_property_set_->erase(session_id));
}

const CdmClientPropertySet* Properties::GetCdmClientPropertySet(
    const CdmSessionId& session_id) {
  if (NULL != session_property_set_.get()) {
    CdmClientPropertySetMap::const_iterator it =
        session_property_set_->find(session_id);
    if (it != session_property_set_->end()) {
      return it->second;
    }
  }
  return NULL;
}

bool Properties::GetApplicationId(const CdmSessionId& session_id,
                                  std::string* app_id) {
  const CdmClientPropertySet* property_set =
      GetCdmClientPropertySet(session_id);
  if (NULL == property_set) {
    return false;
  }
  *app_id = property_set->app_id();
  return true;
}

bool Properties::GetSecurityLevel(const CdmSessionId& session_id,
                                  std::string* security_level) {
  const CdmClientPropertySet* property_set =
      GetCdmClientPropertySet(session_id);
  if (NULL == property_set) {
    return false;
  }
  *security_level = property_set->security_level();
  return true;
}

bool Properties::GetServiceCertificate(const CdmSessionId& session_id,
                                       std::string* service_certificate) {
  const CdmClientPropertySet* property_set =
      GetCdmClientPropertySet(session_id);
  if (NULL == property_set) {
    return false;
  }
  *service_certificate = property_set->service_certificate();
  return true;
}

bool Properties::UsePrivacyMode(const CdmSessionId& session_id) {
  const CdmClientPropertySet* property_set =
      GetCdmClientPropertySet(session_id);
  if (NULL == property_set) {
    return false;
  }
  return property_set->use_privacy_mode();
}

uint32_t Properties::GetSessionSharingId(const CdmSessionId& session_id) {
  const CdmClientPropertySet* property_set =
      GetCdmClientPropertySet(session_id);
  if (NULL == property_set) {
    return 0;
  }
  return property_set->session_sharing_id();
}

bool Properties::GetSecurityLevelDirectories(std::vector<std::string>* dirs) {
  dirs->resize(sizeof(kSecurityLevelDirs) / sizeof(const char*));
  for (size_t i = 0; i < dirs->size(); ++i) {
    (*dirs)[i] = kSecurityLevelDirs[i];
  }
  return true;
}

}  // namespace wvcdm
