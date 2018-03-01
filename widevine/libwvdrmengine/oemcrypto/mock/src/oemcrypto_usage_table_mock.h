// Copyright 2013 Google Inc. All Rights Reserved.
//
//  Mock implementation of OEMCrypto APIs
//
#ifndef WVOEC_MOCK_OEMCRYPTO_USAGE_TABLE_MOCK_H_
#define WVOEC_MOCK_OEMCRYPTO_USAGE_TABLE_MOCK_H_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "clock.h"
#include "lock.h"
#include "OEMCryptoCENC.h"
#include "openssl/sha.h"
#include "wv_cdm_constants.h"

namespace wvoec_mock {

class SessionContext;
class CryptoEngine;

struct StoredUsageEntry {
  // To save disk space, we only store a hash of the pst.
  uint8_t pst_hash[SHA256_DIGEST_LENGTH];
  int64_t time_of_license_received;
  int64_t time_of_first_decrypt;
  int64_t time_of_last_decrypt;
  enum OEMCrypto_Usage_Entry_Status status;
  uint8_t mac_key_server[wvcdm::MAC_KEY_SIZE];
  uint8_t mac_key_client[wvcdm::MAC_KEY_SIZE];
};
typedef union {
  struct StoredUsageEntry entry;
  uint8_t padding[128];  // multiple of block size and bigger than entry size.
} AlignedStoredUsageEntry;

struct StoredUsageTable {
  uint8_t signature[SHA256_DIGEST_LENGTH];
  uint8_t iv[wvcdm::KEY_IV_SIZE];
  int64_t generation;
  size_t count;
  AlignedStoredUsageEntry entries[];
};

class UsageTableEntry {
 public:
  UsageTableEntry(const std::vector<uint8_t> &pst_hash, SessionContext *ctx);
  UsageTableEntry(const StoredUsageEntry *buffer);
  ~UsageTableEntry();
  void SaveToBuffer(StoredUsageEntry *buffer);
  OEMCrypto_Usage_Entry_Status status() const { return status_; }
  void Deactivate();
  bool UpdateTime();
  OEMCryptoResult ReportUsage(SessionContext *session,
                              const std::vector<uint8_t> &pst,
                              OEMCrypto_PST_Report *buffer,
                              size_t *buffer_length);
  // Set them if not set, verify if already set.
  bool VerifyOrSetMacKeys(const std::vector<uint8_t> &server,
                          const std::vector<uint8_t> &client);
  const std::vector<uint8_t> &pst_hash() const { return pst_hash_; }
  void set_session(SessionContext *session) { session_ = session; }

 private:
  std::vector<uint8_t> pst_hash_;
  int64_t time_of_license_received_;
  int64_t time_of_first_decrypt_;
  int64_t time_of_last_decrypt_;
  enum OEMCrypto_Usage_Entry_Status status_;
  std::vector<uint8_t> mac_key_server_;
  std::vector<uint8_t> mac_key_client_;

  SessionContext *session_;
  static wvcdm::Clock clock_;
};

class UsageTable {
 public:
  UsageTable(CryptoEngine *ce);
  ~UsageTable() { Clear(); }
  UsageTableEntry *FindEntry(const std::vector<uint8_t> &pst);
  UsageTableEntry *CreateEntry(const std::vector<uint8_t> &pst,
                               SessionContext *ctx);
  OEMCryptoResult UpdateTable();
  OEMCryptoResult DeactivateEntry(const std::vector<uint8_t> &pst);
  bool DeleteEntry(const std::vector<uint8_t> &pst);
  void Clear();

 private:
  UsageTableEntry *FindEntryLocked(const std::vector<uint8_t> &pst);
  bool SaveToFile();
  bool ComputeHash(const std::vector<uint8_t> &pst,
                   std::vector<uint8_t> &pst_hash);

  typedef std::map<std::vector<uint8_t>, UsageTableEntry *> EntryMap;
  EntryMap table_;
  wvcdm::Lock lock_;
  int64_t generation_;
  CryptoEngine *ce_;
};

};  // namespace wvoec_mock

#endif  // WVOEC_MOCK_OEMCRYPTO_USAGE_TABLE_MOCK_H_
