// Copyright 2013 Google Inc. All Rights Reserved.
//
//  Mock implementation of OEMCrypto APIs
//
#include "oemcrypto_engine_mock.h"

#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <string.h>

#include "log.h"
#include "oemcrypto_key_mock.h"
#include "oemcrypto_logging.h"
#include "oemcrypto_usage_table_mock.h"
#include "openssl/aes.h"
#include "openssl/bio.h"
#include "openssl/cmac.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/hmac.h"
#include "openssl/rand.h"
#include <openssl/rsa.h>
#include "openssl/sha.h"
#include "openssl/x509.h"
#include "string_conversions.h"
#include "wv_cdm_constants.h"

static const int kPssSaltLength = 20;

namespace {
// Increment counter for AES-CTR.  The CENC spec specifies we increment only
// the low 64 bits of the IV counter, and leave the high 64 bits alone.
void ctr128_inc64(uint8_t* counter) {
  uint32_t n = 16;
  do {
    if (++counter[--n] != 0) return;
  } while (n > 8);
}
void dump_openssl_error() {
  while (unsigned long err = ERR_get_error()) {
    char buffer[120];
    LOGE("openssl error -- %lu -- %s",
         err, ERR_error_string(err, buffer));
  }
}
}  // namespace

namespace wvoec_mock {

SessionKeyTable::~SessionKeyTable() {
  for (KeyMap::iterator i = keys_.begin(); i != keys_.end(); ++i) {
    if (NULL != i->second) {
      delete i->second;
    }
  }
}

bool SessionKeyTable::Insert(const KeyId key_id, const Key& key_data) {
  if (keys_.find(key_id) != keys_.end()) return false;
  keys_[key_id] = new Key(key_data);
  return true;
}

Key* SessionKeyTable::Find(const KeyId key_id) {
  if (keys_.find(key_id) == keys_.end()) {
    return NULL;
  }
  return keys_[key_id];
}

void SessionKeyTable::Remove(const KeyId key_id) {
  if (keys_.find(key_id) != keys_.end()) {
    delete keys_[key_id];
    keys_.erase(key_id);
  }
}

void SessionKeyTable::UpdateDuration(const KeyControlBlock& control) {
  for (KeyMap::iterator it = keys_.begin(); it != keys_.end(); ++it) {
    it->second->UpdateDuration(control);
  }
}

SessionContext::~SessionContext() {
  if (usage_entry_) usage_entry_->set_session(NULL);
}

// Internal utility function to derive key using CMAC-128
bool SessionContext::DeriveKey(const std::vector<uint8_t>& key,
                               const std::vector<uint8_t>& context,
                               int counter,
                               std::vector<uint8_t>* out) {
  if (key.empty() || counter > 4 || context.empty() || out == NULL) {
    LOGE("[DeriveKey(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return false;
  }

  const EVP_CIPHER* cipher = EVP_aes_128_cbc();
  CMAC_CTX* cmac_ctx = CMAC_CTX_new();

  if (!CMAC_Init(cmac_ctx, &key[0], key.size(), cipher, 0)) {
    LOGE("[DeriveKey(): OEMCrypto_ERROR_CMAC_FAILURE]");
    return false;
  }

  std::vector<uint8_t> message;
  message.push_back(counter);
  message.insert(message.end(), context.begin(), context.end());

  if (!CMAC_Update(cmac_ctx, &message[0], message.size())) {
    LOGE("[DeriveKey(): OEMCrypto_ERROR_CMAC_FAILURE]");
    return false;
  }

  size_t reslen;
  uint8_t res[128];
  if (!CMAC_Final(cmac_ctx, res, &reslen)) {
    LOGE("[DeriveKey(): OEMCrypto_ERROR_CMAC_FAILURE]");
    return false;
  }

  out->assign(res, res + reslen);

  CMAC_CTX_free(cmac_ctx);

  return true;
}

bool SessionContext::DeriveKeys(const std::vector<uint8_t>& master_key,
                                const std::vector<uint8_t>& mac_key_context,
                                const std::vector<uint8_t>& enc_key_context) {
  // Generate derived key for mac key
  std::vector<uint8_t> mac_key_server;
  std::vector<uint8_t> mac_key_client;
  std::vector<uint8_t> mac_key_part2;
  if (!DeriveKey(master_key, mac_key_context, 1, &mac_key_server)) {
    return false;
  }
  if (!DeriveKey(master_key, mac_key_context, 2, &mac_key_part2)) {
    return false;
  }
  mac_key_server.insert(mac_key_server.end(), mac_key_part2.begin(),
                        mac_key_part2.end());

  if (!DeriveKey(master_key, mac_key_context, 3, &mac_key_client)) {
    return false;
  }
  if (!DeriveKey(master_key, mac_key_context, 4, &mac_key_part2)) {
    return false;
  }
  mac_key_client.insert(mac_key_client.end(), mac_key_part2.begin(),
                        mac_key_part2.end());

  // Generate derived key for encryption key
  std::vector<uint8_t> enc_key;
  if (!DeriveKey(master_key, enc_key_context, 1, &enc_key)) {
    return false;
  }

  if (LogCategoryEnabled(kLoggingDumpDerivedKeys)) {
    LOGI(("  mac_key_context = " + wvcdm::b2a_hex(mac_key_context)).c_str());
    LOGI(("  enc_key_context = " + wvcdm::b2a_hex(enc_key_context)).c_str());
    LOGI(("  mac_key_server = " + wvcdm::b2a_hex(mac_key_server)).c_str());
    LOGI(("  mac_key_client = " + wvcdm::b2a_hex(mac_key_client)).c_str());
    LOGI(("  enc_key = " + wvcdm::b2a_hex(enc_key)).c_str());
  }

  set_mac_key_server(mac_key_server);
  set_mac_key_client(mac_key_client);
  set_encryption_key(enc_key);
  return true;
}

bool SessionContext::RSADeriveKeys(const std::vector<uint8_t>& enc_session_key,
                                   const std::vector<uint8_t>& mac_key_context,
                                   const std::vector<uint8_t>& enc_key_context) {
  if (!rsa_key_) {
    LOGE("[RSADeriveKeys(): no RSA key set]");
    return false;
  }
  if (enc_session_key.size() != static_cast<size_t>(RSA_size(rsa_key_))) {
    LOGE("[RSADeriveKeys(): encrypted session key wrong size:%zu, expected %d]",
         enc_session_key.size(), RSA_size(rsa_key_));
    dump_openssl_error();
    return false;
  }
  session_key_.resize(RSA_size(rsa_key_));
  int decrypted_size = RSA_private_decrypt(enc_session_key.size(),
                                           &enc_session_key[0],
                                           &session_key_[0], rsa_key_,
                                           RSA_PKCS1_OAEP_PADDING);
  if (-1 == decrypted_size) {
    LOGE("[RSADeriveKeys(): error decrypting session key.]");
    dump_openssl_error();
    return false;
  }
  session_key_.resize(decrypted_size);
  if (decrypted_size != static_cast<int>(wvcdm::KEY_SIZE)) {
    LOGE("[RSADeriveKeys(): error.  session key is wrong size: %d.]",
         decrypted_size);
    dump_openssl_error();
    session_key_.clear();
    return false;
  }
  return DeriveKeys(session_key_, mac_key_context, enc_key_context);
}

// Utility function to generate a message signature
bool SessionContext::GenerateSignature(const uint8_t* message,
                                       size_t message_length,
                                       uint8_t* signature,
                                       size_t* signature_length) {
  if (message == NULL || message_length == 0 ||
      signature == NULL || signature_length == 0) {
    LOGE("[OEMCrypto_GenerateSignature(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return false;
  }

  if (mac_key_client_.empty() ||
      mac_key_client_.size() != wvcdm::MAC_KEY_SIZE) {
    LOGE("[GenerateSignature(): No MAC Key]");
    return false;
  }

  if (*signature_length < SHA256_DIGEST_LENGTH) {
    *signature_length = SHA256_DIGEST_LENGTH;
    return false;
  }

  unsigned int md_len = *signature_length;
  if (HMAC(EVP_sha256(), &mac_key_client_[0], mac_key_client_.size(),
           message, message_length, signature, &md_len)) {
    *signature_length = md_len;
    return true;
  }
  return false;
}

size_t SessionContext::RSASignatureSize() {
  if (!rsa_key_) {
    LOGE("[GenerateRSASignature(): no RSA key set]");
    return 0;
  }
  return static_cast<size_t>(RSA_size(rsa_key_));
}

bool SessionContext::GenerateRSASignature(const uint8_t* message,
                                          size_t message_length,
                                          uint8_t* signature,
                                          size_t* signature_length,
                                          RSA_Padding_Scheme padding_scheme) {
  if (message == NULL || message_length == 0 ||
      signature == NULL || signature_length == 0) {
    LOGE("[GenerateRSASignature(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return false;
  }
  if (!rsa_key_) {
    LOGE("[GenerateRSASignature(): no RSA key set]");
    return false;
  }
  if (*signature_length < static_cast<size_t>(RSA_size(rsa_key_))) {
    *signature_length = RSA_size(rsa_key_);
    return false;
  }
  if ((padding_scheme & allowed_schemes_) != padding_scheme) {
    LOGE("[GenerateRSASignature(): padding_scheme not allowed]");
    return false;
  }

  if (padding_scheme == kSign_RSASSA_PSS) {
    // Hash the message using SHA1.
    uint8_t hash[SHA_DIGEST_LENGTH];
    if (!SHA1(message, message_length, hash)) {
      LOGE("[GeneratRSASignature(): error creating signature hash.]");
      dump_openssl_error();
      return false;
    }

    // Add PSS padding.
    std::vector<uint8_t> padded_digest(*signature_length);
    int status = RSA_padding_add_PKCS1_PSS(rsa_key_, &padded_digest[0], hash,
                                           EVP_sha1(), kPssSaltLength);
    if (status == -1) {
      LOGE("[GeneratRSASignature(): error padding hash.]");
      dump_openssl_error();
      return false;
    }

    // Encrypt PSS padded digest.
    status = RSA_private_encrypt(*signature_length, &padded_digest[0], signature,
                                 rsa_key_, RSA_NO_PADDING);
    if (status == -1) {
      LOGE("[GeneratRSASignature(): error in private encrypt.]");
      dump_openssl_error();
      return false;
    }
  } else if (padding_scheme == kSign_PKCS1_Block1) {
    if (message_length > 83) {
      LOGE("[GeneratRSASignature(): RSA digest too large.]");
      return false;
    }
    // Pad the message with PKCS1 padding, and then encrypt.
    size_t status = RSA_private_encrypt(message_length, message, signature,
                                        rsa_key_, RSA_PKCS1_PADDING);
    if (status != *signature_length) {
      LOGE("[GeneratRSASignature(): error in RSA private encrypt. status=%d]", status);
      dump_openssl_error();
      return false;
    }
  } else {  // Bad RSA_Padding_Scheme
    return false;
  }
  return true;
}

// Validate message signature
bool SessionContext::ValidateMessage(const uint8_t* given_message,
                                     size_t message_length,
                                     const uint8_t* given_signature,
                                     size_t signature_length) {
  if (signature_length != SHA256_DIGEST_LENGTH) {
    return false;
  }
  uint8_t computed_signature[SHA256_DIGEST_LENGTH];
  unsigned int md_len = SHA256_DIGEST_LENGTH;
  if (!HMAC(EVP_sha256(), &mac_key_server_[0], mac_key_server_.size(),
            given_message, message_length, computed_signature, &md_len)) {
    LOGE("ValidateMessage: Could not compute signature.");
    return false;
  }
  if (memcmp(given_signature, computed_signature, signature_length)) {
    LOGE("Invalid signature    given: %s",
         wvcdm::HexEncode(given_signature, signature_length).c_str());
    LOGE("Invalid signature computed: %s",
         wvcdm::HexEncode(computed_signature, signature_length).c_str());
    return false;
  }
  return true;
}

bool SessionContext::CheckNonceOrEntry(const KeyControlBlock& key_control_block,
                                       const std::vector<uint8_t>& pst) {
  switch (key_control_block.control_bits() & kControlReplayMask) {
    case kControlNonceRequired:  // Online license. Nonce always required.
      if (!CheckNonce(key_control_block.nonce())) return false;
      if (pst.size() == 0) {
        LOGE("KCB: PST null for kControlNonceRequired.");
        return false;
      }
      if (!(key_control_block.control_bits() & kControlNonceEnabled)) {
        LOGE("KCB: Server provided Nonce_Required but Nonce_Enabled = 0.");
      }
      if (!usage_entry_) {
        if (ce_->usage_table()->FindEntry(pst)) {
          LOGE("KCB: Cannot create duplicate entries in usage table.");
          return false;
        }
        usage_entry_ = ce_->usage_table()->CreateEntry(pst, this);
      }
      break;  // Offline license. Nonce required on first use.
    case kControlNonceOrEntry:
      if (key_control_block.control_bits() & kControlNonceEnabled) {
        LOGE("KCB: Server provided NonceOrEntry but Nonce_Enabled = 1.");
      }
      if (pst.size() == 0) {
        LOGE("KCB: PST null for kControlNonceOrEntry.");
        return false;
      }
      if (!usage_entry_) {
        usage_entry_ = ce_->usage_table()->FindEntry(pst);
        if (usage_entry_) {
          if (usage_entry_->status() == kInactive) return false;
        } else {
          if (!CheckNonce(key_control_block.nonce())) return false;
          usage_entry_ = ce_->usage_table()->CreateEntry(pst, this);
        }
      } else {
        if (usage_entry_->status() == kInactive) return false;
      }
      break;  // Usage table not required.  Look at nonce enabled bit.
    default:
      if ((key_control_block.control_bits() & kControlNonceEnabled) &&
          (!CheckNonce(key_control_block.nonce()))) {
        LOGE("KCB: BAD Nonce");
        return false;
      }
  }
  return true;
}

void SessionContext::StartTimer() {
  timer_start_ = time(NULL);
}

uint32_t SessionContext::CurrentTimer() {
  time_t now = time(NULL);
  return now - timer_start_;
}

OEMCryptoResult SessionContext::LoadKeys(
    const uint8_t* message, size_t message_length, const uint8_t* signature,
    size_t signature_length, const uint8_t* enc_mac_key_iv,
    const uint8_t* enc_mac_keys, size_t num_keys,
    const OEMCrypto_KeyObject* key_array, const uint8_t* pst,
    size_t pst_length) {
  // Validate message signature
  if (!ValidateMessage(message, message_length, signature, signature_length)) {
    return OEMCrypto_ERROR_SIGNATURE_FAILURE;
  }

  StartTimer();

  // Decrypt and install keys in key object
  // Each key will have a key control block.  They will all have the same nonce.
  bool status = true;
  std::vector<uint8_t> key_id;
  std::vector<uint8_t> enc_key_data;
  std::vector<uint8_t> key_data_iv;
  std::vector<uint8_t> key_control;
  std::vector<uint8_t> key_control_iv;
  std::vector<uint8_t> pstv;
  if (pst_length > 0) pstv.assign(pst, pst + pst_length);
  for (unsigned int i = 0; i < num_keys; i++) {
    key_id.assign(key_array[i].key_id,
                  key_array[i].key_id + key_array[i].key_id_length);
    enc_key_data.assign(key_array[i].key_data,
                        key_array[i].key_data + key_array[i].key_data_length);
    key_data_iv.assign(key_array[i].key_data_iv,
                       key_array[i].key_data_iv + wvcdm::KEY_IV_SIZE);
    if (key_array[i].key_control == NULL) {
      status = false;
      break;
    }
    key_control.assign(key_array[i].key_control,
                       key_array[i].key_control + wvcdm::KEY_CONTROL_SIZE);
    key_control_iv.assign(key_array[i].key_control_iv,
                          key_array[i].key_control_iv + wvcdm::KEY_IV_SIZE);

    if (!InstallKey(key_id, enc_key_data, key_data_iv, key_control,
                    key_control_iv, pstv)) {
      status = false;
      break;
    }
  }
  FlushNonces();
  if (!status) return OEMCrypto_ERROR_UNKNOWN_FAILURE;

  // enc_mac_key can be NULL if license renewal is not supported
  if (enc_mac_keys != NULL) {
    // V2.1 license protocol: update mac keys after processing license response
    const std::vector<uint8_t> enc_mac_keys_str = std::vector<uint8_t>(
        enc_mac_keys, enc_mac_keys + 2 * wvcdm::MAC_KEY_SIZE);
    const std::vector<uint8_t> enc_mac_key_iv_str = std::vector<uint8_t>(
        enc_mac_key_iv, enc_mac_key_iv + wvcdm::KEY_IV_SIZE);

    if (!UpdateMacKeys(enc_mac_keys_str, enc_mac_key_iv_str)) {
      return OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  if (usage_entry_) {
    if (!usage_entry_->VerifyOrSetMacKeys(mac_key_server_, mac_key_client_)) {
      return OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  return OEMCrypto_SUCCESS;
}

bool SessionContext::InstallKey(const KeyId& key_id,
                                const std::vector<uint8_t>& key_data,
                                const std::vector<uint8_t>& key_data_iv,
                                const std::vector<uint8_t>& key_control,
                                const std::vector<uint8_t>& key_control_iv,
                                const std::vector<uint8_t>& pst) {
  // Decrypt encrypted key_data using derived encryption key and offered iv
  std::vector<uint8_t> content_key;
  std::vector<uint8_t> key_control_str;

  if (!DecryptMessage(encryption_key_, key_data_iv, key_data, &content_key)) {
    LOGE("[Installkey(): Could not decrypt key data]");
    return false;
  }

  if (LogCategoryEnabled(kLoggingDumpContentKeys)) {
    LOGI(("  InstallKey: key_id      = " +
          wvcdm::b2a_hex(key_id)).c_str());
    LOGI(("  InstallKey: content_key = " +
          wvcdm::b2a_hex(content_key)).c_str());
    LOGI(("  InstallKey: key_control = " +
          wvcdm::b2a_hex(key_control_str)).c_str());
  }

  // Key control must be supplied by license server
  if (key_control.empty()) {
    LOGE("[Installkey(): WARNING: No Key Control]");
    return false;
  }
  if (key_control_iv.empty()) {
    LOGE("[Installkey(): ERROR: No Key Control IV]");
    return false;
  }
  if (!DecryptMessage(content_key, key_control_iv, key_control,
                      &key_control_str)) {
    LOGE("[Installkey(): ERROR: Could not decrypt content key]");
    return false;
  }

  KeyControlBlock key_control_block(key_control_str);
  if (!key_control_block.valid()) {
    LOGE("Error parsing key control.");
    return false;
  }
  if (!CheckNonceOrEntry(key_control_block, pst)) {
    LOGE("Failed Nonce/PST check.");
    return false;
  }

  Key key(content_key, key_control_block);
  session_keys_.Insert(key_id, key);
  return true;
}

bool SessionContext::RefreshKey(const KeyId& key_id,
                                const std::vector<uint8_t>& key_control,
                                const std::vector<uint8_t>& key_control_iv) {
  if (key_id.empty()) {
    // Key control is not encrypted if key id is NULL
    KeyControlBlock key_control_block(key_control);
    if (!key_control_block.valid()) {
      LOGE("Parse key control error.");
      return false;
    }
    if ((key_control_block.control_bits() & kControlNonceEnabled) &&
        (!CheckNonce(key_control_block.nonce()))) {
      LOGE("KCB: BAD Nonce");
      return false;
    }
    // Apply duration to all keys in this session
    session_keys_.UpdateDuration(key_control_block);
    return true;
  }

  Key* content_key = session_keys_.Find(key_id);

  if (NULL == content_key) {
    if (LogCategoryEnabled(kLoggingDumpKeyControlBlocks)) {
      LOGD("Error: no matching content key.");
    }
    return false;
  }

  if (key_control.empty()) {
    if (LogCategoryEnabled(kLoggingDumpKeyControlBlocks)) {
      LOGD("Error: no key_control.");
    }
    return false;
  }

  const std::vector<uint8_t> content_key_value = content_key->value();

  // Decrypt encrypted key control block
  std::vector<uint8_t> control;
  if (key_control_iv.empty()) {
    if (LogCategoryEnabled(kLoggingDumpKeyControlBlocks)) {
      LOGD("Key control block is NOT encrypted.");
    }
    control = key_control;
  } else {
    if (LogCategoryEnabled(kLoggingDumpKeyControlBlocks)) {
      LOGD("Key control block is encrypted.");
    }
    if (!DecryptMessage(content_key_value, key_control_iv, key_control,
                        &control)) {
      if (LogCategoryEnabled(kLoggingDumpKeyControlBlocks)) {
        LOGD("Error decrypting key control block.");
      }
      return false;
    }
  }

  KeyControlBlock key_control_block(control);
  if (!key_control_block.valid()) {
    if (LogCategoryEnabled(kLoggingDumpKeyControlBlocks)) {
      LOGD("Parse key control error.");
    }
    return false;
  }
  if ((key_control_block.control_bits() & kControlNonceEnabled) &&
      (!CheckNonce(key_control_block.nonce()))) {
    LOGE("KCB: BAD Nonce");
    return false;
  }
  content_key->UpdateDuration(key_control_block);
  return true;
}

bool  SessionContext::DecryptRSAKey(const uint8_t* enc_rsa_key,
                                    size_t enc_rsa_key_length,
                                    const uint8_t* enc_rsa_key_iv,
                                    uint8_t* pkcs8_rsa_key)  {
  // Decrypt rsa key with keybox.
  uint8_t iv_buffer[ wvcdm::KEY_IV_SIZE];
  memcpy(iv_buffer, enc_rsa_key_iv, wvcdm::KEY_IV_SIZE);
  AES_KEY aes_key;
  AES_set_decrypt_key(&encryption_key_[0], 128, &aes_key);
  AES_cbc_encrypt(enc_rsa_key, pkcs8_rsa_key, enc_rsa_key_length,
                  &aes_key, iv_buffer, AES_DECRYPT);
  return true;
}

bool SessionContext::EncryptRSAKey(const uint8_t* pkcs8_rsa_key,
                                   size_t enc_rsa_key_length,
                                   const uint8_t* enc_rsa_key_iv,
                                   uint8_t* enc_rsa_key) {
  // Encrypt rsa key with keybox.
  uint8_t iv_buffer[ wvcdm::KEY_IV_SIZE];
  memcpy(iv_buffer, enc_rsa_key_iv, wvcdm::KEY_IV_SIZE);
  AES_KEY aes_key;
  AES_set_encrypt_key(&encryption_key_[0], 128, &aes_key);
  AES_cbc_encrypt(pkcs8_rsa_key, enc_rsa_key, enc_rsa_key_length,
                  &aes_key, iv_buffer, AES_ENCRYPT);
  return true;
}

bool SessionContext::LoadRSAKey(uint8_t* pkcs8_rsa_key,
                                size_t rsa_key_length,
                                const uint8_t* message,
                                size_t message_length,
                                const uint8_t* signature,
                                size_t signature_length) {
  // Validate message signature
  if (!ValidateMessage(message, message_length, signature, signature_length)) {
    LOGE("[LoadRSAKey(): Could not verify signature]");
    return false;
  }
  if (rsa_key_) {
    RSA_free(rsa_key_);
    rsa_key_ = NULL;
  }
  if (rsa_key_length < 8) {
    LOGE("[LoadRSAKey(): Very Short Buffer]");
    return false;
  }
  if( (memcmp(pkcs8_rsa_key, "SIGN", 4) == 0) ) {
    uint32_t *schemes_n = (uint32_t *)(pkcs8_rsa_key + 4);
    allowed_schemes_ = htonl(*schemes_n);
    pkcs8_rsa_key += 8;
    rsa_key_length -= 8;
  }
  BIO *bio = BIO_new_mem_buf(pkcs8_rsa_key, rsa_key_length);
  if ( bio == NULL ) {
    LOGE("[LoadRSAKey(): Could not allocate bio buffer]");
    return false;
  }
  bool success = true;
  PKCS8_PRIV_KEY_INFO *pkcs8_pki = d2i_PKCS8_PRIV_KEY_INFO_bio(bio, NULL);
  if (pkcs8_pki == NULL) {
    LOGE("d2i_PKCS8_PRIV_KEY_INFO_bio returned NULL.");
    success = false;
  }
  EVP_PKEY *evp = NULL;
  if (success) {
    evp = EVP_PKCS82PKEY(pkcs8_pki);
    if (evp == NULL) {
      LOGE("EVP_PKCS82PKEY returned NULL.");
      success = false;
    }
  }
  if (success) {
    rsa_key_ = EVP_PKEY_get1_RSA(evp);
    if (rsa_key_ == NULL) {
      LOGE("PrivateKeyInfo did not contain an RSA key.");
      success = false;
    }
  }
  if (evp != NULL) {
    EVP_PKEY_free(evp);
  }
  if (pkcs8_pki != NULL) {
    PKCS8_PRIV_KEY_INFO_free(pkcs8_pki);
  }
  BIO_free(bio);
  if (!success) {
    return false;
  }
  switch (RSA_check_key(rsa_key_)) {
  case 1:  // valid.
    return true;
  case 0:  // not valid.
    LOGE("[LoadRSAKey(): rsa key not valid]");
    dump_openssl_error();
    return false;
  default:  // -1 == check failed.
    LOGE("[LoadRSAKey(): error checking rsa key]");
    dump_openssl_error();
    return false;
  }
}

OEMCryptoResult SessionContext::Generic_Encrypt(const uint8_t* in_buffer,
                                                size_t buffer_length,
                                                const uint8_t* iv,
                                                OEMCrypto_Algorithm algorithm,
                                                uint8_t* out_buffer) {
  // Check there is a content key
  if (current_content_key() == NULL) {
    LOGE("[Generic_Encrypt(): OEMCrypto_ERROR_NO_CONTENT_KEY]");
    return OEMCrypto_ERROR_NO_CONTENT_KEY;
  }
  const std::vector<uint8_t>& key = current_content_key()->value();
  const KeyControlBlock& control = current_content_key()->control();
  // Set the AES key.
  if (static_cast<int>(key.size()) != AES_BLOCK_SIZE) {
    LOGE("[Generic_Encrypt(): CONTENT_KEY has wrong size: %d", key.size());
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (!(control.control_bits() & kControlAllowEncrypt)) {
    LOGE("[Generic_Encrypt(): control bit says not allowed.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (control.duration() > 0) {
    if (control.duration() < CurrentTimer()) {
      LOGE("[Generic_Encrypt(): key expired.");
      return OEMCrypto_ERROR_KEY_EXPIRED;
    }
  }
  if (control.control_bits() & kControlReplayMask) {
    if (!IsUsageEntryValid()) {
      LOGE("[Generic_Encrypt(): usage entry not valid]");
      return OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  if ( algorithm != OEMCrypto_AES_CBC_128_NO_PADDING ) {
    LOGE("[Generic_Encrypt(): algorithm bad.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if ( buffer_length % AES_BLOCK_SIZE != 0 ) {
    LOGE("[Generic_Encrypt(): buffers size bad.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  const uint8_t* key_u8 = &key[0];
  AES_KEY aes_key;
  if (AES_set_encrypt_key(key_u8, AES_BLOCK_SIZE * 8, &aes_key) != 0) {
    LOGE("[Generic_Encrypt(): FAILURE]");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  uint8_t iv_buffer[ wvcdm::KEY_IV_SIZE];
  memcpy(iv_buffer, iv, wvcdm::KEY_IV_SIZE);
  AES_cbc_encrypt(in_buffer, out_buffer, buffer_length,
                  &aes_key, iv_buffer, AES_ENCRYPT);
  return OEMCrypto_SUCCESS;
}

OEMCryptoResult SessionContext::Generic_Decrypt(const uint8_t* in_buffer,
                                                size_t buffer_length,
                                                const uint8_t* iv,
                                                OEMCrypto_Algorithm algorithm,
                                                uint8_t* out_buffer) {
  // Check there is a content key
  if (current_content_key() == NULL) {
    LOGE("[Generic_Decrypt(): OEMCrypto_ERROR_NO_CONTENT_KEY]");
    return OEMCrypto_ERROR_NO_CONTENT_KEY;
  }
  const std::vector<uint8_t>& key = current_content_key()->value();
  const KeyControlBlock& control = current_content_key()->control();
  // Set the AES key.
  if (static_cast<int>(key.size()) != AES_BLOCK_SIZE) {
    LOGE("[Generic_Decrypt(): CONTENT_KEY has wrong size.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (!(control.control_bits() & kControlAllowDecrypt)) {
    LOGE("[Generic_Decrypt(): control bit says not allowed.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (control.control_bits() & kControlDataPathSecure) {
    LOGE("[Generic_Decrypt(): control bit says secure path only.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (control.duration() > 0) {
    if (control.duration() < CurrentTimer()) {
      LOGE("[Generic_Decrypt(): key expired.");
      return OEMCrypto_ERROR_KEY_EXPIRED;
    }
  }
  if (control.control_bits() & kControlReplayMask) {
    if (!IsUsageEntryValid()) {
      LOGE("[Generic_Decrypt(): usage entry not valid]");
      return OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  if ( algorithm != OEMCrypto_AES_CBC_128_NO_PADDING ) {
    LOGE("[Generic_Decrypt(): bad algorithm.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if ( buffer_length % AES_BLOCK_SIZE != 0 ) {
    LOGE("[Generic_Decrypt(): bad buffer size.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  const uint8_t* key_u8 = &key[0];
  AES_KEY aes_key;
  if (AES_set_decrypt_key(key_u8, AES_BLOCK_SIZE * 8, &aes_key) != 0) {
    LOGE("[Generic_Decrypt(): FAILURE]");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  uint8_t iv_buffer[ wvcdm::KEY_IV_SIZE];
  memcpy(iv_buffer, iv, wvcdm::KEY_IV_SIZE);
  AES_cbc_encrypt(in_buffer, out_buffer, buffer_length,
                  &aes_key, iv_buffer, AES_DECRYPT);
  return OEMCrypto_SUCCESS;
}

OEMCryptoResult SessionContext::Generic_Sign(const uint8_t* in_buffer,
                                             size_t buffer_length,
                                             OEMCrypto_Algorithm algorithm,
                                             uint8_t* signature,
                                             size_t* signature_length) {
  // Check there is a content key
  if (current_content_key() == NULL) {
    LOGE("[Generic_Sign(): OEMCrypto_ERROR_NO_CONTENT_KEY]");
    return OEMCrypto_ERROR_NO_CONTENT_KEY;
  }
  if (*signature_length < SHA256_DIGEST_LENGTH) {
    *signature_length = SHA256_DIGEST_LENGTH;
    LOGE("[Generic_Sign(): bad signature length.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  const std::vector<uint8_t>& key = current_content_key()->value();
  const KeyControlBlock& control = current_content_key()->control();
  if (static_cast<int>(key.size()) != SHA256_DIGEST_LENGTH) {
    LOGE("[Generic_Sign(): CONTENT_KEY has wrong size; %d", key.size());
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (!(control.control_bits() & kControlAllowSign)) {
    LOGE("[Generic_Sign(): control bit says not allowed.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (control.duration() > 0) {
    if (control.duration() < CurrentTimer()) {
      LOGE("[Generic_Sign(): key expired.");
      return OEMCrypto_ERROR_KEY_EXPIRED;
    }
  }
  if (control.control_bits() & kControlReplayMask) {
    if (!IsUsageEntryValid()) {
      LOGE("[Generic_Sign(): usage entry not valid]");
      return OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  if( algorithm != OEMCrypto_HMAC_SHA256 ) {
    LOGE("[Generic_Sign(): bad algorithm.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  unsigned int md_len = *signature_length;
  if (HMAC(EVP_sha256(), &key[0], key.size(),
           in_buffer, buffer_length, signature, &md_len)) {
    *signature_length = md_len;
    return OEMCrypto_SUCCESS;
  }
  LOGE("[Generic_Sign(): hmac failed.");
  dump_openssl_error();
  return OEMCrypto_ERROR_UNKNOWN_FAILURE;
}

OEMCryptoResult SessionContext::Generic_Verify(const uint8_t* in_buffer,
                                               size_t buffer_length,
                                               OEMCrypto_Algorithm algorithm,
                                               const uint8_t* signature,
                                               size_t signature_length) {
  // Check there is a content key
  if (current_content_key() == NULL) {
    LOGE("[Decrypt_Verify(): OEMCrypto_ERROR_NO_CONTENT_KEY]");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (signature_length < SHA256_DIGEST_LENGTH) {
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  const std::vector<uint8_t>& key = current_content_key()->value();
  const KeyControlBlock& control = current_content_key()->control();
  if (static_cast<int>(key.size()) != SHA256_DIGEST_LENGTH) {
    LOGE("[Generic_Verify(): CONTENT_KEY has wrong size: %d", key.size());
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (!(control.control_bits() & kControlAllowVerify)) {
    LOGE("[Generic_Verify(): control bit says not allowed.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (control.duration() > 0) {
    if (control.duration() < CurrentTimer()) {
      LOGE("[Generic_Verify(): key expired.");
      return OEMCrypto_ERROR_KEY_EXPIRED;
    }
  }
  if (control.control_bits() & kControlReplayMask) {
    if (!IsUsageEntryValid()) {
      LOGE("[Generic_Verify(): usage entry not valid]");
      return OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  if ( algorithm != OEMCrypto_HMAC_SHA256 ) {
    LOGE("[Generic_Verify(): bad algorithm.");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  unsigned int md_len = signature_length;
  uint8_t computed_signature[SHA256_DIGEST_LENGTH];
  if (HMAC(EVP_sha256(), &key[0], key.size(),
           in_buffer, buffer_length, computed_signature, &md_len)) {
    if (0 == memcmp(signature, computed_signature, SHA256_DIGEST_LENGTH)) {
      return OEMCrypto_SUCCESS;
    } else {
      return OEMCrypto_ERROR_SIGNATURE_FAILURE;
    }
  }
  LOGE("[Generic_Verify(): HMAC failed.");
  dump_openssl_error();
  return OEMCrypto_ERROR_UNKNOWN_FAILURE;
}

bool SessionContext::UpdateMacKeys(const std::vector<uint8_t>& enc_mac_keys,
                                   const std::vector<uint8_t>& iv) {
  // Decrypt mac key from enc_mac_key using device_keya
  std::vector<uint8_t> mac_keys;
  if (!DecryptMessage(encryption_key_, iv, enc_mac_keys, &mac_keys)) {
    return false;
  }
  mac_key_server_ = std::vector<uint8_t>(mac_keys.begin(),
                                         mac_keys.begin()+wvcdm::MAC_KEY_SIZE);
  mac_key_client_ = std::vector<uint8_t>(mac_keys.begin()+wvcdm::MAC_KEY_SIZE,
                                         mac_keys.end());
  return true;
}

bool SessionContext::SelectContentKey(const KeyId& key_id) {
  const Key* content_key = session_keys_.Find(key_id);

  if (LogCategoryEnabled(kLoggingTraceDecryption)){
    LOGI(( "  Select Key: key_id      = " +
          wvcdm::b2a_hex(key_id) ).c_str());
    LOGI(( "  Select Key: key = " +
          wvcdm::b2a_hex(content_key->value()) ).c_str());
  }

  if (NULL == content_key) {
    LOGE("[SelectContentKey(): No key matches key id]");
    return false;
  }
  current_content_key_ = content_key;
  return true;
}

void SessionContext::AddNonce(uint32_t nonce) {
  nonce_table_.AddNonce(nonce);
}

bool SessionContext::CheckNonce(uint32_t nonce) {
  return nonce_table_.CheckNonce(nonce);
}

void SessionContext::FlushNonces() {
  nonce_table_.Flush();
}

bool SessionContext::IsUsageEntryValid() {
  if (!usage_entry_) return false;
  return usage_entry_->UpdateTime();
}

void SessionContext::ReleaseUsageEntry() { usage_entry_ = NULL; }

CryptoEngine::CryptoEngine() :
    ce_state_(CE_INITIALIZED), current_session_(NULL) {
  valid_ = true;
  ERR_load_crypto_strings();
  // These are made up numbers, just for illustration.
  current_hdcp_capability_ = 0x1;
  maximum_hdcp_capability_ = 0x2;
  // If local_display_ is true, we pretend we are using a built-in display,
  // instead of HDMI or WiFi output.
  local_display_ = false;
  usage_table_ = new UsageTable(this);
}

CryptoEngine::~CryptoEngine() {
  current_session_ = NULL;
  sessions_.clear();
  if (usage_table_) delete usage_table_;
}

void CryptoEngine::Terminate() {
}

KeyboxError CryptoEngine::ValidateKeybox() { return keybox_.Validate(); }

SessionId CryptoEngine::CreateSession() {
  wvcdm::AutoLock lock(session_table_lock_);
  static int unique_id = 1;
  SessionId sid = (SessionId)++unique_id;
  SessionContext* sctx = new SessionContext(this, sid);
  sessions_[sid] = sctx;
  return sid;
}

bool CryptoEngine::DestroySession(SessionId sid) {
  SessionContext* sctx = FindSession(sid);
  wvcdm::AutoLock lock(session_table_lock_);
  if (sctx) {
    sessions_.erase(sid);
    delete sctx;
    return true;
  } else {
    return false;
  }
}

SessionContext* CryptoEngine::FindSession(SessionId sid) {
  wvcdm::AutoLock lock(session_table_lock_);
  ActiveSessions::iterator it = sessions_.find(sid);
  if (it != sessions_.end()) {
    return it->second;
  }
  return NULL;
}

// Internal utility function to decrypt the message
bool SessionContext::DecryptMessage(const std::vector<uint8_t>& key,
                                    const std::vector<uint8_t>& iv,
                                    const std::vector<uint8_t>& message,
                                    std::vector<uint8_t>* decrypted) {
  if (key.empty() || iv.empty() || message.empty() || !decrypted) {
    LOGE("[DecryptMessage(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return false;
  }
  decrypted->resize(message.size());
  uint8_t iv_buffer[16];
  memcpy(iv_buffer, &iv[0], 16);
  AES_KEY aes_key;
  AES_set_decrypt_key(&key[0], 128, &aes_key);
  AES_cbc_encrypt(&message[0], &(decrypted->front()), message.size(),
                  &aes_key, iv_buffer, AES_DECRYPT);
  return true;
}

OEMCryptoResult SessionContext::DecryptCTR(
    const uint8_t* iv, size_t block_offset, const uint8_t* cipher_data,
    size_t cipher_data_length, bool is_encrypted, uint8_t* clear_data,
    BufferType buffer_type) {
  // If the data is clear, we do not need a current key selected.
  if (!is_encrypted) {
    if (buffer_type != kBufferTypeDirect) {
      memcpy(reinterpret_cast<uint8_t*>(clear_data), cipher_data,
             cipher_data_length);
      return OEMCrypto_SUCCESS;
    }
    // For reference implementation, we quietly drop the clear direct video.
    return OEMCrypto_SUCCESS;
  }

  // Check there is a content key
  if (current_content_key() == NULL) {
    LOGE("[DecryptCTR(): OEMCrypto_ERROR_NO_CONTENT_KEY]");
    return OEMCrypto_ERROR_DECRYPT_FAILED;
  }
  const KeyControlBlock& control = current_content_key()->control();
  if (control.control_bits() & kControlDataPathSecure) {
    if (buffer_type == kBufferTypeClear) {
      LOGE("[DecryptCTR(): Secure key with insecure buffer]");
      return OEMCrypto_ERROR_DECRYPT_FAILED;
    }
  }
  if (control.duration() > 0) {
    if (control.duration() < CurrentTimer()) {
      LOGE("[DecryptCTR(): KEY_EXPIRED]");
      return OEMCrypto_ERROR_KEY_EXPIRED;
    }
  }
  if (control.control_bits() & kControlReplayMask) {
    if (!IsUsageEntryValid()) {
      LOGE("[DecryptCTR(): usage entry not valid]");
      return OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  if (!ce_->local_display()) {  // Only look at HDCP if the display is not
                                // local.
    if (control.control_bits() & kControlHDCPRequired) {
      uint8_t required_hdcp =
          (control.control_bits() & kControlHDCPVersionMask) >>
          kControlHDCPVersionShift;
      // For reference implementation, we pretend we can handle the current
      // HDCP version.
      if (required_hdcp > ce_->current_hdcp_capability() ||
          ce_->current_hdcp_capability() == 0) {
        return OEMCrypto_ERROR_INSUFFICIENT_HDCP;
      }
    }
  }
  const std::vector<uint8_t>& content_key = current_content_key()->value();

  // Set the AES key.
  if (static_cast<int>(content_key.size()) != AES_BLOCK_SIZE) {
    LOGE("[DecryptCTR(): CONTENT_KEY has wrong size: %d", content_key.size());
    return OEMCrypto_ERROR_DECRYPT_FAILED;
  }
  const uint8_t* key_u8 = &content_key[0];
  AES_KEY aes_key;
  if (AES_set_encrypt_key(key_u8, AES_BLOCK_SIZE * 8, &aes_key) != 0) {
    LOGE("[DecryptCTR(): FAILURE]");
    return OEMCrypto_ERROR_DECRYPT_FAILED;
  }

  if (buffer_type == kBufferTypeDirect) {
    // For reference implementation, we quietly drop the decrypted direct video.
    return OEMCrypto_SUCCESS;
  }

  if (buffer_type == kBufferTypeSecure) {
    // For reference implementation, we also quietly drop secure data.
    return OEMCrypto_SUCCESS;
  }

  // Local copy (will be modified).
  uint8_t aes_iv[AES_BLOCK_SIZE];
  memcpy(aes_iv, &iv[0], AES_BLOCK_SIZE);

  // Encrypt the IV.
  uint8_t ecount_buf[AES_BLOCK_SIZE];

  // The CENC spec specifies we increment only the low 64 bits of the IV
  // counter, and leave the high 64 bits alone.  This is different from the
  // OpenSSL implementation, which increments the entire 128 bit iv. That is
  // why we implement the CTR loop ourselves.
  size_t l = 0;
  while (l < cipher_data_length) {
    AES_encrypt(aes_iv, ecount_buf, &aes_key);
    for (int n = block_offset; n < AES_BLOCK_SIZE && l < cipher_data_length;
         ++n, ++l) {
      clear_data[l] = cipher_data[l] ^ ecount_buf[n];
    }
    ctr128_inc64(aes_iv);
    block_offset = 0;
  }
  return OEMCrypto_SUCCESS;
}

void NonceTable::AddNonce(uint32_t nonce) {
  int new_slot = -1;
  int oldest_slot = -1;

  // Flush any nonces that have been checked but not flushed.
  // After flush, nonces will be either valid or invalid.
  Flush();

  for (int i = 0; i < kTableSize; ++i) {
    // Increase age of all valid nonces.
    if (kNTStateValid == state_[i]) {
      ++age_[i];
      if (-1 == oldest_slot) {
        oldest_slot = i;
      } else {
        if (age_[i] > age_[oldest_slot]) {
          oldest_slot = i;
        }
      }
    } else {
      if (-1 == new_slot) {
        age_[i] = 0;
        nonces_[i] = nonce;
        state_[i] = kNTStateValid;
        new_slot = i;
      }
    }
  }
  if (-1 == new_slot) {
    // reuse oldest
    // assert (oldest_slot != -1)
    int i = oldest_slot;
    age_[i] = 0;
    nonces_[i] = nonce;
    state_[i] = kNTStateValid;
  }
}

bool NonceTable::CheckNonce(uint32_t nonce) {
  for (int i = 0; i < kTableSize; ++i) {
    if (kNTStateInvalid != state_[i]) {
      if (nonce == nonces_[i]) {
        state_[i] = kNTStateFlushPending;
        return true;
      }
    }
  }
  return false;
}

void NonceTable::Flush() {
  for (int i = 0; i < kTableSize; ++i) {
    if (kNTStateFlushPending == state_[i]) {
      state_[i] = kNTStateInvalid;
    }
  }
}

};  // namespace wvoec_mock
