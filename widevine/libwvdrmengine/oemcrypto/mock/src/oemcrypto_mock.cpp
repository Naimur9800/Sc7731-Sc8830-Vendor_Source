// Copyright 2013 Google Inc. All Rights Reserved.
//
// Reference implementation of OEMCrypto APIs
//
#include "OEMCryptoCENC.h"

#include <iostream>
#include <cstring>
#include <stdio.h>
#include <time.h>
#include <string>
#include <vector>
#include "log.h"
#include "oemcrypto_engine_mock.h"
#include "oemcrypto_logging.h"
#include "oemcrypto_usage_table_mock.h"
#include "openssl/cmac.h"
#include "openssl/evp.h"
#include "openssl/hmac.h"
#include "openssl/rand.h"
#include "openssl/sha.h"
#include "string_conversions.h"
#include "wv_cdm_constants.h"

namespace wvoec_mock {

static CryptoEngine* crypto_engine = NULL;

typedef struct {
  uint8_t signature[wvcdm::MAC_KEY_SIZE];
  uint8_t context[wvcdm::MAC_KEY_SIZE];
  uint8_t iv[wvcdm::KEY_IV_SIZE];
  uint8_t enc_rsa_key[];
} WrappedRSAKey;

extern "C"
OEMCryptoResult OEMCrypto_Initialize(void) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-------------------------  OEMCrypto_Initialize(void)\n");
  }

  crypto_engine = new CryptoEngine;

  if (!crypto_engine || !crypto_engine->Initialized()) {
    LOGE("[OEMCrypto_Initialize(): failed]");
    return OEMCrypto_ERROR_INIT_FAILED;
  }
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGD("[OEMCrypto_Initialize(): success]");
  }
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_Terminate(void) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("----------------- OEMCryptoResult OEMCrypto_Terminate(void)\n");
  }

  if (!crypto_engine) {
    LOGE("[OEMCrypto_Terminate(): failed]");
    return OEMCrypto_ERROR_TERMINATE_FAILED;
  }

  if (crypto_engine->Initialized()) {
    crypto_engine->Terminate();
  }

  delete crypto_engine;
  crypto_engine = NULL;
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGD("[OEMCrypto_Terminate(): success]");
  }
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_OpenSession(OEMCrypto_SESSION* session) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_OpenSession"
         "(OEMCrypto_SESSION *session)\n");
  }
  SessionId sid = crypto_engine->CreateSession();
  *session = (OEMCrypto_SESSION)sid;
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGD("[OEMCrypto_OpenSession(): SID=%08x]", sid);
  }
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_CloseSession(OEMCrypto_SESSION session) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_CloseSession"
         "(OEMCrypto_SESSION session)\n");
  }
  if (!crypto_engine->DestroySession((SessionId)session)) {
    if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
      LOGD("[OEMCrypto_CloseSession(SID=%08X): failed]", session);
    }
    return OEMCrypto_ERROR_CLOSE_SESSION_FAILED;
  } else {
    if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
      LOGD("[OEMCrypto_CloseSession(SID=%08X): success]", session);
    }
    return OEMCrypto_SUCCESS;
  }
}

extern "C"
OEMCryptoResult OEMCrypto_GenerateNonce(OEMCrypto_SESSION session,
                                        uint32_t* nonce) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_GenerateNonce"
         "(OEMCrypto_SESSION session,\n");
  }
  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_GenerateNonce(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  // Prevent nonce flood.
  static time_t last_nonce_time = 0;
  static int nonce_count = 0;
  time_t now = time(NULL);
  if (now == last_nonce_time) {
    nonce_count++;
    if (nonce_count > 20) {
      LOGE("[OEMCrypto_GenerateNonce(): Nonce Flood detected]");
      return OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  } else {
    nonce_count = 1;
    last_nonce_time = now;
  }

  uint32_t nonce_value;
  uint8_t* nonce_string = reinterpret_cast<uint8_t*>(&nonce_value);

  // Generate 4 bytes of random data
  if (!RAND_bytes(nonce_string, 4)) {
    LOGE("[OEMCrypto_GenerateNonce(): Random bytes failure]");
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  session_ctx->AddNonce(nonce_value);
  *nonce = nonce_value;
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("nonce = %08x\n", nonce_value);
  }
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_GenerateDerivedKeys(OEMCrypto_SESSION session,
                                              const uint8_t* mac_key_context,
                                              uint32_t mac_key_context_length,
                                              const uint8_t* enc_key_context,
                                              uint32_t enc_key_context_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_GenerateDerivedKeys(\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("mac_key_context", mac_key_context,
               (size_t)mac_key_context_length);
      dump_hex("enc_key_context", enc_key_context,
               (size_t)enc_key_context_length);
    }
  }
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_GenerateDerivedKeys(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_GenerateDerivedKeys(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  const std::vector<uint8_t>
      mac_ctx_str(mac_key_context, mac_key_context + mac_key_context_length);
  const std::vector<uint8_t>
      enc_ctx_str(enc_key_context, enc_key_context + enc_key_context_length);

  // Generate mac and encryption keys for current session context
  if (!session_ctx->DeriveKeys(crypto_engine->keybox().device_key(),
                               mac_ctx_str, enc_ctx_str)) {
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("mac_key_server", &session_ctx->mac_key_server()[0],
               session_ctx->mac_key_server().size());
      dump_hex("mac_key_client", &session_ctx->mac_key_client()[0],
               session_ctx->mac_key_client().size());
      dump_hex("enc_key", &session_ctx->encryption_key()[0],
               session_ctx->encryption_key().size());
    }
  }
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_GenerateSignature(
                            OEMCrypto_SESSION session,
                            const uint8_t* message,
                            size_t message_length,
                            uint8_t* signature,
                            size_t* signature_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_GenerateSignature(\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("message", message, message_length);
    }
  }

  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_GenerateSignature(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }

  if (*signature_length < SHA256_DIGEST_LENGTH) {
    *signature_length = SHA256_DIGEST_LENGTH;
    return OEMCrypto_ERROR_SHORT_BUFFER;
  }

  if (message == NULL || message_length == 0 ||
      signature == NULL || signature_length == 0) {
    LOGE("[OEMCrypto_GenerateSignature(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_GenerateSignature(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  if (session_ctx->GenerateSignature(message,
                                     message_length,
                                     signature,
                                     signature_length)) {
    if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
      if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
        dump_hex("signature", signature, *signature_length);
      }
    }
    return OEMCrypto_SUCCESS;
  }
  return OEMCrypto_ERROR_UNKNOWN_FAILURE;
}

