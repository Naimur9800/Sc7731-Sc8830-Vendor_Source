//
// Copyright 2013 Google Inc. All Rights Reserved.
//

#ifndef WV_GENERIC_CRYPTO_INTERFACE_H_
#define WV_GENERIC_CRYPTO_INTERFACE_H_

#include <stdint.h>

#include "OEMCryptoCENC.h"
#include "media/stagefright/foundation/ABase.h"
#include "utils/Vector.h"

namespace wvdrm {

class WVGenericCryptoInterface {
 public:
  WVGenericCryptoInterface() {}
  virtual ~WVGenericCryptoInterface() {}

  virtual OEMCryptoResult selectKey(const OEMCrypto_SESSION session,
                                    const uint8_t* key_id,
                                    size_t key_id_length) {
    return OEMCrypto_SelectKey(session, key_id, key_id_length);
  }

  virtual OEMCryptoResult encrypt(OEMCrypto_SESSION session,
                                  const uint8_t* in_buffer,
                                  size_t buffer_length, const uint8_t* iv,
                                  OEMCrypto_Algorithm algorithm,
                                  uint8_t* out_buffer) {
    return OEMCrypto_Generic_Encrypt(session, in_buffer, buffer_length, iv,
                                     algorithm, out_buffer);
  }

  virtual OEMCryptoResult decrypt(OEMCrypto_SESSION session,
                                  const uint8_t* in_buffer,
                                  size_t buffer_length, const uint8_t* iv,
                                  OEMCrypto_Algorithm algorithm,
                                  uint8_t* out_buffer) {
    return OEMCrypto_Generic_Decrypt(session, in_buffer, buffer_length, iv,
                                     algorithm, out_buffer);
  }

  virtual OEMCryptoResult sign(OEMCrypto_SESSION session,
                               const uint8_t* in_buffer, size_t buffer_length,
                               OEMCrypto_Algorithm algorithm,
                               uint8_t* signature, size_t* signature_length) {
    return OEMCrypto_Generic_Sign(session, in_buffer, buffer_length, algorithm,
                                  signature, signature_length);
  }

  virtual OEMCryptoResult verify(OEMCrypto_SESSION session,
                                 const uint8_t* in_buffer, size_t buffer_length,
                                 OEMCrypto_Algorithm algorithm,
                                 const uint8_t* signature,
                                 size_t signature_length) {
    return OEMCrypto_Generic_Verify(session, in_buffer, buffer_length,
                                    algorithm, signature, signature_length);
  }

  virtual OEMCryptoResult signRSA(const uint8_t* wrapped_rsa_key,
                                  size_t wrapped_rsa_key_length,
                                  const uint8_t* message,
                                  size_t message_length,
                                  android::Vector<uint8_t>& signature,
                                  RSA_Padding_Scheme padding_scheme);


  virtual OEMCryptoResult loadDeviceRSAKey(OEMCrypto_SESSION session,
                                           const uint8_t* wrapped_rsa_key,
                                           size_t wrapped_rsa_key_length) {
    return OEMCrypto_LoadDeviceRSAKey(session, wrapped_rsa_key,
                                      wrapped_rsa_key_length);
  }

  virtual OEMCryptoResult generateRSASignature(
      OEMCrypto_SESSION session,
      const uint8_t* message,
      size_t message_length,
      uint8_t* signature,
      size_t* signature_length,
      RSA_Padding_Scheme padding_scheme) {
  return OEMCrypto_GenerateRSASignature(session, message, message_length,
                                        signature, signature_length,
                                        padding_scheme);
}

 private:
  DISALLOW_EVIL_CONSTRUCTORS(WVGenericCryptoInterface);
};

} // namespace wvdrm

#endif // WV_GENERIC_CRYPTO_INTERFACE_H_
