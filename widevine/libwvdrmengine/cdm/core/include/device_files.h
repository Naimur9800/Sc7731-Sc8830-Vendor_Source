// Copyright 2013 Google Inc. All Rights Reserved.
//
#ifndef WVCDM_CORE_DEVICE_FILES_H_
#define WVCDM_CORE_DEVICE_FILES_H_

#include "scoped_ptr.h"
#include "wv_cdm_types.h"

#if defined(UNIT_TEST)
#include <gtest/gtest_prod.h>
#endif

namespace wvcdm {

class File;

class DeviceFiles {
 public:
  typedef enum {
    kLicenseStateActive,
    kLicenseStateReleasing,
    kLicenseStateUnknown,
  } LicenseState;

  DeviceFiles();
  virtual ~DeviceFiles();

  virtual bool Init(CdmSecurityLevel security_level);
  virtual bool Reset(CdmSecurityLevel security_level) {
      return Init(security_level);
  }

  virtual bool StoreCertificate(const std::string& certificate,
                                const std::string& wrapped_private_key);
  virtual bool RetrieveCertificate(std::string* certificate,
                                   std::string* wrapped_private_key);

  virtual bool StoreLicense(const std::string& key_set_id,
                            const LicenseState state,
                            const CdmInitData& pssh_data,
                            const CdmKeyMessage& key_request,
                            const CdmKeyResponse& key_response,
                            const CdmKeyMessage& key_renewal_request,
                            const CdmKeyResponse& key_renewal_response,
                            const std::string& release_server_url,
                            int64_t playback_start_time,
                            int64_t last_playback_time);
  virtual bool RetrieveLicense(const std::string& key_set_id,
                               LicenseState* state,
                               CdmInitData* pssh_data,
                               CdmKeyMessage* key_request,
                               CdmKeyResponse* key_response,
                               CdmKeyMessage* key_renewal_request,
                               CdmKeyResponse* key_renewal_response,
                               std::string* release_server_url,
                               int64_t* playback_start_time,
                               int64_t* last_playback_time);
  virtual bool DeleteLicense(const std::string& key_set_id);
  virtual bool DeleteAllFiles();
  virtual bool DeleteAllLicenses();
  virtual bool LicenseExists(const std::string& key_set_id);

  virtual bool StoreUsageInfo(const std::string& provider_session_token,
                              const CdmKeyMessage& key_request,
                              const CdmKeyResponse& key_response,
                              const std::string& app_id);
  virtual bool DeleteUsageInfo(const std::string& app_id,
                               const std::string& provider_session_token);
  virtual bool DeleteAllUsageInfoForApp(const std::string& app_id);
  // Retrieve one usage info from the file.  Subsequent calls will retrieve
  // subsequent entries in the table for this app_id.
  virtual bool RetrieveUsageInfo(
      const std::string& app_id,
      std::vector<std::pair<CdmKeyMessage, CdmKeyResponse> >* usage_info);
  // Retrieve the usage info entry specified by |provider_session_token|.
  // Returns false if the entry could not be found.
  virtual bool RetrieveUsageInfo(const std::string& app_id,
                                 const std::string& provider_session_token,
                                 CdmKeyMessage* license_request,
                                 CdmKeyResponse* license_response);

 private:
  bool StoreFile(const char* name, const std::string& serialized_file);
  bool RetrieveFile(const char* name, std::string* serialized_file);

  // Certificate and offline licenses are now stored in security
  // level specific directories. In an earlier version they were
  // stored in a common directory and need to be copied over.
  virtual void SecurityLevelPathBackwardCompatibility();

  // For testing only:
  static std::string GetCertificateFileName();
  static std::string GetLicenseFileNameExtension();
  static std::string GetUsageInfoFileName(const std::string& app_id);
  void SetTestFile(File* file);
#if defined(UNIT_TEST)
  FRIEND_TEST(DeviceFilesSecurityLevelTest, SecurityLevel);
  FRIEND_TEST(DeviceFilesStoreTest, StoreCertificate);
  FRIEND_TEST(DeviceFilesStoreTest, StoreLicense);
  FRIEND_TEST(DeviceFilesTest, DeleteLicense);
  FRIEND_TEST(DeviceFilesTest, ReadCertificate);
  FRIEND_TEST(DeviceFilesTest, RetrieveLicenses);
  FRIEND_TEST(DeviceFilesTest, SecurityLevelPathBackwardCompatibility);
  FRIEND_TEST(DeviceFilesTest, StoreLicenses);
  FRIEND_TEST(DeviceFilesTest, UpdateLicenseState);
  FRIEND_TEST(DeviceFilesUsageInfoTest, Delete);
  FRIEND_TEST(DeviceFilesUsageInfoTest, Read);
  FRIEND_TEST(DeviceFilesUsageInfoTest, Store);
  FRIEND_TEST(WvCdmRequestLicenseTest, UnprovisionTest);
  FRIEND_TEST(WvCdmRequestLicenseTest, ForceL3Test);
  FRIEND_TEST(WvCdmRequestLicenseTest, UsageInfoRetryTest);
  FRIEND_TEST(WvCdmUsageInfoTest, UsageInfo);
  FRIEND_TEST(WvCdmExtendedDurationTest, UsageOverflowTest);
#endif

  scoped_ptr<File> file_;
  CdmSecurityLevel security_level_;
  bool initialized_;

  bool test_file_;

  CORE_DISALLOW_COPY_AND_ASSIGN(DeviceFiles);
};

}  // namespace wvcdm

#endif  // WVCDM_CORE_DEVICE_FILES_H_