bool RangeCheck(const uint8_t* message,
                uint32_t message_length,
                const uint8_t* field,
                uint32_t field_length,
                bool allow_null) {
  if (field == NULL) return allow_null;
  if (field < message) return false;
  if (field + field_length > message + message_length) return false;
  return true;
}

extern "C"
OEMCryptoResult OEMCrypto_LoadKeys(OEMCrypto_SESSION session,
                                   const uint8_t* message,
                                   size_t message_length,
                                   const uint8_t* signature,
                                   size_t signature_length,
                                   const uint8_t* enc_mac_key_iv,
                                   const uint8_t* enc_mac_keys,
                                   size_t num_keys,
                                   const OEMCrypto_KeyObject* key_array,
                                   const uint8_t* pst,
                                   size_t pst_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_LoadKeys(OEMCrypto_SESSION session,\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("message", message, message_length);
      dump_hex("signature", signature, signature_length);
      dump_hex("enc_mac_key_iv", enc_mac_key_iv, wvcdm::KEY_IV_SIZE);
      dump_hex("enc_mac_keys", enc_mac_keys, 2*wvcdm::MAC_KEY_SIZE);
      dump_hex("pst", pst, pst_length);
      for (size_t i = 0; i < num_keys; i++) {
        LOGV("key_array[%zu].key_id_length=%zu;\n", i,
               key_array[i].key_id_length);
        dump_array_part("key_array", i, "key_id",
                        key_array[i].key_id, key_array[i].key_id_length);
        dump_array_part("key_array", i, "key_data_iv",
                        key_array[i].key_data_iv, wvcdm::KEY_IV_SIZE);
        dump_array_part("key_array", i, "key_data",
                        key_array[i].key_data, key_array[i].key_data_length);
        dump_array_part("key_array", i, "key_control_iv",
                        key_array[i].key_control_iv, wvcdm::KEY_IV_SIZE);
        dump_array_part("key_array", i, "key_control",
                        key_array[i].key_control, wvcdm::KEY_IV_SIZE);
      }
    }
  }



  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_LoadKeys(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_LoadKeys(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  if (message == NULL || message_length == 0 ||
      signature == NULL || signature_length == 0 ||
      key_array == NULL || num_keys == 0) {
    LOGE("[OEMCrypto_LoadKeys(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  // Later on, we use pst_length to verify the the pst is valid.  This makes
  // sure that we aren't given a null string but told it has postiive length.
  if ((pst == NULL && pst_length > 0) || (pst != NULL && pst_length == 0)) {
    LOGE("[OEMCrypto_LoadKeys(): OEMCrypto_ERROR_INVALID_ONCTEXT - null pst.]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  // Range check
  if (!RangeCheck(message, message_length, enc_mac_keys,
                  2 * wvcdm::MAC_KEY_SIZE, true) ||
      !RangeCheck(message, message_length, enc_mac_key_iv, wvcdm::KEY_IV_SIZE,
                  true) ||
      !RangeCheck(message, message_length, pst, pst_length, true)) {
    LOGE("[OEMCrypto_LoadKeys(): OEMCrypto_ERROR_INVALID_CONTEXT - range check.]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  for (unsigned int i = 0; i < num_keys; i++) {
    if (!RangeCheck(message, message_length, key_array[i].key_id,
                    key_array[i].key_id_length, false) ||
        !RangeCheck(message, message_length, key_array[i].key_data,
                    key_array[i].key_data_length, false) ||
        !RangeCheck(message, message_length, key_array[i].key_data_iv,
                    wvcdm::KEY_IV_SIZE, false) ||
        !RangeCheck(message, message_length, key_array[i].key_control,
                    wvcdm::KEY_CONTROL_SIZE, false) ||
        !RangeCheck(message, message_length, key_array[i].key_control_iv,
                    wvcdm::KEY_IV_SIZE, false)) {
      LOGE("[OEMCrypto_LoadKeys(): OEMCrypto_ERROR_SIGNATURE_FAILURE -range check %d]", i);
      return OEMCrypto_ERROR_SIGNATURE_FAILURE;
    }
  }

  return session_ctx->LoadKeys(message, message_length, signature,
                               signature_length, enc_mac_key_iv, enc_mac_keys,
                               num_keys, key_array, pst, pst_length);
}

extern "C"
OEMCryptoResult OEMCrypto_RefreshKeys(
    OEMCrypto_SESSION session,
    const uint8_t* message,
    size_t message_length,
    const uint8_t* signature,
    size_t signature_length,
    size_t num_keys,
    const OEMCrypto_KeyRefreshObject* key_array) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_RefreshKeys(num_keys=%zu)\n",
           num_keys);
  }

  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_RefreshKeys(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_RefreshKeys(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  if (message == NULL || message_length == 0 ||
      signature == NULL || signature_length == 0 ||
      num_keys == 0) {
    LOGE("[OEMCrypto_RefreshKeys(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  // Range check
  for (unsigned int i = 0; i < num_keys; i++) {
    if (!RangeCheck(message, message_length, key_array[i].key_id,
                    key_array[i].key_id_length, true) ||
        !RangeCheck(message, message_length, key_array[i].key_control,
                    wvcdm::KEY_CONTROL_SIZE, false) ||
        !RangeCheck(message, message_length, key_array[i].key_control_iv,
                    wvcdm::KEY_IV_SIZE, true)) {
      LOGE("[OEMCrypto_RefreshKeys(): Range Check %d]", i);
      return OEMCrypto_ERROR_SIGNATURE_FAILURE;
    }
  }

  // Validate message signature
  if (!session_ctx->ValidateMessage(message, message_length,
                                    signature, signature_length)) {
    LOGE("[OEMCrypto_RefreshKeys(): signature was invalid]");
    return OEMCrypto_ERROR_SIGNATURE_FAILURE;
  }

  // Decrypt and refresh keys in key refresh object
  bool status = true;
  std::vector<uint8_t> key_id;
  std::vector<uint8_t> key_control;
  std::vector<uint8_t> key_control_iv;
  for (unsigned int i = 0; i < num_keys; i++) {
    if (key_array[i].key_id != NULL) {
      key_id.assign(key_array[i].key_id,
                    key_array[i].key_id + key_array[i].key_id_length);
      key_control.assign(key_array[i].key_control,
                         key_array[i].key_control + wvcdm::KEY_CONTROL_SIZE);
      if ( key_array[i].key_control_iv == NULL ) {
        key_control_iv.clear();
      } else {
        key_control_iv.assign(key_array[i].key_control_iv,
                              key_array[i].key_control_iv + wvcdm::KEY_IV_SIZE);
      }
    } else {
      // key_id could be null if special control key type
      // key_control is not encrypted in this case
      key_id.clear();
      key_control_iv.clear();
      key_control.assign(key_array[i].key_control,
                         key_array[i].key_control + wvcdm::KEY_CONTROL_SIZE);
    }

    if (!session_ctx->RefreshKey(key_id, key_control, key_control_iv)) {
      LOGE("[OEMCrypto_RefreshKeys():  error in key %i]", i);
      status = false;
      break;
    }
  }

  session_ctx->FlushNonces();
  if (!status) return OEMCrypto_ERROR_UNKNOWN_FAILURE;

  session_ctx->StartTimer();
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_SelectKey(const OEMCrypto_SESSION session,
                                    const uint8_t* key_id,
                                    size_t key_id_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_SelectKey"
         "(const OEMCrypto_SESSION session,\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("key_id", key_id, key_id_length);
    }
  }
#ifndef NDEBUG
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_SelectKey(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }
#endif

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_SelectKey(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  const std::vector<uint8_t> key_id_str =
      std::vector<uint8_t>(key_id, key_id + key_id_length);
  if (!session_ctx->SelectContentKey(key_id_str)) {
    LOGE("[OEMCrypto_SelectKey(): FAIL]");
    return OEMCrypto_ERROR_NO_CONTENT_KEY;
  }

  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_DecryptCTR(OEMCrypto_SESSION session,
                                     const uint8_t* data_addr,
                                     size_t data_length,
                                     bool is_encrypted,
                                     const uint8_t* iv,
                                     size_t block_offset,
                                     const OEMCrypto_DestBufferDesc* out_buffer,
                                     uint8_t subsample_flags) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_DecryptCTR"
         "(OEMCrypto_SESSION session,\n");
  }
  wvoec_mock::BufferType buffer_type = kBufferTypeDirect;
  uint8_t* destination = NULL;
  size_t max_length = 0;
  switch (out_buffer->type) {
    case OEMCrypto_BufferType_Clear:
      buffer_type = kBufferTypeClear;
      destination = out_buffer->buffer.clear.address;
      max_length =  out_buffer->buffer.clear.max_length;
      break;
    case OEMCrypto_BufferType_Secure:
      buffer_type = kBufferTypeSecure;
      destination =
          reinterpret_cast<uint8_t*>(out_buffer->buffer.secure.handle)
          + out_buffer->buffer.secure.offset;
      max_length =  out_buffer->buffer.secure.max_length;
      break;
    default:
    case OEMCrypto_BufferType_Direct:
      buffer_type = kBufferTypeDirect;
      destination = NULL;
      break;
  }

  if (buffer_type != kBufferTypeDirect && max_length < data_length) {
    LOGE("[OEMCrypto_DecryptCTR(): OEMCrypto_ERROR_SHORT_BUFFER]");
    return OEMCrypto_ERROR_SHORT_BUFFER;
  }

#ifndef NDEBUG
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_DecryptCTR(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }
#endif

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_DecryptCTR(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  if (data_addr == NULL || data_length == 0 ||
      iv == NULL || out_buffer == NULL) {
    LOGE("[OEMCrypto_DecryptCTR(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  return session_ctx->DecryptCTR(iv, block_offset, data_addr, data_length,
                                 is_encrypted, destination, buffer_type);
}

extern "C"
OEMCryptoResult OEMCrypto_InstallKeybox(const uint8_t* keybox,
                                        size_t keyBoxLength) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_InstallKeybox(const uint8_t *keybox,\n");
  }
  if (crypto_engine->keybox().InstallKeybox(keybox, keyBoxLength)) {
    return OEMCrypto_SUCCESS;
  }
  return OEMCrypto_ERROR_WRITE_KEYBOX;
}

extern "C"
OEMCryptoResult OEMCrypto_IsKeyboxValid(void) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_IsKeyboxValid(void) {\n");
  }
  switch(crypto_engine->ValidateKeybox()) {
    case NO_ERROR:    return OEMCrypto_SUCCESS;
    case BAD_CRC:     return OEMCrypto_ERROR_BAD_CRC;
    case BAD_MAGIC:   return OEMCrypto_ERROR_BAD_MAGIC;
    default:
    case OTHER_ERROR: return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
}

extern "C"
OEMCryptoResult OEMCrypto_GetDeviceID(uint8_t* deviceID,
                                      size_t* idLength) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_GetDeviceID(uint8_t* deviceID,\n");
  }
  std::vector<uint8_t> dev_id_string = crypto_engine->keybox().device_id();
  if (dev_id_string.empty()) {
    LOGE("[OEMCrypto_GetDeviceId(): Keybox Invalid]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }

  size_t dev_id_len = dev_id_string.size();
  if (*idLength < dev_id_len) {
    *idLength = dev_id_len;
    LOGE("[OEMCrypto_GetDeviceId(): ERROR_SHORT_BUFFER]");
    return OEMCrypto_ERROR_SHORT_BUFFER;
  }
  memset(deviceID, 0, *idLength);
  memcpy(deviceID, &dev_id_string[0], dev_id_len);
  *idLength = dev_id_len;
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGD("[OEMCrypto_GetDeviceId(): success]");
  }
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_GetKeyData(uint8_t* keyData,
                                     size_t* keyDataLength) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_GetKeyData(uint8_t* keyData,\n");
  }
  size_t length = crypto_engine->keybox().key_data_length();
  if (*keyDataLength < length) {
    *keyDataLength = length;
    LOGE("[OEMCrypto_GetKeyData(): ERROR_SHORT_BUFFER]");
    return OEMCrypto_ERROR_SHORT_BUFFER;
  }
  memset(keyData, 0, *keyDataLength);
  memcpy(keyData, crypto_engine->keybox().key_data(), length);
  *keyDataLength = length;
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGD("[OEMCrypto_GetKeyData(): success]");
  }
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_GetRandom(uint8_t* randomData, size_t dataLength) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_GetRandom"
         "(uint8_t* randomData, size_t dataLength) {\n");
  }
  if (!randomData) {
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (RAND_bytes(randomData, dataLength)) {
    return OEMCrypto_SUCCESS;
  }
  return OEMCrypto_ERROR_UNKNOWN_FAILURE;
}

extern "C"
OEMCryptoResult OEMCrypto_WrapKeybox(const uint8_t* keybox,
                                     size_t keyBoxLength,
                                     uint8_t* wrappedKeybox,
                                     size_t* wrappedKeyBoxLength,
                                     const uint8_t* transportKey,
                                     size_t transportKeyLength) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_WrapKeybox(const uint8_t *keybox,\n");
  }
  if (!keybox || !wrappedKeybox || !wrappedKeyBoxLength
      || (keyBoxLength != *wrappedKeyBoxLength)) {
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  // This implementation ignores the transport key.  For test keys, we
  // don't need to encrypt the keybox.
  memcpy(wrappedKeybox, keybox, keyBoxLength);
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_RewrapDeviceRSAKey(OEMCrypto_SESSION session,
                                             const uint8_t* message,
                                             size_t message_length,
                                             const uint8_t* signature,
                                             size_t signature_length,
                                             const uint32_t* nonce,
                                             const uint8_t* enc_rsa_key,
                                             size_t enc_rsa_key_length,
                                             const uint8_t* enc_rsa_key_iv,
                                             uint8_t* wrapped_rsa_key,
                                             size_t*  wrapped_rsa_key_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls | kLoggingTraceNonce)) {
    LOGI("-- OEMCryptoResult OEMCrypto_RewrapDeviceRSAKey()\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("message", message, message_length);
      dump_hex("signature", signature, signature_length);
    }
    LOGI("nonce = %08X;\n", *nonce);
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("enc_rsa_key", enc_rsa_key, enc_rsa_key_length);
      dump_hex("enc_rsa_key_iv", enc_rsa_key_iv, wvcdm::KEY_IV_SIZE);
    }
  }
  if (wrapped_rsa_key_length == NULL) {
    LOGE("[OEMCrypto_RewrapDeviceRSAKey(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  // For the reference implementation, the wrapped key and the encrypted
  // key are the same size -- just encrypted with different keys.
  // We add 32 bytes for a context, 32 for iv, and 32 bytes for a signature.
  // Important: This layout must match OEMCrypto_LoadDeviceRSAKey below.
  size_t buffer_size = enc_rsa_key_length + sizeof(WrappedRSAKey);

  if (wrapped_rsa_key == NULL || *wrapped_rsa_key_length < buffer_size) {
    if (LogCategoryEnabled(kLoggingDumpDerivedKeys)) {
      LOGW("[OEMCrypto_RewrapDeviceRSAKey(): Wrapped Keybox Short Buffer]");
    }
    *wrapped_rsa_key_length = buffer_size;
    return OEMCrypto_ERROR_SHORT_BUFFER;
  }
  *wrapped_rsa_key_length = buffer_size;  // Tell caller how much space we used.
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_RewrapDeviceRSAKey(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }
  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_RewrapDeviceRSAKey(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }
  if (message == NULL || message_length == 0 || signature == NULL
      || signature_length == 0 || nonce == NULL || enc_rsa_key == NULL) {
    LOGE("[OEMCrypto_RewrapDeviceRSAKey(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  // Range check
  if (!RangeCheck(message, message_length,
                  reinterpret_cast<const uint8_t*>(nonce),
                  sizeof(uint32_t), true) ||
      !RangeCheck(message, message_length, enc_rsa_key, enc_rsa_key_length,
                  true) ||
      !RangeCheck(message, message_length, enc_rsa_key_iv, wvcdm::KEY_IV_SIZE,
                  true)) {
    LOGE("[OEMCrypto_RewrapDeviceRSAKey():  - range check.]");
    return OEMCrypto_ERROR_SIGNATURE_FAILURE;
  }

  // Validate nonce
  if (!session_ctx->CheckNonce(*nonce)) {
    return OEMCrypto_ERROR_INVALID_NONCE;
  }
  session_ctx->FlushNonces();

  // Decrypt RSA key.
  uint8_t* pkcs8_rsa_key = new uint8_t[enc_rsa_key_length];
  OEMCryptoResult result = OEMCrypto_SUCCESS;
  if (!session_ctx->DecryptRSAKey(enc_rsa_key, enc_rsa_key_length,
                                  enc_rsa_key_iv, pkcs8_rsa_key)) {
    result = OEMCrypto_ERROR_INVALID_RSA_KEY;
  }
  size_t padding = pkcs8_rsa_key[enc_rsa_key_length - 1];
  if (result == OEMCrypto_SUCCESS) {
    if (padding > 16) {
      LOGE("[RewrapRSAKey(): Encrypted RSA has bad padding: %d]", padding);
      result = OEMCrypto_ERROR_INVALID_RSA_KEY;
    }
  }
  size_t rsa_key_length = enc_rsa_key_length - padding;
  // verify signature, verify RSA key, and load it.
  if (result == OEMCrypto_SUCCESS) {
    if (!session_ctx->LoadRSAKey(pkcs8_rsa_key, rsa_key_length,
                                 message, message_length,
                                 signature, signature_length)) {
      result = OEMCrypto_ERROR_SIGNATURE_FAILURE;
      // return OEMCrypto_ERROR_INVALID_RSA_KEY;
    }
  }

  // Now we generate a wrapped keybox.
  WrappedRSAKey* wrapped = reinterpret_cast<WrappedRSAKey*>(wrapped_rsa_key);
  // Pick a random context and IV for generating keys.
  if (result == OEMCrypto_SUCCESS) {
    if (!RAND_bytes(wrapped->context, sizeof(wrapped->context))) {
      result = OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
    if (!RAND_bytes(wrapped->iv, sizeof(wrapped->iv))) {
      result =  OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  const std::vector<uint8_t>
      context(wrapped->context, wrapped->context + sizeof(wrapped->context));
  // Generate mac and encryption keys for encrypting the signature.
  if (result == OEMCrypto_SUCCESS) {
    if (!session_ctx->DeriveKeys(crypto_engine->keybox().device_key(), context,
                                 context)) {
      result = OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }

  // Encrypt rsa key with keybox.
  if (result == OEMCrypto_SUCCESS) {
    if (!session_ctx->EncryptRSAKey(pkcs8_rsa_key, enc_rsa_key_length,
                                    wrapped->iv, wrapped->enc_rsa_key)) {
      result = OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  delete[] pkcs8_rsa_key;

  // The wrapped keybox must be signed with the same key we verify with. I'll
  // pick the server key, so I don't have to modify LoadRSAKey.
  if (result == OEMCrypto_SUCCESS) {
    unsigned int sig_length = sizeof(wrapped->signature);
    if (!HMAC(EVP_sha256(), &session_ctx->mac_key_server()[0],
              session_ctx->mac_key_server().size(), wrapped->context,
              buffer_size - sizeof(wrapped->signature), wrapped->signature,
              &sig_length)) {
      result = OEMCrypto_ERROR_UNKNOWN_FAILURE;
    }
  }
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("wrapped_rsa_key", wrapped_rsa_key, *wrapped_rsa_key_length);
      dump_hex("signature", wrapped->signature, sizeof(wrapped->signature));
      dump_hex("context", wrapped->context, sizeof(wrapped->context));
      dump_hex("iv", wrapped->iv, sizeof(wrapped->iv));
    }
  }
  return result;
}

extern "C"
OEMCryptoResult OEMCrypto_LoadDeviceRSAKey(OEMCrypto_SESSION session,
                                           const uint8_t* wrapped_rsa_key,
                                           size_t wrapped_rsa_key_length) {
  if (wrapped_rsa_key == NULL) {
    LOGE("[OEMCrypto_LoadDeviceRSAKey(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  const WrappedRSAKey* wrapped
    = reinterpret_cast<const WrappedRSAKey*>(wrapped_rsa_key);
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_LoadDeviceRSAKey()\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("wrapped_rsa_key", wrapped_rsa_key, wrapped_rsa_key_length);
      dump_hex("signature", wrapped->signature, sizeof(wrapped->signature));
      dump_hex("context", wrapped->context, sizeof(wrapped->context));
      dump_hex("iv", wrapped->iv, sizeof(wrapped->iv));
    }
  }
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_LoadDeviceRSAKey(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_LoadDeviceRSAKey(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }
  const std::vector<uint8_t>
      context(wrapped->context, wrapped->context + sizeof(wrapped->context));
  // Generate mac and encryption keys for encrypting the signature.
  if (!session_ctx->DeriveKeys(crypto_engine->keybox().device_key(), context,
                               context)) {
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  // Decrypt RSA key.
  uint8_t* pkcs8_rsa_key = new uint8_t[wrapped_rsa_key_length
                                       - sizeof(wrapped->signature)];
  size_t enc_rsa_key_length = wrapped_rsa_key_length - sizeof(WrappedRSAKey);
  OEMCryptoResult result = OEMCrypto_SUCCESS;
  if (!session_ctx->DecryptRSAKey(wrapped->enc_rsa_key, enc_rsa_key_length,
                                  wrapped->iv, pkcs8_rsa_key)) {
    result = OEMCrypto_ERROR_INVALID_RSA_KEY;
  }
  size_t padding = pkcs8_rsa_key[enc_rsa_key_length - 1];
  if (result == OEMCrypto_SUCCESS) {
    if (padding > 16) {
      LOGE("[LoadDeviceRSAKey(): Encrypted RSA has bad padding: %d]", padding);
      result = OEMCrypto_ERROR_INVALID_RSA_KEY;
    }
  }
  size_t rsa_key_length = enc_rsa_key_length - padding;
  // verify signature.
  if (result == OEMCrypto_SUCCESS) {
    if (!session_ctx->LoadRSAKey(
            pkcs8_rsa_key, rsa_key_length, wrapped->context,
            wrapped_rsa_key_length - sizeof(wrapped->signature),
            wrapped->signature, sizeof(wrapped->signature))) {
      result = OEMCrypto_ERROR_SIGNATURE_FAILURE;
      // return OEMCrypto_ERROR_INVALID_RSA_KEY;
    }
  }
  delete[] pkcs8_rsa_key;
  return result;
}

extern "C"
OEMCryptoResult OEMCrypto_GenerateRSASignature(
    OEMCrypto_SESSION session,
    const uint8_t* message,
    size_t message_length,
    uint8_t* signature,
    size_t* signature_length,
    RSA_Padding_Scheme padding_scheme) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_GenerateRSASignature()\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("message", message, message_length);
      dump_hex("message", message, message_length);
    }
  }
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_GenerateRSASignature(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }

  if (signature_length == 0) {
    LOGE("[OEMCrypto_GenerateRSASignature(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_GenerateRSASignature(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  size_t required_size = session_ctx->RSASignatureSize();
  if (*signature_length < required_size) {
    *signature_length = required_size;
    return OEMCrypto_ERROR_SHORT_BUFFER;
  }

  if (message == NULL || message_length == 0 ||
      signature == NULL || signature_length == 0) {
    LOGE("[OEMCrypto_GenerateRSASignature(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }

  if (session_ctx->GenerateRSASignature(message,
                                        message_length,
                                        signature,
                                        signature_length,
                                        padding_scheme)) {
    if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
      if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
        dump_hex("signature", signature, *signature_length);
      }
    }
    return OEMCrypto_SUCCESS;
  }
  return OEMCrypto_ERROR_UNKNOWN_FAILURE;;
}

extern "C"
OEMCryptoResult OEMCrypto_DeriveKeysFromSessionKey(
    OEMCrypto_SESSION session,
    const uint8_t* enc_session_key,
    size_t enc_session_key_length,
    const uint8_t* mac_key_context,
    size_t mac_key_context_length,
    const uint8_t* enc_key_context,
    size_t enc_key_context_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_DeriveKeysFromSessionKey(\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("enc_session_key", enc_session_key, enc_session_key_length);
      dump_hex("mac_key_context", mac_key_context,
               (size_t)mac_key_context_length);
      dump_hex("enc_key_context", enc_key_context,
               (size_t)enc_key_context_length);
    }
  }
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_GenerateDerivedKeys(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }

  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_GenerateDerivedKeys(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }

  if (session_ctx->allowed_schemes() != kSign_RSASSA_PSS) {
    LOGE("[OEMCrypto_GenerateDerivedKeys(): x509 key used to derive keys]");
    return OEMCrypto_ERROR_INVALID_RSA_KEY;
  }

  const std::vector<uint8_t>
      ssn_key_str(enc_session_key, enc_session_key + enc_session_key_length);
  const std::vector<uint8_t>
      mac_ctx_str(mac_key_context, mac_key_context + mac_key_context_length);
  const std::vector<uint8_t>
      enc_ctx_str(enc_key_context, enc_key_context + enc_key_context_length);

  // Generate mac and encryption keys for current session context
  if (!session_ctx->RSADeriveKeys(ssn_key_str, mac_ctx_str, enc_ctx_str)) {
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("mac_key_server", &session_ctx->mac_key_server()[0],
               session_ctx->mac_key_server().size());
      dump_hex("mac_key", &session_ctx->mac_key_client()[0],
               session_ctx->mac_key_client().size());
      dump_hex("enc_key", &session_ctx->encryption_key()[0],
               session_ctx->encryption_key().size());
    }
  }
  return OEMCrypto_SUCCESS;
}

extern "C"
uint32_t OEMCrypto_APIVersion() {
  return 9;
}

extern "C"
const char* OEMCrypto_SecurityLevel() {
  return "L3";
}

extern "C"
OEMCryptoResult OEMCrypto_GetHDCPCapability(
    OEMCrypto_HDCP_Capability *current,
    OEMCrypto_HDCP_Capability *maximum) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_GetHDCPCapability(%p, %p)\n",
           current, maximum);
  }
  if (current == NULL) return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  if (maximum == NULL) return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  *current = crypto_engine->current_hdcp_capability();
  *maximum = crypto_engine->maximum_hdcp_capability();
  return OEMCrypto_SUCCESS;
}

extern "C"
OEMCryptoResult OEMCrypto_Generic_Encrypt(OEMCrypto_SESSION session,
                                          const uint8_t* in_buffer,
                                          size_t buffer_length,
                                          const uint8_t* iv,
                                          OEMCrypto_Algorithm algorithm,
                                          uint8_t* out_buffer) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_Generic_Encrypt( algorithm=%d\n",
           algorithm);
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("in_buffer", in_buffer, buffer_length);
      dump_hex("iv", iv, wvcdm::KEY_IV_SIZE);
    }
  }
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_Generic_Enrypt(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }
  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_Generic_Enrypt(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }
  if (in_buffer == NULL || buffer_length == 0 ||
      iv == NULL || out_buffer == NULL) {
    LOGE("[OEMCrypto_Generic_Enrypt(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  OEMCryptoResult sts =
      session_ctx->Generic_Encrypt(in_buffer, buffer_length, iv, algorithm,
                                   out_buffer);
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("out_buffer", out_buffer, buffer_length);
    }
  }
  return sts;
}

extern "C"
OEMCryptoResult OEMCrypto_Generic_Decrypt(OEMCrypto_SESSION session,
                                          const uint8_t* in_buffer,
                                          size_t buffer_length,
                                          const uint8_t* iv,
                                          OEMCrypto_Algorithm algorithm,
                                          uint8_t* out_buffer) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_Generic_Decrypt( algorithm=%d\n",
           algorithm);
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("in_buffer", in_buffer, buffer_length);
      dump_hex("iv", iv, wvcdm::KEY_IV_SIZE);
    }
  }
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_Generic_Decrypt(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }
  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_Generic_Decrypt(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }
  if (in_buffer == NULL || buffer_length == 0 ||
      iv == NULL || out_buffer == NULL) {
    LOGE("[OEMCrypto_Generic_Decrypt(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  OEMCryptoResult sts =
      session_ctx->Generic_Decrypt(in_buffer, buffer_length, iv, algorithm,
                                   out_buffer);
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE){
      dump_hex("out_buffer", out_buffer, buffer_length);
    }
  }
  return sts;
}

