// Copyright 2013 Google Inc. All Rights Reserved.

#include "device_files.h"

#if defined(__APPLE__)
# include <CommonCrypto/CommonDigest.h>
# define SHA256 CC_SHA256
# define SHA256_DIGEST_LENGTH CC_SHA256_DIGEST_LENGTH
#else
# include <openssl/sha.h>
#include <openssl/md5.h>
#endif

#include <cstring>
#include <string>

#include "device_files.pb.h"
#include "file_store.h"
#include "log.h"
#include "properties.h"
#include "string_conversions.h"

// Protobuf generated classes.
using video_widevine_client::sdk::DeviceCertificate;
using video_widevine_client::sdk::HashedFile;
using video_widevine_client::sdk::License;
using video_widevine_client::sdk::License_LicenseState_ACTIVE;
using video_widevine_client::sdk::License_LicenseState_RELEASING;
using video_widevine_client::sdk::UsageInfo;
using video_widevine_client::sdk::UsageInfo_ProviderSession;

namespace {

const char kCertificateFileName[] = "cert.bin";
const char kUsageInfoFileNamePrefix[] = "usage";
const char kUsageInfoFileNameExt[] = ".bin";
const char kLicenseFileNameExt[] = ".lic";
const char kWildcard[] = "*";
const char kDirectoryDelimiter = '/';
const char* kSecurityLevelPathCompatibilityExclusionList[] = {"ay64.dat"};
size_t kSecurityLevelPathCompatibilityExclusionListSize =
    sizeof(kSecurityLevelPathCompatibilityExclusionList) /
    sizeof(*kSecurityLevelPathCompatibilityExclusionList);

bool Hash(const std::string& data, std::string* hash) {
  if (!hash) return false;
  hash->resize(SHA256_DIGEST_LENGTH);

  const unsigned char* input =
    reinterpret_cast<const unsigned char*>(data.data());
  unsigned char* output = reinterpret_cast<unsigned char*>(&(*hash)[0]);
  SHA256(input, data.size(), output);
  return true;
}

}  // unnamed namespace

