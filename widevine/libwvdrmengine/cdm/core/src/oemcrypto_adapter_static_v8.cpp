/*******************************************************************************
 *
 * Copyright 2013 Google Inc. All Rights Reserved.
 *
 * Wrapper of OEMCrypto APIs for platforms that support Level 1 only.
 * This should be used when liboemcrypto.so is linked with the CDM code at
 * compile time.
 * An implementation should compile either oemcrypto_adapter_dynamic.cpp or
 * oemcrypto_adapter_static.cpp, but not both.
 * This version contains shim code to allow an older, version 8 API, oemcrypto,
 * to be linked with CDM.
 *
 ******************************************************************************/

#include "OEMCryptoCENC.h"
#include "oemcrypto_adapter.h"

extern "C"
OEMCryptoResult OEMCrypto_LoadKeys_V8(OEMCrypto_SESSION session,
                                   const uint8_t* message,
                                   size_t message_length,
                                   const uint8_t* signature,
                                   size_t signature_length,
                                   const uint8_t* enc_mac_keys_iv,
                                   const uint8_t* enc_mac_keys,
                                   size_t num_keys,
                                   const OEMCrypto_KeyObject* key_array);

extern "C"
OEMCryptoResult OEMCrypto_GenerateRSASignature_V8(OEMCrypto_SESSION session,
                                               const uint8_t* message,
                                               size_t message_length,
                                               uint8_t* signature,
                                               size_t *signature_length);



namespace wvcdm {

OEMCryptoResult OEMCrypto_OpenSession(OEMCrypto_SESSION* session,
                                      SecurityLevel level) {
  return ::OEMCrypto_OpenSession(session);
}

OEMCryptoResult OEMCrypto_IsKeyboxValid(SecurityLevel level) {
  return ::OEMCrypto_IsKeyboxValid();
}

OEMCryptoResult OEMCrypto_GetDeviceID(uint8_t* deviceID, size_t* idLength,
                                      SecurityLevel level) {
  return ::OEMCrypto_GetDeviceID(deviceID, idLength);
}

OEMCryptoResult OEMCrypto_GetKeyData(uint8_t* keyData, size_t* keyDataLength,
                                     SecurityLevel level) {
  return ::OEMCrypto_GetKeyData(keyData, keyDataLength);
}

OEMCryptoResult OEMCrypto_InstallKeybox(const uint8_t* keybox,
                                        size_t keyBoxLength,
                                        SecurityLevel level) {
  return ::OEMCrypto_InstallKeybox(keybox, keyBoxLength);
}

uint32_t OEMCrypto_APIVersion(SecurityLevel level) {
  return ::OEMCrypto_APIVersion();
}

const char* OEMCrypto_SecurityLevel(SecurityLevel level) {
  return ::OEMCrypto_SecurityLevel();
}

extern "C" OEMCryptoResult OEMCrypto_LoadKeys(
    OEMCrypto_SESSION session, const uint8_t* message, size_t message_length,
    const uint8_t* signature, size_t signature_length,
    const uint8_t* enc_mac_key_iv, const uint8_t* enc_mac_key, size_t num_keys,
    const OEMCrypto_KeyObject* key_array,
    const uint8_t* pst, size_t pst_length) {
  return OEMCrypto_LoadKeys_V8(session, message, message_length, signature,
                               signature_length, enc_mac_key_iv, enc_mac_key,
                               num_keys, key_array);
}

extern "C" OEMCryptoResult OEMCrypto_GenerateRSASignature(
    OEMCrypto_SESSION session, const uint8_t* message, size_t message_length,
    uint8_t* signature, size_t* signature_length, RSA_Padding_Scheme padding_scheme) {
  return OEMCrypto_GenerateRSASignature_V8(session, message, message_length,
                                 signature, signature_length);
}

extern "C"
OEMCryptoResult OEMCrypto_GetHDCPCapability(OEMCrypto_HDCP_Capability *current,
                                            OEMCrypto_HDCP_Capability *maximum) {
  return OEMCrypto_ERROR_NOT_IMPLEMENTED;
}

extern "C" bool OEMCrypto_SupportsUsageTable() {
  return false;
}

extern "C" OEMCryptoResult OEMCrypto_UpdateUsageTable() {
  return OEMCrypto_ERROR_NOT_IMPLEMENTED;
}

extern "C" OEMCryptoResult OEMCrypto_DeactivateUsageEntry(const uint8_t *pst,
                                                          size_t pst_length) {
  return OEMCrypto_ERROR_NOT_IMPLEMENTED;
}

extern "C" OEMCryptoResult OEMCrypto_ReportUsage(OEMCrypto_SESSION session,
                                                 const uint8_t *pst,
                                                 size_t pst_length,
                                                 OEMCrypto_PST_Report *buffer,
                                                 size_t *buffer_length) {
  return OEMCrypto_ERROR_NOT_IMPLEMENTED;
}

extern "C" OEMCryptoResult OEMCrypto_DeleteUsageEntry(OEMCrypto_SESSION session,
                                                      const uint8_t* pst,
                                                      size_t pst_length,
                                                      const uint8_t *message,
                                                      size_t message_length,
                                                      const uint8_t *signature,
                                                      size_t signature_length) {
  return OEMCrypto_ERROR_NOT_IMPLEMENTED;
}
};  // namespace wvcdm