extern "C"
OEMCryptoResult OEMCrypto_Generic_Sign(OEMCrypto_SESSION session,
                                       const uint8_t* in_buffer,
                                       size_t buffer_length,
                                       OEMCrypto_Algorithm algorithm,
                                       uint8_t* signature,
                                       size_t* signature_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_Generic_Sign( algorithm=%d\n",
           algorithm);
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("in_buffer", in_buffer, buffer_length);
    }
  }
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_Generic_Sign(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }
  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_Generic_Sign(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }
  if (*signature_length < SHA256_DIGEST_LENGTH) {
    *signature_length = SHA256_DIGEST_LENGTH;
    return OEMCrypto_ERROR_SHORT_BUFFER;
  }
  if (in_buffer == NULL || buffer_length == 0 || signature == NULL) {
    LOGE("[OEMCrypto_Generic_Sign(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  OEMCryptoResult sts =
      session_ctx->Generic_Sign(in_buffer, buffer_length, algorithm,
                                signature, signature_length);
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("signature", signature, *signature_length);
    }
  }
  return sts;
}

extern "C"
OEMCryptoResult OEMCrypto_Generic_Verify(OEMCrypto_SESSION session,
                                         const uint8_t* in_buffer,
                                         size_t buffer_length,
                                         OEMCrypto_Algorithm algorithm,
                                         const uint8_t* signature,
                                         size_t signature_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_Generic_Verify( algorithm=%d\n",
           algorithm);
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("in_buffer", in_buffer, buffer_length);
      dump_hex("signature", signature, signature_length);
    }
  }
  if (NO_ERROR != crypto_engine->ValidateKeybox()) {
    LOGE("[OEMCrypto_Generic_Verify(): ERROR_KEYBOX_INVALID]");
    return OEMCrypto_ERROR_KEYBOX_INVALID;
  }
  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_Generic_Verify(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }
  if (signature_length != SHA256_DIGEST_LENGTH) {
    return OEMCrypto_ERROR_UNKNOWN_FAILURE;
  }
  if (in_buffer == NULL || buffer_length == 0 || signature == NULL) {
    LOGE("[OEMCrypto_Generic_Verify(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  return session_ctx->Generic_Verify(in_buffer, buffer_length, algorithm,
                                     signature, signature_length);
}