namespace wvcdm {

DeviceFiles::DeviceFiles()
    : file_(NULL), security_level_(kSecurityLevelUninitialized),
      initialized_(false), test_file_(false) {
}

DeviceFiles::~DeviceFiles() {
  if (test_file_) file_.release();
}

bool DeviceFiles::Init(CdmSecurityLevel security_level) {
  switch (security_level) {
    case kSecurityLevelL1:
    case kSecurityLevelL2:
    case kSecurityLevelL3:
      break;
    default:
      LOGW("DeviceFiles::Init: Unsupported security level %d", security_level);
      return false;
  }
  if (!test_file_) file_.reset(new File());
  security_level_ = security_level;
  initialized_ = true;
  return true;
}

bool DeviceFiles::StoreCertificate(const std::string& certificate,
                                   const std::string& wrapped_private_key) {
  if (!initialized_) {
    LOGW("DeviceFiles::StoreCertificate: not initialized");
    return false;
  }

  // Fill in file information
  video_widevine_client::sdk::File file;

  file.set_type(video_widevine_client::sdk::File::DEVICE_CERTIFICATE);
  file.set_version(video_widevine_client::sdk::File::VERSION_1);

  DeviceCertificate* device_certificate = file.mutable_device_certificate();
  device_certificate->set_certificate(certificate);
  device_certificate->set_wrapped_private_key(wrapped_private_key);

  std::string serialized_file;
  file.SerializeToString(&serialized_file);

  return StoreFile(kCertificateFileName, serialized_file);
}

bool DeviceFiles::RetrieveCertificate(std::string* certificate,
                                      std::string* wrapped_private_key) {
  if (!initialized_) {
    LOGW("DeviceFiles::RetrieveCertificate: not initialized");
    return false;
  }

  if (Properties::security_level_path_backward_compatibility_support()) {
    SecurityLevelPathBackwardCompatibility();
  }

  std::string serialized_file;
  if (!RetrieveFile(kCertificateFileName, &serialized_file))
    return false;

  video_widevine_client::sdk::File file;
  if (!file.ParseFromString(serialized_file)) {
    LOGW("DeviceFiles::RetrieveCertificate: Unable to parse file");
    return false;
  }

  if (file.type() != video_widevine_client::sdk::File::DEVICE_CERTIFICATE) {
    LOGW("DeviceFiles::RetrieveCertificate: Incorrect file type");
    return false;
  }

  if (file.version() != video_widevine_client::sdk::File::VERSION_1) {
    LOGW("DeviceFiles::RetrieveCertificate: Incorrect file version");
    return false;
  }

  if (!file.has_device_certificate()) {
    LOGW("DeviceFiles::RetrieveCertificate: Certificate not present");
    return false;
  }

  DeviceCertificate device_certificate = file.device_certificate();

  *certificate = device_certificate.certificate();
  *wrapped_private_key = device_certificate.wrapped_private_key();
  return true;
}

bool DeviceFiles::StoreLicense(const std::string& key_set_id,
                               const LicenseState state,
                               const CdmInitData& pssh_data,
                               const CdmKeyMessage& license_request,
                               const CdmKeyResponse& license_message,
                               const CdmKeyMessage& license_renewal_request,
                               const CdmKeyResponse& license_renewal,
                               const std::string& release_server_url,
                               int64_t playback_start_time,
                               int64_t last_playback_time) {
  if (!initialized_) {
    LOGW("DeviceFiles::StoreLicense: not initialized");
    return false;
  }

  // Fill in file information
  video_widevine_client::sdk::File file;

  file.set_type(video_widevine_client::sdk::File::LICENSE);
  file.set_version(video_widevine_client::sdk::File::VERSION_1);

  License* license = file.mutable_license();
  switch (state) {
    case kLicenseStateActive:
      license->set_state(License_LicenseState_ACTIVE);
      break;
    case kLicenseStateReleasing:
      license->set_state(License_LicenseState_RELEASING);
      break;
    default:
      LOGW("DeviceFiles::StoreLicense: Unknown license state: %u", state);
      return false;
      break;
  }
  license->set_pssh_data(pssh_data);
  license->set_license_request(license_request);
  license->set_license(license_message);
  license->set_renewal_request(license_renewal_request);
  license->set_renewal(license_renewal);
  license->set_release_server_url(release_server_url);
  license->set_playback_start_time(playback_start_time);
  license->set_last_playback_time(last_playback_time);

  std::string serialized_file;
  file.SerializeToString(&serialized_file);

  std::string file_name = key_set_id + kLicenseFileNameExt;
  return StoreFile(file_name.c_str(), serialized_file);
}

bool DeviceFiles::RetrieveLicense(const std::string& key_set_id,
                                  LicenseState* state, CdmInitData* pssh_data,
                                  CdmKeyMessage* license_request,
                                  CdmKeyResponse* license_message,
                                  CdmKeyMessage* license_renewal_request,
                                  CdmKeyResponse* license_renewal,
                                  std::string* release_server_url,
                                  int64_t* playback_start_time,
                                  int64_t* last_playback_time) {
  if (!initialized_) {
    LOGW("DeviceFiles::RetrieveLicense: not initialized");
    return false;
  }

  std::string serialized_file;
  std::string file_name = key_set_id + kLicenseFileNameExt;
  if (!RetrieveFile(file_name.c_str(), &serialized_file)) return false;

  video_widevine_client::sdk::File file;
  if (!file.ParseFromString(serialized_file)) {
    LOGW("DeviceFiles::RetrieveLicense: Unable to parse file");
    return false;
  }

  if (file.type() != video_widevine_client::sdk::File::LICENSE) {
    LOGW("DeviceFiles::RetrieveLicense: Incorrect file type");
    return false;
  }

  if (file.version() != video_widevine_client::sdk::File::VERSION_1) {
    LOGW("DeviceFiles::RetrieveLicense: Incorrect file version");
    return false;
  }

  if (!file.has_license()) {
    LOGW("DeviceFiles::RetrieveLicense: License not present");
    return false;
  }

  License license = file.license();

  switch (license.state()) {
    case License_LicenseState_ACTIVE:
      *state = kLicenseStateActive;
      break;
    case License_LicenseState_RELEASING:
      *state = kLicenseStateReleasing;
      break;
    default:
      LOGW("DeviceFiles::RetrieveLicense: Unrecognized license state: %u",
           kLicenseStateUnknown);
      *state = kLicenseStateUnknown;
      break;
  }
  *pssh_data = license.pssh_data();
  *license_request = license.license_request();
  *license_message = license.license();
  *license_renewal_request = license.renewal_request();
  *license_renewal = license.renewal();
  *release_server_url = license.release_server_url();
  *playback_start_time = license.playback_start_time();
  *last_playback_time = license.last_playback_time();
  return true;
}

bool DeviceFiles::DeleteLicense(const std::string& key_set_id) {
  if (!initialized_) {
    LOGW("DeviceFiles::DeleteLicense: not initialized");
    return false;
  }

  std::string path;
  if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
    LOGW("DeviceFiles::DeleteLicense: Unable to get base path");
    return false;
  }
  path.append(key_set_id);
  path.append(kLicenseFileNameExt);

