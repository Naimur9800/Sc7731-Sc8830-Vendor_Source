// Copyright 2013 Google Inc. All Rights Reserved.
//
//  Mock implementation of OEMCrypto APIs
//
#include "oemcrypto_usage_table_mock.h"

#include <cstring>
#include <string>
#include <vector>

#include "clock.h"
#include "log.h"
#include "file_store.h"
#include "properties.h"
#include "oemcrypto_engine_mock.h"
#include "oemcrypto_logging.h"
#include "openssl/aes.h"
#include "openssl/rand.h"
#include "openssl/sha.h"
#include "openssl/hmac.h"
#include "string_conversions.h"
#include "wv_cdm_constants.h"

namespace wvoec_mock {

wvcdm::Clock UsageTableEntry::clock_;

UsageTableEntry::UsageTableEntry(const std::vector<uint8_t> &pst_hash,
                                 SessionContext *ctx)
    : pst_hash_(pst_hash),
      time_of_license_received_(clock_.GetCurrentTime()),
      time_of_first_decrypt_(0),
      time_of_last_decrypt_(0),
      status_(kUnused),
      session_(ctx) {}

UsageTableEntry::~UsageTableEntry() {
  if (session_) session_->ReleaseUsageEntry();
}

UsageTableEntry::UsageTableEntry(const StoredUsageEntry *buffer) {
  pst_hash_.assign(buffer->pst_hash, buffer->pst_hash + SHA256_DIGEST_LENGTH);
  time_of_license_received_ = buffer->time_of_license_received;
  time_of_first_decrypt_ = buffer->time_of_first_decrypt;
  time_of_last_decrypt_ = buffer->time_of_last_decrypt;
  status_ = buffer->status;
  mac_key_server_.assign(buffer->mac_key_server,
                         buffer->mac_key_server + wvcdm::MAC_KEY_SIZE);
  mac_key_client_.assign(buffer->mac_key_client,
                         buffer->mac_key_client + wvcdm::MAC_KEY_SIZE);
  session_ = NULL;
}

void UsageTableEntry::SaveToBuffer(StoredUsageEntry *buffer) {
  if (pst_hash_.size() != sizeof(buffer->pst_hash)) {
    LOGE("Coding Error: pst hash has wrong size.");
    return;
  }
  memcpy(buffer->pst_hash, &pst_hash_[0], pst_hash_.size());
  buffer->time_of_license_received = time_of_license_received_;
  buffer->time_of_first_decrypt = time_of_first_decrypt_;
  buffer->time_of_last_decrypt = time_of_last_decrypt_;
  buffer->status = status_;
  memcpy(buffer->mac_key_server, &mac_key_server_[0], wvcdm::MAC_KEY_SIZE);
  memcpy(buffer->mac_key_client, &mac_key_client_[0], wvcdm::MAC_KEY_SIZE);
}

void UsageTableEntry::Deactivate() {
  status_ = kInactive;
  if (session_) {
    session_->ReleaseUsageEntry();
    session_ = NULL;
  }
}

bool UsageTableEntry::UpdateTime() {
  int64_t now = clock_.GetCurrentTime();
  switch (status_) {
    case kUnused:
      status_ = kActive;
      time_of_first_decrypt_ = now;
      time_of_last_decrypt_ = now;
      return true;
    case kActive:
      time_of_last_decrypt_ = now;
      return true;
    case kInactive:
      return false;
  }
  return false;
}

OEMCryptoResult UsageTableEntry::ReportUsage(SessionContext *session,
                                             const std::vector<uint8_t> &pst,
                                             OEMCrypto_PST_Report *buffer,
                                             size_t *buffer_length) {
  size_t length_needed = sizeof(OEMCrypto_PST_Report) + pst.size();
  if (*buffer_length < length_needed) {
    *buffer_length = length_needed;
    return OEMCrypto_ERROR_SHORT_BUFFER;
  }
  if (!buffer) {
    LOGE("ReportUsage: buffer was null pointer.");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  int64_t now = clock_.GetCurrentTime();
  buffer->seconds_since_license_received =
      wvcdm::htonll64(now - time_of_license_received_);
  buffer->seconds_since_first_decrypt =
      wvcdm::htonll64(now - time_of_first_decrypt_);
  buffer->seconds_since_last_decrypt =
      wvcdm::htonll64(now - time_of_last_decrypt_);
  buffer->status = status_;
  buffer->clock_security_level = kSecureTimer;
  buffer->pst_length = static_cast<uint8_t>(pst.size());
  memcpy(buffer->pst, &pst[0], length_needed - sizeof(OEMCrypto_PST_Report));
  unsigned int md_len = sizeof(buffer->signature);
  if (!HMAC(EVP_sha1(), &mac_key_client_[0], mac_key_client_.size(),
            reinterpret_cast<uint8_t *>(buffer) + SHA_DIGEST_LENGTH,
            length_needed - SHA_DIGEST_LENGTH, buffer->signature, &md_len)) {
    LOGE("UsageTableEntry: could not compute signature.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  session->set_mac_key_server(mac_key_server_);
  session->set_mac_key_client(mac_key_client_);

  return OEMCrypto_SUCCESS;
}

bool UsageTableEntry::VerifyOrSetMacKeys(const std::vector<uint8_t> &server,
                                         const std::vector<uint8_t> &client) {
  if (mac_key_server_.size() == 0) {  // Not set yet, so set it now.
    mac_key_server_ = server;
    mac_key_client_ = client;
    return true;
  } else {
    return (mac_key_server_ == server && mac_key_client_ == client);
  }
}

UsageTable::UsageTable(CryptoEngine *ce) {
  ce_ = ce;
  generation_ = 0;
  table_.clear();

  // Load saved table.
  wvcdm::File file;
  std::string path;
  // Note: this path is OK for a real implementation, but using security level 1
  // would be better.
  if (!wvcdm::Properties::GetDeviceFilesBasePath(wvcdm::kSecurityLevelL3,
                                                 &path)) {
    LOGE("UsageTable: Unable to get base path");
    return;
  }
  if (!file.IsDirectory(path)) {
    if (!file.CreateDirectory(path)) {
      LOGE("UsageTable: could not create directory: %s", path.c_str());
      return;
    }
  }

  std::string filename = path + "UsageTable.dat";
  if (!file.Exists(filename)) {
    if (LogCategoryEnabled(kLoggingTraceUsageTable)) {
      LOGI("UsageTable: No saved usage table. Creating new table.");
    }
    return;
  }
  size_t file_size = file.FileSize(filename);
  uint8_t encrypted_buffer[file_size];
  uint8_t buffer[file_size];
  StoredUsageTable *stored_table = reinterpret_cast<StoredUsageTable *>(buffer);
  StoredUsageTable *encrypted_table =
      reinterpret_cast<StoredUsageTable *>(encrypted_buffer);

  if (!file.Open(filename, wvcdm::File::kReadOnly | wvcdm::File::kBinary)) {
    LOGE("UsageTable: File open failed: %s", path.c_str());
    return;
  }
  file.Read(reinterpret_cast<char *>(encrypted_buffer), file_size);
  file.Close();

  // First, verify the signature of the usage table file.
  std::vector<uint8_t> &key = ce_->keybox().device_key();
  unsigned int sig_length = sizeof(stored_table->signature);
  uint8_t computed_signature[SHA256_DIGEST_LENGTH];
  if (!HMAC(EVP_sha256(), &key[0], key.size(),
            encrypted_buffer + SHA256_DIGEST_LENGTH,
            file_size - SHA256_DIGEST_LENGTH, computed_signature,
            &sig_length)) {
    LOGE("UsageTable: Could not recreate signature.");
    table_.clear();
    return;
  }
  if (memcmp(encrypted_table->signature, computed_signature, sig_length)) {
    LOGE("UsageTable: Invalid signature    given: %s",
         wvcdm::HexEncode(encrypted_buffer, sig_length).c_str());
    LOGE("UsageTable: Invalid signature computed: %s",
         wvcdm::HexEncode(computed_signature, sig_length).c_str());
    table_.clear();
    return;
  }

  // Next, decrypt the table.
  memset(buffer, 0, file_size);
  uint8_t iv_buffer[wvcdm::KEY_IV_SIZE];
  memcpy(iv_buffer, encrypted_table->iv, wvcdm::KEY_IV_SIZE);
  AES_KEY aes_key;
  AES_set_decrypt_key(&key[0], 128, &aes_key);
  AES_cbc_encrypt(encrypted_buffer + SHA256_DIGEST_LENGTH + wvcdm::KEY_IV_SIZE,
                  buffer + SHA256_DIGEST_LENGTH + wvcdm::KEY_IV_SIZE,
                  file_size - SHA256_DIGEST_LENGTH - wvcdm::KEY_IV_SIZE,
                  &aes_key, iv_buffer, AES_DECRYPT);

  // Next, read the generation number from a different location.
  // On a real implementation, you should NOT put the generation number in
  // a file in user space.  It should be stored in secure memory. For the
  // reference implementation, we'll just pretend this is secure.
  std::string filename2 = path + "GenerationNumber.dat";
  if (!file.Exists(filename2) ||
      !file.Open(filename2, wvcdm::File::kReadOnly | wvcdm::File::kBinary)) {
    LOGE("UsageTable: File open failed: %s (clearing table)", path.c_str());
    generation_ = 0;
    table_.clear();
    return;
  }
  file.Read(reinterpret_cast<char *>(&generation_), sizeof(int64_t));
  file.Close();
  if (stored_table->generation == generation_ + 1) {
    if (LogCategoryEnabled(kLoggingTraceUsageTable)) {
      LOGW("UsageTable: File is one generation old.  Acceptable rollback.");
    }
  } else if (stored_table->generation == generation_ - 1) {
    if (LogCategoryEnabled(kLoggingTraceUsageTable)) {
      LOGW("UsageTable: File is one generation new.  Acceptable rollback.");
    }
    // This might happen if the generation number was rolled back?
  } else if (stored_table->generation != generation_) {
    LOGE("UsageTable: Rollback detected. Clearing Usage Table. %lx -> %lx",
         generation_, stored_table->generation);
    table_.clear();
    generation_ = 0;
    return;
  }

  // At this point, the stored table looks valid. We can load in all the
  // entries.
  for (size_t i = 0; i < stored_table->count; i++) {
    UsageTableEntry *entry =
        new UsageTableEntry(&stored_table->entries[i].entry);
    table_[entry->pst_hash()] = entry;
  }
  if (LogCategoryEnabled(kLoggingTraceUsageTable)) {
    LOGI("UsageTable: loaded %d entries.", stored_table->count);
  }
}

bool UsageTable::SaveToFile() {
  // This is always called by a locking function.
  // Update the generation number so we can detect rollback.
  generation_++;
  // Now save data to the file as seen in the constructor, above.
  size_t file_size = sizeof(StoredUsageTable) +
                     table_.size() * sizeof(AlignedStoredUsageEntry);
  uint8_t buffer[file_size];
  uint8_t encrypted_buffer[file_size];
  StoredUsageTable *stored_table = reinterpret_cast<StoredUsageTable *>(buffer);
  StoredUsageTable *encrypted_table =
      reinterpret_cast<StoredUsageTable *>(encrypted_buffer);
  memset(buffer, 0, file_size);
  stored_table->generation = generation_;
  stored_table->count = 0;
  for (EntryMap::iterator i = table_.begin(); i != table_.end(); ++i) {
    UsageTableEntry *entry = i->second;
    entry->SaveToBuffer(&stored_table->entries[stored_table->count].entry);
    stored_table->count++;
  }

  // This should be encrypted and signed with a device specific key.
  // For the reference implementation, I'm just going to use the keybox key.
  std::vector<uint8_t> &key = ce_->keybox().device_key();

  // Encrypt the table.
  RAND_bytes(encrypted_table->iv, wvcdm::KEY_IV_SIZE);
  uint8_t iv_buffer[wvcdm::KEY_IV_SIZE];
  memcpy(iv_buffer, encrypted_table->iv, wvcdm::KEY_IV_SIZE);
  AES_KEY aes_key;
  AES_set_encrypt_key(&key[0], 128, &aes_key);
  AES_cbc_encrypt(buffer + SHA256_DIGEST_LENGTH + wvcdm::KEY_IV_SIZE,
                  encrypted_buffer + SHA256_DIGEST_LENGTH + wvcdm::KEY_IV_SIZE,
                  file_size - SHA256_DIGEST_LENGTH - wvcdm::KEY_IV_SIZE,
                  &aes_key, iv_buffer, AES_ENCRYPT);

  // Sign the table.
  unsigned int sig_length = sizeof(stored_table->signature);
  if (!HMAC(EVP_sha256(), &key[0], key.size(),
            encrypted_buffer + SHA256_DIGEST_LENGTH,
            file_size - SHA256_DIGEST_LENGTH, encrypted_table->signature,
            &sig_length)) {
    LOGE("UsageTable: Could not sign table.");
    return false;
  }

  wvcdm::File file;
  std::string path;
  // Note: this path is OK for a real implementation, but using security level 1
  // would be better.
  if (!wvcdm::Properties::GetDeviceFilesBasePath(wvcdm::kSecurityLevelL3,
                                                 &path)) {
    LOGE("UsageTable: Unable to get base path");
    return false;
  }
  if (!file.IsDirectory(path)) {
    if (!file.CreateDirectory(path)) {
      LOGE("UsageTable: could not create directory: %s", path.c_str());
      return false;
    }
  }

  std::string filename = path + "UsageTable.dat";
  if (!file.Exists(filename)) {
    if (LogCategoryEnabled(kLoggingTraceUsageTable)) {
      LOGI("UsageTable: No saved usage table. Creating new table.");
    }
  }

  if (!file.Open(filename, wvcdm::File::kCreate | wvcdm::File::kTruncate |
                               wvcdm::File::kBinary)) {
    LOGE("UsageTable: Could not save usage table: %s", path.c_str());
    return false;
  }
  file.Write(reinterpret_cast<char *>(encrypted_buffer), file_size);
  file.Close();

  // On a real implementation, you should NOT put the generation number in
  // a file in user space.  It should be stored in secure memory.
  std::string filename2 = path + "GenerationNumber.dat";
  if (!file.Open(filename2, wvcdm::File::kCreate | wvcdm::File::kTruncate |
                                wvcdm::File::kBinary)) {
    LOGE("UsageTable: File open failed: %s", path.c_str());
    return false;
  }
  file.Write(reinterpret_cast<char *>(&generation_), sizeof(int64_t));
  file.Close();
  return true;
}

UsageTableEntry *UsageTable::FindEntry(const std::vector<uint8_t> &pst) {
  wvcdm::AutoLock lock(lock_);
  return FindEntryLocked(pst);
}

UsageTableEntry *UsageTable::FindEntryLocked(const std::vector<uint8_t> &pst) {
  std::vector<uint8_t> pst_hash;
  if (!ComputeHash(pst, pst_hash)) {
    LOGE("UsageTable: Could not compute hash of pst.");
    return NULL;
  }
  EntryMap::iterator it = table_.find(pst_hash);
  if (it == table_.end()) {
    return NULL;
  }
  return it->second;
}

UsageTableEntry *UsageTable::CreateEntry(const std::vector<uint8_t> &pst,
                                         SessionContext *ctx) {
  std::vector<uint8_t> pst_hash;
  if (!ComputeHash(pst, pst_hash)) {
    LOGE("UsageTable: Could not compute hash of pst.");
    return NULL;
  }
  UsageTableEntry *entry = new UsageTableEntry(pst_hash, ctx);
  wvcdm::AutoLock lock(lock_);
  table_[pst_hash] = entry;
  return entry;
}

OEMCryptoResult UsageTable::UpdateTable() {
  wvcdm::AutoLock lock(lock_);
  if (SaveToFile()) return OEMCrypto_SUCCESS;
  return OEMCrypto_ERROR_UNKNOWN_FAILURE;
}

OEMCryptoResult UsageTable::DeactivateEntry(const std::vector<uint8_t> &pst) {
  wvcdm::AutoLock lock(lock_);
  UsageTableEntry *entry = FindEntryLocked(pst);
  if (!entry) return OEMCrypto_ERROR_INVALID_CONTEXT;
  entry->Deactivate();
  if (SaveToFile()) return OEMCrypto_SUCCESS;
  return OEMCrypto_ERROR_UNKNOWN_FAILURE;
}

bool UsageTable::DeleteEntry(const std::vector<uint8_t> &pst) {
  std::vector<uint8_t> pst_hash;
  if (!ComputeHash(pst, pst_hash)) {
    LOGE("UsageTable: Could not compute hash of pst.");
    return false;
  }
  wvcdm::AutoLock lock(lock_);
  EntryMap::iterator it = table_.find(pst_hash);
  if (it == table_.end()) return false;
  if (it->second) delete it->second;
  table_.erase(it);
  return SaveToFile();
}

void UsageTable::Clear() {
  wvcdm::AutoLock lock(lock_);
  for (EntryMap::iterator i = table_.begin(); i != table_.end(); ++i) {
    if (i->second) delete i->second;
  }
  table_.clear();
}

bool UsageTable::ComputeHash(const std::vector<uint8_t> &pst,
                             std::vector<uint8_t> &pst_hash) {
  // The PST is not fixed size, and we have no promises that it is reasonbly
  // sized, so we compute a hash of it, and store that instead.
  pst_hash.resize(SHA256_DIGEST_LENGTH);
  SHA256_CTX context;
  if (!SHA256_Init(&context)) return false;
  if (!SHA256_Update(&context, &pst[0], pst.size())) return false;
  if (!SHA256_Final(&pst_hash[0], &context)) return false;
  return true;
}
};  // namespace wvoec_mock