extern "C"
bool OEMCrypto_SupportsUsageTable() {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- bool OEMCrypto_SupportsUsageTable(); // returns true.\n");
  }
  return true;
}
extern "C"
OEMCryptoResult OEMCrypto_UpdateUsageTable() {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_UpdateUsageTable();\n");
  }
  return crypto_engine->usage_table()->UpdateTable();
}

extern "C"
OEMCryptoResult OEMCrypto_DeactivateUsageEntry(const uint8_t *pst,
                                               size_t pst_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_DeactivateUsageEntry(\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("pst", pst, pst_length);
    }
  }
  std::vector<uint8_t> pstv(pst, pst + pst_length);
  return crypto_engine->usage_table()->DeactivateEntry(pstv);
}

extern "C"
OEMCryptoResult OEMCrypto_ReportUsage(OEMCrypto_SESSION session,
                                      const uint8_t *pst,
                                      size_t pst_length,
                                      OEMCrypto_PST_Report *buffer,
                                      size_t *buffer_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_ReportUsage(\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("pst", pst, pst_length);
    }
  }
  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_ReportUsage(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }
  std::vector<uint8_t> pstv(pst, pst + pst_length);
  UsageTableEntry* entry = crypto_engine->usage_table()->FindEntry(pstv);
  if (!entry) {
    LOGE("[OEMCrypto_ReportUsage(): Usage Table Entry not found]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  OEMCryptoResult sts =
      entry->ReportUsage(session_ctx, pstv, buffer, buffer_length);
  crypto_engine->usage_table()->UpdateTable();
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("usage buffer", reinterpret_cast<uint8_t*>(buffer),
               *buffer_length);
    }
  }
  return sts;
}