  return file_->Remove(path);
}

bool DeviceFiles::DeleteAllLicenses() {
  if (!initialized_) {
    LOGW("DeviceFiles::DeleteAllLicenses: not initialized");
    return false;
  }

  std::string path;
  if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
    LOGW("DeviceFiles::DeleteAllLicenses: Unable to get base path");
    return false;
  }
  path.append(kWildcard);
  path.append(kLicenseFileNameExt);

  return file_->Remove(path);
}

bool DeviceFiles::DeleteAllFiles() {
  if (!initialized_) {
    LOGW("DeviceFiles::DeleteAllFiles: not initialized");
    return false;
  }

  std::string path;
  if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
    LOGW("DeviceFiles::DeleteAllFiles: Unable to get base path");
    return false;
  }

  return file_->Remove(path);
}

bool DeviceFiles::LicenseExists(const std::string& key_set_id) {
  if (!initialized_) {
    LOGW("DeviceFiles::LicenseExists: not initialized");
    return false;
  }

  std::string path;
  if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
    LOGW("DeviceFiles::StoreFile: Unable to get base path");
    return false;
  }
  path.append(key_set_id);
  path.append(kLicenseFileNameExt);

  return file_->Exists(path);
}

bool DeviceFiles::StoreUsageInfo(const std::string& provider_session_token,
                                 const CdmKeyMessage& key_request,
                                 const CdmKeyResponse& key_response,
                                 const std::string& app_id) {
  if (!initialized_) {
    LOGW("DeviceFiles::StoreUsageInfo: not initialized");
    return false;
  }

  std::string serialized_file;
  video_widevine_client::sdk::File file;
  std::string file_name = GetUsageInfoFileName(app_id);
  if (!RetrieveFile(file_name.c_str(), &serialized_file)) {
    file.set_type(video_widevine_client::sdk::File::USAGE_INFO);
    file.set_version(video_widevine_client::sdk::File::VERSION_1);
  } else {
    if (!file.ParseFromString(serialized_file)) {
      LOGW("DeviceFiles::StoreUsageInfo: Unable to parse file");
      return false;
    }
  }

  UsageInfo* usage_info = file.mutable_usage_info();
  UsageInfo_ProviderSession* provider_session = usage_info->add_sessions();

  provider_session->set_token(provider_session_token.data(),
                              provider_session_token.size());
  provider_session->set_license_request(key_request.data(),
                                        key_request.size());
  provider_session->set_license(key_response.data(),
                                key_response.size());

  file.SerializeToString(&serialized_file);
  return StoreFile(file_name.c_str(), serialized_file);
}