extern "C"
OEMCryptoResult OEMCrypto_DeleteUsageEntry(OEMCrypto_SESSION session,
                                           const uint8_t* pst,
                                           size_t pst_length,
                                           const uint8_t *message,
                                           size_t message_length,
                                           const uint8_t *signature,
                                           size_t signature_length) {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_DeleteUsageEntry(\n");
    if (wvcdm::g_cutoff >= wvcdm::LOG_VERBOSE) {
      dump_hex("pst", pst, pst_length);
      dump_hex("message", message, message_length);
      dump_hex("signature", signature, signature_length);
    }
  }
  SessionContext* session_ctx = crypto_engine->FindSession(session);
  if (!session_ctx || !session_ctx->isValid()) {
    LOGE("[OEMCrypto_DeleteUsageEntry(): ERROR_INVALID_SESSION]");
    return OEMCrypto_ERROR_INVALID_SESSION;
  }
  if (message == NULL || message_length == 0 || signature == NULL ||
      signature_length == 0 || pst == NULL || pst_length == 0) {
    LOGE("[OEMCrypto_DeleteUsageEntry(): OEMCrypto_ERROR_INVALID_CONTEXT]");
    return OEMCrypto_ERROR_INVALID_CONTEXT;
  }
  if (!RangeCheck(message, message_length, pst, pst_length, false)) {
    LOGE("[OEMCrypto_DeleteUsageEntry(): range check.]");
    return OEMCrypto_ERROR_SIGNATURE_FAILURE;
  }
  // Validate message signature
  if (!session_ctx->ValidateMessage(message, message_length, signature,
                                    signature_length)) {
    LOGE("[OEMCrypto_DeleteUsageEntry(): OEMCrypto_ERROR_SIGNATURE_FAILURE.]");
    return OEMCrypto_ERROR_SIGNATURE_FAILURE;
  }
  std::vector<uint8_t> pstv(pst, pst + pst_length);
  if (crypto_engine->usage_table()->DeleteEntry(pstv)) {
    return OEMCrypto_SUCCESS;
  }
  return OEMCrypto_ERROR_UNKNOWN_FAILURE;
}

extern "C"
OEMCryptoResult OEMCrypto_DeleteUsageTable() {
  if (LogCategoryEnabled(kLoggingTraceOEMCryptoCalls)) {
    LOGI("-- OEMCryptoResult OEMCrypto_DeleteUsageTable()\n");
  }
  crypto_engine->usage_table()->Clear();
  return OEMCrypto_SUCCESS;
}

};  // namespace wvoec_mock