bool DeviceFiles::DeleteUsageInfo(const std::string& app_id,
                                  const std::string& provider_session_token) {
  if (!initialized_) {
    LOGW("DeviceFiles::DeleteUsageInfo: not initialized");
    return false;
  }

  std::string serialized_file;
  std::string file_name = GetUsageInfoFileName(app_id);
  if (!RetrieveFile(file_name.c_str(), &serialized_file)) return false;

  video_widevine_client::sdk::File file;
  if (!file.ParseFromString(serialized_file)) {
    LOGW("DeviceFiles::DeleteUsageInfo: Unable to parse file");
    return false;
  }

  UsageInfo* usage_info = file.mutable_usage_info();
  int index = 0;
  bool found = false;
  for (; index < usage_info->sessions_size(); ++index) {
    if (usage_info->sessions(index).token().compare(provider_session_token) == 0) {
      found = true;
      break;
    }
  }

  if (!found) {
    LOGW("DeviceFiles::DeleteUsageInfo: Unable to find provider session "
        "token: %s", b2a_hex(provider_session_token).c_str());
    return false;
  }

  google::protobuf::RepeatedPtrField<UsageInfo_ProviderSession>* sessions =
      usage_info->mutable_sessions();
  if (index < usage_info->sessions_size() - 1) {
    sessions->SwapElements(index, usage_info->sessions_size() - 1);
  }
  sessions->RemoveLast();

  file.SerializeToString(&serialized_file);
  return StoreFile(file_name.c_str(), serialized_file);
}

bool DeviceFiles::DeleteAllUsageInfoForApp(const std::string& app_id) {
  if (!initialized_) {
    LOGW("DeviceFiles::DeleteAllUsageInfoForApp: not initialized");
    return false;
  }

  std::string path;
  if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
    LOGW("DeviceFiles::DeleteAllUsageInfoForApp: Unable to get base path");
    return false;
  }
  std::string file_name = GetUsageInfoFileName(app_id);
  path.append(file_name);

  return file_->Remove(path);
}

bool DeviceFiles::RetrieveUsageInfo(
    const std::string &app_id,
    std::vector<std::pair<CdmKeyMessage, CdmKeyResponse> >* usage_info) {
  if (!initialized_) {
    LOGW("DeviceFiles::RetrieveUsageInfo: not initialized");
    return false;
  }

  if (NULL == usage_info) {
    LOGW("DeviceFiles::RetrieveUsageInfo: license destination not "
        "provided");
    return false;
  }

  std::string serialized_file;
  std::string file_name = GetUsageInfoFileName(app_id);
  if (!RetrieveFile(file_name.c_str(), &serialized_file)) {
    std::string path;
    if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
      return false;
    }

    path += file_name;

    if (!file_->Exists(path) || 0 == file_->FileSize(path)) {
      usage_info->resize(0);
      return true;
    }

    return false;
  }

  video_widevine_client::sdk::File file;
  if (!file.ParseFromString(serialized_file)) {
    LOGW("DeviceFiles::RetrieveUsageInfo: Unable to parse file");
    return false;
  }

  usage_info->resize(file.usage_info().sessions_size());
  for (int i = 0; i < file.usage_info().sessions_size(); ++i) {
    (*usage_info)[i] =
        std::make_pair(file.usage_info().sessions(i).license_request(),
                       file.usage_info().sessions(i).license());
  }

  return true;
}

bool DeviceFiles::RetrieveUsageInfo(const std::string& app_id,
                                    const std::string& provider_session_token,
                                    CdmKeyMessage* license_request,
                                    CdmKeyResponse* license_response) {
  if (!initialized_) {
    LOGW("DeviceFiles::RetrieveUsageInfo: not initialized");
    return false;
  }
  std::string serialized_file;
  std::string file_name = GetUsageInfoFileName(app_id);
  if (!RetrieveFile(file_name.c_str(), &serialized_file)) return false;

  video_widevine_client::sdk::File file;
  if (!file.ParseFromString(serialized_file)) {
    LOGW("DeviceFiles::RetrieveUsageInfo: Unable to parse file");
    return false;
  }
  int index = 0;
  bool found = false;
  for (; index < file.usage_info().sessions_size(); ++index) {
    if (file.usage_info().sessions(index).token().compare(
            provider_session_token) == 0) {
      found = true;
      break;
    }
  }

  if (!found) {
    return false;
  }

  *license_request = file.usage_info().sessions(index).license_request();
  *license_response = file.usage_info().sessions(index).license();
  return true;
}

bool DeviceFiles::StoreFile(const char* name,
                            const std::string& serialized_file) {
  if (!file_.get()) {
    LOGW("DeviceFiles::StoreFile: Invalid file handle");
    return false;
  }

  if (!name) {
    LOGW("DeviceFiles::StoreFile: Unspecified file name parameter");
    return false;
  }

  // calculate SHA hash
  std::string hash;
  if (!Hash(serialized_file, &hash)) {
    LOGW("DeviceFiles::StoreFile: Hash computation failed");
    return false;
  }

  // Fill in hashed file data
  HashedFile hash_file;
  hash_file.set_file(serialized_file);
  hash_file.set_hash(hash);

  std::string serialized_hash_file;
  hash_file.SerializeToString(&serialized_hash_file);

  std::string path;
  if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
    LOGW("DeviceFiles::StoreFile: Unable to get base path");
    return false;
  }

  if (!file_->IsDirectory(path)) {
    if (!file_->CreateDirectory(path)) return false;
  }

  path += name;

  if (!file_->Open(path, File::kCreate | File::kTruncate | File::kBinary)) {
    LOGW("DeviceFiles::StoreFile: File open failed: %s", path.c_str());
    return false;
  }

  ssize_t bytes = file_->Write(serialized_hash_file.data(),
                               serialized_hash_file.size());
  file_->Close();

  if (bytes != static_cast<ssize_t>(serialized_hash_file.size())) {
    LOGW("DeviceFiles::StoreFile: write failed: (actual: %d, expected: %d)",
        bytes,
        serialized_hash_file.size());
    return false;
  }

  LOGV("DeviceFiles::StoreFile: success: %s (%db)",
      path.c_str(),
      serialized_hash_file.size());
  return true;
}

bool DeviceFiles::RetrieveFile(const char* name, std::string* serialized_file) {
  if (!file_.get()) {
    LOGW("DeviceFiles::RetrieveFile: Invalid file handle");
    return false;
  }

  if (!name) {
    LOGW("DeviceFiles::RetrieveFile: Unspecified file name parameter");
    return false;
  }

  if (!serialized_file) {
    LOGW("DeviceFiles::RetrieveFile: Unspecified serialized_file parameter");
    return false;
  }

  std::string path;
  if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
    LOGW("DeviceFiles::StoreFile: Unable to get base path");
    return false;
  }

  path += name;

  if (!file_->Exists(path)) {
    LOGW("DeviceFiles::RetrieveFile: %s does not exist", path.c_str());
    return false;
  }

  ssize_t bytes = file_->FileSize(path);
  if (bytes <= 0) {
    LOGW("DeviceFiles::RetrieveFile: File size invalid: %s", path.c_str());
    // Remove the corrupted file so the caller will not get the same error
    // when trying to access the file repeatedly, causing the system to stall.
    file_->Remove(path);
    return false;
  }

  if (!file_->Open(path, File::kReadOnly | File::kBinary)) {
    return false;
  }

  std::string serialized_hash_file;
  serialized_hash_file.resize(bytes);
  bytes = file_->Read(&serialized_hash_file[0], serialized_hash_file.size());
  file_->Close();

  if (bytes != static_cast<ssize_t>(serialized_hash_file.size())) {
    LOGW("DeviceFiles::RetrieveFile: read failed");
    return false;
  }

  LOGV("DeviceFiles::RetrieveFile: success: %s (%db)", path.c_str(),
       serialized_hash_file.size());

  HashedFile hash_file;
  if (!hash_file.ParseFromString(serialized_hash_file)) {
    LOGW("DeviceFiles::RetrieveFile: Unable to parse hash file");
    return false;
  }

  std::string hash;
  if (!Hash(hash_file.file(), &hash)) {
    LOGW("DeviceFiles::RetrieveFile: Hash computation failed");
    return false;
  }

  if (hash.compare(hash_file.hash())) {
    LOGW("DeviceFiles::RetrieveFile: Hash mismatch");
    // Remove the corrupted file so the caller will not get the same error
    // when trying to access the file repeatedly, causing the system to stall.
    file_->Remove(path);
    return false;
  }

  *serialized_file = hash_file.file();
  return true;
}

void DeviceFiles::SecurityLevelPathBackwardCompatibility() {
  std::string path;
  if (!Properties::GetDeviceFilesBasePath(security_level_, &path)) {
    LOGW(
        "DeviceFiles::SecurityLevelPathBackwardCompatibility: Unable to "
        "get base path");
    return;
  }

  std::vector<std::string> security_dirs;
  if (!Properties::GetSecurityLevelDirectories(&security_dirs)) {
    LOGW(
        "DeviceFiles::SecurityLevelPathBackwardCompatibility: Unable to "
        "get security directories");
    return;
  }

  size_t pos = std::string::npos;
  for (size_t i = 0; i < security_dirs.size(); ++i) {
    pos = path.find(security_dirs[i]);
    if (pos != std::string::npos && pos > 0 &&
        pos == path.size() - security_dirs[i].size() &&
        path[pos - 1] == kDirectoryDelimiter) {
      break;
    }
  }

  if (pos == std::string::npos) {
    LOGV(
        "DeviceFiles::SecurityLevelPathBackwardCompatibility: Security level "
        "specific path not found. Check properties?");
    return;
  }

  std::string from_dir(path, 0, pos);

  std::vector<std::string> files;
  if (!file_->List(from_dir, &files)) {
    return;
  }

  for (size_t i = 0; i < files.size(); ++i) {
    std::string from = from_dir + files[i];
    bool exclude = false;
    for (size_t j = 0; j < kSecurityLevelPathCompatibilityExclusionListSize;
         ++j) {
      if (files[i] == kSecurityLevelPathCompatibilityExclusionList[j]) {
        exclude = true;
        break;
      }
    }
    if (exclude) continue;
    if (!file_->IsRegularFile(from)) continue;

    for (size_t j = 0; j < security_dirs.size(); ++j) {
      std::string to_dir = from_dir + security_dirs[j];
      if (!file_->Exists(to_dir)) file_->CreateDirectory(to_dir);
      std::string to = to_dir + files[i];
      file_->Copy(from, to);
    }
    file_->Remove(from);
  }
}

std::string DeviceFiles::GetCertificateFileName() {
  return kCertificateFileName;
}

std::string DeviceFiles::GetLicenseFileNameExtension() {
  return kLicenseFileNameExt;
}

std::string DeviceFiles::GetUsageInfoFileName(const std::string& app_id) {
  if (app_id == "") return std::string(kUsageInfoFileNamePrefix)
                        + std::string(kUsageInfoFileNameExt);
  std::vector<uint8_t> hash(MD5_DIGEST_LENGTH);
  const unsigned char* input =
      reinterpret_cast<const unsigned char*>(app_id.data());
  MD5(input, app_id.size(), &hash[0]);
  return std::string(kUsageInfoFileNamePrefix) + wvcdm::Base64SafeEncode(hash) +
         std::string(kUsageInfoFileNameExt);
}

void DeviceFiles::SetTestFile(File* file) {
  file_.reset(file);
  test_file_ = true;
}

}  // namespace wvcdm
