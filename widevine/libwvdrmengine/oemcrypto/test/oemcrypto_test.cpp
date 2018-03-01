// Copyright 2013 Google Inc. All Rights Reserved.
//
// OEMCrypto unit tests
//
#include <arpa/inet.h>  // needed for ntoh()
#include <ctype.h>
#include <getopt.h>
#include <gtest/gtest.h>
#include <openssl/aes.h>
#include <openssl/cmac.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <utility>
#include <string>
#include <vector>

#include "OEMCryptoCENC.h"
#include "oemcrypto_key_mock.h"
#include "properties.h"
#include "string_conversions.h"
#include "wv_cdm_constants.h"
#include "wv_keybox.h"

using namespace std;
using ::testing::WithParamInterface;
using ::testing::Range;
using ::testing::Values;

namespace {
const size_t kNumKeys = 4;
#if defined(TEST_SPEED_MULTIPLIER)  // Can slow test time limits when
                                    // debugging is slowing everything.
const int kSpeedMultiplier = TEST_SPEED_MULTIPLIER;
#else
const int kSpeedMultiplier = 1;
#endif
const int kShortSleep = 1 * kSpeedMultiplier;
const int kLongSleep = 2 * kSpeedMultiplier;
const uint32_t kDuration = 2 * kSpeedMultiplier;
const uint32_t kLongDuration = 5 * kSpeedMultiplier;
}

namespace wvoec {

typedef struct {
  uint8_t verification[4];
  uint32_t duration;
  uint32_t nonce;
  uint32_t control_bits;
} KeyControlBlock;

const size_t kTestKeyIdLength = 12;  // pick a length. any length.
typedef struct {
  uint8_t key_id[kTestKeyIdLength];
  uint8_t key_data[wvcdm::MAC_KEY_SIZE];
  size_t key_data_length;
  uint8_t key_iv[wvcdm::KEY_IV_SIZE];
  uint8_t control_iv[wvcdm::KEY_IV_SIZE];
  KeyControlBlock control;
} MessageKeyData;

struct MessageData {
  MessageKeyData keys[kNumKeys];
  uint8_t mac_key_iv[wvcdm::KEY_IV_SIZE];
  uint8_t mac_keys[2 * wvcdm::MAC_KEY_SIZE];
  uint8_t pst[kTestKeyIdLength];
};

const size_t kMaxTestRSAKeyLength = 2000;  // Rough estimate.
struct RSAPrivateKeyMessage {
  uint8_t rsa_key[kMaxTestRSAKeyLength];
  uint8_t rsa_key_iv[wvcdm::KEY_IV_SIZE];
  size_t rsa_key_length;
  uint32_t nonce;
};

struct PaddedPSTReport {
  OEMCrypto_PST_Report report;
  uint8_t padding[256];
};

// These are test keyboxes.  They will not be accepted by production systems.
// By using known keyboxes for these tests, the results for a given set of
// inputs to a test are predictable and can be compared to the actual results.
const wvoec_mock::WidevineKeybox kDefaultKeybox = {
  // Sample keybox used for test vectors
  {
    // deviceID
    0x54, 0x65, 0x73, 0x74, 0x4b, 0x65, 0x79, 0x30,  // TestKey01
    0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
  }, {
    // key
    0xfb, 0xda, 0x04, 0x89, 0xa1, 0x58, 0x16, 0x0e,
    0xa4, 0x02, 0xe9, 0x29, 0xe3, 0xb6, 0x8f, 0x04,
  }, {
    // data
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x10, 0x19,
    0x07, 0xd9, 0xff, 0xde, 0x13, 0xaa, 0x95, 0xc1,
    0x22, 0x67, 0x80, 0x53, 0x36, 0x21, 0x36, 0xbd,
    0xf8, 0x40, 0x8f, 0x82, 0x76, 0xe4, 0xc2, 0xd8,
    0x7e, 0xc5, 0x2b, 0x61, 0xaa, 0x1b, 0x9f, 0x64,
    0x6e, 0x58, 0x73, 0x49, 0x30, 0xac, 0xeb, 0xe8,
    0x99, 0xb3, 0xe4, 0x64, 0x18, 0x9a, 0x14, 0xa8,
    0x72, 0x02, 0xfb, 0x02, 0x57, 0x4e, 0x70, 0x64,
    0x0b, 0xd2, 0x2e, 0xf4, 0x4b, 0x2d, 0x7e, 0x39,
  }, {
    // magic
    0x6b, 0x62, 0x6f, 0x78,
  }, {
    // Crc
    0x0a, 0x7a, 0x2c, 0x35,
  }
};

static wvoec_mock::WidevineKeybox kValidKeybox02 = {
  // Sample keybox used for test vectors
  {
    // deviceID
    0x54, 0x65, 0x73, 0x74, 0x4b, 0x65, 0x79, 0x30, // TestKey02
    0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
  }, {
    // key
    0x76, 0x5d, 0xce, 0x01, 0x04, 0x89, 0xb3, 0xd0,
    0xdf, 0xce, 0x54, 0x8a, 0x49, 0xda, 0xdc, 0xb6,
  }, {
    // data
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x10, 0x19,
    0x92, 0x27, 0x0b, 0x1f, 0x1a, 0xd5, 0xc6, 0x93,
    0x19, 0x3f, 0xaa, 0x74, 0x1f, 0xdd, 0x5f, 0xb4,
    0xe9, 0x40, 0x2f, 0x34, 0xa4, 0x92, 0xf4, 0xae,
    0x9a, 0x52, 0x39, 0xbc, 0xb7, 0x24, 0x38, 0x13,
    0xab, 0xf4, 0x92, 0x96, 0xc4, 0x81, 0x60, 0x33,
    0xd8, 0xb8, 0x09, 0xc7, 0x55, 0x0e, 0x12, 0xfa,
    0xa8, 0x98, 0x62, 0x8a, 0xec, 0xea, 0x74, 0x8a,
    0x4b, 0xfa, 0x5a, 0x9e, 0xb6, 0x49, 0x0d, 0x80,
  }, {
    // magic
    0x6b, 0x62, 0x6f, 0x78,
  }, {
    // Crc
    0x2a, 0x3b, 0x3e, 0xe4,
  }
};

static wvoec_mock::WidevineKeybox kValidKeybox03 = {
  // Sample keybox used for test vectors
  {
    // deviceID
    0x54, 0x65, 0x73, 0x74, 0x4b, 0x65, 0x79, 0x30, // TestKey03
    0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
  }, {
    // key
    0x25, 0xe5, 0x2a, 0x02, 0x29, 0x68, 0x04, 0xa2,
    0x92, 0xfd, 0x7c, 0x67, 0x0b, 0x67, 0x1f, 0x31,
  }, {
    // data
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x10, 0x19,
    0xf4, 0x0a, 0x0e, 0xa2, 0x0a, 0x71, 0xd5, 0x92,
    0xfa, 0xa3, 0x25, 0xc6, 0x4b, 0x76, 0xf1, 0x64,
    0xf4, 0x60, 0xa0, 0x30, 0x72, 0x23, 0xbe, 0x03,
    0xcd, 0xde, 0x7a, 0x06, 0xd4, 0x01, 0xeb, 0xdc,
    0xe0, 0x50, 0xc0, 0x53, 0x0a, 0x50, 0xb0, 0x37,
    0xe5, 0x05, 0x25, 0x0e, 0xa4, 0xc8, 0x5a, 0xff,
    0x46, 0x6e, 0xa5, 0x31, 0xf3, 0xdd, 0x94, 0xb7,
    0xe0, 0xd3, 0xf9, 0x04, 0xb2, 0x54, 0xb1, 0x64,
  }, {
    // magic
    0x6b, 0x62, 0x6f, 0x78,
  }, {
    // Crc
    0xa1, 0x99, 0x5f, 0x46,
  }
};

// A 2048-bit test RSA Private Key
// This is used to verify the functions that manipulate RSA keys.
static const uint8_t kTestPKCS1RSAPrivateKey2_2048[] = {
  0x30, 0x82, 0x04, 0xa2, 0x02, 0x01, 0x00, 0x02,
  0x82, 0x01, 0x01, 0x00, 0xa7, 0x00, 0x36, 0x60,
  0x65, 0xdc, 0xbd, 0x54, 0x5a, 0x2a, 0x40, 0xb4,
  0xe1, 0x15, 0x94, 0x58, 0x11, 0x4f, 0x94, 0x58,
  0xdd, 0xde, 0xa7, 0x1f, 0x3c, 0x2c, 0xe0, 0x88,
  0x09, 0x29, 0x61, 0x57, 0x67, 0x5e, 0x56, 0x7e,
  0xee, 0x27, 0x8f, 0x59, 0x34, 0x9a, 0x2a, 0xaa,
  0x9d, 0xb4, 0x4e, 0xfa, 0xa7, 0x6a, 0xd4, 0xc9,
  0x7a, 0x53, 0xc1, 0x4e, 0x9f, 0xe3, 0x34, 0xf7,
  0x3d, 0xb7, 0xc9, 0x10, 0x47, 0x4f, 0x28, 0xda,
  0x3f, 0xce, 0x31, 0x7b, 0xfd, 0x06, 0x10, 0xeb,
  0xf7, 0xbe, 0x92, 0xf9, 0xaf, 0xfb, 0x3e, 0x68,
  0xda, 0xee, 0x1a, 0x64, 0x4c, 0xf3, 0x29, 0xf2,
  0x73, 0x9e, 0x39, 0xd8, 0xf6, 0x6f, 0xd8, 0xb2,
  0x80, 0x82, 0x71, 0x8e, 0xb5, 0xa4, 0xf2, 0xc2,
  0x3e, 0xcd, 0x0a, 0xca, 0xb6, 0x04, 0xcd, 0x9a,
  0x13, 0x8b, 0x54, 0x73, 0x54, 0x25, 0x54, 0x8c,
  0xbe, 0x98, 0x7a, 0x67, 0xad, 0xda, 0xb3, 0x4e,
  0xb3, 0xfa, 0x82, 0xa8, 0x4a, 0x67, 0x98, 0x56,
  0x57, 0x54, 0x71, 0xcd, 0x12, 0x7f, 0xed, 0xa3,
  0x01, 0xc0, 0x6a, 0x8b, 0x24, 0x03, 0x96, 0x88,
  0xbe, 0x97, 0x66, 0x2a, 0xbc, 0x53, 0xc9, 0x83,
  0x06, 0x51, 0x5a, 0x88, 0x65, 0x13, 0x18, 0xe4,
  0x3a, 0xed, 0x6b, 0xf1, 0x61, 0x5b, 0x4c, 0xc8,
  0x1e, 0xf4, 0xc2, 0xae, 0x08, 0x5e, 0x2d, 0x5f,
  0xf8, 0x12, 0x7f, 0xa2, 0xfc, 0xbb, 0x21, 0x18,
  0x30, 0xda, 0xfe, 0x40, 0xfb, 0x01, 0xca, 0x2e,
  0x37, 0x0e, 0xce, 0xdd, 0x76, 0x87, 0x82, 0x46,
  0x0b, 0x3a, 0x77, 0x8f, 0xc0, 0x72, 0x07, 0x2c,
  0x7f, 0x9d, 0x1e, 0x86, 0x5b, 0xed, 0x27, 0x29,
  0xdf, 0x03, 0x97, 0x62, 0xef, 0x44, 0xd3, 0x5b,
  0x3d, 0xdb, 0x9c, 0x5e, 0x1b, 0x7b, 0x39, 0xb4,
  0x0b, 0x6d, 0x04, 0x6b, 0xbb, 0xbb, 0x2c, 0x5f,
  0xcf, 0xb3, 0x7a, 0x05, 0x02, 0x03, 0x01, 0x00,
  0x01, 0x02, 0x82, 0x01, 0x00, 0x5e, 0x79, 0x65,
  0x49, 0xa5, 0x76, 0x79, 0xf9, 0x05, 0x45, 0x0f,
  0xf4, 0x03, 0xbd, 0xa4, 0x7d, 0x29, 0xd5, 0xde,
  0x33, 0x63, 0xd8, 0xb8, 0xac, 0x97, 0xeb, 0x3f,
  0x5e, 0x55, 0xe8, 0x7d, 0xf3, 0xe7, 0x3b, 0x5c,
  0x2d, 0x54, 0x67, 0x36, 0xd6, 0x1d, 0x46, 0xf5,
  0xca, 0x2d, 0x8b, 0x3a, 0x7e, 0xdc, 0x45, 0x38,
  0x79, 0x7e, 0x65, 0x71, 0x5f, 0x1c, 0x5e, 0x79,
  0xb1, 0x40, 0xcd, 0xfe, 0xc5, 0xe1, 0xc1, 0x6b,
  0x78, 0x04, 0x4e, 0x8e, 0x79, 0xf9, 0x0a, 0xfc,
  0x79, 0xb1, 0x5e, 0xb3, 0x60, 0xe3, 0x68, 0x7b,
  0xc6, 0xef, 0xcb, 0x71, 0x4c, 0xba, 0xa7, 0x79,
  0x5c, 0x7a, 0x81, 0xd1, 0x71, 0xe7, 0x00, 0x21,
  0x13, 0xe2, 0x55, 0x69, 0x0e, 0x75, 0xbe, 0x09,
  0xc3, 0x4f, 0xa9, 0xc9, 0x68, 0x22, 0x0e, 0x97,
  0x8d, 0x89, 0x6e, 0xf1, 0xe8, 0x88, 0x7a, 0xd1,
  0xd9, 0x09, 0x5d, 0xd3, 0x28, 0x78, 0x25, 0x0b,
  0x1c, 0x47, 0x73, 0x25, 0xcc, 0x21, 0xb6, 0xda,
  0xc6, 0x24, 0x5a, 0xd0, 0x37, 0x14, 0x46, 0xc7,
  0x94, 0x69, 0xe4, 0x43, 0x6f, 0x47, 0xde, 0x00,
  0x33, 0x4d, 0x8f, 0x95, 0x72, 0xfa, 0x68, 0x71,
  0x17, 0x66, 0x12, 0x1a, 0x87, 0x27, 0xf7, 0xef,
  0x7e, 0xe0, 0x35, 0x58, 0xf2, 0x4d, 0x6f, 0x35,
  0x01, 0xaa, 0x96, 0xe2, 0x3d, 0x51, 0x13, 0x86,
  0x9c, 0x79, 0xd0, 0xb7, 0xb6, 0x64, 0xe8, 0x86,
  0x65, 0x50, 0xbf, 0xcc, 0x27, 0x53, 0x1f, 0x51,
  0xd4, 0xca, 0xbe, 0xf5, 0xdd, 0x77, 0x70, 0x98,
  0x0f, 0xee, 0xa8, 0x96, 0x07, 0x5f, 0x45, 0x6a,
  0x7a, 0x0d, 0x03, 0x9c, 0x4f, 0x29, 0xf6, 0x06,
  0xf3, 0x5d, 0x58, 0x6c, 0x47, 0xd0, 0x96, 0xa9,
  0x03, 0x17, 0xbb, 0x4e, 0xc9, 0x21, 0xe0, 0xac,
  0xcd, 0x78, 0x78, 0xb2, 0xfe, 0x81, 0xb2, 0x51,
  0x53, 0xa6, 0x1f, 0x98, 0x45, 0x02, 0x81, 0x81,
  0x00, 0xcf, 0x73, 0x8c, 0xbe, 0x6d, 0x45, 0x2d,
  0x0c, 0x0b, 0x5d, 0x5c, 0x6c, 0x75, 0x78, 0xcc,
  0x35, 0x48, 0xb6, 0x98, 0xf1, 0xb9, 0x64, 0x60,
  0x8c, 0x43, 0xeb, 0x85, 0xab, 0x04, 0xb6, 0x7d,
  0x1b, 0x71, 0x75, 0x06, 0xe2, 0xda, 0x84, 0x68,
  0x2e, 0x7f, 0x4c, 0xe3, 0x73, 0xb4, 0xde, 0x51,
  0x4b, 0xb6, 0x51, 0x86, 0x7b, 0xd0, 0xe6, 0x4d,
  0xf3, 0xd1, 0xcf, 0x1a, 0xfe, 0x7f, 0x3a, 0x83,
  0xba, 0xb3, 0xe1, 0xff, 0x54, 0x13, 0x93, 0xd7,
  0x9c, 0x27, 0x80, 0xb7, 0x1e, 0x64, 0x9e, 0xf7,
  0x32, 0x2b, 0x46, 0x29, 0xf7, 0xf8, 0x18, 0x6c,
  0xf7, 0x4a, 0xbe, 0x4b, 0xee, 0x96, 0x90, 0x8f,
  0xa2, 0x16, 0x22, 0x6a, 0xcc, 0x48, 0x06, 0x74,
  0x63, 0x43, 0x7f, 0x27, 0x22, 0x44, 0x3c, 0x2d,
  0x3b, 0x62, 0xf1, 0x1c, 0xb4, 0x27, 0x33, 0x85,
  0x26, 0x60, 0x48, 0x16, 0xcb, 0xef, 0xf8, 0xcd,
  0x37, 0x02, 0x81, 0x81, 0x00, 0xce, 0x15, 0x43,
  0x6e, 0x4b, 0x0f, 0xf9, 0x3f, 0x87, 0xc3, 0x41,
  0x45, 0x97, 0xb1, 0x49, 0xc2, 0x19, 0x23, 0x87,
  0xe4, 0x24, 0x1c, 0x64, 0xe5, 0x28, 0xcb, 0x43,
  0x10, 0x14, 0x14, 0x0e, 0x19, 0xcb, 0xbb, 0xdb,
  0xfd, 0x11, 0x9d, 0x17, 0x68, 0x78, 0x6d, 0x61,
  0x70, 0x63, 0x3a, 0xa1, 0xb3, 0xf3, 0xa7, 0x5b,
  0x0e, 0xff, 0xb7, 0x61, 0x11, 0x54, 0x91, 0x99,
  0xe5, 0x91, 0x32, 0x2d, 0xeb, 0x3f, 0xd8, 0x3e,
  0xf7, 0xd4, 0xcb, 0xd2, 0xa3, 0x41, 0xc1, 0xee,
  0xc6, 0x92, 0x13, 0xeb, 0x7f, 0x42, 0x58, 0xf4,
  0xd0, 0xb2, 0x74, 0x1d, 0x8e, 0x87, 0x46, 0xcd,
  0x14, 0xb8, 0x16, 0xad, 0xb5, 0xbd, 0x0d, 0x6c,
  0x95, 0x5a, 0x16, 0xbf, 0xe9, 0x53, 0xda, 0xfb,
  0xed, 0x83, 0x51, 0x67, 0xa9, 0x55, 0xab, 0x54,
  0x02, 0x95, 0x20, 0xa6, 0x68, 0x17, 0x53, 0xa8,
  0xea, 0x43, 0xe5, 0xb0, 0xa3, 0x02, 0x81, 0x80,
  0x67, 0x9c, 0x32, 0x83, 0x39, 0x57, 0xff, 0x73,
  0xb0, 0x89, 0x64, 0x8b, 0xd6, 0xf0, 0x0a, 0x2d,
  0xe2, 0xaf, 0x30, 0x1c, 0x2a, 0x97, 0xf3, 0x90,
  0x9a, 0xab, 0x9b, 0x0b, 0x1b, 0x43, 0x79, 0xa0,
  0xa7, 0x3d, 0xe7, 0xbe, 0x8d, 0x9c, 0xeb, 0xdb,
  0xad, 0x40, 0xdd, 0xa9, 0x00, 0x80, 0xb8, 0xe1,
  0xb3, 0xa1, 0x6c, 0x25, 0x92, 0xe4, 0x33, 0xb2,
  0xbe, 0xeb, 0x4d, 0x74, 0x26, 0x5f, 0x37, 0x43,
  0x9c, 0x6c, 0x17, 0x76, 0x0a, 0x81, 0x20, 0x82,
  0xa1, 0x48, 0x2c, 0x2d, 0x45, 0xdc, 0x0f, 0x62,
  0x43, 0x32, 0xbb, 0xeb, 0x59, 0x41, 0xf9, 0xca,
  0x58, 0xce, 0x4a, 0x66, 0x53, 0x54, 0xc8, 0x28,
  0x10, 0x1e, 0x08, 0x71, 0x16, 0xd8, 0x02, 0x71,
  0x41, 0x58, 0xd4, 0x56, 0xcc, 0xf5, 0xb1, 0x31,
  0xa3, 0xed, 0x00, 0x85, 0x09, 0xbf, 0x35, 0x95,
  0x41, 0x29, 0x40, 0x19, 0x83, 0x35, 0x24, 0x69,
  0x02, 0x81, 0x80, 0x55, 0x10, 0x0b, 0xcc, 0x3b,
  0xa9, 0x75, 0x3d, 0x16, 0xe1, 0xae, 0x50, 0x76,
  0x63, 0x94, 0x49, 0x4c, 0xad, 0x10, 0xcb, 0x47,
  0x68, 0x7c, 0xf0, 0xe5, 0xdc, 0xb8, 0x6a, 0xab,
  0x8e, 0xf7, 0x9f, 0x08, 0x2c, 0x1b, 0x8a, 0xa2,
  0xb9, 0x8f, 0xce, 0xec, 0x5e, 0x61, 0xa8, 0xcd,
  0x1c, 0x87, 0x60, 0x4a, 0xc3, 0x1a, 0x5f, 0xdf,
  0x87, 0x26, 0xc6, 0xcb, 0x7c, 0x69, 0xe4, 0x8b,
  0x01, 0x06, 0x59, 0x22, 0xfa, 0x34, 0x4b, 0x81,
  0x87, 0x3c, 0x03, 0x6d, 0x02, 0x0a, 0x77, 0xe6,
  0x15, 0xd8, 0xcf, 0xa7, 0x68, 0x26, 0x6c, 0xfa,
  0x2b, 0xd9, 0x83, 0x5a, 0x2d, 0x0c, 0x3b, 0x70,
  0x1c, 0xd4, 0x48, 0xbe, 0xa7, 0x0a, 0xd9, 0xbe,
  0xdc, 0xc3, 0x0c, 0x21, 0x33, 0xb3, 0x66, 0xff,
  0x1c, 0x1b, 0xc8, 0x96, 0x76, 0xe8, 0x6f, 0x44,
  0x74, 0xbc, 0x9b, 0x1c, 0x7d, 0xc8, 0xac, 0x21,
  0xa8, 0x6e, 0x37, 0x02, 0x81, 0x80, 0x2c, 0x7c,
  0xad, 0x1e, 0x75, 0xf6, 0x69, 0x1d, 0xe7, 0xa6,
  0xca, 0x74, 0x7d, 0x67, 0xc8, 0x65, 0x28, 0x66,
  0xc4, 0x43, 0xa6, 0xbd, 0x40, 0x57, 0xae, 0xb7,
  0x65, 0x2c, 0x52, 0xf9, 0xe4, 0xc7, 0x81, 0x7b,
  0x56, 0xa3, 0xd2, 0x0d, 0xe8, 0x33, 0x70, 0xcf,
  0x06, 0x84, 0xb3, 0x4e, 0x44, 0x50, 0x75, 0x61,
  0x96, 0x86, 0x4b, 0xb6, 0x2b, 0xad, 0xf0, 0xad,
  0x57, 0xd0, 0x37, 0x0d, 0x1d, 0x35, 0x50, 0xcb,
  0x69, 0x22, 0x39, 0x29, 0xb9, 0x3a, 0xd3, 0x29,
  0x23, 0x02, 0x60, 0xf7, 0xab, 0x30, 0x40, 0xda,
  0x8e, 0x4d, 0x45, 0x70, 0x26, 0xf4, 0xa2, 0x0d,
  0xd0, 0x64, 0x5d, 0x47, 0x3c, 0x18, 0xf4, 0xd4,
  0x52, 0x95, 0x00, 0xae, 0x84, 0x6b, 0x47, 0xb2,
  0x3c, 0x82, 0xd3, 0x72, 0x53, 0xde, 0x72, 0x2c,
  0xf7, 0xc1, 0x22, 0x36, 0xd9, 0x18, 0x56, 0xfe,
  0x39, 0x28, 0x33, 0xe0, 0xdb, 0x03 };

// A 2048 bit RSA key in PKCS#8 PrivateKeyInfo format
// Used to verify the functions that manipulate RSA keys.
static const uint8_t kTestRSAPKCS8PrivateKeyInfo2_2048[] = {
  0x30, 0x82, 0x04, 0xbc, 0x02, 0x01, 0x00, 0x30,
  0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
  0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82,
  0x04, 0xa6, 0x30, 0x82, 0x04, 0xa2, 0x02, 0x01,
  0x00, 0x02, 0x82, 0x01, 0x01, 0x00, 0xa7, 0x00,
  0x36, 0x60, 0x65, 0xdc, 0xbd, 0x54, 0x5a, 0x2a,
  0x40, 0xb4, 0xe1, 0x15, 0x94, 0x58, 0x11, 0x4f,
  0x94, 0x58, 0xdd, 0xde, 0xa7, 0x1f, 0x3c, 0x2c,
  0xe0, 0x88, 0x09, 0x29, 0x61, 0x57, 0x67, 0x5e,
  0x56, 0x7e, 0xee, 0x27, 0x8f, 0x59, 0x34, 0x9a,
  0x2a, 0xaa, 0x9d, 0xb4, 0x4e, 0xfa, 0xa7, 0x6a,
  0xd4, 0xc9, 0x7a, 0x53, 0xc1, 0x4e, 0x9f, 0xe3,
  0x34, 0xf7, 0x3d, 0xb7, 0xc9, 0x10, 0x47, 0x4f,
  0x28, 0xda, 0x3f, 0xce, 0x31, 0x7b, 0xfd, 0x06,
  0x10, 0xeb, 0xf7, 0xbe, 0x92, 0xf9, 0xaf, 0xfb,
  0x3e, 0x68, 0xda, 0xee, 0x1a, 0x64, 0x4c, 0xf3,
  0x29, 0xf2, 0x73, 0x9e, 0x39, 0xd8, 0xf6, 0x6f,
  0xd8, 0xb2, 0x80, 0x82, 0x71, 0x8e, 0xb5, 0xa4,
  0xf2, 0xc2, 0x3e, 0xcd, 0x0a, 0xca, 0xb6, 0x04,
  0xcd, 0x9a, 0x13, 0x8b, 0x54, 0x73, 0x54, 0x25,
  0x54, 0x8c, 0xbe, 0x98, 0x7a, 0x67, 0xad, 0xda,
  0xb3, 0x4e, 0xb3, 0xfa, 0x82, 0xa8, 0x4a, 0x67,
  0x98, 0x56, 0x57, 0x54, 0x71, 0xcd, 0x12, 0x7f,
  0xed, 0xa3, 0x01, 0xc0, 0x6a, 0x8b, 0x24, 0x03,
  0x96, 0x88, 0xbe, 0x97, 0x66, 0x2a, 0xbc, 0x53,
  0xc9, 0x83, 0x06, 0x51, 0x5a, 0x88, 0x65, 0x13,
  0x18, 0xe4, 0x3a, 0xed, 0x6b, 0xf1, 0x61, 0x5b,
  0x4c, 0xc8, 0x1e, 0xf4, 0xc2, 0xae, 0x08, 0x5e,
  0x2d, 0x5f, 0xf8, 0x12, 0x7f, 0xa2, 0xfc, 0xbb,
  0x21, 0x18, 0x30, 0xda, 0xfe, 0x40, 0xfb, 0x01,
  0xca, 0x2e, 0x37, 0x0e, 0xce, 0xdd, 0x76, 0x87,
  0x82, 0x46, 0x0b, 0x3a, 0x77, 0x8f, 0xc0, 0x72,
  0x07, 0x2c, 0x7f, 0x9d, 0x1e, 0x86, 0x5b, 0xed,
  0x27, 0x29, 0xdf, 0x03, 0x97, 0x62, 0xef, 0x44,
  0xd3, 0x5b, 0x3d, 0xdb, 0x9c, 0x5e, 0x1b, 0x7b,
  0x39, 0xb4, 0x0b, 0x6d, 0x04, 0x6b, 0xbb, 0xbb,
  0x2c, 0x5f, 0xcf, 0xb3, 0x7a, 0x05, 0x02, 0x03,
  0x01, 0x00, 0x01, 0x02, 0x82, 0x01, 0x00, 0x5e,
  0x79, 0x65, 0x49, 0xa5, 0x76, 0x79, 0xf9, 0x05,
  0x45, 0x0f, 0xf4, 0x03, 0xbd, 0xa4, 0x7d, 0x29,
  0xd5, 0xde, 0x33, 0x63, 0xd8, 0xb8, 0xac, 0x97,
  0xeb, 0x3f, 0x5e, 0x55, 0xe8, 0x7d, 0xf3, 0xe7,
  0x3b, 0x5c, 0x2d, 0x54, 0x67, 0x36, 0xd6, 0x1d,
  0x46, 0xf5, 0xca, 0x2d, 0x8b, 0x3a, 0x7e, 0xdc,
  0x45, 0x38, 0x79, 0x7e, 0x65, 0x71, 0x5f, 0x1c,
  0x5e, 0x79, 0xb1, 0x40, 0xcd, 0xfe, 0xc5, 0xe1,
  0xc1, 0x6b, 0x78, 0x04, 0x4e, 0x8e, 0x79, 0xf9,
  0x0a, 0xfc, 0x79, 0xb1, 0x5e, 0xb3, 0x60, 0xe3,
  0x68, 0x7b, 0xc6, 0xef, 0xcb, 0x71, 0x4c, 0xba,
  0xa7, 0x79, 0x5c, 0x7a, 0x81, 0xd1, 0x71, 0xe7,
  0x00, 0x21, 0x13, 0xe2, 0x55, 0x69, 0x0e, 0x75,
  0xbe, 0x09, 0xc3, 0x4f, 0xa9, 0xc9, 0x68, 0x22,
  0x0e, 0x97, 0x8d, 0x89, 0x6e, 0xf1, 0xe8, 0x88,
  0x7a, 0xd1, 0xd9, 0x09, 0x5d, 0xd3, 0x28, 0x78,
  0x25, 0x0b, 0x1c, 0x47, 0x73, 0x25, 0xcc, 0x21,
  0xb6, 0xda, 0xc6, 0x24, 0x5a, 0xd0, 0x37, 0x14,
  0x46, 0xc7, 0x94, 0x69, 0xe4, 0x43, 0x6f, 0x47,
  0xde, 0x00, 0x33, 0x4d, 0x8f, 0x95, 0x72, 0xfa,
  0x68, 0x71, 0x17, 0x66, 0x12, 0x1a, 0x87, 0x27,
  0xf7, 0xef, 0x7e, 0xe0, 0x35, 0x58, 0xf2, 0x4d,
  0x6f, 0x35, 0x01, 0xaa, 0x96, 0xe2, 0x3d, 0x51,
  0x13, 0x86, 0x9c, 0x79, 0xd0, 0xb7, 0xb6, 0x64,
  0xe8, 0x86, 0x65, 0x50, 0xbf, 0xcc, 0x27, 0x53,
  0x1f, 0x51, 0xd4, 0xca, 0xbe, 0xf5, 0xdd, 0x77,
  0x70, 0x98, 0x0f, 0xee, 0xa8, 0x96, 0x07, 0x5f,
  0x45, 0x6a, 0x7a, 0x0d, 0x03, 0x9c, 0x4f, 0x29,
  0xf6, 0x06, 0xf3, 0x5d, 0x58, 0x6c, 0x47, 0xd0,
  0x96, 0xa9, 0x03, 0x17, 0xbb, 0x4e, 0xc9, 0x21,
  0xe0, 0xac, 0xcd, 0x78, 0x78, 0xb2, 0xfe, 0x81,
  0xb2, 0x51, 0x53, 0xa6, 0x1f, 0x98, 0x45, 0x02,
  0x81, 0x81, 0x00, 0xcf, 0x73, 0x8c, 0xbe, 0x6d,
  0x45, 0x2d, 0x0c, 0x0b, 0x5d, 0x5c, 0x6c, 0x75,
  0x78, 0xcc, 0x35, 0x48, 0xb6, 0x98, 0xf1, 0xb9,
  0x64, 0x60, 0x8c, 0x43, 0xeb, 0x85, 0xab, 0x04,
  0xb6, 0x7d, 0x1b, 0x71, 0x75, 0x06, 0xe2, 0xda,
  0x84, 0x68, 0x2e, 0x7f, 0x4c, 0xe3, 0x73, 0xb4,
  0xde, 0x51, 0x4b, 0xb6, 0x51, 0x86, 0x7b, 0xd0,
  0xe6, 0x4d, 0xf3, 0xd1, 0xcf, 0x1a, 0xfe, 0x7f,
  0x3a, 0x83, 0xba, 0xb3, 0xe1, 0xff, 0x54, 0x13,
  0x93, 0xd7, 0x9c, 0x27, 0x80, 0xb7, 0x1e, 0x64,
  0x9e, 0xf7, 0x32, 0x2b, 0x46, 0x29, 0xf7, 0xf8,
  0x18, 0x6c, 0xf7, 0x4a, 0xbe, 0x4b, 0xee, 0x96,
  0x90, 0x8f, 0xa2, 0x16, 0x22, 0x6a, 0xcc, 0x48,
  0x06, 0x74, 0x63, 0x43, 0x7f, 0x27, 0x22, 0x44,
  0x3c, 0x2d, 0x3b, 0x62, 0xf1, 0x1c, 0xb4, 0x27,
  0x33, 0x85, 0x26, 0x60, 0x48, 0x16, 0xcb, 0xef,
  0xf8, 0xcd, 0x37, 0x02, 0x81, 0x81, 0x00, 0xce,
  0x15, 0x43, 0x6e, 0x4b, 0x0f, 0xf9, 0x3f, 0x87,
  0xc3, 0x41, 0x45, 0x97, 0xb1, 0x49, 0xc2, 0x19,
  0x23, 0x87, 0xe4, 0x24, 0x1c, 0x64, 0xe5, 0x28,
  0xcb, 0x43, 0x10, 0x14, 0x14, 0x0e, 0x19, 0xcb,
  0xbb, 0xdb, 0xfd, 0x11, 0x9d, 0x17, 0x68, 0x78,
  0x6d, 0x61, 0x70, 0x63, 0x3a, 0xa1, 0xb3, 0xf3,
  0xa7, 0x5b, 0x0e, 0xff, 0xb7, 0x61, 0x11, 0x54,
  0x91, 0x99, 0xe5, 0x91, 0x32, 0x2d, 0xeb, 0x3f,
  0xd8, 0x3e, 0xf7, 0xd4, 0xcb, 0xd2, 0xa3, 0x41,
  0xc1, 0xee, 0xc6, 0x92, 0x13, 0xeb, 0x7f, 0x42,
  0x58, 0xf4, 0xd0, 0xb2, 0x74, 0x1d, 0x8e, 0x87,
  0x46, 0xcd, 0x14, 0xb8, 0x16, 0xad, 0xb5, 0xbd,
  0x0d, 0x6c, 0x95, 0x5a, 0x16, 0xbf, 0xe9, 0x53,
  0xda, 0xfb, 0xed, 0x83, 0x51, 0x67, 0xa9, 0x55,
  0xab, 0x54, 0x02, 0x95, 0x20, 0xa6, 0x68, 0x17,
  0x53, 0xa8, 0xea, 0x43, 0xe5, 0xb0, 0xa3, 0x02,
  0x81, 0x80, 0x67, 0x9c, 0x32, 0x83, 0x39, 0x57,
  0xff, 0x73, 0xb0, 0x89, 0x64, 0x8b, 0xd6, 0xf0,
  0x0a, 0x2d, 0xe2, 0xaf, 0x30, 0x1c, 0x2a, 0x97,
  0xf3, 0x90, 0x9a, 0xab, 0x9b, 0x0b, 0x1b, 0x43,
  0x79, 0xa0, 0xa7, 0x3d, 0xe7, 0xbe, 0x8d, 0x9c,
  0xeb, 0xdb, 0xad, 0x40, 0xdd, 0xa9, 0x00, 0x80,
  0xb8, 0xe1, 0xb3, 0xa1, 0x6c, 0x25, 0x92, 0xe4,
  0x33, 0xb2, 0xbe, 0xeb, 0x4d, 0x74, 0x26, 0x5f,
  0x37, 0x43, 0x9c, 0x6c, 0x17, 0x76, 0x0a, 0x81,
  0x20, 0x82, 0xa1, 0x48, 0x2c, 0x2d, 0x45, 0xdc,
  0x0f, 0x62, 0x43, 0x32, 0xbb, 0xeb, 0x59, 0x41,
  0xf9, 0xca, 0x58, 0xce, 0x4a, 0x66, 0x53, 0x54,
  0xc8, 0x28, 0x10, 0x1e, 0x08, 0x71, 0x16, 0xd8,
  0x02, 0x71, 0x41, 0x58, 0xd4, 0x56, 0xcc, 0xf5,
  0xb1, 0x31, 0xa3, 0xed, 0x00, 0x85, 0x09, 0xbf,
  0x35, 0x95, 0x41, 0x29, 0x40, 0x19, 0x83, 0x35,
  0x24, 0x69, 0x02, 0x81, 0x80, 0x55, 0x10, 0x0b,
  0xcc, 0x3b, 0xa9, 0x75, 0x3d, 0x16, 0xe1, 0xae,
  0x50, 0x76, 0x63, 0x94, 0x49, 0x4c, 0xad, 0x10,
  0xcb, 0x47, 0x68, 0x7c, 0xf0, 0xe5, 0xdc, 0xb8,
  0x6a, 0xab, 0x8e, 0xf7, 0x9f, 0x08, 0x2c, 0x1b,
  0x8a, 0xa2, 0xb9, 0x8f, 0xce, 0xec, 0x5e, 0x61,
  0xa8, 0xcd, 0x1c, 0x87, 0x60, 0x4a, 0xc3, 0x1a,
  0x5f, 0xdf, 0x87, 0x26, 0xc6, 0xcb, 0x7c, 0x69,
  0xe4, 0x8b, 0x01, 0x06, 0x59, 0x22, 0xfa, 0x34,
  0x4b, 0x81, 0x87, 0x3c, 0x03, 0x6d, 0x02, 0x0a,
  0x77, 0xe6, 0x15, 0xd8, 0xcf, 0xa7, 0x68, 0x26,
  0x6c, 0xfa, 0x2b, 0xd9, 0x83, 0x5a, 0x2d, 0x0c,
  0x3b, 0x70, 0x1c, 0xd4, 0x48, 0xbe, 0xa7, 0x0a,
  0xd9, 0xbe, 0xdc, 0xc3, 0x0c, 0x21, 0x33, 0xb3,
  0x66, 0xff, 0x1c, 0x1b, 0xc8, 0x96, 0x76, 0xe8,
  0x6f, 0x44, 0x74, 0xbc, 0x9b, 0x1c, 0x7d, 0xc8,
  0xac, 0x21, 0xa8, 0x6e, 0x37, 0x02, 0x81, 0x80,
  0x2c, 0x7c, 0xad, 0x1e, 0x75, 0xf6, 0x69, 0x1d,
  0xe7, 0xa6, 0xca, 0x74, 0x7d, 0x67, 0xc8, 0x65,
  0x28, 0x66, 0xc4, 0x43, 0xa6, 0xbd, 0x40, 0x57,
  0xae, 0xb7, 0x65, 0x2c, 0x52, 0xf9, 0xe4, 0xc7,
  0x81, 0x7b, 0x56, 0xa3, 0xd2, 0x0d, 0xe8, 0x33,
  0x70, 0xcf, 0x06, 0x84, 0xb3, 0x4e, 0x44, 0x50,
  0x75, 0x61, 0x96, 0x86, 0x4b, 0xb6, 0x2b, 0xad,
  0xf0, 0xad, 0x57, 0xd0, 0x37, 0x0d, 0x1d, 0x35,
  0x50, 0xcb, 0x69, 0x22, 0x39, 0x29, 0xb9, 0x3a,
  0xd3, 0x29, 0x23, 0x02, 0x60, 0xf7, 0xab, 0x30,
  0x40, 0xda, 0x8e, 0x4d, 0x45, 0x70, 0x26, 0xf4,
  0xa2, 0x0d, 0xd0, 0x64, 0x5d, 0x47, 0x3c, 0x18,
  0xf4, 0xd4, 0x52, 0x95, 0x00, 0xae, 0x84, 0x6b,
  0x47, 0xb2, 0x3c, 0x82, 0xd3, 0x72, 0x53, 0xde,
  0x72, 0x2c, 0xf7, 0xc1, 0x22, 0x36, 0xd9, 0x18,
  0x56, 0xfe, 0x39, 0x28, 0x33, 0xe0, 0xdb, 0x03 };

// A 2048 bit RSA Public key
// Used to verify the functions that manipulate RSA keys.
static const uint8_t kTestRSAPublicKey2_2048[] = {
  0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
  0x00, 0xa7, 0x00, 0x36, 0x60, 0x65, 0xdc, 0xbd,
  0x54, 0x5a, 0x2a, 0x40, 0xb4, 0xe1, 0x15, 0x94,
  0x58, 0x11, 0x4f, 0x94, 0x58, 0xdd, 0xde, 0xa7,
  0x1f, 0x3c, 0x2c, 0xe0, 0x88, 0x09, 0x29, 0x61,
  0x57, 0x67, 0x5e, 0x56, 0x7e, 0xee, 0x27, 0x8f,
  0x59, 0x34, 0x9a, 0x2a, 0xaa, 0x9d, 0xb4, 0x4e,
  0xfa, 0xa7, 0x6a, 0xd4, 0xc9, 0x7a, 0x53, 0xc1,
  0x4e, 0x9f, 0xe3, 0x34, 0xf7, 0x3d, 0xb7, 0xc9,
  0x10, 0x47, 0x4f, 0x28, 0xda, 0x3f, 0xce, 0x31,
  0x7b, 0xfd, 0x06, 0x10, 0xeb, 0xf7, 0xbe, 0x92,
  0xf9, 0xaf, 0xfb, 0x3e, 0x68, 0xda, 0xee, 0x1a,
  0x64, 0x4c, 0xf3, 0x29, 0xf2, 0x73, 0x9e, 0x39,
  0xd8, 0xf6, 0x6f, 0xd8, 0xb2, 0x80, 0x82, 0x71,
  0x8e, 0xb5, 0xa4, 0xf2, 0xc2, 0x3e, 0xcd, 0x0a,
  0xca, 0xb6, 0x04, 0xcd, 0x9a, 0x13, 0x8b, 0x54,
  0x73, 0x54, 0x25, 0x54, 0x8c, 0xbe, 0x98, 0x7a,
  0x67, 0xad, 0xda, 0xb3, 0x4e, 0xb3, 0xfa, 0x82,
  0xa8, 0x4a, 0x67, 0x98, 0x56, 0x57, 0x54, 0x71,
  0xcd, 0x12, 0x7f, 0xed, 0xa3, 0x01, 0xc0, 0x6a,
  0x8b, 0x24, 0x03, 0x96, 0x88, 0xbe, 0x97, 0x66,
  0x2a, 0xbc, 0x53, 0xc9, 0x83, 0x06, 0x51, 0x5a,
  0x88, 0x65, 0x13, 0x18, 0xe4, 0x3a, 0xed, 0x6b,
  0xf1, 0x61, 0x5b, 0x4c, 0xc8, 0x1e, 0xf4, 0xc2,
  0xae, 0x08, 0x5e, 0x2d, 0x5f, 0xf8, 0x12, 0x7f,
  0xa2, 0xfc, 0xbb, 0x21, 0x18, 0x30, 0xda, 0xfe,
  0x40, 0xfb, 0x01, 0xca, 0x2e, 0x37, 0x0e, 0xce,
  0xdd, 0x76, 0x87, 0x82, 0x46, 0x0b, 0x3a, 0x77,
  0x8f, 0xc0, 0x72, 0x07, 0x2c, 0x7f, 0x9d, 0x1e,
  0x86, 0x5b, 0xed, 0x27, 0x29, 0xdf, 0x03, 0x97,
  0x62, 0xef, 0x44, 0xd3, 0x5b, 0x3d, 0xdb, 0x9c,
  0x5e, 0x1b, 0x7b, 0x39, 0xb4, 0x0b, 0x6d, 0x04,
  0x6b, 0xbb, 0xbb, 0x2c, 0x5f, 0xcf, 0xb3, 0x7a,
  0x05, 0x02, 0x03, 0x01, 0x00, 0x01 };

// A second 2048-bit RSA private key
// This is used to verify the functions that manipulate RSA keys.
static const uint8_t kTestPKCS1RSAPrivateKey3_2048[] = {
  0x30, 0x82, 0x04, 0xa4, 0x02, 0x01, 0x00, 0x02,
  0x82, 0x01, 0x01, 0x00, 0xa5, 0xd0, 0xd7, 0x3e,
  0x0e, 0x2d, 0xfb, 0x43, 0x51, 0x99, 0xea, 0x40,
  0x1e, 0x2d, 0x89, 0xe4, 0xa2, 0x3e, 0xfc, 0x51,
  0x3d, 0x0e, 0x83, 0xa7, 0xe0, 0xa5, 0x41, 0x04,
  0x1e, 0x14, 0xc5, 0xa7, 0x5c, 0x61, 0x36, 0x44,
  0xb3, 0x08, 0x05, 0x5b, 0x14, 0xde, 0x01, 0x0c,
  0x32, 0x3c, 0x9a, 0x91, 0x00, 0x50, 0xa8, 0x1d,
  0xcc, 0x9f, 0x8f, 0x35, 0xb7, 0xc2, 0x75, 0x08,
  0x32, 0x8b, 0x10, 0x3a, 0x86, 0xf9, 0xd7, 0x78,
  0xa3, 0x9d, 0x74, 0x10, 0xc6, 0x24, 0xb1, 0x7f,
  0xa5, 0xbf, 0x5f, 0xc2, 0xd7, 0x15, 0xa3, 0x1d,
  0xe0, 0x15, 0x6b, 0x1b, 0x0e, 0x38, 0xba, 0x34,
  0xbc, 0x95, 0x47, 0x94, 0x40, 0x70, 0xac, 0x99,
  0x1f, 0x0b, 0x8e, 0x56, 0x93, 0x36, 0x2b, 0x6d,
  0x04, 0xe7, 0x95, 0x1a, 0x37, 0xda, 0x16, 0x57,
  0x99, 0xee, 0x03, 0x68, 0x16, 0x31, 0xaa, 0xc3,
  0xb7, 0x92, 0x75, 0x53, 0xfc, 0xf6, 0x20, 0x55,
  0x44, 0xf8, 0xd4, 0x8d, 0x78, 0x15, 0xc7, 0x1a,
  0xb6, 0xde, 0x6c, 0xe8, 0x49, 0x5d, 0xaf, 0xa8,
  0x4e, 0x6f, 0x7c, 0xe2, 0x6a, 0x4c, 0xd5, 0xe7,
  0x8c, 0x8f, 0x0b, 0x5d, 0x3a, 0x09, 0xd6, 0xb3,
  0x44, 0xab, 0xe0, 0x35, 0x52, 0x7c, 0x66, 0x85,
  0xa4, 0x40, 0xd7, 0x20, 0xec, 0x24, 0x05, 0x06,
  0xd9, 0x84, 0x51, 0x5a, 0xd2, 0x38, 0xd5, 0x1d,
  0xea, 0x70, 0x2a, 0x21, 0xe6, 0x82, 0xfd, 0xa4,
  0x46, 0x1c, 0x4f, 0x59, 0x6e, 0x29, 0x3d, 0xae,
  0xb8, 0x8e, 0xee, 0x77, 0x1f, 0x15, 0x33, 0xcf,
  0x94, 0x1d, 0x87, 0x3c, 0x37, 0xc5, 0x89, 0xe8,
  0x7d, 0x85, 0xb3, 0xbc, 0xe8, 0x62, 0x6a, 0x84,
  0x7f, 0xfe, 0x9a, 0x85, 0x3f, 0x39, 0xe8, 0xaa,
  0x16, 0xa6, 0x8f, 0x87, 0x7f, 0xcb, 0xc1, 0xd6,
  0xf2, 0xec, 0x2b, 0xa7, 0xdd, 0x49, 0x98, 0x7b,
  0x6f, 0xdd, 0x69, 0x6d, 0x02, 0x03, 0x01, 0x00,
  0x01, 0x02, 0x82, 0x01, 0x00, 0x43, 0x8f, 0x19,
  0x83, 0xb1, 0x27, 0x4e, 0xee, 0x98, 0xba, 0xcb,
  0x54, 0xa0, 0x77, 0x11, 0x6d, 0xd4, 0x25, 0x31,
  0x8c, 0xb0, 0x01, 0xcf, 0xe6, 0x80, 0x83, 0x14,
  0x40, 0x67, 0x39, 0x33, 0x67, 0x03, 0x1e, 0xa0,
  0x8b, 0xd1, 0x1d, 0xfd, 0x80, 0xa4, 0xb9, 0xe7,
  0x57, 0x5e, 0xc8, 0x8e, 0x79, 0x71, 0xd5, 0x6b,
  0x09, 0xe9, 0x2b, 0x41, 0xa0, 0x33, 0x64, 0xc9,
  0x66, 0x33, 0xa1, 0xb1, 0x55, 0x07, 0x55, 0x98,
  0x53, 0x10, 0xe6, 0xc0, 0x39, 0x6d, 0x61, 0xd9,
  0xe8, 0x16, 0x52, 0x28, 0xe4, 0x2b, 0xda, 0x27,
  0x01, 0xaf, 0x21, 0x4a, 0xe8, 0x55, 0x1d, 0x0b,
  0xd1, 0x1c, 0xdc, 0xfd, 0xb3, 0x0b, 0xa6, 0x5c,
  0xcc, 0x6e, 0x77, 0xb8, 0xe0, 0xd1, 0x4e, 0x0a,
  0xd7, 0x7a, 0x5e, 0x18, 0xc3, 0xfb, 0xe9, 0xa1,
  0x9c, 0xc3, 0x9c, 0xd4, 0x4a, 0x7e, 0x70, 0x72,
  0x11, 0x18, 0x24, 0x56, 0x24, 0xdf, 0xf8, 0xba,
  0xac, 0x5b, 0x54, 0xd3, 0xc4, 0x65, 0x69, 0xc8,
  0x79, 0x94, 0x16, 0x88, 0x9a, 0x68, 0x1c, 0xbc,
  0xd4, 0xca, 0xec, 0x5e, 0x07, 0x4a, 0xc9, 0x54,
  0x7a, 0x4b, 0xdb, 0x19, 0x88, 0xf6, 0xbe, 0x50,
  0x9d, 0x9e, 0x9d, 0x88, 0x5b, 0x4a, 0x23, 0x86,
  0x2b, 0xa9, 0xa6, 0x6c, 0x70, 0x7d, 0xe1, 0x11,
  0xba, 0xbf, 0x03, 0x2e, 0xf1, 0x46, 0x7e, 0x1b,
  0xed, 0x06, 0x11, 0x57, 0xad, 0x4a, 0xcb, 0xe5,
  0xb1, 0x11, 0x05, 0x0a, 0x30, 0xb1, 0x73, 0x79,
  0xcd, 0x7a, 0x04, 0xcc, 0x70, 0xe9, 0x95, 0xe4,
  0x27, 0xc2, 0xd5, 0x2d, 0x92, 0x44, 0xdf, 0xb4,
  0x94, 0xa8, 0x73, 0xa1, 0x4a, 0xc3, 0xcc, 0xc4,
  0x0e, 0x8d, 0xa1, 0x6a, 0xc2, 0xd8, 0x03, 0x7f,
  0xfa, 0xa7, 0x76, 0x0d, 0xad, 0x87, 0x88, 0xa0,
  0x77, 0xaf, 0x3b, 0x23, 0xd1, 0x66, 0x0b, 0x31,
  0x2b, 0xaf, 0xef, 0xd5, 0x41, 0x02, 0x81, 0x81,
  0x00, 0xdb, 0xc1, 0xe7, 0xdd, 0xba, 0x3c, 0x1f,
  0x9c, 0x64, 0xca, 0xa0, 0x63, 0xdb, 0xd2, 0x47,
  0x5c, 0x6e, 0x8a, 0xa3, 0x16, 0xd5, 0xda, 0xc2,
  0x25, 0x64, 0x0a, 0x02, 0xbc, 0x7d, 0x7f, 0x50,
  0xab, 0xe0, 0x66, 0x03, 0x53, 0x7d, 0x77, 0x6d,
  0x6c, 0x61, 0x58, 0x09, 0x73, 0xcd, 0x18, 0xe9,
  0x53, 0x0b, 0x5c, 0xa2, 0x71, 0x14, 0x02, 0xfd,
  0x55, 0xda, 0xe9, 0x77, 0x24, 0x7c, 0x2a, 0x4e,
  0xb9, 0xd9, 0x5d, 0x58, 0xf6, 0x26, 0xd0, 0xd8,
  0x3d, 0xcf, 0x8c, 0x89, 0x65, 0x6c, 0x35, 0x19,
  0xb6, 0x63, 0xff, 0xa0, 0x71, 0x49, 0xcd, 0x6d,
  0x5b, 0x3d, 0x8f, 0xea, 0x6f, 0xa9, 0xba, 0x43,
  0xe5, 0xdd, 0x39, 0x3a, 0x78, 0x8f, 0x07, 0xb8,
  0xab, 0x58, 0x07, 0xb7, 0xd2, 0xf8, 0x07, 0x02,
  0x9b, 0x79, 0x26, 0x32, 0x22, 0x38, 0x91, 0x01,
  0x90, 0x81, 0x29, 0x94, 0xad, 0x77, 0xeb, 0x86,
  0xb9, 0x02, 0x81, 0x81, 0x00, 0xc1, 0x29, 0x88,
  0xbd, 0x96, 0x31, 0x33, 0x7b, 0x77, 0x5d, 0x32,
  0x12, 0x5e, 0xdf, 0x28, 0x0c, 0x96, 0x0d, 0xa8,
  0x22, 0xdf, 0xd3, 0x35, 0xd7, 0xb0, 0x41, 0xcb,
  0xe7, 0x94, 0x8a, 0xa4, 0xed, 0xd2, 0xfb, 0xd2,
  0xf3, 0xf2, 0x95, 0xff, 0xd8, 0x33, 0x3f, 0x8c,
  0xd7, 0x65, 0xe4, 0x0c, 0xcc, 0xfe, 0x32, 0x66,
  0xfa, 0x50, 0xe2, 0xcf, 0xf0, 0xbe, 0x05, 0xb1,
  0xbc, 0xbe, 0x44, 0x09, 0xb4, 0xfe, 0x95, 0x06,
  0x18, 0xd7, 0x59, 0xc6, 0xef, 0x2d, 0x22, 0xa0,
  0x73, 0x5e, 0x77, 0xdf, 0x8d, 0x09, 0x2c, 0xb8,
  0xcc, 0xeb, 0x10, 0x4d, 0xa7, 0xd0, 0x4b, 0x46,
  0xba, 0x7d, 0x8b, 0x6a, 0x55, 0x47, 0x55, 0xd3,
  0xd7, 0xb1, 0x88, 0xfd, 0x27, 0x3e, 0xf9, 0x5b,
  0x7b, 0xae, 0x6d, 0x08, 0x9f, 0x0c, 0x2a, 0xe1,
  0xdd, 0xb9, 0xe3, 0x55, 0x13, 0x55, 0xa3, 0x6d,
  0x06, 0xbb, 0xe0, 0x1e, 0x55, 0x02, 0x81, 0x80,
  0x61, 0x73, 0x3d, 0x64, 0xff, 0xdf, 0x05, 0x8d,
  0x8e, 0xcc, 0xa4, 0x0f, 0x64, 0x3d, 0x7d, 0x53,
  0xa9, 0xd9, 0x64, 0xb5, 0x0d, 0xa4, 0x72, 0x8f,
  0xae, 0x2b, 0x1a, 0x47, 0x87, 0xc7, 0x5b, 0x78,
  0xbc, 0x8b, 0xc0, 0x51, 0xd7, 0xc3, 0x8c, 0x0c,
  0x91, 0xa6, 0x3e, 0x9a, 0xd1, 0x8a, 0x88, 0x7d,
  0x40, 0xfe, 0x95, 0x32, 0x5b, 0xd3, 0x6f, 0x90,
  0x11, 0x01, 0x92, 0xc9, 0xe5, 0x1d, 0xc5, 0xc7,
  0x78, 0x72, 0x82, 0xae, 0xb5, 0x4b, 0xcb, 0x78,
  0xad, 0x7e, 0xfe, 0xb6, 0xb1, 0x23, 0x63, 0x01,
  0x94, 0x9a, 0x99, 0x05, 0x63, 0xda, 0xea, 0xf1,
  0x98, 0xfd, 0x26, 0xd2, 0xd9, 0x8b, 0x35, 0xec,
  0xcb, 0x0b, 0x43, 0xb8, 0x8e, 0x84, 0xb8, 0x09,
  0x93, 0x81, 0xe8, 0xac, 0x6f, 0x3c, 0x7c, 0x95,
  0x81, 0x45, 0xc4, 0xd9, 0x94, 0x08, 0x09, 0x8f,
  0x91, 0x17, 0x65, 0x4c, 0xff, 0x6e, 0xbc, 0x51,
  0x02, 0x81, 0x81, 0x00, 0xc1, 0x0d, 0x9d, 0xd8,
  0xbd, 0xaf, 0x56, 0xe0, 0xe3, 0x1f, 0x85, 0xd7,
  0xce, 0x72, 0x02, 0x38, 0xf2, 0x0f, 0x9c, 0x27,
  0x9e, 0xc4, 0x1d, 0x60, 0x00, 0x8d, 0x02, 0x19,
  0xe5, 0xdf, 0xdb, 0x8e, 0xc5, 0xfb, 0x61, 0x8e,
  0xe6, 0xb8, 0xfc, 0x07, 0x3c, 0xd1, 0x1b, 0x16,
  0x7c, 0x83, 0x3c, 0x37, 0xf5, 0x26, 0xb2, 0xbd,
  0x22, 0xf2, 0x4d, 0x19, 0x33, 0x11, 0xc5, 0xdd,
  0xf9, 0xdb, 0x4e, 0x48, 0x52, 0xd8, 0xe6, 0x4b,
  0x15, 0x90, 0x68, 0xbe, 0xca, 0xc1, 0x7c, 0xd3,
  0x51, 0x6b, 0x45, 0x46, 0x54, 0x11, 0x1a, 0x71,
  0xd3, 0xcd, 0x6b, 0x8f, 0x79, 0x22, 0x83, 0x02,
  0x08, 0x4f, 0xba, 0x6a, 0x98, 0xed, 0x32, 0xd8,
  0xb4, 0x5b, 0x51, 0x88, 0x53, 0xec, 0x2c, 0x7e,
  0xa4, 0x89, 0xdc, 0xbf, 0xf9, 0x0d, 0x32, 0xc8,
  0xc3, 0xec, 0x6d, 0x2e, 0xf1, 0xbc, 0x70, 0x4e,
  0xf6, 0x9e, 0xbc, 0x31, 0x02, 0x81, 0x81, 0x00,
  0xd3, 0x35, 0x1b, 0x19, 0x75, 0x3f, 0x61, 0xf2,
  0x55, 0x03, 0xce, 0x25, 0xa9, 0xdf, 0x0c, 0x0a,
  0x3b, 0x47, 0x42, 0xdc, 0x38, 0x4b, 0x13, 0x4d,
  0x1f, 0x86, 0x58, 0x4f, 0xd8, 0xee, 0xfa, 0x76,
  0x15, 0xfb, 0x6e, 0x55, 0x31, 0xf2, 0xd2, 0x62,
  0x32, 0xa5, 0xc4, 0x23, 0x5e, 0x08, 0xa9, 0x83,
  0x07, 0xac, 0x8c, 0xa3, 0x7e, 0x18, 0xc0, 0x1c,
  0x57, 0x63, 0x8d, 0x05, 0x17, 0x47, 0x1b, 0xd3,
  0x74, 0x73, 0x20, 0x04, 0xfb, 0xc8, 0x1a, 0x43,
  0x04, 0x36, 0xc8, 0x19, 0xbe, 0xdc, 0xa6, 0xe5,
  0x0f, 0x25, 0x62, 0x24, 0x96, 0x92, 0xb6, 0xb3,
  0x97, 0xad, 0x57, 0x9a, 0x90, 0x37, 0x4e, 0x31,
  0x44, 0x74, 0xfa, 0x7c, 0xb4, 0xea, 0xfc, 0x15,
  0xa7, 0xb0, 0x51, 0xcc, 0xee, 0x1e, 0xed, 0x5b,
  0x98, 0x18, 0x0e, 0x65, 0xb6, 0x4b, 0x69, 0x0b,
  0x21, 0xdc, 0x86, 0x17, 0x6e, 0xc8, 0xee, 0x24 };

// A second 2048 bit RSA key in PKCS#8 PrivateKeyInfo format
// Used to verify the functions that manipulate RSA keys.
static const uint8_t kTestRSAPKCS8PrivateKeyInfo3_2048[] = {
  0x30, 0x82, 0x04, 0xbe, 0x02, 0x01, 0x00, 0x30,
  0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
  0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82,
  0x04, 0xa8, 0x30, 0x82, 0x04, 0xa4, 0x02, 0x01,
  0x00, 0x02, 0x82, 0x01, 0x01, 0x00, 0xa5, 0xd0,
  0xd7, 0x3e, 0x0e, 0x2d, 0xfb, 0x43, 0x51, 0x99,
  0xea, 0x40, 0x1e, 0x2d, 0x89, 0xe4, 0xa2, 0x3e,
  0xfc, 0x51, 0x3d, 0x0e, 0x83, 0xa7, 0xe0, 0xa5,
  0x41, 0x04, 0x1e, 0x14, 0xc5, 0xa7, 0x5c, 0x61,
  0x36, 0x44, 0xb3, 0x08, 0x05, 0x5b, 0x14, 0xde,
  0x01, 0x0c, 0x32, 0x3c, 0x9a, 0x91, 0x00, 0x50,
  0xa8, 0x1d, 0xcc, 0x9f, 0x8f, 0x35, 0xb7, 0xc2,
  0x75, 0x08, 0x32, 0x8b, 0x10, 0x3a, 0x86, 0xf9,
  0xd7, 0x78, 0xa3, 0x9d, 0x74, 0x10, 0xc6, 0x24,
  0xb1, 0x7f, 0xa5, 0xbf, 0x5f, 0xc2, 0xd7, 0x15,
  0xa3, 0x1d, 0xe0, 0x15, 0x6b, 0x1b, 0x0e, 0x38,
  0xba, 0x34, 0xbc, 0x95, 0x47, 0x94, 0x40, 0x70,
  0xac, 0x99, 0x1f, 0x0b, 0x8e, 0x56, 0x93, 0x36,
  0x2b, 0x6d, 0x04, 0xe7, 0x95, 0x1a, 0x37, 0xda,
  0x16, 0x57, 0x99, 0xee, 0x03, 0x68, 0x16, 0x31,
  0xaa, 0xc3, 0xb7, 0x92, 0x75, 0x53, 0xfc, 0xf6,
  0x20, 0x55, 0x44, 0xf8, 0xd4, 0x8d, 0x78, 0x15,
  0xc7, 0x1a, 0xb6, 0xde, 0x6c, 0xe8, 0x49, 0x5d,
  0xaf, 0xa8, 0x4e, 0x6f, 0x7c, 0xe2, 0x6a, 0x4c,
  0xd5, 0xe7, 0x8c, 0x8f, 0x0b, 0x5d, 0x3a, 0x09,
  0xd6, 0xb3, 0x44, 0xab, 0xe0, 0x35, 0x52, 0x7c,
  0x66, 0x85, 0xa4, 0x40, 0xd7, 0x20, 0xec, 0x24,
  0x05, 0x06, 0xd9, 0x84, 0x51, 0x5a, 0xd2, 0x38,
  0xd5, 0x1d, 0xea, 0x70, 0x2a, 0x21, 0xe6, 0x82,
  0xfd, 0xa4, 0x46, 0x1c, 0x4f, 0x59, 0x6e, 0x29,
  0x3d, 0xae, 0xb8, 0x8e, 0xee, 0x77, 0x1f, 0x15,
  0x33, 0xcf, 0x94, 0x1d, 0x87, 0x3c, 0x37, 0xc5,
  0x89, 0xe8, 0x7d, 0x85, 0xb3, 0xbc, 0xe8, 0x62,
  0x6a, 0x84, 0x7f, 0xfe, 0x9a, 0x85, 0x3f, 0x39,
  0xe8, 0xaa, 0x16, 0xa6, 0x8f, 0x87, 0x7f, 0xcb,
  0xc1, 0xd6, 0xf2, 0xec, 0x2b, 0xa7, 0xdd, 0x49,
  0x98, 0x7b, 0x6f, 0xdd, 0x69, 0x6d, 0x02, 0x03,
  0x01, 0x00, 0x01, 0x02, 0x82, 0x01, 0x00, 0x43,
  0x8f, 0x19, 0x83, 0xb1, 0x27, 0x4e, 0xee, 0x98,
  0xba, 0xcb, 0x54, 0xa0, 0x77, 0x11, 0x6d, 0xd4,
  0x25, 0x31, 0x8c, 0xb0, 0x01, 0xcf, 0xe6, 0x80,
  0x83, 0x14, 0x40, 0x67, 0x39, 0x33, 0x67, 0x03,
  0x1e, 0xa0, 0x8b, 0xd1, 0x1d, 0xfd, 0x80, 0xa4,
  0xb9, 0xe7, 0x57, 0x5e, 0xc8, 0x8e, 0x79, 0x71,
  0xd5, 0x6b, 0x09, 0xe9, 0x2b, 0x41, 0xa0, 0x33,
  0x64, 0xc9, 0x66, 0x33, 0xa1, 0xb1, 0x55, 0x07,
  0x55, 0x98, 0x53, 0x10, 0xe6, 0xc0, 0x39, 0x6d,
  0x61, 0xd9, 0xe8, 0x16, 0x52, 0x28, 0xe4, 0x2b,
  0xda, 0x27, 0x01, 0xaf, 0x21, 0x4a, 0xe8, 0x55,
  0x1d, 0x0b, 0xd1, 0x1c, 0xdc, 0xfd, 0xb3, 0x0b,
  0xa6, 0x5c, 0xcc, 0x6e, 0x77, 0xb8, 0xe0, 0xd1,
  0x4e, 0x0a, 0xd7, 0x7a, 0x5e, 0x18, 0xc3, 0xfb,
  0xe9, 0xa1, 0x9c, 0xc3, 0x9c, 0xd4, 0x4a, 0x7e,
  0x70, 0x72, 0x11, 0x18, 0x24, 0x56, 0x24, 0xdf,
  0xf8, 0xba, 0xac, 0x5b, 0x54, 0xd3, 0xc4, 0x65,
  0x69, 0xc8, 0x79, 0x94, 0x16, 0x88, 0x9a, 0x68,
  0x1c, 0xbc, 0xd4, 0xca, 0xec, 0x5e, 0x07, 0x4a,
  0xc9, 0x54, 0x7a, 0x4b, 0xdb, 0x19, 0x88, 0xf6,
  0xbe, 0x50, 0x9d, 0x9e, 0x9d, 0x88, 0x5b, 0x4a,
  0x23, 0x86, 0x2b, 0xa9, 0xa6, 0x6c, 0x70, 0x7d,
  0xe1, 0x11, 0xba, 0xbf, 0x03, 0x2e, 0xf1, 0x46,
  0x7e, 0x1b, 0xed, 0x06, 0x11, 0x57, 0xad, 0x4a,
  0xcb, 0xe5, 0xb1, 0x11, 0x05, 0x0a, 0x30, 0xb1,
  0x73, 0x79, 0xcd, 0x7a, 0x04, 0xcc, 0x70, 0xe9,
  0x95, 0xe4, 0x27, 0xc2, 0xd5, 0x2d, 0x92, 0x44,
  0xdf, 0xb4, 0x94, 0xa8, 0x73, 0xa1, 0x4a, 0xc3,
  0xcc, 0xc4, 0x0e, 0x8d, 0xa1, 0x6a, 0xc2, 0xd8,
  0x03, 0x7f, 0xfa, 0xa7, 0x76, 0x0d, 0xad, 0x87,
  0x88, 0xa0, 0x77, 0xaf, 0x3b, 0x23, 0xd1, 0x66,
  0x0b, 0x31, 0x2b, 0xaf, 0xef, 0xd5, 0x41, 0x02,
  0x81, 0x81, 0x00, 0xdb, 0xc1, 0xe7, 0xdd, 0xba,
  0x3c, 0x1f, 0x9c, 0x64, 0xca, 0xa0, 0x63, 0xdb,
  0xd2, 0x47, 0x5c, 0x6e, 0x8a, 0xa3, 0x16, 0xd5,
  0xda, 0xc2, 0x25, 0x64, 0x0a, 0x02, 0xbc, 0x7d,
  0x7f, 0x50, 0xab, 0xe0, 0x66, 0x03, 0x53, 0x7d,
  0x77, 0x6d, 0x6c, 0x61, 0x58, 0x09, 0x73, 0xcd,
  0x18, 0xe9, 0x53, 0x0b, 0x5c, 0xa2, 0x71, 0x14,
  0x02, 0xfd, 0x55, 0xda, 0xe9, 0x77, 0x24, 0x7c,
  0x2a, 0x4e, 0xb9, 0xd9, 0x5d, 0x58, 0xf6, 0x26,
  0xd0, 0xd8, 0x3d, 0xcf, 0x8c, 0x89, 0x65, 0x6c,
  0x35, 0x19, 0xb6, 0x63, 0xff, 0xa0, 0x71, 0x49,
  0xcd, 0x6d, 0x5b, 0x3d, 0x8f, 0xea, 0x6f, 0xa9,
  0xba, 0x43, 0xe5, 0xdd, 0x39, 0x3a, 0x78, 0x8f,
  0x07, 0xb8, 0xab, 0x58, 0x07, 0xb7, 0xd2, 0xf8,
  0x07, 0x02, 0x9b, 0x79, 0x26, 0x32, 0x22, 0x38,
  0x91, 0x01, 0x90, 0x81, 0x29, 0x94, 0xad, 0x77,
  0xeb, 0x86, 0xb9, 0x02, 0x81, 0x81, 0x00, 0xc1,
  0x29, 0x88, 0xbd, 0x96, 0x31, 0x33, 0x7b, 0x77,
  0x5d, 0x32, 0x12, 0x5e, 0xdf, 0x28, 0x0c, 0x96,
  0x0d, 0xa8, 0x22, 0xdf, 0xd3, 0x35, 0xd7, 0xb0,
  0x41, 0xcb, 0xe7, 0x94, 0x8a, 0xa4, 0xed, 0xd2,
  0xfb, 0xd2, 0xf3, 0xf2, 0x95, 0xff, 0xd8, 0x33,
  0x3f, 0x8c, 0xd7, 0x65, 0xe4, 0x0c, 0xcc, 0xfe,
  0x32, 0x66, 0xfa, 0x50, 0xe2, 0xcf, 0xf0, 0xbe,
  0x05, 0xb1, 0xbc, 0xbe, 0x44, 0x09, 0xb4, 0xfe,
  0x95, 0x06, 0x18, 0xd7, 0x59, 0xc6, 0xef, 0x2d,
  0x22, 0xa0, 0x73, 0x5e, 0x77, 0xdf, 0x8d, 0x09,
  0x2c, 0xb8, 0xcc, 0xeb, 0x10, 0x4d, 0xa7, 0xd0,
  0x4b, 0x46, 0xba, 0x7d, 0x8b, 0x6a, 0x55, 0x47,
  0x55, 0xd3, 0xd7, 0xb1, 0x88, 0xfd, 0x27, 0x3e,
  0xf9, 0x5b, 0x7b, 0xae, 0x6d, 0x08, 0x9f, 0x0c,
  0x2a, 0xe1, 0xdd, 0xb9, 0xe3, 0x55, 0x13, 0x55,
  0xa3, 0x6d, 0x06, 0xbb, 0xe0, 0x1e, 0x55, 0x02,
  0x81, 0x80, 0x61, 0x73, 0x3d, 0x64, 0xff, 0xdf,
  0x05, 0x8d, 0x8e, 0xcc, 0xa4, 0x0f, 0x64, 0x3d,
  0x7d, 0x53, 0xa9, 0xd9, 0x64, 0xb5, 0x0d, 0xa4,
  0x72, 0x8f, 0xae, 0x2b, 0x1a, 0x47, 0x87, 0xc7,
  0x5b, 0x78, 0xbc, 0x8b, 0xc0, 0x51, 0xd7, 0xc3,
  0x8c, 0x0c, 0x91, 0xa6, 0x3e, 0x9a, 0xd1, 0x8a,
  0x88, 0x7d, 0x40, 0xfe, 0x95, 0x32, 0x5b, 0xd3,
  0x6f, 0x90, 0x11, 0x01, 0x92, 0xc9, 0xe5, 0x1d,
  0xc5, 0xc7, 0x78, 0x72, 0x82, 0xae, 0xb5, 0x4b,
  0xcb, 0x78, 0xad, 0x7e, 0xfe, 0xb6, 0xb1, 0x23,
  0x63, 0x01, 0x94, 0x9a, 0x99, 0x05, 0x63, 0xda,
  0xea, 0xf1, 0x98, 0xfd, 0x26, 0xd2, 0xd9, 0x8b,
  0x35, 0xec, 0xcb, 0x0b, 0x43, 0xb8, 0x8e, 0x84,
  0xb8, 0x09, 0x93, 0x81, 0xe8, 0xac, 0x6f, 0x3c,
  0x7c, 0x95, 0x81, 0x45, 0xc4, 0xd9, 0x94, 0x08,
  0x09, 0x8f, 0x91, 0x17, 0x65, 0x4c, 0xff, 0x6e,
  0xbc, 0x51, 0x02, 0x81, 0x81, 0x00, 0xc1, 0x0d,
  0x9d, 0xd8, 0xbd, 0xaf, 0x56, 0xe0, 0xe3, 0x1f,
  0x85, 0xd7, 0xce, 0x72, 0x02, 0x38, 0xf2, 0x0f,
  0x9c, 0x27, 0x9e, 0xc4, 0x1d, 0x60, 0x00, 0x8d,
  0x02, 0x19, 0xe5, 0xdf, 0xdb, 0x8e, 0xc5, 0xfb,
  0x61, 0x8e, 0xe6, 0xb8, 0xfc, 0x07, 0x3c, 0xd1,
  0x1b, 0x16, 0x7c, 0x83, 0x3c, 0x37, 0xf5, 0x26,
  0xb2, 0xbd, 0x22, 0xf2, 0x4d, 0x19, 0x33, 0x11,
  0xc5, 0xdd, 0xf9, 0xdb, 0x4e, 0x48, 0x52, 0xd8,
  0xe6, 0x4b, 0x15, 0x90, 0x68, 0xbe, 0xca, 0xc1,
  0x7c, 0xd3, 0x51, 0x6b, 0x45, 0x46, 0x54, 0x11,
  0x1a, 0x71, 0xd3, 0xcd, 0x6b, 0x8f, 0x79, 0x22,
  0x83, 0x02, 0x08, 0x4f, 0xba, 0x6a, 0x98, 0xed,
  0x32, 0xd8, 0xb4, 0x5b, 0x51, 0x88, 0x53, 0xec,
  0x2c, 0x7e, 0xa4, 0x89, 0xdc, 0xbf, 0xf9, 0x0d,
  0x32, 0xc8, 0xc3, 0xec, 0x6d, 0x2e, 0xf1, 0xbc,
  0x70, 0x4e, 0xf6, 0x9e, 0xbc, 0x31, 0x02, 0x81,
  0x81, 0x00, 0xd3, 0x35, 0x1b, 0x19, 0x75, 0x3f,
  0x61, 0xf2, 0x55, 0x03, 0xce, 0x25, 0xa9, 0xdf,
  0x0c, 0x0a, 0x3b, 0x47, 0x42, 0xdc, 0x38, 0x4b,
  0x13, 0x4d, 0x1f, 0x86, 0x58, 0x4f, 0xd8, 0xee,
  0xfa, 0x76, 0x15, 0xfb, 0x6e, 0x55, 0x31, 0xf2,
  0xd2, 0x62, 0x32, 0xa5, 0xc4, 0x23, 0x5e, 0x08,
  0xa9, 0x83, 0x07, 0xac, 0x8c, 0xa3, 0x7e, 0x18,
  0xc0, 0x1c, 0x57, 0x63, 0x8d, 0x05, 0x17, 0x47,
  0x1b, 0xd3, 0x74, 0x73, 0x20, 0x04, 0xfb, 0xc8,
  0x1a, 0x43, 0x04, 0x36, 0xc8, 0x19, 0xbe, 0xdc,
  0xa6, 0xe5, 0x0f, 0x25, 0x62, 0x24, 0x96, 0x92,
  0xb6, 0xb3, 0x97, 0xad, 0x57, 0x9a, 0x90, 0x37,
  0x4e, 0x31, 0x44, 0x74, 0xfa, 0x7c, 0xb4, 0xea,
  0xfc, 0x15, 0xa7, 0xb0, 0x51, 0xcc, 0xee, 0x1e,
  0xed, 0x5b, 0x98, 0x18, 0x0e, 0x65, 0xb6, 0x4b,
  0x69, 0x0b, 0x21, 0xdc, 0x86, 0x17, 0x6e, 0xc8,
  0xee, 0x24 };

// A second 2048 bit RSA Public key
// Used to verify the functions that manipulate RSA keys.
static const uint8_t kTestRSAPublicKey3_2048[] = {
  0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
  0x00, 0xa5, 0xd0, 0xd7, 0x3e, 0x0e, 0x2d, 0xfb,
  0x43, 0x51, 0x99, 0xea, 0x40, 0x1e, 0x2d, 0x89,
  0xe4, 0xa2, 0x3e, 0xfc, 0x51, 0x3d, 0x0e, 0x83,
  0xa7, 0xe0, 0xa5, 0x41, 0x04, 0x1e, 0x14, 0xc5,
  0xa7, 0x5c, 0x61, 0x36, 0x44, 0xb3, 0x08, 0x05,
  0x5b, 0x14, 0xde, 0x01, 0x0c, 0x32, 0x3c, 0x9a,
  0x91, 0x00, 0x50, 0xa8, 0x1d, 0xcc, 0x9f, 0x8f,
  0x35, 0xb7, 0xc2, 0x75, 0x08, 0x32, 0x8b, 0x10,
  0x3a, 0x86, 0xf9, 0xd7, 0x78, 0xa3, 0x9d, 0x74,
  0x10, 0xc6, 0x24, 0xb1, 0x7f, 0xa5, 0xbf, 0x5f,
  0xc2, 0xd7, 0x15, 0xa3, 0x1d, 0xe0, 0x15, 0x6b,
  0x1b, 0x0e, 0x38, 0xba, 0x34, 0xbc, 0x95, 0x47,
  0x94, 0x40, 0x70, 0xac, 0x99, 0x1f, 0x0b, 0x8e,
  0x56, 0x93, 0x36, 0x2b, 0x6d, 0x04, 0xe7, 0x95,
  0x1a, 0x37, 0xda, 0x16, 0x57, 0x99, 0xee, 0x03,
  0x68, 0x16, 0x31, 0xaa, 0xc3, 0xb7, 0x92, 0x75,
  0x53, 0xfc, 0xf6, 0x20, 0x55, 0x44, 0xf8, 0xd4,
  0x8d, 0x78, 0x15, 0xc7, 0x1a, 0xb6, 0xde, 0x6c,
  0xe8, 0x49, 0x5d, 0xaf, 0xa8, 0x4e, 0x6f, 0x7c,
  0xe2, 0x6a, 0x4c, 0xd5, 0xe7, 0x8c, 0x8f, 0x0b,
  0x5d, 0x3a, 0x09, 0xd6, 0xb3, 0x44, 0xab, 0xe0,
  0x35, 0x52, 0x7c, 0x66, 0x85, 0xa4, 0x40, 0xd7,
  0x20, 0xec, 0x24, 0x05, 0x06, 0xd9, 0x84, 0x51,
  0x5a, 0xd2, 0x38, 0xd5, 0x1d, 0xea, 0x70, 0x2a,
  0x21, 0xe6, 0x82, 0xfd, 0xa4, 0x46, 0x1c, 0x4f,
  0x59, 0x6e, 0x29, 0x3d, 0xae, 0xb8, 0x8e, 0xee,
  0x77, 0x1f, 0x15, 0x33, 0xcf, 0x94, 0x1d, 0x87,
  0x3c, 0x37, 0xc5, 0x89, 0xe8, 0x7d, 0x85, 0xb3,
  0xbc, 0xe8, 0x62, 0x6a, 0x84, 0x7f, 0xfe, 0x9a,
  0x85, 0x3f, 0x39, 0xe8, 0xaa, 0x16, 0xa6, 0x8f,
  0x87, 0x7f, 0xcb, 0xc1, 0xd6, 0xf2, 0xec, 0x2b,
  0xa7, 0xdd, 0x49, 0x98, 0x7b, 0x6f, 0xdd, 0x69,
  0x6d, 0x02, 0x03, 0x01, 0x00, 0x01 };

static void dump_openssl_error() {
  while (unsigned long err = ERR_get_error()) {
    char buffer[120];
    cout << "openssl error -- " << ERR_error_string(err, buffer) << "\n";
  }
}

// We don't expect exact timing.
#define EXPECT_ALMOST(A, B)           \
  EXPECT_GE(A + kSpeedMultiplier, B); \
  EXPECT_LE(A - kSpeedMultiplier, B);

class Session {
 public:
  Session()
      : open_(false),
        session_id_(0),
        mac_key_server_(wvcdm::MAC_KEY_SIZE),
        mac_key_client_(wvcdm::MAC_KEY_SIZE),
        enc_key_(wvcdm::KEY_SIZE),
        public_rsa_(0) {}

  ~Session() {
    if (open_) close();
    if (public_rsa_) RSA_free(public_rsa_);
  }

  bool isOpen() { return open_; }
  OEMCryptoResult getStatus() { return session_status_; }
  uint32_t get_nonce() { return nonce_; }

  uint32_t session_id() { return (uint32_t)session_id_; }
  void set_session_id(uint32_t newsession) {
    session_id_ = (OEMCrypto_SESSION)newsession;
  }

  void open() {
    EXPECT_TRUE(!open_);
    session_status_ = OEMCrypto_OpenSession(&session_id_);
    if (OEMCrypto_SUCCESS == session_status_) {
      open_ = true;
    }
  }

  void close() {
    session_status_ = OEMCrypto_CloseSession(session_id_);
    if (OEMCrypto_SUCCESS == session_status_) {
      open_ = false;
    }
  }

  void GenerateNonce(uint32_t* nonce, int* error_counter = NULL) {
    if (OEMCrypto_SUCCESS == OEMCrypto_GenerateNonce(session_id(), nonce)) {
      return;
    }
    if (error_counter) {
      (*error_counter)++;
    } else {
      sleep(1);  // wait a second, then try again.
      ASSERT_EQ(OEMCrypto_SUCCESS,
                OEMCrypto_GenerateNonce(session_id(), nonce));
    }
  }

  void FillDefaultContext(vector<uint8_t>* mac_context,
                          vector<uint8_t>* enc_context) {
    /* Context strings
     * These context strings are normally created by the CDM layer
     * from a license request message.
     * They are used to test MAC and ENC key generation.
     */
    *mac_context = wvcdm::a2b_hex(
        "41555448454e5449434154494f4e000a4c08001248000000020000101907d9ff"
        "de13aa95c122678053362136bdf8408f8276e4c2d87ec52b61aa1b9f646e5873"
        "4930acebe899b3e464189a14a87202fb02574e70640bd22ef44b2d7e3912250a"
        "230a14080112100915007caa9b5931b76a3a85f046523e10011a093938373635"
        "34333231180120002a0c31383836373837343035000000000200");
    *enc_context = wvcdm::a2b_hex(
        "454e4352595054494f4e000a4c08001248000000020000101907d9ffde13aa95"
        "c122678053362136bdf8408f8276e4c2d87ec52b61aa1b9f646e58734930aceb"
        "e899b3e464189a14a87202fb02574e70640bd22ef44b2d7e3912250a230a1408"
        "0112100915007caa9b5931b76a3a85f046523e10011a09393837363534333231"
        "180120002a0c31383836373837343035000000000080");
  }

  void GenerateDerivedKeys() {
    GenerateNonce(&nonce_);
    vector<uint8_t> mac_context;
    vector<uint8_t> enc_context;
    FillDefaultContext(&mac_context, &enc_context);
    ASSERT_EQ(OEMCrypto_SUCCESS,
              OEMCrypto_GenerateDerivedKeys(session_id(), &mac_context[0],
                                            mac_context.size(), &enc_context[0],
                                            enc_context.size()));

    // Expected MAC and ENC keys generated from context strings
    // with test keybox "installed".
    mac_key_server_ = wvcdm::a2b_hex(
        "3CFD60254786AF350B353B4FBB700AB382558400356866BA16C256BCD8C502BF");
    mac_key_client_ = wvcdm::a2b_hex(
        "A9DE7B3E4E199ED8D1FBC29CD6B4C772CC4538C8B0D3E208B3E76F2EC0FD6F47");
    enc_key_ = wvcdm::a2b_hex("D0BFC35DA9E33436E81C4229E78CB9F4");
  }

  void LoadTestKeys(const std::string& pst = "", bool new_mac_keys = true) {
    uint8_t* pst_ptr = NULL;
    if (pst.length() > 0) {
      pst_ptr = encrypted_license_.pst;
    }
    if (new_mac_keys) {
      ASSERT_EQ(OEMCrypto_SUCCESS,
                OEMCrypto_LoadKeys(
                    session_id(), message_ptr(), sizeof(MessageData),
                    &signature_[0], signature_.size(),
                    encrypted_license_.mac_key_iv, encrypted_license_.mac_keys,
                    kNumKeys, key_array_, pst_ptr, pst.length()));
      // Update new generated keys.
      memcpy(&mac_key_server_[0], license_.mac_keys, wvcdm::MAC_KEY_SIZE);
      memcpy(&mac_key_client_[0], license_.mac_keys + wvcdm::MAC_KEY_SIZE,
             wvcdm::MAC_KEY_SIZE);
    } else {
      ASSERT_EQ(
          OEMCrypto_SUCCESS,
          OEMCrypto_LoadKeys(session_id(), message_ptr(), sizeof(MessageData),
                             &signature_[0], signature_.size(), NULL, NULL,
                             kNumKeys, key_array_, pst_ptr, pst.length()));
    }
  }

  void RefreshTestKeys(const size_t key_count, uint32_t control_bits,
                       uint32_t nonce, bool expect_good) {
    // Note: we store the message in encrypted_license_, but the refresh key
    // message is not actually encrypted.  It is, however, signed.
    FillRefreshMessage(key_count, control_bits, nonce);
    ServerSignMessage(encrypted_license_, &signature_);
    OEMCrypto_KeyRefreshObject key_array[key_count];
    FillRefreshArray(key_array, key_count);
    OEMCryptoResult sts = OEMCrypto_RefreshKeys(
        session_id(), message_ptr(), sizeof(MessageData), &signature_[0],
        signature_.size(), key_count, key_array);
    if (expect_good) {
      ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    } else {
      ASSERT_NE(OEMCrypto_SUCCESS, sts);
    }

    TestDecryptCTR();
    sleep(kShortSleep);  //  Should still be valid key.
    TestDecryptCTR(false);
    sleep(kShortSleep + kLongSleep);  // Should be after first expiration.
    if (expect_good) {
      TestDecryptCTR(false, OEMCrypto_SUCCESS);
    } else {
      TestDecryptCTR(false, OEMCrypto_ERROR_UNKNOWN_FAILURE);
    }
  }

  void FillSimpleMessage(uint32_t duration, uint32_t control, uint32_t nonce,
                         const std::string& pst = "") {
    OEMCrypto_GetRandom(license_.mac_key_iv, sizeof(license_.mac_key_iv));
    OEMCrypto_GetRandom(license_.mac_keys, sizeof(license_.mac_keys));
    for (unsigned int i = 0; i < kNumKeys; i++) {
      memset(license_.keys[i].key_id, i, kTestKeyIdLength);
      OEMCrypto_GetRandom(license_.keys[i].key_data,
                          sizeof(license_.keys[i].key_data));
      license_.keys[i].key_data_length = wvcdm::KEY_SIZE;
      OEMCrypto_GetRandom(license_.keys[i].key_iv,
                          sizeof(license_.keys[i].key_iv));
      OEMCrypto_GetRandom(license_.keys[i].control_iv,
                          sizeof(license_.keys[i].control_iv));
      if (control & (wvoec_mock::kControlHDCPVersionMask |
                     wvoec_mock::kControlReplayMask)) {
        memcpy(license_.keys[i].control.verification, "kc09", 4);
      } else {
        memcpy(license_.keys[i].control.verification, "kctl", 4);
      }
      license_.keys[i].control.duration = htonl(duration);
      license_.keys[i].control.nonce = htonl(nonce);
      license_.keys[i].control.control_bits = htonl(control);
    }
    memcpy(license_.pst, pst.c_str(), min(sizeof(license_.pst), pst.length()));

    // The first key for the canned decryption content.
    vector<uint8_t> key = wvcdm::a2b_hex("39AD33E5719656069F9EDE9EBBA7A77D");
    memcpy(license_.keys[0].key_data, &key[0], key.size());
  }

  void FillRefreshMessage(size_t key_count, uint32_t control_bits,
                          uint32_t nonce) {
    for (unsigned int i = 0; i < kNumKeys; i++) {
      memset(encrypted_license_.keys[i].key_id, i, kTestKeyIdLength);
      memcpy(encrypted_license_.keys[i].control.verification, "kctl", 4);
      encrypted_license_.keys[i].control.duration = htonl(kLongDuration);
      encrypted_license_.keys[i].control.nonce = htonl(nonce);
      encrypted_license_.keys[i].control.control_bits = htonl(control_bits);
    }
  }

  void EncryptAndSign() {
    encrypted_license_ = license_;

    uint8_t iv_buffer[16];
    memcpy(iv_buffer, &license_.mac_key_iv[0], wvcdm::KEY_IV_SIZE);
    AES_KEY aes_key;
    AES_set_encrypt_key(&enc_key_[0], 128, &aes_key);
    AES_cbc_encrypt(&license_.mac_keys[0], &encrypted_license_.mac_keys[0],
                    2 * wvcdm::MAC_KEY_SIZE, &aes_key, iv_buffer, AES_ENCRYPT);

    for (unsigned int i = 0; i < kNumKeys; i++) {
      memcpy(iv_buffer, &license_.keys[i].control_iv[0], wvcdm::KEY_IV_SIZE);
      AES_set_encrypt_key(&license_.keys[i].key_data[0], 128, &aes_key);
      AES_cbc_encrypt(
          reinterpret_cast<const uint8_t*>(&license_.keys[i].control),
          reinterpret_cast<uint8_t*>(&encrypted_license_.keys[i].control),
          wvcdm::KEY_SIZE, &aes_key, iv_buffer, AES_ENCRYPT);

      memcpy(iv_buffer, &license_.keys[i].key_iv[0], wvcdm::KEY_IV_SIZE);
      AES_set_encrypt_key(&enc_key_[0], 128, &aes_key);
      AES_cbc_encrypt(&license_.keys[i].key_data[0],
                      &encrypted_license_.keys[i].key_data[0],
                      license_.keys[i].key_data_length, &aes_key, iv_buffer,
                      AES_ENCRYPT);
    }
    memcpy(encrypted_license_.pst, license_.pst, sizeof(license_.pst));
    ServerSignMessage(encrypted_license_, &signature_);
    FillKeyArray(encrypted_license_, key_array_);
  }

  void EncryptMessage(RSAPrivateKeyMessage* data,
                      RSAPrivateKeyMessage* encrypted) {
    *encrypted = *data;
    size_t padding = wvcdm::KEY_SIZE - (data->rsa_key_length % wvcdm::KEY_SIZE);
    memset(data->rsa_key + data->rsa_key_length, static_cast<uint8_t>(padding),
           padding);
    encrypted->rsa_key_length = data->rsa_key_length + padding;
    uint8_t iv_buffer[16];
    memcpy(iv_buffer, &data->rsa_key_iv[0], wvcdm::KEY_IV_SIZE);
    AES_KEY aes_key;
    AES_set_encrypt_key(&enc_key_[0], 128, &aes_key);
    AES_cbc_encrypt(&data->rsa_key[0], &encrypted->rsa_key[0],
                    encrypted->rsa_key_length, &aes_key, iv_buffer,
                    AES_ENCRYPT);
  }

  template <typename T>
  void ServerSignMessage(const T& data, std::vector<uint8_t>* signature) {
    signature->resize(SHA256_DIGEST_LENGTH);
    unsigned int md_len = SHA256_DIGEST_LENGTH;
    HMAC(EVP_sha256(), &mac_key_server_[0], mac_key_server_.size(),
         reinterpret_cast<const uint8_t*>(&data), sizeof(data),
         &(signature->front()), &md_len);
  }

  void ClientSignMessage(const vector<uint8_t>& data,
                         std::vector<uint8_t>* signature) {
    signature->resize(SHA256_DIGEST_LENGTH);
    unsigned int md_len = SHA256_DIGEST_LENGTH;
    HMAC(EVP_sha256(), &mac_key_client_[0], mac_key_client_.size(),
         &(data.front()), data.size(), &(signature->front()), &md_len);
  }

  void FillKeyArray(const MessageData& data, OEMCrypto_KeyObject* key_array) {
    for (unsigned int i = 0; i < kNumKeys; i++) {
      key_array[i].key_id = data.keys[i].key_id;
      key_array[i].key_id_length = kTestKeyIdLength;
      key_array[i].key_data_iv = data.keys[i].key_iv;
      key_array[i].key_data = data.keys[i].key_data;
      key_array[i].key_data_length = data.keys[i].key_data_length;
      key_array[i].key_control_iv = data.keys[i].control_iv;
      key_array[i].key_control =
          reinterpret_cast<const uint8_t*>(&data.keys[i].control);
    }
  }

  void FillRefreshArray(OEMCrypto_KeyRefreshObject* key_array,
                        size_t key_count) {
    for (size_t i = 0; i < key_count; i++) {
      if (key_count > 1) {
        key_array[i].key_id = encrypted_license_.keys[i].key_id;
        key_array[i].key_id_length = kTestKeyIdLength;
      } else {
        key_array[i].key_id = NULL;
        key_array[i].key_id_length = 0;
      }
      key_array[i].key_control_iv = NULL;
      key_array[i].key_control =
          reinterpret_cast<const uint8_t*>(&encrypted_license_.keys[i].control);
    }
  }

  void TestDecryptCTR(bool select_key_first = true,
                      OEMCryptoResult expected_result = OEMCrypto_SUCCESS) {
    OEMCryptoResult sts;
    if (select_key_first) {
      // Select the key (from FillSimpleMessage)
      vector<uint8_t> keyId = wvcdm::a2b_hex("000000000000000000000000");
      sts = OEMCrypto_SelectKey(session_id(), &keyId[0], keyId.size());
      ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    }

    // Set up our expected input and output
    // This is dummy encrypted data.
    vector<uint8_t> encryptedData = wvcdm::a2b_hex(
        "ec261c115f9d5cda1d5cc7d33c4e37362d1397c89efdd1da5f0065c4848b0462"
        "337ba14693735203c9b4184e362439c0cea5e5d1a628425eddf8a6bf9ba901ca"
        "46f5a9fd973cffbbe3c276af9919e2e8f6f3f420538b7a0d6dc41487874d96b8"
        "efaedb45a689b91beb8c20d36140ad467d9d620b19a5fc6f223b57e0e6a7f913"
        "00fd899e5e1b89963e83067ca0912aa5b79df683e2530b55a9645be341bc5f07"
        "cffc724790af635c959e2644e51ba7f23bae710eb55a1f2f4e060c3c1dd1387c"
        "74415dc880492dd1d5b9ecf3f01de48a44baeb4d3ea5cc4f8d561d0865afcabb"
        "fc14a9ab9647e6e31adabb72d792f0c9ba99dc3e9205657d28fc7771d64e6d4b");
    vector<uint8_t> encryptionIv =
        wvcdm::a2b_hex("719dbcb253b2ec702bb8c1b1bc2f3bc6");
    // This is the expected decrypted data.
    vector<uint8_t> unencryptedData = wvcdm::a2b_hex(
        "19ef4361e16e6825b336e2012ad8ffc9ce176ab2256e1b98aa15b7877bd8c626"
        "fa40b2e88373457cbcf4f1b4b9793434a8ac03a708f85974cff01bddcbdd7a8e"
        "e33fd160c1d5573bfd8104efd23237edcf28205c3673920553f8dd5e916604b0"
        "1082345181dceeae5ea39d829c7f49e1850c460645de33c288723b7ae3d91a17"
        "a3f04195cd1945ba7b0f37fef7e82368be30f04365d877766f6d56f67d22a244"
        "ef2596d3053f657c1b5d90b64e11797edf1c198a23a7bfc20e4d44c74ae41280"
        "a8317f443255f4020eda850ff0954e308f53a634cbce799ae58911bc59ccd6a5"
        "de2ac53ee0fa7ea15fc692cc892acc0090865dc57becacddf362a092dfd3040b");

    // Describe the output
    uint8_t outputBuffer[256];
    OEMCrypto_DestBufferDesc destBuffer;
    destBuffer.type = OEMCrypto_BufferType_Clear;
    destBuffer.buffer.clear.address = outputBuffer;
    destBuffer.buffer.clear.max_length = sizeof(outputBuffer);
    // Decrypt the data
    sts = OEMCrypto_DecryptCTR(
        session_id(), &encryptedData[0], encryptedData.size(), true,
        &encryptionIv[0], 0, &destBuffer,
        OEMCrypto_FirstSubsample | OEMCrypto_LastSubsample);
    // We only have a few errors that we test are reported.
    if (expected_result == OEMCrypto_SUCCESS) {  // No error.
      ASSERT_EQ(OEMCrypto_SUCCESS, sts);
      ASSERT_EQ(
          0, memcmp(&unencryptedData[0], outputBuffer, unencryptedData.size()));
    } else if (expected_result == OEMCrypto_ERROR_KEY_EXPIRED) {
      // Report stale keys.
      ASSERT_EQ(OEMCrypto_ERROR_KEY_EXPIRED, sts);
      ASSERT_NE(
          0, memcmp(&unencryptedData[0], outputBuffer, unencryptedData.size()));
    } else if (expected_result == OEMCrypto_ERROR_INSUFFICIENT_HDCP) {
      // Report HDCP errors.
      ASSERT_EQ(OEMCrypto_ERROR_INSUFFICIENT_HDCP, sts);
      ASSERT_NE(
          0, memcmp(&unencryptedData[0], outputBuffer, unencryptedData.size()));
    } else {
      // OEM's can fine tune other error codes for debugging.
      ASSERT_NE(OEMCrypto_SUCCESS, sts);
      ASSERT_NE(
          0, memcmp(&unencryptedData[0], outputBuffer, unencryptedData.size()));
    }
  }

  void MakeRSACertificate(struct RSAPrivateKeyMessage* encrypted,
                          std::vector<uint8_t>* signature,
                          uint32_t allowed_schemes,
                          const uint8_t* rsa_key = NULL,
                          size_t rsa_key_length = 0) {
    if (rsa_key == NULL) {
      rsa_key = kTestRSAPKCS8PrivateKeyInfo2_2048;
      rsa_key_length = sizeof(kTestRSAPKCS8PrivateKeyInfo2_2048);
    }

    // Dummy context for testing signature generation.
    vector<uint8_t> context = wvcdm::a2b_hex(
        "0a4c08001248000000020000101907d9ffde13aa95c122678053362136bdf840"
        "8f8276e4c2d87ec52b61aa1b9f646e58734930acebe899b3e464189a14a87202"
        "fb02574e70640bd22ef44b2d7e3912250a230a14080112100915007caa9b5931"
        "b76a3a85f046523e10011a09393837363534333231180120002a0c3138383637"
        "38373430350000");

    OEMCryptoResult sts;

    // Generate signature
    size_t gen_signature_length = 0;
    sts = OEMCrypto_GenerateSignature(session_id(), &context[0], context.size(),
                                      NULL, &gen_signature_length);
    ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
    ASSERT_EQ(static_cast<size_t>(32), gen_signature_length);
    static const uint32_t SignatureBufferMaxLength = 256;
    uint8_t gen_signature[SignatureBufferMaxLength];
    sts = OEMCrypto_GenerateSignature(session_id(), &context[0], context.size(),
                                      gen_signature, &gen_signature_length);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    std::vector<uint8_t> expected_signature;
    ClientSignMessage(context, &expected_signature);
    ASSERT_EQ(0, memcmp(&expected_signature[0], gen_signature,
                        expected_signature.size()));

    // Rewrap Canned Response

    // In the real world, the signature above would just have been used to
    // contact the certificate provisioning server to get this response.

    struct RSAPrivateKeyMessage message;
    if (allowed_schemes != kSign_RSASSA_PSS) {
      uint32_t algorithm_n = htonl(allowed_schemes);
      memcpy(message.rsa_key, "SIGN", 4);
      memcpy(message.rsa_key + 4, &algorithm_n, 4);
      memcpy(message.rsa_key + 8, rsa_key, rsa_key_length);
      message.rsa_key_length = 8 + rsa_key_length;
    } else {
      memcpy(message.rsa_key, rsa_key, rsa_key_length);
      message.rsa_key_length = rsa_key_length;
    }
    OEMCrypto_GetRandom(message.rsa_key_iv, wvcdm::KEY_IV_SIZE);
    message.nonce = nonce_;

    EncryptMessage(&message, encrypted);
    ServerSignMessage(*encrypted, signature);
  }

  void RewrapRSAKey(const struct RSAPrivateKeyMessage& encrypted,
                    const std::vector<uint8_t>& signature,
                    vector<uint8_t>* wrapped_key, bool force) {
    size_t wrapped_key_length = 0;
    const uint8_t* message_ptr = reinterpret_cast<const uint8_t*>(&encrypted);

    ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER,
              OEMCrypto_RewrapDeviceRSAKey(
                  session_id(), message_ptr, sizeof(encrypted), &signature[0],
                  signature.size(), &encrypted.nonce, encrypted.rsa_key,
                  encrypted.rsa_key_length, encrypted.rsa_key_iv, NULL,
                  &wrapped_key_length));
    wrapped_key->clear();
    wrapped_key->resize(wrapped_key_length);
    OEMCryptoResult sts = OEMCrypto_RewrapDeviceRSAKey(
        session_id(), message_ptr, sizeof(encrypted), &signature[0],
        signature.size(), &encrypted.nonce, encrypted.rsa_key,
        encrypted.rsa_key_length, encrypted.rsa_key_iv, &(wrapped_key->front()),
        &wrapped_key_length);
    if (force) {
      ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    }
    if (OEMCrypto_SUCCESS != sts) {
      wrapped_key->clear();
    }
  }

  void PreparePublicKey(const uint8_t* rsa_key = NULL,
                        size_t rsa_key_length = 0) {
    if (rsa_key == NULL) {
      rsa_key = kTestRSAPKCS8PrivateKeyInfo2_2048;
      rsa_key_length = sizeof(kTestRSAPKCS8PrivateKeyInfo2_2048);
    }
    uint8_t* p = const_cast<uint8_t*>(rsa_key);
    BIO* bio = BIO_new_mem_buf(p, rsa_key_length);
    ASSERT_TRUE(NULL != bio);
    PKCS8_PRIV_KEY_INFO* pkcs8_pki = d2i_PKCS8_PRIV_KEY_INFO_bio(bio, NULL);
    ASSERT_TRUE(NULL != pkcs8_pki);
    EVP_PKEY* evp = NULL;
    evp = EVP_PKCS82PKEY(pkcs8_pki);
    ASSERT_TRUE(NULL != evp);
    if (public_rsa_) RSA_free(public_rsa_);
    public_rsa_ = EVP_PKEY_get1_RSA(evp);
    EVP_PKEY_free(evp);
    PKCS8_PRIV_KEY_INFO_free(pkcs8_pki);
    BIO_free(bio);
    if (!public_rsa_) {
      cout << "d2i_RSAPrivateKey failed. ";
      dump_openssl_error();
      ASSERT_TRUE(false);
    }
    switch (RSA_check_key(public_rsa_)) {
      case 1:  // valid.
        ASSERT_TRUE(true);
        return;
      case 0:  // not valid.
        cout << "[rsa key not valid] ";
        dump_openssl_error();
        ASSERT_TRUE(false);
      default:  // -1 == check failed.
        cout << "[error checking rsa key] ";
        dump_openssl_error();
        ASSERT_TRUE(false);
    }
  }

  void VerifyRSASignature(const uint8_t* message, size_t message_length,
                          const uint8_t* signature, size_t signature_length,
                          RSA_Padding_Scheme padding_scheme) {
    EXPECT_TRUE(NULL != public_rsa_)
        << "No public RSA key loaded in test code.\n";
    EXPECT_EQ(static_cast<size_t>(RSA_size(public_rsa_)), signature_length)
        << "Signature size is wrong. " << signature_length << ", should be "
        << RSA_size(public_rsa_) << "\n";

    if (padding_scheme == kSign_RSASSA_PSS) {
      // Hash the message using SHA1.
      uint8_t hash[SHA_DIGEST_LENGTH];
      if (!SHA1(message, message_length, hash)) {
        dump_openssl_error();
        EXPECT_TRUE(false) << "Error computing SHA1. ";
      }

      // Decrypt signature to padded digest.
      uint8_t padded_digest[signature_length];
      int status;
      status = RSA_public_decrypt(signature_length, signature, padded_digest,
                                  public_rsa_, RSA_NO_PADDING);
      if (status == -1) {
        dump_openssl_error();
        EXPECT_TRUE(false) << "VerifyRSASignature. in RSA_Public_digest ";
      }
      status = RSA_verify_PKCS1_PSS(public_rsa_, hash, EVP_sha1(),
                                    padded_digest, SHA_DIGEST_LENGTH);
      if (status != 1) {
        dump_openssl_error();
        EXPECT_TRUE(false) << "VerifyRSASignature. in RSA_verify_PKCS1_PSS ";
      }
    } else if (padding_scheme == kSign_PKCS1_Block1) {
      uint8_t padded_digest[signature_length];
      int status;
      // RSA_public_decrypt decrypts the signature, and then verifies that
      // it was padded with RSA PKCS1 padding.
      status = RSA_public_decrypt(signature_length, signature, padded_digest,
                                  public_rsa_, RSA_PKCS1_PADDING);
      EXPECT_EQ(0, memcmp(message, padded_digest, message_length));
    } else {
      EXPECT_TRUE(false) << "Padding scheme not supported.";
    }
  }

  bool GenerateRSASessionKey(vector<uint8_t>* enc_session_key) {
    if (!public_rsa_) {
      cout << "No public RSA key loaded in test code.\n";
      return false;
    }
    vector<uint8_t> session_key =
        wvcdm::a2b_hex("6fa479c731d2770b6a61a5d1420bb9d1");
    enc_session_key->assign(RSA_size(public_rsa_), 0);
    int status = RSA_public_encrypt(session_key.size(), &session_key[0],
                                    &(enc_session_key->front()), public_rsa_,
                                    RSA_PKCS1_OAEP_PADDING);
    if (status != RSA_size(public_rsa_)) {
      cout << "GenerateRSASessionKey error encrypting session key. ";
      dump_openssl_error();
      return false;
    }
    return true;
  }

  void InstallRSASessionTestKey(const vector<uint8_t>& wrapped_rsa_key) {
    ASSERT_EQ(OEMCrypto_SUCCESS,
              OEMCrypto_LoadDeviceRSAKey(session_id(), &wrapped_rsa_key[0],
                                         wrapped_rsa_key.size()));
    GenerateNonce(&nonce_);
    vector<uint8_t> enc_session_key;
    PreparePublicKey();
    ASSERT_TRUE(GenerateRSASessionKey(&enc_session_key));
    vector<uint8_t> mac_context;
    vector<uint8_t> enc_context;
    FillDefaultContext(&mac_context, &enc_context);
    ASSERT_EQ(OEMCrypto_SUCCESS,
              OEMCrypto_DeriveKeysFromSessionKey(
                  session_id(), &enc_session_key[0], enc_session_key.size(),
                  &mac_context[0], mac_context.size(), &enc_context[0],
                  enc_context.size()));

    // Expected MAC and ENC keys generated from context strings
    // with RSA certificate "installed".
    mac_key_server_ = wvcdm::a2b_hex(
        "1E451E59CB663DA1646194DD28880788ED8ED2EFF913CBD6A0D535D1D5A90381");
    mac_key_client_ = wvcdm::a2b_hex(
        "F9AAE74690909F2207B53B13307FCA096CA8C49CC6DFE3659873CB952889A74B");
    enc_key_ = wvcdm::a2b_hex("CB477D09014D72C9B8DCE76C33EA43B3");
  }

  void DisallowDeriveKeys() {
    GenerateNonce(&nonce_);
    vector<uint8_t> enc_session_key;
    PreparePublicKey();
    ASSERT_TRUE(GenerateRSASessionKey(&enc_session_key));
    vector<uint8_t> mac_context;
    vector<uint8_t> enc_context;
    FillDefaultContext(&mac_context, &enc_context);
    ASSERT_EQ(OEMCrypto_ERROR_INVALID_RSA_KEY,
              OEMCrypto_DeriveKeysFromSessionKey(
                  session_id(), &enc_session_key[0], enc_session_key.size(),
                  &mac_context[0], mac_context.size(), &enc_context[0],
                  enc_context.size()));
  }

  void GenerateReport(const std::string& pst, bool expect_success = true,
                      Session* other = 0) {
    if (other) {  // If other is specified, copy mac keys.
      mac_key_server_ = other->mac_key_server_;
      mac_key_client_ = other->mac_key_client_;
    }
    size_t length = 0;
    OEMCryptoResult sts = OEMCrypto_ReportUsage(
        session_id(), reinterpret_cast<const uint8_t*>(pst.c_str()),
        pst.length(), &pst_report_.report, &length);
    if (expect_success) {
      ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
    }
    if (sts == OEMCrypto_ERROR_SHORT_BUFFER) {
      ASSERT_GE(sizeof(PaddedPSTReport), length);
    }
    sts = OEMCrypto_ReportUsage(session_id(),
                                reinterpret_cast<const uint8_t*>(pst.c_str()),
                                pst.length(), &pst_report_.report, &length);
    if (!expect_success) {
      ASSERT_NE(OEMCrypto_SUCCESS, sts);
      return;
    }
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    uint8_t computed_signature[SHA_DIGEST_LENGTH];
    unsigned int sig_len = SHA_DIGEST_LENGTH;
    HMAC(EVP_sha1(), &mac_key_client_[0], mac_key_client_.size(),
         reinterpret_cast<uint8_t*>(&pst_report_.report) + SHA_DIGEST_LENGTH,
         length - SHA_DIGEST_LENGTH, computed_signature, &sig_len);
    EXPECT_EQ(0, memcmp(computed_signature, pst_report_.report.signature,
                        SHA_DIGEST_LENGTH));
    EXPECT_GE(kInactive, pst_report_.report.status);
    EXPECT_GE(kHardwareSecureClock, pst_report_.report.clock_security_level);
    EXPECT_EQ(pst.length(), pst_report_.report.pst_length);
    EXPECT_EQ(0, memcmp(pst.c_str(), pst_report_.report.pst, pst.length()));
  }
  OEMCrypto_PST_Report* pst_report() { return &pst_report_.report; }

  void DeleteEntry(const std::string& pst) {
    uint8_t* pst_ptr = encrypted_license_.pst;
    memcpy(pst_ptr, pst.c_str(), min(sizeof(license_.pst), pst.length()));
    ServerSignMessage(encrypted_license_, &signature_);
    ASSERT_EQ(OEMCrypto_SUCCESS,
              OEMCrypto_DeleteUsageEntry(session_id(), pst_ptr, pst.length(),
                                         message_ptr(), sizeof(MessageData),
                                         &signature_[0], signature_.size()));
  }

  MessageData& license() { return license_; }
  MessageData& encrypted_license() { return encrypted_license_; }
  const uint8_t* message_ptr() {
    return reinterpret_cast<const uint8_t*>(&encrypted_license_);
  }
  OEMCrypto_KeyObject* key_array() { return key_array_; }
  std::vector<uint8_t>& signature() { return signature_; }

 private:
  bool open_;
  OEMCrypto_SESSION session_id_;
  OEMCryptoResult session_status_;
  vector<uint8_t> mac_key_server_;
  vector<uint8_t> mac_key_client_;
  vector<uint8_t> enc_key_;
  uint32_t nonce_;
  RSA* public_rsa_;
  PaddedPSTReport pst_report_;
  MessageData license_;
  MessageData encrypted_license_;
  OEMCrypto_KeyObject key_array_[kNumKeys];
  std::vector<uint8_t> signature_;
};

class OEMCryptoClientTest : public ::testing::Test {
 protected:
  OEMCryptoClientTest() {}

  void SetUp() {
    ::testing::Test::SetUp();
    wvcdm::Properties::Init();
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_Initialize());
  }

  void TearDown() {
    OEMCrypto_Terminate();
    ::testing::Test::TearDown();
  }

  void CreateWrappedRSAKey(vector<uint8_t>* wrapped_key,
                           uint32_t allowed_schemes, bool force,
                           const uint8_t* rsa_key = NULL,
                           size_t rsa_key_length = 0) {
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    struct RSAPrivateKeyMessage encrypted;
    std::vector<uint8_t> signature;
    s.MakeRSACertificate(&encrypted, &signature, allowed_schemes, rsa_key,
                         rsa_key_length);
    s.RewrapRSAKey(encrypted, signature, wrapped_key, force);
  }

  const uint8_t* find(const vector<uint8_t>& message,
                      const vector<uint8_t>& substring) {
    vector<uint8_t>::const_iterator pos = search(
        message.begin(), message.end(), substring.begin(), substring.end());
    if (pos == message.end()) {
      return NULL;
    }
    return &(*pos);
  }
};

//
// Keybox Tests
// These two tests are first, becuase it might give an idea why other
// tests are failing when the device has the wrong keybox installed.
TEST_F(OEMCryptoClientTest, VersionNumber) {
  const char* level = OEMCrypto_SecurityLevel();
  ASSERT_NE((char*)NULL, level);
  ASSERT_EQ('L', level[0]);
  cout << "             OEMCrypto Security Level is " << level << endl;
  uint32_t version = OEMCrypto_APIVersion();
  cout << "             OEMCrypto API version is " << version << endl;
  ASSERT_LE(8, version);
  ASSERT_GE(9, version);
}

TEST_F(OEMCryptoClientTest, NormalGetKeyData) {
  OEMCryptoResult sts;
  uint8_t key_data[256];
  size_t key_data_len = sizeof(key_data);
  sts = OEMCrypto_GetKeyData(key_data, &key_data_len);

  uint32_t* data = reinterpret_cast<uint32_t*>(key_data);
  printf("             NormalGetKeyData: system_id = %d = 0x%04X, version=%d\n",
         htonl(data[1]), htonl(data[1]), htonl(data[0]));
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  uint32_t system_id = htonl(data[1]);
  if (system_id == 0x1019) {
    cout << "========================================================================\n"
         << "= If you run this as \"oemcrypto_test --gtest_also_run_disabled_tests\", =\n"
         << "= then a test keybox will be installed, and all tests will be run.     =\n"
         << "========================================================================\n";
  }
}

const char* HDCPCapabilityAsString(OEMCrypto_HDCP_Capability value) {
  switch (value) {
    case 0x0:
      return "No HDCP supported, no secure data path";
    case 0x1:
      return "HDCP version 1.0";
    case 0x2:
      return "HDCP version 2.0";
    case 0x3:
      return "HDCP version 2.1";
    case 0x4:
      return "HDCP version 2.2";
    case 0xFF:
      return "No HDCP device attached/using local display with secure path";
    default:
      return "<INVALID VALUE>";
  }
}

TEST_F(OEMCryptoClientTest, CheckHDCPCapability) {
  OEMCryptoResult sts;
  OEMCrypto_HDCP_Capability current, maximum;
  sts = OEMCrypto_GetHDCPCapability(&current, &maximum);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  printf("             Current HDCP Capability: 0x%02x = %s.\n", current,
         HDCPCapabilityAsString(current));
  printf("             Maximum HDCP Capability: 0x%02x = %s.\n", maximum,
         HDCPCapabilityAsString(maximum));
}

TEST_F(OEMCryptoClientTest, KeyboxValid) {
  ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_IsKeyboxValid());
}

TEST_F(OEMCryptoClientTest, NormalGetDeviceId) {
  OEMCryptoResult sts;
  uint8_t dev_id[128] = {0};
  size_t dev_id_len = 128;
  sts = OEMCrypto_GetDeviceID(dev_id, &dev_id_len);
  cout << "             NormalGetDeviceId: dev_id = " << dev_id
       << " len = " << dev_id_len << endl;
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
}

TEST_F(OEMCryptoClientTest, GetDeviceIdShortBuffer) {
  OEMCryptoResult sts;
  uint8_t dev_id[128];
  uint32_t req_len = 0;
  for (int i = 0; i < 128; ++i) {
    dev_id[i] = 0x55;
  }
  dev_id[127] = '\0';
  size_t dev_id_len = req_len;
  sts = OEMCrypto_GetDeviceID(dev_id, &dev_id_len);
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
  // On short buffer error, function should return minimum buffer length
  ASSERT_TRUE(dev_id_len > req_len);
  // Should also return short buffer if passed a zero length and a null buffer.
  dev_id_len = req_len;
  sts = OEMCrypto_GetDeviceID(NULL, &dev_id_len);
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
  // On short buffer error, function should return minimum buffer length
  ASSERT_TRUE(dev_id_len > req_len);
}

TEST_F(OEMCryptoClientTest, DefaultKeybox) {
  ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_IsKeyboxValid());
}

//
// initialization tests
//
TEST_F(OEMCryptoClientTest, NormalInitTermination) {
  // Should be able to terminate OEMCrypto, and then restart it.
  ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_Terminate());
  ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_Initialize());
}

//
// Session Tests
//
TEST_F(OEMCryptoClientTest, NormalSessionOpenClose) {
  Session s;
  s.open();
  ASSERT_EQ(OEMCrypto_SUCCESS, s.getStatus());
  ASSERT_TRUE(s.isOpen());
  s.close();
  ASSERT_EQ(OEMCrypto_SUCCESS, s.getStatus());
  ASSERT_FALSE(s.isOpen());
}

TEST_F(OEMCryptoClientTest, TwoSessionsOpenClose) {
  Session s1;
  Session s2;

  s1.open();
  ASSERT_EQ(OEMCrypto_SUCCESS, s1.getStatus());
  ASSERT_TRUE(s1.isOpen());

  s2.open();
  ASSERT_EQ(OEMCrypto_SUCCESS, s2.getStatus());
  ASSERT_TRUE(s2.isOpen());

  s1.close();
  ASSERT_EQ(OEMCrypto_SUCCESS, s1.getStatus());
  ASSERT_FALSE(s1.isOpen());

  s2.close();
  ASSERT_EQ(OEMCrypto_SUCCESS, s2.getStatus());
  ASSERT_FALSE(s2.isOpen());
}

TEST_F(OEMCryptoClientTest, EightSessionsOpenClose) {
  Session s[8];
  for (int i = 0; i < 8; i++) {
    s[i].open();
    ASSERT_EQ(OEMCrypto_SUCCESS, s[i].getStatus());
    ASSERT_TRUE(s[i].isOpen());
  }
  for (int i = 0; i < 8; i++) {
    s[i].close();
    ASSERT_EQ(OEMCrypto_SUCCESS, s[i].getStatus());
    ASSERT_FALSE(s[i].isOpen());
  }
}

TEST_F(OEMCryptoClientTest, GenerateNonce) {
  Session s;
  s.open();
  uint32_t nonce;
  s.GenerateNonce(&nonce);
}

TEST_F(OEMCryptoClientTest, GenerateTwoNonces) {
  Session s;
  s.open();
  uint32_t nonce1;
  uint32_t nonce2;

  s.GenerateNonce(&nonce1);
  s.GenerateNonce(&nonce2);
  ASSERT_TRUE(nonce1 != nonce2);
}

TEST_F(OEMCryptoClientTest, PreventNonceFlood) {
  Session s;
  s.open();
  int error_counter = 0;
  uint32_t nonce;
  time_t test_start = time(NULL);
  // More than 20 nonces per second should generate an error.
  // To allow for some slop, we actually test for more than 40.
  for (int i = 0; i < 60; i++) {
    s.GenerateNonce(&nonce, &error_counter);
  }
  time_t test_end = time(NULL);
  // Either there should be a two second delay, or there should have been
  // at least 20 errors.
  if (20 > error_counter) {
    EXPECT_LE(2, test_end - test_start);
  } else {
    EXPECT_LE(20, error_counter);
  }
  error_counter = 0;
  sleep(2);  // After a pause, we should be able to regenerate nonces.
  s.GenerateNonce(&nonce, &error_counter);
  EXPECT_EQ(0, error_counter);
}

// Prevent a nonce flood even if each nonce is in a different session.
TEST_F(OEMCryptoClientTest, PreventNonceFlood2) {
  int error_counter = 0;
  uint32_t nonce;
  time_t test_start = time(NULL);
  // More than 20 nonces per second should generate an error.
  // To allow for some slop, we actually test for more than 40.
  for (int i = 0; i < 60; i++) {
    Session s;
    s.open();
    EXPECT_TRUE(s.isOpen());
    s.GenerateNonce(&nonce, &error_counter);
  }
  time_t test_end = time(NULL);
  // Either there should be a two second delay, or there should have been
  // at least 20 errors.
  if (20 > error_counter) {
    EXPECT_LE(2, test_end - test_start);
  } else {
    EXPECT_LE(20, error_counter);
  }
  error_counter = 0;
  sleep(2);  // After a pause, we should be able to regenerate nonces.
  Session s;
  s.open();
  s.GenerateNonce(&nonce, &error_counter);
  EXPECT_EQ(0, error_counter);
}

// Prevent a nonce flood even if some nonces are in a different session.  This
// is different from the test above because there are several session open at
// the same time.  We want to make sure you can't get a flood of nonces by
// opening a flood of sessions.
TEST_F(OEMCryptoClientTest, PreventNonceFlood3) {
  int error_counter = 0;
  uint32_t nonce;
  time_t test_start = time(NULL);
  // More than 20 nonces per second should generate an error.
  // To allow for some slop, we actually test for more than 40.
  Session s[6];
  for (int i = 0; i < 6; i++) {
    s[i].open();
    EXPECT_TRUE(s[i].isOpen());
    for (int j = 0; j < 10; j++) {
      s[i].GenerateNonce(&nonce, &error_counter);
    }
  }
  time_t test_end = time(NULL);
  // Either there should be a two second delay, or there should have been
  // at least 20 errors.
  if (20 > error_counter) {
    EXPECT_LE(2, test_end - test_start);
  } else {
    EXPECT_LE(20, error_counter);
  }
  error_counter = 0;
  sleep(2);  // After a pause, we should be able to regenerate nonces.
  Session s7;
  s7.open();
  EXPECT_TRUE(s7.isOpen());
  s7.GenerateNonce(&nonce, &error_counter);
  EXPECT_EQ(0, error_counter);
}

TEST_F(OEMCryptoClientTest, GenerateDerivedKeys) {
  Session s;
  s.open();
  s.GenerateDerivedKeys();
}

//
// AddKey Tests
//
// These tests will install a test keybox.  This may be a problem
// on a production device, so they are disabled by default.
// Run this program with the command line argument:
//      "--gtest_also_run_disabled_tests"
// to enable all of these tests.

class DISABLED_TestKeybox : public OEMCryptoClientTest {
 protected:
  void SetUp() {
    OEMCryptoClientTest::SetUp();
    InstallKeybox(kDefaultKeybox, true);
    OEMCrypto_DeleteUsageTable();
  }

  void InstallKeybox(const wvoec_mock::WidevineKeybox& keybox, bool good) {
    OEMCryptoResult sts;
    uint8_t wrapped[sizeof(wvoec_mock::WidevineKeybox)];
    size_t length = sizeof(wvoec_mock::WidevineKeybox);
    sts = OEMCrypto_WrapKeybox(reinterpret_cast<const uint8_t*>(&keybox),
                               sizeof(keybox), wrapped, &length, NULL, 0);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    sts = OEMCrypto_InstallKeybox(wrapped, sizeof(keybox));
    if (good) {
      ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    } else {
      // Can return error now, or return error on IsKeyboxValid.
    }
  }
};

TEST_F(OEMCryptoClientTest, DISABLED_CheckSystemID) {
  OEMCryptoResult sts;
  uint8_t key_data[256];
  size_t key_data_len = sizeof(key_data);
  sts = OEMCrypto_GetKeyData(key_data, &key_data_len);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  uint32_t* data = reinterpret_cast<uint32_t*>(key_data);
  uint32_t system_id = htonl(data[1]);
  if (system_id != 0x1019) {
    cout << "================================================================\n"
         << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"
         << "WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING \n"
         << "You have enabled the keybox tests.  This code WILL INSTALL A \n"
         << "TEST KEYBOX.  IT WILL REPLACE THE EXISTING KEYBOX, and you will.\n"
         << "NOT have access to production content.  Your current keybox has \n"
         << "system id " << system_id << ".\n"
         << "\n"
         << "Continue? [y/N]:\n";
    int answer = getchar();
    if (tolower(answer) != 'y') {
      cout << "Quitting tests.\n";
      exit(1);
    }
  }
}

TEST_F(DISABLED_TestKeybox, GoodKeybox) {
  wvoec_mock::WidevineKeybox keybox = kValidKeybox02;
  OEMCryptoResult sts;
  InstallKeybox(keybox, true);
  sts = OEMCrypto_IsKeyboxValid();
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  keybox = kValidKeybox03;
  InstallKeybox(keybox, true);
  sts = OEMCrypto_IsKeyboxValid();
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, BadCRCKeybox) {
  wvoec_mock::WidevineKeybox keybox = kValidKeybox02;
  keybox.crc_[1] ^= 42;
  OEMCryptoResult sts;
  InstallKeybox(keybox, false);
  sts = OEMCrypto_IsKeyboxValid();
  ASSERT_EQ(OEMCrypto_ERROR_BAD_CRC, sts);
  InstallKeybox(kDefaultKeybox, true);
}

TEST_F(DISABLED_TestKeybox, BadMagicKeybox) {
  wvoec_mock::WidevineKeybox keybox = kValidKeybox02;
  keybox.magic_[1] ^= 42;
  OEMCryptoResult sts;
  InstallKeybox(keybox, false);
  sts = OEMCrypto_IsKeyboxValid();
  ASSERT_EQ(OEMCrypto_ERROR_BAD_MAGIC, sts);
  InstallKeybox(kDefaultKeybox, true);
}

TEST_F(DISABLED_TestKeybox, BadDataKeybox) {
  wvoec_mock::WidevineKeybox keybox = kValidKeybox02;
  keybox.data_[1] ^= 42;
  OEMCryptoResult sts;
  InstallKeybox(keybox, false);
  sts = OEMCrypto_IsKeyboxValid();
  ASSERT_EQ(OEMCrypto_ERROR_BAD_CRC, sts);
  InstallKeybox(kDefaultKeybox, true);
}

TEST_F(DISABLED_TestKeybox, GenerateSignature) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();

  // Dummy context for testing signature generation.
  vector<uint8_t> context = wvcdm::a2b_hex(
      "0a4c08001248000000020000101907d9ffde13aa95c122678053362136bdf840"
      "8f8276e4c2d87ec52b61aa1b9f646e58734930acebe899b3e464189a14a87202"
      "fb02574e70640bd22ef44b2d7e3912250a230a14080112100915007caa9b5931"
      "b76a3a85f046523e10011a09393837363534333231180120002a0c3138383637"
      "38373430350000");

  static const uint32_t SignatureBufferMaxLength = 256;
  uint8_t signature[SignatureBufferMaxLength];
  size_t signature_length = SignatureBufferMaxLength;

  OEMCryptoResult sts;
  sts = OEMCrypto_GenerateSignature(s.session_id(), &context[0], context.size(),
                                    signature, &signature_length);

  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  static const uint32_t SignatureExpectedLength = 32;
  ASSERT_EQ(signature_length, SignatureExpectedLength);

  std::vector<uint8_t> expected_signature;
  s.ClientSignMessage(context, &expected_signature);
  ASSERT_EQ(0, memcmp(&expected_signature[0], signature,
                      expected_signature.size()));
}

TEST_F(DISABLED_TestKeybox, LoadKeyNoNonce) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, 0, 42);
  s.EncryptAndSign();
  s.LoadTestKeys();
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithNonce) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, wvoec_mock::kControlNonceEnabled, s.get_nonce());
  s.EncryptAndSign();
  s.LoadTestKeys();
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithNoMAC) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  s.LoadTestKeys("", false);

  vector<uint8_t> context = wvcdm::a2b_hex(
      "0a4c08001248000000020000101907d9ffde13aa95c122678053362136bdf840"
      "8f8276e4c2d87ec52b61aa1b9f646e58734930acebe899b3e464189a14a87202"
      "fb02574e70640bd22ef44b2d7e3912250a230a14080112100915007caa9b5931"
      "b76a3a85f046523e10011a09393837363534333231180120002a0c3138383637"
      "38373430350000");

  static const uint32_t SignatureBufferMaxLength = 256;
  uint8_t signature[SignatureBufferMaxLength];
  size_t signature_length = SignatureBufferMaxLength;

  OEMCryptoResult sts;
  sts = OEMCrypto_GenerateSignature(s.session_id(), &context[0], context.size(),
                                    signature, &signature_length);

  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  static const uint32_t SignatureExpectedLength = 32;
  ASSERT_EQ(signature_length, SignatureExpectedLength);

  std::vector<uint8_t> expected_signature;
  s.ClientSignMessage(context, &expected_signature);
  ASSERT_EQ(
      0, memcmp(&expected_signature[0], signature, expected_signature.size()));
}

/* The Bad Range tests verify that OEMCrypto_LoadKeys checks the range
   of all the pointers.  It should reject a message if the pointer does
   not point into the message buffer */
TEST_F(DISABLED_TestKeybox, LoadKeyWithBadRange1) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  vector<uint8_t> mac_keys(
      s.encrypted_license().mac_keys,
      s.encrypted_license().mac_keys + sizeof(s.encrypted_license().mac_keys));

  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      &mac_keys[0],  // Not pointing into buffer.
      kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithBadRange2) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  vector<uint8_t> mac_key_iv(s.encrypted_license().mac_key_iv,
                             s.encrypted_license().mac_key_iv +
                                 sizeof(s.encrypted_license().mac_key_iv));

  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(),
      &mac_key_iv[0],  // bad.
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithBadRange3) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  vector<uint8_t> bad_buffer(
      s.encrypted_license().keys[0].key_id,
      s.encrypted_license().keys[0].key_id + kTestKeyIdLength);
  s.key_array()[0].key_id = &bad_buffer[0];

  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithBadRange4) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();

  vector<uint8_t> bad_buffer(
      s.encrypted_license().keys[1].key_data,
      s.encrypted_license().keys[1].key_data + wvcdm::KEY_SIZE);
  s.key_array()[1].key_data = &bad_buffer[0];

  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithBadRange5) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  vector<uint8_t> bad_buffer(s.encrypted_license().keys[1].key_iv,
                             s.encrypted_license().keys[1].key_iv +
                                 sizeof(s.encrypted_license().keys[1].key_iv));
  s.key_array()[1].key_data_iv = &bad_buffer[0];
  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithBadRange6) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();

  vector<uint8_t> bad_buffer(s.key_array()[2].key_control,
                             s.key_array()[2].key_control +
                                 sizeof(s.encrypted_license().keys[1].control));
  s.key_array()[2].key_control = &bad_buffer[0];

  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithBadRange7) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  vector<uint8_t> bad_buffer(
      s.key_array()[2].key_control_iv,
      s.key_array()[2].key_control_iv +
          sizeof(s.encrypted_license().keys[1].control_iv));
  s.key_array()[2].key_control_iv = &bad_buffer[0];

  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithBadNonce) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, wvoec_mock::kControlNonceEnabled, 42);  // bad nonce.
  s.EncryptAndSign();
  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);

  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithRepeatNonce) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  uint32_t nonce = s.get_nonce();
  s.FillSimpleMessage(0, wvoec_mock::kControlNonceEnabled, nonce);
  s.EncryptAndSign();
  s.LoadTestKeys();
  s.close();

  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, wvoec_mock::kControlNonceEnabled,
                      nonce);  // same old nonce.
  s.EncryptAndSign();
  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);

  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeyWithBadVerification) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.license().keys[1].control.verification[2] = 'Z';
  s.EncryptAndSign();
  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);

  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeysBadSignature) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  s.signature()[0] ^= 42;  // Bad signature.
  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, LoadKeysWithNoDerivedKeys) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  // s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  OEMCryptoResult sts = OEMCrypto_LoadKeys(
      s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
      s.signature().size(), s.encrypted_license().mac_key_iv,
      s.encrypted_license().mac_keys, kNumKeys, s.key_array(), NULL, 0);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
}

class DISABLED_DecryptWithHDCP : public DISABLED_TestKeybox,
                                 public WithParamInterface<int> {
 public:
  void DecryptWithHDCP(OEMCrypto_HDCP_Capability version) {
    OEMCryptoResult sts;
    InstallKeybox(kDefaultKeybox, true);
    OEMCrypto_HDCP_Capability current, maximum;
    sts = OEMCrypto_GetHDCPCapability(&current, &maximum);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    s.FillSimpleMessage(0, (version << wvoec_mock::kControlHDCPVersionShift) |
                               wvoec_mock::kControlObserveHDCP |
                               wvoec_mock::kControlHDCPRequired,
                        0);
    s.EncryptAndSign();
    s.LoadTestKeys();

    if (version > current) {
      s.TestDecryptCTR(true, OEMCrypto_ERROR_INSUFFICIENT_HDCP);
    } else {
      s.TestDecryptCTR(true, OEMCrypto_SUCCESS);
    }
  }
};

TEST_P(DISABLED_DecryptWithHDCP, Decrypt) {
  // Test parameterized by HDCP version.
  DecryptWithHDCP(static_cast<OEMCrypto_HDCP_Capability>(GetParam()));
}
INSTANTIATE_TEST_CASE_P(TestHDCP, DISABLED_DecryptWithHDCP, Range(1, 5));

//
// Load, Refresh Keys Test
//
class DISABLED_RefreshKeyTest
    : public DISABLED_TestKeybox,
      public WithParamInterface<std::pair<bool, int> > {
 public:
  virtual void SetUp() {
    DISABLED_TestKeybox::SetUp();
    new_mac_keys_ =
        GetParam().first;  // Whether to put new mac keys in LoadKeys.
    num_keys_ = static_cast<size_t>(GetParam().second);  // # keys in refresh.
  }

 protected:
  bool new_mac_keys_;
  size_t num_keys_;
};

TEST_P(DISABLED_RefreshKeyTest, RefreshWithNonce) {
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, wvoec_mock::kControlNonceEnabled,
                      s.get_nonce());
  s.EncryptAndSign();
  s.LoadTestKeys("", new_mac_keys_);
  uint32_t nonce;
  s.GenerateNonce(&nonce);
  s.RefreshTestKeys(num_keys_, wvoec_mock::kControlNonceEnabled, nonce, true);
}

TEST_P(DISABLED_RefreshKeyTest, RefresNoNonce) {
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, 0, 0);
  s.EncryptAndSign();
  s.LoadTestKeys("", new_mac_keys_);
  uint32_t nonce;
  s.GenerateNonce(&nonce);
  s.RefreshTestKeys(num_keys_, 0, 0, true);
}

TEST_P(DISABLED_RefreshKeyTest, RefreshOldNonce) {
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, wvoec_mock::kControlNonceEnabled,
                      s.get_nonce());
  s.EncryptAndSign();
  s.LoadTestKeys("", new_mac_keys_);
  uint32_t nonce = s.get_nonce();
  s.RefreshTestKeys(num_keys_, wvoec_mock::kControlNonceEnabled, nonce, false);
}

TEST_P(DISABLED_RefreshKeyTest, RefreshBadNonce) {
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, wvoec_mock::kControlNonceEnabled,
                      s.get_nonce());
  s.EncryptAndSign();
  s.LoadTestKeys("", new_mac_keys_);
  uint32_t nonce;
  s.GenerateNonce(&nonce);
  nonce ^= 42;
  s.RefreshTestKeys(num_keys_, wvoec_mock::kControlNonceEnabled, nonce, false);
}

// Of only one key control block in the refesh, we update all the keys.
INSTANTIATE_TEST_CASE_P(TestRefreshAllKeys, DISABLED_RefreshKeyTest,
                        Values(std::make_pair(true, 1),
                               std::make_pair(false, 1)));

// If multiple key control blocks, we update each key separately.
INSTANTIATE_TEST_CASE_P(TestRefreshEachKeys, DISABLED_RefreshKeyTest,
                        Values(std::make_pair(true, kNumKeys),
                               std::make_pair(false, kNumKeys)));

//
// Decrypt Tests
//
TEST_F(DISABLED_TestKeybox, Decrypt) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, 0, 0);
  s.EncryptAndSign();
  s.LoadTestKeys();
  s.TestDecryptCTR();
}

TEST_F(DISABLED_TestKeybox, DecryptZeroDuration) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();
  s.FillSimpleMessage(0, 0, 0);
  s.EncryptAndSign();
  s.LoadTestKeys();
  s.TestDecryptCTR();
}

TEST_F(DISABLED_TestKeybox, DecryptWithOffset) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, 0, 0);
  s.EncryptAndSign();
  s.LoadTestKeys();

  // Select the key (from FillSimpleMessage)
  vector<uint8_t> keyId = wvcdm::a2b_hex("000000000000000000000000");
  sts = OEMCrypto_SelectKey(s.session_id(), &keyId[0], keyId.size());
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  // Set up our expected input and output
  // This is dummy encrypted data.
  vector<uint8_t> encryptedData = wvcdm::a2b_hex(
      "c17055d4e3ab8e892b40ca2deed7cd46b406cd41d50f23d5877b36"
      "ad351887df2b3774dc413904afd958ba766cc6ab51a3ffd8f845296c5d8326ee"
      "39c9d0fec79885515e6b8a12911831d9fb158ca2fd3dfcfcf228741a63734685"
      "8dffc30f5871260c5cef8be61cfa08b191c837901f077046664c0c56db81d412"
      "98b59e5655cd94871c3c226dc3565144297f1459cddba069d5d2d6206cfd5798"
      "eda4b82e01a9966d48984d6ef3fbd326ba0f6fcbe52c95786d478c2f33398c62"
      "ae5210c7472d7d8dc7d12f981679f4ea9793736f354747ef14165367b94e07fc"
      "4bcc7bd14746304fea100dc6465ab51241355bb19e6c2cfb2bb6bbf709765d13");
  vector<uint8_t> encryptionIv = wvcdm::a2b_hex(
      "c09454479a280829c946df3c22f25539");
  // This is the expected decrypted data.
  vector<uint8_t> unencryptedData = wvcdm::a2b_hex(
      "f344d9cfe336c94cf4e3ea9e3446d1427bc02d2debe6dec5b272b8"
      "a4004b696c4b37e01d7418510abf32bb071f9a4bc0d2ad7e874b648e50bd0e4f"
      "7085b70bf9ad2c7f37025dd45f93e90304739b1ce098a52e7b99a90f92544a9b"
      "dca6f49e0006c80a0cfa018600523ad30e483141fe720d045394815d5c875ad4"
      "b4387b8d09b6119bd0943e51b0b9103034496b3a83ba593f79baa188aeb6e08f"
      "f6475933e9ce1bb95fbb526424e7966e25830c20da73c65c6fbff110b08e4def"
      "eae94f98296770275b0d738207a8217cd6118f6ebc6e393428f2268cfedf800e"
      "a7ebc606471b9a9dfccd1589e86d88fde508261eaf190efd20554ce9e14ff3c9");

  // Describe the output
  uint8_t outputBuffer[256];
  OEMCrypto_DestBufferDesc destBuffer;
  destBuffer.type = OEMCrypto_BufferType_Clear;
  destBuffer.buffer.clear.address = outputBuffer;
  destBuffer.buffer.clear.max_length = sizeof(outputBuffer);

  // Decrypt the data
  sts = OEMCrypto_DecryptCTR(
      s.session_id(), &encryptedData[0], encryptedData.size(), true,
      &encryptionIv[0], 5, &destBuffer,
      OEMCrypto_FirstSubsample | OEMCrypto_LastSubsample);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(&unencryptedData[0], outputBuffer,
                      unencryptedData.size()));
}

// Increment counter for AES-CTR.  The CENC spec specifies we increment only
// the low 64 bits of the IV counter, and leave the high 64 bits alone.  This is
// different from the OpenSSL implementation, so we implement the CTR loop
// ourselves.
void ctr128_inc64(uint8_t* counter) {
  uint32_t n = 16;
  do {
    if (++counter[--n] != 0) return;
  } while (n > 8);
}
vector<uint8_t> EncryptCTR(const vector<uint8_t>& key,
                           const vector<uint8_t>& iv, const vector<uint8_t>& in,
                           size_t block_offset) {
  AES_KEY aes_key;
  AES_set_encrypt_key(&key[0], AES_BLOCK_SIZE * 8, &aes_key);

  uint8_t aes_iv[AES_BLOCK_SIZE];
  memcpy(aes_iv, &iv[0], AES_BLOCK_SIZE);

  // Encrypt the IV.
  uint8_t ecount_buf[AES_BLOCK_SIZE];

  vector<uint8_t> out(in.size());

  size_t cipher_data_length = in.size();
  size_t l = 0;
  while (l < cipher_data_length) {
    AES_encrypt(aes_iv, ecount_buf, &aes_key);
    for (int n = block_offset; n < AES_BLOCK_SIZE && l < cipher_data_length;
         ++n, ++l) {
      out[l] = in[l] ^ ecount_buf[n];
    }
    ctr128_inc64(aes_iv);
    block_offset = 0;
  }
  return out;
}

TEST_F(DISABLED_TestKeybox, DecryptWithNearWrap) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, 0, 0);
  s.EncryptAndSign();
  s.LoadTestKeys();

  // Select the key (from FillSimpleMessage)
  vector<uint8_t> keyId = wvcdm::a2b_hex("000000000000000000000000");
  sts = OEMCrypto_SelectKey(s.session_id(), &keyId[0], keyId.size());
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  // Set up our expected input and output
  vector<uint8_t> key = wvcdm::a2b_hex("39AD33E5719656069F9EDE9EBBA7A77D");
  vector<uint8_t> encryptionIv = wvcdm::a2b_hex(
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE");
  // This is dummy decrypted data.
  vector<uint8_t> unencryptedData = wvcdm::a2b_hex(
      "f344d9cfe336c94cf4e3ea9e3446d1427bc02d2debe6dec5b272b8"
      "a4004b696c4b37e01d7418510abf32bb071f9a4bc0d2ad7e874b648e50bd0e4f"
      "7085b70bf9ad2c7f37025dd45f93e90304739b1ce098a52e7b99a90f92544a9b"
      "dca6f49e0006c80a0cfa018600523ad30e483141fe720d045394815d5c875ad4"
      "b4387b8d09b6119bd0943e51b0b9103034496b3a83ba593f79baa188aeb6e08f"
      "f6475933e9ce1bb95fbb526424e7966e25830c20da73c65c6fbff110b08e4def"
      "eae94f98296770275b0d738207a8217cd6118f6ebc6e393428f2268cfedf800e"
      "a7ebc606471b9a9dfccd1589e86d88fde508261eaf190efd20554ce9e14ff3c9");
  size_t block_offset = 5;
  vector<uint8_t> encryptedData =
      EncryptCTR(key, encryptionIv, unencryptedData, block_offset);

  // Describe the output
  uint8_t outputBuffer[256];
  OEMCrypto_DestBufferDesc destBuffer;
  destBuffer.type = OEMCrypto_BufferType_Clear;
  destBuffer.buffer.clear.address = outputBuffer;
  destBuffer.buffer.clear.max_length = sizeof(outputBuffer);

  // Decrypt the data
  sts = OEMCrypto_DecryptCTR(
      s.session_id(), &encryptedData[0], encryptedData.size(), true,
      &encryptionIv[0], block_offset, &destBuffer,
      OEMCrypto_FirstSubsample | OEMCrypto_LastSubsample);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(&unencryptedData[0], outputBuffer,
                      unencryptedData.size()));
}

TEST_F(DISABLED_TestKeybox, DecryptUnencrypted) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, 0, 0);
  s.EncryptAndSign();
  s.LoadTestKeys();

  // Select the key (from FillSimpleMessage)
  vector<uint8_t> keyId = wvcdm::a2b_hex("000000000000000000000000");
  sts = OEMCrypto_SelectKey(s.session_id(), &keyId[0], keyId.size());
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  // Set up our expected input and output
  // This is dummy decrypted data.
  vector<uint8_t> unencryptedData = wvcdm::a2b_hex(
      "1558497b6d994be343ed1c6d6313e0537b843e9a9c0836d1e83fe33154191ce9"
      "a14d8d95bebaddc03bd471827170f527c0a166b9068b273d1bc57fbb13975ee4"
      "f6b9a31743da6c447acbb712e81b13eddfd4e96c76010ac9b8aa1b6b3152b0fc"
      "39ad33e5719656069f9ede9ebba7a77dd2e2074eec5c1b7ffc427a6f1be168f0"
      "b5857713a44623862c903284bc53417e23c65602b52c1cb699e8352453eb9698"
      "0b31459b90c26c907b549c1ab293725e414d4e45f5b30af7a55f95499a7dc89f"
      "7d13ba90b34aef6b49484b0701bf96ea8b660c24bb4e92a2d1c43beb434fa386"
      "1071380388799ac31d79285f5817687ed3e2eeb73a30744e77b757686c9ba5ad");
  vector<uint8_t> encryptionIv = wvcdm::a2b_hex(
      "49fc3efaaf614ed81d595847b928edd0");

  // Describe the output
  uint8_t outputBuffer[256];
  OEMCrypto_DestBufferDesc destBuffer;
  destBuffer.type = OEMCrypto_BufferType_Clear;
  destBuffer.buffer.clear.address = outputBuffer;
  destBuffer.buffer.clear.max_length = sizeof(outputBuffer);

  // Decrypt the data
  sts = OEMCrypto_DecryptCTR(
      s.session_id(), &unencryptedData[0], unencryptedData.size(), false,
      &encryptionIv[0], 0, &destBuffer,
      OEMCrypto_FirstSubsample | OEMCrypto_LastSubsample);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(&unencryptedData[0], outputBuffer,
                      unencryptedData.size()));
}

TEST_F(DISABLED_TestKeybox, DecryptUnencryptedNoKey) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();

  // CLear data should be copied even if there is no key selected.

  // Set up our expected input and output
  // This is dummy decrypted data.
  vector<uint8_t> unencryptedData = wvcdm::a2b_hex(
      "1558497b6d994be343ed1c6d6313e0537b843e9a9c0836d1e83fe33154191ce9"
      "a14d8d95bebaddc03bd471827170f527c0a166b9068b273d1bc57fbb13975ee4"
      "f6b9a31743da6c447acbb712e81b13eddfd4e96c76010ac9b8aa1b6b3152b0fc"
      "39ad33e5719656069f9ede9ebba7a77dd2e2074eec5c1b7ffc427a6f1be168f0"
      "b5857713a44623862c903284bc53417e23c65602b52c1cb699e8352453eb9698"
      "0b31459b90c26c907b549c1ab293725e414d4e45f5b30af7a55f95499a7dc89f"
      "7d13ba90b34aef6b49484b0701bf96ea8b660c24bb4e92a2d1c43beb434fa386"
      "1071380388799ac31d79285f5817687ed3e2eeb73a30744e77b757686c9ba5ad");
  vector<uint8_t> encryptionIv = wvcdm::a2b_hex(
      "49fc3efaaf614ed81d595847b928edd0");

  // Describe the output
  uint8_t outputBuffer[256];
  OEMCrypto_DestBufferDesc destBuffer;
  destBuffer.type = OEMCrypto_BufferType_Clear;
  destBuffer.buffer.clear.address = outputBuffer;
  destBuffer.buffer.clear.max_length = sizeof(outputBuffer);

  // Decrypt the data
  sts = OEMCrypto_DecryptCTR(
      s.session_id(), &unencryptedData[0], unencryptedData.size(), false,
      &encryptionIv[0], 0, &destBuffer,
      OEMCrypto_FirstSubsample | OEMCrypto_LastSubsample);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(&unencryptedData[0], outputBuffer,
                      unencryptedData.size()));
}

TEST_F(DISABLED_TestKeybox, DecryptSecureToClear) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, wvoec_mock::kControlObserveDataPath |
                                     wvoec_mock::kControlDataPathSecure,
                      0);
  s.EncryptAndSign();
  s.LoadTestKeys();
  s.TestDecryptCTR(true, OEMCrypto_ERROR_UNKNOWN_FAILURE);
}

TEST_F(DISABLED_TestKeybox, KeyDuration) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  s.FillSimpleMessage(kDuration, wvoec_mock::kControlNonceEnabled,
                      s.get_nonce());
  s.EncryptAndSign();
  s.LoadTestKeys();
  s.TestDecryptCTR(true, OEMCrypto_SUCCESS);
  sleep(kShortSleep);  //  Should still be valid key.
  s.TestDecryptCTR(false, OEMCrypto_SUCCESS);
  sleep(kLongSleep);  // Should be expired key.
  s.TestDecryptCTR(false, OEMCrypto_ERROR_KEY_EXPIRED);
}

//
// Certificate Root of Trust Tests
//
void TestKey(const uint8_t key[], size_t length) {
  uint8_t const* p = key;
  RSA* rsa = d2i_RSAPrivateKey(0, &p, length);
  if (!rsa) {
    cout << "d2i_RSAPrivateKey failed. ";
    dump_openssl_error();
    ASSERT_TRUE(false);
  }
  switch (RSA_check_key(rsa)) {
    case 1:  // valid.
      ASSERT_TRUE(true);
      return;
    case 0:  // not valid.
      cout << "[TestKey(): rsa key not valid] ";
      dump_openssl_error();
      ASSERT_TRUE(false);
    default:  // -1 == check failed.
      cout << "[TestKey(): error checking rsa key] ";
      dump_openssl_error();
      ASSERT_TRUE(false);
  }
}
TEST_F(DISABLED_TestKeybox, ValidateRSATestKeys) {
  TestKey(kTestPKCS1RSAPrivateKey2_2048, sizeof(kTestPKCS1RSAPrivateKey2_2048));
  TestKey(kTestPKCS1RSAPrivateKey3_2048, sizeof(kTestPKCS1RSAPrivateKey3_2048));
}

TEST_F(DISABLED_TestKeybox, CertificateProvision) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  struct RSAPrivateKeyMessage encrypted;
  std::vector<uint8_t> signature;
  s.MakeRSACertificate(&encrypted, &signature, kSign_RSASSA_PSS);
  vector<uint8_t> wrapped_key;
  s.RewrapRSAKey(encrypted, signature, &wrapped_key, true);

  vector<uint8_t> clear_key(kTestRSAPKCS8PrivateKeyInfo2_2048,
                            kTestRSAPKCS8PrivateKeyInfo2_2048 +
                                sizeof(kTestRSAPKCS8PrivateKeyInfo2_2048));
  ASSERT_EQ(NULL, find(wrapped_key, clear_key));
}

TEST_F(DISABLED_TestKeybox, CertificateProvisionBadRange1) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  struct RSAPrivateKeyMessage encrypted;
  std::vector<uint8_t> signature;
  s.MakeRSACertificate(&encrypted, &signature, kSign_RSASSA_PSS);
  vector<uint8_t> wrapped_key;
  const uint8_t* message_ptr = reinterpret_cast<const uint8_t*>(&encrypted);
  size_t wrapped_key_length = 0;
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv, NULL,
                &wrapped_key_length));
  wrapped_key.clear();
  wrapped_key.resize(wrapped_key_length);
  uint32_t nonce = encrypted.nonce;
  ASSERT_NE(
      OEMCrypto_SUCCESS,
      OEMCrypto_RewrapDeviceRSAKey(
          s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
          signature.size(), &nonce, encrypted.rsa_key, encrypted.rsa_key_length,
          encrypted.rsa_key_iv, &(wrapped_key.front()), &wrapped_key_length));
}

TEST_F(DISABLED_TestKeybox, CertificateProvisionBadRange2) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  struct RSAPrivateKeyMessage encrypted;
  std::vector<uint8_t> signature;
  s.MakeRSACertificate(&encrypted, &signature, kSign_RSASSA_PSS);
  vector<uint8_t> wrapped_key;
  const uint8_t* message_ptr = reinterpret_cast<const uint8_t*>(&encrypted);
  size_t wrapped_key_length = 0;
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv, NULL,
                &wrapped_key_length));
  wrapped_key.clear();
  wrapped_key.resize(wrapped_key_length);
  vector<uint8_t> bad_buffer(encrypted.rsa_key,
                             encrypted.rsa_key + sizeof(encrypted.rsa_key));

  ASSERT_NE(OEMCrypto_SUCCESS,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, &bad_buffer[0],
                encrypted.rsa_key_length, encrypted.rsa_key_iv,
                &(wrapped_key.front()), &wrapped_key_length));
}

TEST_F(DISABLED_TestKeybox, CertificateProvisionBadRange3) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  struct RSAPrivateKeyMessage encrypted;
  std::vector<uint8_t> signature;
  s.MakeRSACertificate(&encrypted, &signature, kSign_RSASSA_PSS);
  const uint8_t* message_ptr = reinterpret_cast<const uint8_t*>(&encrypted);
  vector<uint8_t> wrapped_key;

  size_t wrapped_key_length = 0;
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv, NULL,
                &wrapped_key_length));
  wrapped_key.clear();
  wrapped_key.resize(wrapped_key_length);
  vector<uint8_t> bad_buffer(encrypted.rsa_key,
                             encrypted.rsa_key + sizeof(encrypted.rsa_key));

  ASSERT_NE(OEMCrypto_SUCCESS,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, &bad_buffer[0],
                &(wrapped_key.front()), &wrapped_key_length));
}

TEST_F(DISABLED_TestKeybox, CertificateProvisionBadSignature) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  struct RSAPrivateKeyMessage encrypted;
  std::vector<uint8_t> signature;
  s.MakeRSACertificate(&encrypted, &signature, kSign_RSASSA_PSS);
  vector<uint8_t> wrapped_key;
  const uint8_t* message_ptr = reinterpret_cast<const uint8_t*>(&encrypted);

  size_t wrapped_key_length = 0;
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv, NULL,
                &wrapped_key_length));
  wrapped_key.clear();
  wrapped_key.resize(wrapped_key_length);
  signature[4] ^= 42;  // bad signature.
  ASSERT_NE(OEMCrypto_SUCCESS,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv,
                &(wrapped_key.front()), &wrapped_key_length));
}

TEST_F(DISABLED_TestKeybox, CertificateProvisionBadNonce) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  struct RSAPrivateKeyMessage encrypted;
  std::vector<uint8_t> signature;
  s.MakeRSACertificate(&encrypted, &signature, kSign_RSASSA_PSS);
  vector<uint8_t> wrapped_key;
  const uint8_t* message_ptr = reinterpret_cast<const uint8_t*>(&encrypted);

  size_t wrapped_key_length = 0;
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv, NULL,
                &wrapped_key_length));
  wrapped_key.clear();
  wrapped_key.resize(wrapped_key_length);
  encrypted.nonce ^= 42;  // Almost surely a bad nonce.
  ASSERT_NE(OEMCrypto_SUCCESS,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv,
                &(wrapped_key.front()), &wrapped_key_length));
}

TEST_F(DISABLED_TestKeybox, CertificateProvisionBadRSAKey) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  struct RSAPrivateKeyMessage encrypted;
  std::vector<uint8_t> signature;
  s.MakeRSACertificate(&encrypted, &signature, kSign_RSASSA_PSS);
  vector<uint8_t> wrapped_key;
  const uint8_t* message_ptr = reinterpret_cast<const uint8_t*>(&encrypted);

  size_t wrapped_key_length = 0;
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv, NULL,
                &wrapped_key_length));
  wrapped_key.clear();
  wrapped_key.resize(wrapped_key_length);
  encrypted.rsa_key[1] ^= 42;  // Almost surely a bad key.
  ASSERT_NE(OEMCrypto_SUCCESS,
            OEMCrypto_RewrapDeviceRSAKey(
                s.session_id(), message_ptr, sizeof(encrypted), &signature[0],
                signature.size(), &encrypted.nonce, encrypted.rsa_key,
                encrypted.rsa_key_length, encrypted.rsa_key_iv,
                &(wrapped_key.front()), &wrapped_key_length));
}

TEST_F(DISABLED_TestKeybox, LoadWrappedRSAKey) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  std::vector<uint8_t> wrapped_rsa_key;
  CreateWrappedRSAKey(&wrapped_rsa_key, kSign_RSASSA_PSS, true);

  Session s;
  s.open();
  sts = OEMCrypto_LoadDeviceRSAKey(s.session_id(), &wrapped_rsa_key[0],
                                   wrapped_rsa_key.size());
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_TestKeybox, RSASignature) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  std::vector<uint8_t> wrapped_rsa_key;
  CreateWrappedRSAKey(&wrapped_rsa_key, kSign_RSASSA_PSS, true);

  Session s;
  s.open();
  sts = OEMCrypto_LoadDeviceRSAKey(s.session_id(), &wrapped_rsa_key[0],
                                   wrapped_rsa_key.size());
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  // Sign a Message
  vector<uint8_t> licenseRequest(500);
  OEMCrypto_GetRandom(&licenseRequest[0], licenseRequest.size());
  size_t signature_length = 0;

  uint8_t signature[500];

  sts = OEMCrypto_GenerateRSASignature(s.session_id(), &licenseRequest[0],
                                       licenseRequest.size(), signature,
                                       &signature_length, kSign_RSASSA_PSS);

  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
  ASSERT_NE(static_cast<size_t>(0), signature_length);
  ASSERT_GE(sizeof(signature), signature_length);

  sts = OEMCrypto_GenerateRSASignature(s.session_id(), &licenseRequest[0],
                                       licenseRequest.size(), signature,
                                       &signature_length, kSign_RSASSA_PSS);

  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  // In the real world, the signature above would just have been used to contact
  // the license server to get this response.
  s.PreparePublicKey();
  s.VerifyRSASignature(&licenseRequest[0], licenseRequest.size(), signature,
                       signature_length, kSign_RSASSA_PSS);
}

TEST_F(DISABLED_TestKeybox, LoadRSASessionKey) {
  InstallKeybox(kDefaultKeybox, true);
  std::vector<uint8_t> wrapped_rsa_key;
  CreateWrappedRSAKey(&wrapped_rsa_key, kSign_RSASSA_PSS, true);
  Session s;
  s.open();
  s.InstallRSASessionTestKey(wrapped_rsa_key);
}

TEST_F(DISABLED_TestKeybox, CertificateDecrypt) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  std::vector<uint8_t> wrapped_rsa_key;
  CreateWrappedRSAKey(&wrapped_rsa_key, kSign_RSASSA_PSS, true);
  Session s;
  s.open();

  s.InstallRSASessionTestKey(wrapped_rsa_key);
  s.FillSimpleMessage(kDuration, 0, 0);
  s.EncryptAndSign();
  s.LoadTestKeys();
  s.TestDecryptCTR();
}

// This test attempts to use alternate algorithms for main device certs.
class DISABLED_AlternateRSAAlgorithms : public DISABLED_TestKeybox {
 protected:
  void DisallowForbiddenPadding(RSA_Padding_Scheme scheme, size_t size) {
    OEMCryptoResult sts;
    Session s;
    s.open();
    sts = OEMCrypto_LoadDeviceRSAKey(s.session_id(), &wrapped_rsa_key_[0],
                                     wrapped_rsa_key_.size());
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);

    // Sign a Message
    vector<uint8_t> licenseRequest(size);
    OEMCrypto_GetRandom(&licenseRequest[0], licenseRequest.size());
    size_t signature_length = 256;
    uint8_t* signature = new uint8_t[signature_length];
    memset(signature, 0, signature_length);
    sts = OEMCrypto_GenerateRSASignature(s.session_id(), &licenseRequest[0],
                                         licenseRequest.size(), signature,
                                         &signature_length, scheme);
    // Allow OEMCrypto to request a full buffer.
    if (sts == OEMCrypto_ERROR_SHORT_BUFFER) {
      ASSERT_NE(static_cast<size_t>(0), signature_length);
      delete[] signature;
      signature = new uint8_t[signature_length];
      memset(signature, 0, signature_length);
      sts = OEMCrypto_GenerateRSASignature(s.session_id(), &licenseRequest[0],
                                           licenseRequest.size(), signature,
                                           &signature_length, scheme);
    }

    ASSERT_NE(OEMCrypto_SUCCESS, sts)
        << "Signed with forbidden padding scheme=" << (int)scheme
        << ", size=" << (int)size;
    ASSERT_EQ(signature[0], 0);  // signature should not be computed.
    ASSERT_EQ(memcmp(signature, signature + 1, signature_length - 1), 0);
    delete[] signature;
  }

  void TestSignature(RSA_Padding_Scheme scheme, size_t size) {
    OEMCryptoResult sts;
    Session s;
    s.open();
    sts = OEMCrypto_LoadDeviceRSAKey(s.session_id(), &wrapped_rsa_key_[0],
                                     wrapped_rsa_key_.size());
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);

    vector<uint8_t> licenseRequest(size);
    OEMCrypto_GetRandom(&licenseRequest[0], licenseRequest.size());
    size_t signature_length = 0;
    sts = OEMCrypto_GenerateRSASignature(s.session_id(), &licenseRequest[0],
                                         licenseRequest.size(), NULL,
                                         &signature_length, scheme);
    ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
    ASSERT_NE(static_cast<size_t>(0), signature_length);

    uint8_t* signature = new uint8_t[signature_length];
    sts = OEMCrypto_GenerateRSASignature(s.session_id(), &licenseRequest[0],
                                         licenseRequest.size(), signature,
                                         &signature_length, scheme);

    ASSERT_EQ(OEMCrypto_SUCCESS, sts)
        << "Failed to sign with padding scheme=" << (int)scheme
        << ", size=" << (int)size;
    s.PreparePublicKey();
    s.VerifyRSASignature(&licenseRequest[0], licenseRequest.size(), signature,
                         signature_length, scheme);
    delete[] signature;
  }

  void DisallowDeriveKeys() {
    OEMCryptoResult sts;
    Session s;
    s.open();
    sts = OEMCrypto_LoadDeviceRSAKey(s.session_id(), &wrapped_rsa_key_[0],
                                     wrapped_rsa_key_.size());
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    s.DisallowDeriveKeys();
  }

  void LoadWithAllowedSchemes(uint32_t schemes, bool force) {
    InstallKeybox(kDefaultKeybox, true);
    CreateWrappedRSAKey(&wrapped_rsa_key_, schemes, force);
    key_loaded_ = (wrapped_rsa_key_.size() > 0);
  }

  std::vector<uint8_t> wrapped_rsa_key_;
  bool key_loaded_;
};

TEST_F(DISABLED_AlternateRSAAlgorithms, DisallowForbiddenPadding) {
  LoadWithAllowedSchemes(kSign_RSASSA_PSS, true);  // Use default padding scheme
  DisallowForbiddenPadding(kSign_PKCS1_Block1, 50);
}

TEST_F(DISABLED_AlternateRSAAlgorithms, TestSignaturePKCS1) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    cout << "===============================================================\n"
         << "== This device does load x509 Certs, I'll test them.        ===\n"
         << "===============================================================\n";
    DisallowForbiddenPadding(kSign_RSASSA_PSS, 83);
    DisallowDeriveKeys();
    TestSignature(kSign_PKCS1_Block1, 83);
    TestSignature(kSign_PKCS1_Block1, 50);
    DisallowForbiddenPadding(kSign_PKCS1_Block1, 84);  // too big.
  } else {
    cout << "===============================================================\n"
         << "==  This device does not load x509 Certs!                   ===\n"
         << "===============================================================\n";
  }
}

TEST_F(DISABLED_AlternateRSAAlgorithms, TestSignatureBoth) {
  LoadWithAllowedSchemes(kSign_RSASSA_PSS | kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    DisallowDeriveKeys();
    TestSignature(kSign_RSASSA_PSS, 200);
    TestSignature(kSign_PKCS1_Block1, 83);
    TestSignature(kSign_PKCS1_Block1, 50);
    DisallowForbiddenPadding(kSign_PKCS1_Block1, 84);
  }
}

// This tests attempts to use alternate algorithms for main device certs.
class DISABLED_AlternateRSAKey : public DISABLED_TestKeybox {
 protected:
  vector<uint8_t> encode(uint8_t type, const vector<uint8_t>& substring) {
    vector<uint8_t> result;
    result.push_back(type);
    if (substring.size() < 0x80) {
      uint8_t length = substring.size();
      result.push_back(length);
    } else if (substring.size() < 0x100) {
      result.push_back(0x81);
      uint8_t length = substring.size();
      result.push_back(length);
    } else {
      result.push_back(0x82);
      uint16_t length = substring.size();
      result.push_back(length >> 8);
      result.push_back(length & 0xFF);
    }
    result.insert(result.end(), substring.begin(), substring.end());
    return result;
  }
  vector<uint8_t> concat(const vector<uint8_t>& a, const vector<uint8_t>& b) {
    vector<uint8_t> result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
  }

  void BuildRSAKey() {
    vector<uint8_t> field_n =
        encode(0x02, wvcdm::a2b_hex(
                         "00"
                         "df271fd25f8644496b0c81be4bd50297"
                         "ef099b002a6fd67727eb449cea566ed6"
                         "a3981a71312a141cabc9815c1209e320"
                         "a25b32464e9999f18ca13a9fd3892558"
                         "f9e0adefdd3650dd23a3f036d60fe398"
                         "843706a40b0b8462c8bee3bce12f1f28"
                         "60c2444cdc6a44476a75ff4aa24273cc"
                         "be3bf80248465f8ff8c3a7f3367dfc0d"
                         "f5b6509a4f82811cedd81cdaaa73c491"
                         "da412170d544d4ba96b97f0afc806549"
                         "8d3a49fd910992a1f0725be24f465cfe"
                         "7e0eabf678996c50bc5e7524abf73f15"
                         "e5bef7d518394e3138ce4944506aaaaf"
                         "3f9b236dcab8fc00f87af596fdc3d9d6"
                         "c75cd508362fae2cbeddcc4c7450b17b"
                         "776c079ecca1f256351a43b97dbe2153"));
    vector<uint8_t> field_e = encode(0x02, wvcdm::a2b_hex("010001"));
    vector<uint8_t> field_d =
        encode(0x02, wvcdm::a2b_hex(
                         "00"
                         "5bd910257830dce17520b03441a51a8c"
                         "ab94020ac6ecc252c808f3743c95b7c8"
                         "3b8c8af1a5014346ebc4242cdfb5d718"
                         "e30a733e71f291e4d473b61bfba6daca"
                         "ed0a77bd1f0950ae3c91a8f901118825"
                         "89e1d62765ee671e7baeea309f64d447"
                         "bbcfa9ea12dce05e9ea8939bc5fe6108"
                         "581279c982b308794b3448e7f7b95229"
                         "2df88c80cb40142c4b5cf5f8ddaa0891"
                         "678d610e582fcb880f0d707caf47d09a"
                         "84e14ca65841e5a3abc5e9dba94075a9"
                         "084341f0edad9b68e3b8e082b80b6e6e"
                         "8a0547b44fb5061b6a9131603a5537dd"
                         "abd01d8e863d8922e9aa3e4bfaea0b39"
                         "d79283ad2cbc8a59cce7a6ecf4e4c81e"
                         "d4c6591c807defd71ab06866bb5e7745"));
    vector<uint8_t> field_p =
        encode(0x02, wvcdm::a2b_hex(
                         "00"
                         "f44f5e4246391f482b2f5296e3602eb3"
                         "4aa136427710f7c0416d403fd69d4b29"
                         "130cfebef34e885abdb1a8a0a5f0e9b5"
                         "c33e1fc3bfc285b1ae17e40cc67a1913"
                         "dd563719815ebaf8514c2a7aa0018e63"
                         "b6c631dc315a46235716423d11ff5803"
                         "4e610645703606919f5c7ce2660cd148"
                         "bd9efc123d9c54b6705590d006cfcf3f"));
    vector<uint8_t> field_q =
        encode(0x02, wvcdm::a2b_hex(
                         "00"
                         "e9d49841e0e0a6ad0d517857133e36dc"
                         "72c1bdd90f9174b52e26570f373640f1"
                         "c185e7ea8e2ed7f1e4ebb951f70a5802"
                         "3633b0097aec67c6dcb800fc1a67f9bb"
                         "0563610f08ebc8746ad129772136eb1d"
                         "daf46436450d318332a84982fe5d28db"
                         "e5b3e912407c3e0e03100d87d436ee40"
                         "9eec1cf85e80aba079b2e6106b97bced"));
    vector<uint8_t> field_exp1 =
        encode(0x02, wvcdm::a2b_hex(
                         "00"
                         "ed102acdb26871534d1c414ecad9a4d7"
                         "32fe95b10eea370da62f05de2c393b1a"
                         "633303ea741b6b3269c97f704b352702"
                         "c9ae79922f7be8d10db67f026a8145de"
                         "41b30c0a42bf923bac5f7504c248604b"
                         "9faa57ed6b3246c6ba158e36c644f8b9"
                         "548fcf4f07e054a56f768674054440bc"
                         "0dcbbc9b528f64a01706e05b0b91106f"));
    vector<uint8_t> field_exp2 =
        encode(0x02, wvcdm::a2b_hex(
                         "00"
                         "6827924a85e88b55ba00f8219128bd37"
                         "24c6b7d1dfe5629ef197925fecaff5ed"
                         "b9cdf3a7befd8ea2e8dd3707138b3ff8"
                         "7c3c39c57f439e562e2aa805a39d7cd7"
                         "9966d2ece7845f1dbc16bee99999e4d0"
                         "bf9eeca45fcda8a8500035fe6b5f03bc"
                         "2f6d1bfc4d4d0a3723961af0cdce4a01"
                         "eec82d7f5458ec19e71b90eeef7dff61"));
    vector<uint8_t> field_invq =
        encode(0x02, wvcdm::a2b_hex(
                         "00"
                         "57b73888d183a99a6307422277551a3d"
                         "9e18adf06a91e8b55ceffef9077c8496"
                         "948ecb3b16b78155cb2a3a57c119d379"
                         "951c010aa635edcf62d84c5a122a8d67"
                         "ab5fa9e5a4a8772a1e943bafc70ae3a4"
                         "c1f0f3a4ddffaefd1892c8cb33bb0d0b"
                         "9590e963a69110fb34db7b906fc4ba28"
                         "36995aac7e527490ac952a02268a4f18"));

    // Header of rsa key is constant.
    encoded_key_ = wvcdm::a2b_hex(
        //  0x02 0x01 0x00 == integer, size 1 byte, value = 0 (field=version)
        "020100"
        //  0x30, sequence, size = d = 13 (field=pkeyalg)  AlgorithmIdentifier
        "300d"
        // 0x06 = object identifier. length = 9
        //  (this should be 1.2.840.113549.1.1.1) (field=algorithm)
        "0609"
        "2a"    // 1*0x40 + 2 = 42 = 0x2a.
        "8648"  // 840 = 0x348,  0x03 *2 + 0x80 + (0x48>>15) = 0x86.
        // 0x48 -> 0x48
        "86f70d"  // 113549 = 0x1668d -> (110 , 1110111, 0001101)
        // -> (0x80+0x06, 0x80+0x77, 0x0d)
        "01"  // 1
        "01"  // 1
        "01"  // 1
        "05"  // null object.  (field=parameter?)
        "00"  // size of null object
        );

    vector<uint8_t> pkey = wvcdm::a2b_hex("020100");  // integer, version = 0.
    pkey = concat(pkey, field_n);
    pkey = concat(pkey, field_e);
    pkey = concat(pkey, field_d);
    pkey = concat(pkey, field_p);
    pkey = concat(pkey, field_q);
    pkey = concat(pkey, field_exp1);
    pkey = concat(pkey, field_exp2);
    pkey = concat(pkey, field_invq);
    pkey = encode(0x30, pkey);
    pkey = encode(0x04, pkey);

    encoded_key_ = concat(encoded_key_, pkey);
    encoded_key_ = encode(0x30, encoded_key_);  // 0x30=sequence
  }

  void DisallowForbiddenPadding(RSA_Padding_Scheme scheme, size_t size) {
    OEMCryptoResult sts;
    Session s;
    s.open();
    sts = OEMCrypto_LoadDeviceRSAKey(s.session_id(), &wrapped_rsa_key_[0],
                                     wrapped_rsa_key_.size());
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);

    // Sign a Message
    vector<uint8_t> licenseRequest(size);
    OEMCrypto_GetRandom(&licenseRequest[0], licenseRequest.size());
    size_t signature_length = 256;
    uint8_t* signature = new uint8_t[signature_length];
    memset(signature, 0, signature_length);
    sts = OEMCrypto_GenerateRSASignature(s.session_id(), &licenseRequest[0],
                                         licenseRequest.size(), signature,
                                         &signature_length, scheme);
    // Allow OEMCrypto to request a full buffer.
    if (sts == OEMCrypto_ERROR_SHORT_BUFFER) {
      ASSERT_NE(static_cast<size_t>(0), signature_length);
      delete[] signature;
      signature = new uint8_t[signature_length];
      memset(signature, 0, signature_length);
      sts = OEMCrypto_GenerateRSASignature(s.session_id(), &licenseRequest[0],
                                           licenseRequest.size(), signature,
                                           &signature_length, scheme);
    }

    ASSERT_NE(OEMCrypto_SUCCESS, sts)
        << "Signed with forbidden padding scheme=" << (int)scheme
        << ", size=" << (int)size;
    ASSERT_EQ(signature[0], 0);  // signature should not be computed.
    ASSERT_EQ(memcmp(signature, signature + 1, signature_length - 1), 0);
    delete[] signature;
  }

  // This is used to test a signature from the file pkcs1v15sign-vectors.txt.
  void TestSignature(RSA_Padding_Scheme scheme, const vector<uint8_t>& message,
                     const vector<uint8_t>& correct_signature) {
    OEMCryptoResult sts;
    Session s;
    s.open();
    sts = OEMCrypto_LoadDeviceRSAKey(s.session_id(), &wrapped_rsa_key_[0],
                                     wrapped_rsa_key_.size());
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);

    // The application will compute the SHA-1 Hash of the message, so this
    // test must do that also.
    uint8_t hash[SHA_DIGEST_LENGTH];
    if (!SHA1(&message[0], message.size(), hash)) {
      dump_openssl_error();
      ASSERT_TRUE(false) << "openssl error creating SHA1 hash.";
    }

    // The application will prepend the digest info to the hash.
    // SHA-1 digest info prefix = 0x30 0x21 0x30 ...
    vector<uint8_t> digest = wvcdm::a2b_hex("3021300906052b0e03021a05000414");
    digest.insert(digest.end(), hash, hash + SHA_DIGEST_LENGTH);

    // OEMCrypto will apply the padding, and encrypt to generate the signature.
    size_t signature_length = 0;
    sts = OEMCrypto_GenerateRSASignature(s.session_id(), &digest[0],
                                         digest.size(), NULL, &signature_length,
                                         scheme);
    ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
    ASSERT_NE(static_cast<size_t>(0), signature_length);

    uint8_t* signature = new uint8_t[signature_length];
    sts = OEMCrypto_GenerateRSASignature(s.session_id(), &digest[0],
                                         digest.size(), signature,
                                         &signature_length, scheme);

    ASSERT_EQ(OEMCrypto_SUCCESS, sts)
        << "Failed to sign with padding scheme=" << (int)scheme
        << ", size=" << (int)message.size();
    s.PreparePublicKey(&encoded_key_[0], encoded_key_.size());

    // Verify that the signature matches the official test vector.
    ASSERT_EQ(correct_signature.size(), signature_length);
    ASSERT_EQ(0, memcmp(&correct_signature[0], signature, signature_length));

    // Also verify that our verification algorithm agrees.  This is not needed
    // to test OEMCrypto, but it does verify that this test is valid.
    s.VerifyRSASignature(&digest[0], digest.size(), signature, signature_length,
                         scheme);
    s.VerifyRSASignature(&digest[0], digest.size(), &correct_signature[0],
                         correct_signature.size(), scheme);
    delete[] signature;
  }

  void LoadWithAllowedSchemes(uint32_t schemes, bool force) {
    BuildRSAKey();
    InstallKeybox(kDefaultKeybox, true);
    CreateWrappedRSAKey(&wrapped_rsa_key_, schemes, force, &encoded_key_[0],
                        encoded_key_.size());
    key_loaded_ = (wrapped_rsa_key_.size() > 0);
  }

  std::vector<uint8_t> encoded_key_;
  std::vector<uint8_t> wrapped_rsa_key_;
  bool key_loaded_;
};

// # PKCS#1 v1.5 Signature Example 15.1
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_1) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "f45d55f35551e975d6a8dc7ea9f48859"
        "3940cc75694a278f27e578a163d839b3"
        "4040841808cf9c58c9b8728bf5f9ce8e"
        "e811ea91714f47bab92d0f6d5a26fcfe"
        "ea6cd93b910c0a2c963e64eb1823f102"
        "753d41f0335910ad3a977104f1aaf6c3"
        "742716a9755d11b8eed690477f445c5d"
        "27208b2e284330fa3d301423fa7f2d08"
        "6e0ad0b892b9db544e456d3f0dab85d9"
        "53c12d340aa873eda727c8a649db7fa6"
        "3740e25e9af1533b307e61329993110e"
        "95194e039399c3824d24c51f22b26bde"
        "1024cd395958a2dfeb4816a6e8adedb5"
        "0b1f6b56d0b3060ff0f1c4cb0d0e001d"
        "d59d73be12");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "b75a5466b65d0f300ef53833f2175c8a"
        "347a3804fc63451dc902f0b71f908345"
        "9ed37a5179a3b723a53f1051642d7737"
        "4c4c6c8dbb1ca20525f5c9f32db77695"
        "3556da31290e22197482ceb69906c46a"
        "758fb0e7409ba801077d2a0a20eae7d1"
        "d6d392ab4957e86b76f0652d68b83988"
        "a78f26e11172ea609bf849fbbd78ad7e"
        "dce21de662a081368c040607cee29db0"
        "627227f44963ad171d2293b633a392e3"
        "31dca54fe3082752f43f63c161b447a4"
        "c65a6875670d5f6600fcc860a1caeb0a"
        "88f8fdec4e564398a5c46c87f68ce070"
        "01f6213abe0ab5625f87d19025f08d81"
        "dac7bd4586bc9382191f6d2880f6227e"
        "5df3eed21e7792d249480487f3655261");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}
// # PKCS#1 v1.5 Signature Example 15.2
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_2) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "c14b4c6075b2f9aad661def4ecfd3cb9"
        "33c623f4e63bf53410d2f016d1ab98e2"
        "729eccf8006cd8e08050737d95fdbf29"
        "6b66f5b9792a902936c4f7ac69f51453"
        "ce4369452dc22d96f037748114662000"
        "dd9cd3a5e179f4e0f81fa6a0311ca1ae"
        "e6519a0f63cec78d27bb726393fb7f1f"
        "88cde7c97f8a66cd66301281dac3f3a4"
        "33248c75d6c2dcd708b6a97b0a3f325e"
        "0b2964f8a5819e479b");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "afa7343462bea122cc149fca70abdae7"
        "9446677db5373666af7dc313015f4de7"
        "86e6e394946fad3cc0e2b02bedba5047"
        "fe9e2d7d099705e4a39f28683279cf0a"
        "c85c1530412242c0e918953be000e939"
        "cf3bf182525e199370fa7907eba69d5d"
        "b4631017c0e36df70379b5db8d4c695a"
        "979a8e6173224065d7dc15132ef28cd8"
        "22795163063b54c651141be86d36e367"
        "35bc61f31fca574e5309f3a3bbdf91ef"
        "f12b99e9cc1744f1ee9a1bd22c5bad96"
        "ad481929251f0343fd36bcf0acde7f11"
        "e5ad60977721202796fe061f9ada1fc4"
        "c8e00d6022a8357585ffe9fdd59331a2"
        "8c4aa3121588fb6cf68396d8ac054659"
        "9500c9708500a5972bd54f72cf8db0c8");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.3
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_3) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "d02371ad7ee48bbfdb2763de7a843b94"
        "08ce5eb5abf847ca3d735986df84e906"
        "0bdbcdd3a55ba55dde20d4761e1a21d2"
        "25c1a186f4ac4b3019d3adf78fe63346"
        "67f56f70c901a0a2700c6f0d56add719"
        "592dc88f6d2306c7009f6e7a635b4cb3"
        "a502dfe68ddc58d03be10a1170004fe7"
        "4dd3e46b82591ff75414f0c4a03e605e"
        "20524f2416f12eca589f111b75d639c6"
        "1baa80cafd05cf3500244a219ed9ced9"
        "f0b10297182b653b526f400f2953ba21"
        "4d5bcd47884132872ae90d4d6b1f4215"
        "39f9f34662a56dc0e7b4b923b6231e30"
        "d2676797817f7c337b5ac824ba93143b"
        "3381fa3dce0e6aebd38e67735187b1eb"
        "d95c02");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "3bac63f86e3b70271203106b9c79aabd"
        "9f477c56e4ee58a4fce5baf2cab4960f"
        "88391c9c23698be75c99aedf9e1abf17"
        "05be1dac33140adb48eb31f450bb9efe"
        "83b7b90db7f1576d33f40c1cba4b8d6b"
        "1d3323564b0f1774114fa7c08e6d1e20"
        "dd8fbba9b6ac7ad41e26b4568f4a8aac"
        "bfd178a8f8d2c9d5f5b88112935a8bc9"
        "ae32cda40b8d20375510735096536818"
        "ce2b2db71a9772c9b0dda09ae10152fa"
        "11466218d091b53d92543061b7294a55"
        "be82ff35d5c32fa233f05aaac7585030"
        "7ecf81383c111674397b1a1b9d3bf761"
        "2ccbe5bacd2b38f0a98397b24c83658f"
        "b6c0b4140ef11970c4630d44344e76ea"
        "ed74dcbee811dbf6575941f08a6523b8");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
};

// # PKCS#1 v1.5 Signature Example 15.4
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_4) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "29035584ab7e0226a9ec4b02e8dcf127"
        "2dc9a41d73e2820007b0f6e21feccd5b"
        "d9dbb9ef88cd6758769ee1f956da7ad1"
        "8441de6fab8386dbc693");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "28d8e3fcd5dddb21ffbd8df1630d7377"
        "aa2651e14cad1c0e43ccc52f907f946d"
        "66de7254e27a6c190eb022ee89ecf622"
        "4b097b71068cd60728a1aed64b80e545"
        "7bd3106dd91706c937c9795f2b36367f"
        "f153dc2519a8db9bdf2c807430c451de"
        "17bbcd0ce782b3e8f1024d90624dea7f"
        "1eedc7420b7e7caa6577cef43141a726"
        "4206580e44a167df5e41eea0e69a8054"
        "54c40eefc13f48e423d7a32d02ed42c0"
        "ab03d0a7cf70c5860ac92e03ee005b60"
        "ff3503424b98cc894568c7c56a023355"
        "1cebe588cf8b0167b7df13adcad82867"
        "6810499c704da7ae23414d69e3c0d2db"
        "5dcbc2613bc120421f9e3653c5a87672"
        "97643c7e0740de016355453d6c95ae72");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.5
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_5) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex("bda3a1c79059eae598308d3df609");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "a156176cb96777c7fb96105dbd913bc4"
        "f74054f6807c6008a1a956ea92c1f81c"
        "b897dc4b92ef9f4e40668dc7c556901a"
        "cb6cf269fe615b0fb72b30a513386923"
        "14b0e5878a88c2c7774bd16939b5abd8"
        "2b4429d67bd7ac8e5ea7fe924e20a6ec"
        "662291f2548d734f6634868b039aa5f9"
        "d4d906b2d0cb8585bf428547afc91c6e"
        "2052ddcd001c3ef8c8eefc3b6b2a82b6"
        "f9c88c56f2e2c3cb0be4b80da95eba37"
        "1d8b5f60f92538743ddbb5da2972c71f"
        "e7b9f1b790268a0e770fc5eb4d5dd852"
        "47d48ae2ec3f26255a3985520206a1f2"
        "68e483e9dbb1d5cab190917606de31e7"
        "c5182d8f151bf41dfeccaed7cde690b2"
        "1647106b490c729d54a8fe2802a6d126");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.6
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_6) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "c187915e4e87da81c08ed4356a0cceac"
        "1c4fb5c046b45281b387ec28f1abfd56"
        "7e546b236b37d01ae71d3b2834365d3d"
        "f380b75061b736b0130b070be58ae8a4"
        "6d12166361b613dbc47dfaeb4ca74645"
        "6c2e888385525cca9dd1c3c7a9ada76d"
        "6c");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "9cab74163608669f7555a333cf196fe3"
        "a0e9e5eb1a32d34bb5c85ff689aaab0e"
        "3e65668ed3b1153f94eb3d8be379b8ee"
        "f007c4a02c7071ce30d8bb341e58c620"
        "f73d37b4ecbf48be294f6c9e0ecb5e63"
        "fec41f120e5553dfa0ebebbb72640a95"
        "37badcb451330229d9f710f62e3ed8ec"
        "784e50ee1d9262b42671340011d7d098"
        "c6f2557b2131fa9bd0254636597e88ec"
        "b35a240ef0fd85957124df8080fee1e1"
        "49af939989e86b26c85a5881fae8673d"
        "9fd40800dd134eb9bdb6410f420b0aa9"
        "7b20efcf2eb0c807faeb83a3ccd9b51d"
        "4553e41dfc0df6ca80a1e81dc234bb83"
        "89dd195a38b42de4edc49d346478b9f1"
        "1f0557205f5b0bd7ffe9c850f396d7c4");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.7
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_7) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "abfa2ecb7d29bd5bcb9931ce2bad2f74"
        "383e95683cee11022f08e8e7d0b8fa05"
        "8bf9eb7eb5f98868b5bb1fb5c31ceda3"
        "a64f1a12cdf20fcd0e5a246d7a1773d8"
        "dba0e3b277545babe58f2b96e3f4edc1"
        "8eabf5cd2a560fca75fe96e07d859def"
        "b2564f3a34f16f11e91b3a717b41af53"
        "f6605323001aa406c6");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "c4b437bcf703f352e1faf74eb9622039"
        "426b5672caf2a7b381c6c4f0191e7e4a"
        "98f0eebcd6f41784c2537ff0f99e7498"
        "2c87201bfbc65eae832db71d16dacadb"
        "0977e5c504679e40be0f9db06ffd848d"
        "d2e5c38a7ec021e7f68c47dfd38cc354"
        "493d5339b4595a5bf31e3f8f13816807"
        "373df6ad0dc7e731e51ad19eb4754b13"
        "4485842fe709d378444d8e36b1724a4f"
        "da21cafee653ab80747f7952ee804dea"
        "b1039d84139945bbf4be82008753f3c5"
        "4c7821a1d241f42179c794ef7042bbf9"
        "955656222e45c34369a384697b6ae742"
        "e18fa5ca7abad27d9fe71052e3310d0f"
        "52c8d12ea33bf053a300f4afc4f098df"
        "4e6d886779d64594d369158fdbc1f694");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.8
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_8) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "df4044a89a83e9fcbf1262540ae3038b"
        "bc90f2b2628bf2a4467ac67722d8546b"
        "3a71cb0ea41669d5b4d61859c1b4e47c"
        "ecc5933f757ec86db0644e311812d00f"
        "b802f03400639c0e364dae5aebc5791b"
        "c655762361bc43c53d3c7886768f7968"
        "c1c544c6f79f7be820c7e2bd2f9d73e6"
        "2ded6d2e937e6a6daef90ee37a1a52a5"
        "4f00e31addd64894cf4c02e16099e29f"
        "9eb7f1a7bb7f84c47a2b594813be02a1"
        "7b7fc43b34c22c91925264126c89f86b"
        "b4d87f3ef131296c53a308e0331dac8b"
        "af3b63422266ecef2b90781535dbda41"
        "cbd0cf22a8cbfb532ec68fc6afb2ac06");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "1414b38567ae6d973ede4a06842dcc0e"
        "0559b19e65a4889bdbabd0fd02806829"
        "13bacd5dc2f01b30bb19eb810b7d9ded"
        "32b284f147bbe771c930c6052aa73413"
        "90a849f81da9cd11e5eccf246dbae95f"
        "a95828e9ae0ca3550325326deef9f495"
        "30ba441bed4ac29c029c9a2736b1a419"
        "0b85084ad150426b46d7f85bd702f48d"
        "ac5f71330bc423a766c65cc1dcab20d3"
        "d3bba72b63b3ef8244d42f157cb7e3a8"
        "ba5c05272c64cc1ad21a13493c3911f6"
        "0b4e9f4ecc9900eb056ee59d6fe4b8ff"
        "6e8048ccc0f38f2836fd3dfe91bf4a38"
        "6e1ecc2c32839f0ca4d1b27a568fa940"
        "dd64ad16bd0125d0348e383085f08894"
        "861ca18987227d37b42b584a8357cb04");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.9
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_9) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "ea941ff06f86c226927fcf0e3b11b087"
        "2676170c1bfc33bda8e265c77771f9d0"
        "850164a5eecbcc5ce827fbfa07c85214"
        "796d8127e8caa81894ea61ceb1449e72"
        "fea0a4c943b2da6d9b105fe053b9039a"
        "9cc53d420b7539fab2239c6b51d17e69"
        "4c957d4b0f0984461879a0759c4401be"
        "ecd4c606a0afbd7a076f50a2dfc2807f"
        "24f1919baa7746d3a64e268ed3f5f8e6"
        "da83a2a5c9152f837cb07812bd5ba7d3"
        "a07985de88113c1796e9b466ec299c5a"
        "c1059e27f09415");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "ceeb84ccb4e9099265650721eea0e8ec"
        "89ca25bd354d4f64564967be9d4b08b3"
        "f1c018539c9d371cf8961f2291fbe0dc"
        "2f2f95fea47b639f1e12f4bc381cef0c"
        "2b7a7b95c3adf27605b7f63998c3cbad"
        "542808c3822e064d4ad14093679e6e01"
        "418a6d5c059684cd56e34ed65ab605b8"
        "de4fcfa640474a54a8251bbb7326a42d"
        "08585cfcfc956769b15b6d7fdf7da84f"
        "81976eaa41d692380ff10eaecfe0a579"
        "682909b5521fade854d797b8a0345b9a"
        "864e0588f6caddbf65f177998e180d1f"
        "102443e6dca53a94823caa9c3b35f322"
        "583c703af67476159ec7ec93d1769b30"
        "0af0e7157dc298c6cd2dee2262f8cddc"
        "10f11e01741471bbfd6518a175734575");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.10
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_10) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "d8b81645c13cd7ecf5d00ed2c91b9acd"
        "46c15568e5303c4a9775ede76b48403d"
        "6be56c05b6b1cf77c6e75de096c5cb35"
        "51cb6fa964f3c879cf589d28e1da2f9d"
        "ec");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "2745074ca97175d992e2b44791c323c5"
        "7167165cdd8da579cdef4686b9bb404b"
        "d36a56504eb1fd770f60bfa188a7b24b"
        "0c91e881c24e35b04dc4dd4ce38566bc"
        "c9ce54f49a175fc9d0b22522d9579047"
        "f9ed42eca83f764a10163997947e7d2b"
        "52ff08980e7e7c2257937b23f3d279d4"
        "cd17d6f495546373d983d536efd7d1b6"
        "7181ca2cb50ac616c5c7abfbb9260b91"
        "b1a38e47242001ff452f8de10ca6eaea"
        "dcaf9edc28956f28a711291fc9a80878"
        "b8ba4cfe25b8281cb80bc9cd6d2bd182"
        "5246eebe252d9957ef93707352084e6d"
        "36d423551bf266a85340fb4a6af37088"
        "0aab07153d01f48d086df0bfbec05e7b"
        "443b97e71718970e2f4bf62023e95b67");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.11
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_11) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "e5739b6c14c92d510d95b826933337ff"
        "0d24ef721ac4ef64c2bad264be8b44ef"
        "a1516e08a27eb6b611d3301df0062dae"
        "fc73a8c0d92e2c521facbc7b26473876"
        "7ea6fc97d588a0baf6ce50adf79e600b"
        "d29e345fcb1dba71ac5c0289023fe4a8"
        "2b46a5407719197d2e958e3531fd54ae"
        "f903aabb4355f88318994ed3c3dd62f4"
        "20a7");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "be40a5fb94f113e1b3eff6b6a33986f2"
        "02e363f07483b792e68dfa5554df0466"
        "cc32150950783b4d968b639a04fd2fb9"
        "7f6eb967021f5adccb9fca95acc8f2cd"
        "885a380b0a4e82bc760764dbab88c1e6"
        "c0255caa94f232199d6f597cc9145b00"
        "e3d4ba346b559a8833ad1516ad5163f0"
        "16af6a59831c82ea13c8224d84d0765a"
        "9d12384da460a8531b4c407e04f4f350"
        "709eb9f08f5b220ffb45abf6b75d1579"
        "fd3f1eb55fc75b00af8ba3b087827fe9"
        "ae9fb4f6c5fa63031fe582852fe2834f"
        "9c89bff53e2552216bc7c1d4a3d5dc2b"
        "a6955cd9b17d1363e7fee8ed7629753f"
        "f3125edd48521ae3b9b03217f4496d0d"
        "8ede57acbc5bd4deae74a56f86671de2");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.12
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_12) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "7af42835917a88d6b3c6716ba2f5b0d5"
        "b20bd4e2e6e574e06af1eef7c81131be"
        "22bf8128b9cbc6ec00275ba80294a5d1"
        "172d0824a79e8fdd830183e4c00b9678"
        "2867b1227fea249aad32ffc5fe007bc5"
        "1f21792f728deda8b5708aa99cabab20"
        "a4aa783ed86f0f27b5d563f42e07158c"
        "ea72d097aa6887ec411dd012912a5e03"
        "2bbfa678507144bcc95f39b58be7bfd1"
        "759adb9a91fa1d6d8226a8343a8b849d"
        "ae76f7b98224d59e28f781f13ece605f"
        "84f6c90bae5f8cf378816f4020a7dda1"
        "bed90c92a23634d203fac3fcd86d68d3"
        "182a7d9ccabe7b0795f5c655e9acc4e3"
        "ec185140d10cef053464ab175c83bd83"
        "935e3dabaf3462eebe63d15f573d269a");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "4e78c5902b807914d12fa537ae6871c8"
        "6db8021e55d1adb8eb0ccf1b8f36ab7d"
        "ad1f682e947a627072f03e627371781d"
        "33221d174abe460dbd88560c22f69011"
        "6e2fbbe6e964363a3e5283bb5d946ef1"
        "c0047eba038c756c40be7923055809b0"
        "e9f34a03a58815ebdde767931f018f6f"
        "1878f2ef4f47dd374051dd48685ded6e"
        "fb3ea8021f44be1d7d149398f98ea9c0"
        "8d62888ebb56192d17747b6b8e170954"
        "31f125a8a8e9962aa31c285264e08fb2"
        "1aac336ce6c38aa375e42bc92ab0ab91"
        "038431e1f92c39d2af5ded7e43bc151e"
        "6ebea4c3e2583af3437e82c43c5e3b5b"
        "07cf0359683d2298e35948ed806c063c"
        "606ea178150b1efc15856934c7255cfe");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.13
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_13) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "ebaef3f9f23bdfe5fa6b8af4c208c189"
        "f2251bf32f5f137b9de4406378686b3f"
        "0721f62d24cb8688d6fc41a27cbae21d"
        "30e429feacc7111941c277");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "c48dbef507114f03c95fafbeb4df1bfa"
        "88e0184a33cc4f8a9a1035ff7f822a5e"
        "38cda18723915ff078244429e0f6081c"
        "14fd83331fa65c6ba7bb9a12dbf66223"
        "74cd0ca57de3774e2bd7ae823677d061"
        "d53ae9c4040d2da7ef7014f3bbdc95a3"
        "61a43855c8ce9b97ecabce174d926285"
        "142b534a3087f9f4ef74511ec742b0d5"
        "685603faf403b5072b985df46adf2d25"
        "29a02d40711e2190917052371b79b749"
        "b83abf0ae29486c3f2f62477b2bd362b"
        "039c013c0c5076ef520dbb405f42cee9"
        "5425c373a975e1cdd032c49622c85079"
        "b09e88dab2b13969ef7a723973781040"
        "459f57d5013638483de2d91cb3c490da"
        "81c46de6cd76ea8a0c8f6fe331712d24");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.14
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_14) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "c5a2711278761dfcdd4f0c99e6f5619d"
        "6c48b5d4c1a80982faa6b4cf1cf7a60f"
        "f327abef93c801429efde08640858146"
        "1056acc33f3d04f5ada21216cacd5fd1"
        "f9ed83203e0e2fe6138e3eae8424e591"
        "5a083f3f7ab76052c8be55ae882d6ec1"
        "482b1e45c5dae9f41015405327022ec3"
        "2f0ea2429763b255043b1958ee3cf6d6"
        "3983596eb385844f8528cc9a9865835d"
        "c5113c02b80d0fca68aa25e72bcaaeb3"
        "cf9d79d84f984fd417");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "6bd5257aa06611fb4660087cb4bc4a9e"
        "449159d31652bd980844daf3b1c7b353"
        "f8e56142f7ea9857433b18573b4deede"
        "818a93b0290297783f1a2f23cbc72797"
        "a672537f01f62484cd4162c3214b9ac6"
        "28224c5de01f32bb9b76b27354f2b151"
        "d0e8c4213e4615ad0bc71f515e300d6a"
        "64c6743411fffde8e5ff190e54923043"
        "126ecfc4c4539022668fb675f25c07e2"
        "0099ee315b98d6afec4b1a9a93dc3349"
        "6a15bd6fde1663a7d49b9f1e639d3866"
        "4b37a010b1f35e658682d9cd63e57de0"
        "f15e8bdd096558f07ec0caa218a8c06f"
        "4788453940287c9d34b6d40a3f09bf77"
        "99fe98ae4eb49f3ff41c5040a50cefc9"
        "bdf2394b749cf164480df1ab6880273b");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.15
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_15) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "9bf8aa253b872ea77a7e23476be26b23"
        "29578cf6ac9ea2805b357f6fc3ad130d"
        "baeb3d869a13cce7a808bbbbc969857e"
        "03945c7bb61df1b5c2589b8e046c2a5d"
        "7e4057b1a74f24c711216364288529ec"
        "9570f25197213be1f5c2e596f8bf8b2c"
        "f3cb38aa56ffe5e31df7395820e94ecf"
        "3b1189a965dcf9a9cb4298d3c88b2923"
        "c19fc6bc34aacecad4e0931a7c4e5d73"
        "dc86dfa798a8476d82463eefaa90a8a9"
        "192ab08b23088dd58e1280f7d72e4548"
        "396baac112252dd5c5346adb2004a2f7"
        "101ccc899cc7fafae8bbe295738896a5"
        "b2012285014ef6");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "27f7f4da9bd610106ef57d32383a448a"
        "8a6245c83dc1309c6d770d357ba89e73"
        "f2ad0832062eb0fe0ac915575bcd6b8b"
        "cadb4e2ba6fa9da73a59175152b2d4fe"
        "72b070c9b7379e50000e55e6c269f665"
        "8c937972797d3add69f130e34b85bdec"
        "9f3a9b392202d6f3e430d09caca82277"
        "59ab825f7012d2ff4b5b62c8504dbad8"
        "55c05edd5cab5a4cccdc67f01dd6517c"
        "7d41c43e2a4957aff19db6f18b17859a"
        "f0bc84ab67146ec1a4a60a17d7e05f8b"
        "4f9ced6ad10908d8d78f7fc88b76adc8"
        "290f87daf2a7be10ae408521395d54ed"
        "2556fb7661854a730ce3d82c71a8d493"
        "ec49a378ac8a3c74439f7cc555ba13f8"
        "59070890ee18ff658fa4d741969d70a5");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.16
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_16) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "32474830e2203754c8bf0681dc4f842a"
        "fe360930378616c108e833656e5640c8"
        "6856885bb05d1eb9438efede679263de"
        "07cb39553f6a25e006b0a52311a063ca"
        "088266d2564ff6490c46b5609818548f"
        "88764dad34a25e3a85d575023f0b9e66"
        "5048a03c350579a9d32446c7bb96cc92"
        "e065ab94d3c8952e8df68ef0d9fa456b"
        "3a06bb80e3bbc4b28e6a94b6d0ff7696"
        "a64efe05e735fea025d7bdbc4139f3a3"
        "b546075cba7efa947374d3f0ac80a68d"
        "765f5df6210bca069a2d88647af7ea04"
        "2dac690cb57378ec0777614fb8b65ff4"
        "53ca6b7dce6098451a2f8c0da9bfecf1"
        "fdf391bbaa4e2a91ca18a1121a7523a2"
        "abd42514f489e8");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "6917437257c22ccb5403290c3dee82d9"
        "cf7550b31bd31c51bd57bfd35d452ab4"
        "db7c4be6b2e25ac9a59a1d2a7feb627f"
        "0afd4976b3003cc9cffd8896505ec382"
        "f265104d4cf8c932fa9fe86e00870795"
        "9912389da4b2d6b369b36a5e72e29d24"
        "c9a98c9d31a3ab44e643e6941266a47a"
        "45e3446ce8776abe241a8f5fc6423b24"
        "b1ff250dc2c3a8172353561077e850a7"
        "69b25f0325dac88965a3b9b472c494e9"
        "5f719b4eac332caa7a65c7dfe46d9aa7"
        "e6e00f525f303dd63ab7919218901868"
        "f9337f8cd26aafe6f33b7fb2c98810af"
        "19f7fcb282ba1577912c1d368975fd5d"
        "440b86e10c199715fa0b6f4250b53373"
        "2d0befe1545150fc47b876de09b00a94");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.17
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_17) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "008e59505eafb550aae5e845584cebb0"
        "0b6de1733e9f95d42c882a5bbeb5ce1c"
        "57e119e7c0d4daca9f1ff7870217f7cf"
        "d8a6b373977cac9cab8e71e420");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "922503b673ee5f3e691e1ca85e9ff417"
        "3cf72b05ac2c131da5603593e3bc259c"
        "94c1f7d3a06a5b9891bf113fa39e59ff"
        "7c1ed6465e908049cb89e4e125cd37d2"
        "ffd9227a41b4a0a19c0a44fbbf3de55b"
        "ab802087a3bb8d4ff668ee6bbb8ad89e"
        "6857a79a9c72781990dfcf92cd519404"
        "c950f13d1143c3184f1d250c90e17ac6"
        "ce36163b9895627ad6ffec1422441f55"
        "e4499dba9be89546ae8bc63cca01dd08"
        "463ae7f1fce3d893996938778c1812e6"
        "74ad9c309c5acca3fde44e7dd8695993"
        "e9c1fa87acda99ece5c8499e468957ad"
        "66359bf12a51adbe78d3a213b449bf0b"
        "5f8d4d496acf03d3033b7ccd196bc22f"
        "68fb7bef4f697c5ea2b35062f48a36dd");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.18
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_18) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "6abc54cf8d1dff1f53b17d8160368878"
        "a8788cc6d22fa5c2258c88e660b09a89"
        "33f9f2c0504ddadc21f6e75e0b833beb"
        "555229dee656b9047b92f62e76b8ffcc"
        "60dab06b80");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "0b6daf42f7a862147e417493c2c401ef"
        "ae32636ab4cbd44192bbf5f195b50ae0"
        "96a475a1614f0a9fa8f7a026cb46c650"
        "6e518e33d83e56477a875aca8c7e714c"
        "e1bdbd61ef5d535239b33f2bfdd61771"
        "bab62776d78171a1423cea8731f82e60"
        "766d6454265620b15f5c5a584f55f95b"
        "802fe78c574ed5dacfc831f3cf2b0502"
        "c0b298f25ccf11f973b31f85e4744219"
        "85f3cff702df3946ef0a6605682111b2"
        "f55b1f8ab0d2ea3a683c69985ead93ed"
        "449ea48f0358ddf70802cb41de2fd83f"
        "3c808082d84936948e0c84a131b49278"
        "27460527bb5cd24bfab7b48e071b2417"
        "1930f99763272f9797bcb76f1d248157"
        "5558fcf260b1f0e554ebb3df3cfcb958");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.19
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_19) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "af2d78152cf10efe01d274f217b177f6"
        "b01b5e749f1567715da324859cd3dd88"
        "db848ec79f48dbba7b6f1d33111ef31b"
        "64899e7391c2bffd69f49025cf201fc5"
        "85dbd1542c1c778a2ce7a7ee108a309f"
        "eca26d133a5ffedc4e869dcd7656596a"
        "c8427ea3ef6e3fd78fe99d8ddc71d839"
        "f6786e0da6e786bd62b3a4f19b891a56"
        "157a554ec2a2b39e25a1d7c7d37321c7"
        "a1d946cf4fbe758d9276f08563449d67"
        "414a2c030f4251cfe2213d04a5410637"
        "87");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "209c61157857387b71e24bf3dd564145"
        "50503bec180ff53bdd9bac062a2d4995"
        "09bf991281b79527df9136615b7a6d9d"
        "b3a103b535e0202a2caca197a7b74e53"
        "56f3dd595b49acfd9d30049a98ca88f6"
        "25bca1d5f22a392d8a749efb6eed9b78"
        "21d3110ac0d244199ecb4aa3d735a83a"
        "2e8893c6bf8581383ccaee834635b7fa"
        "1faffa45b13d15c1da33af71e89303d6"
        "8090ff62ee615fdf5a84d120711da53c"
        "2889198ab38317a9734ab27d67924cea"
        "74156ff99bef9876bb5c339e93745283"
        "e1b34e072226b88045e017e9f05b2a8c"
        "416740258e223b2690027491732273f3"
        "229d9ef2b1b3807e321018920ad3e53d"
        "ae47e6d9395c184b93a374c671faa2ce");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

// # PKCS#1 v1.5 Signature Example 15.20
TEST_F(DISABLED_AlternateRSAKey, TestSignaturePKCS1_15_20) {
  LoadWithAllowedSchemes(kSign_PKCS1_Block1, false);
  if (key_loaded_) {
    vector<uint8_t> message = wvcdm::a2b_hex(
        "40ee992458d6f61486d25676a96dd2cb"
        "93a37f04b178482f2b186cf88215270d"
        "ba29d786d774b0c5e78c7f6e56a956e7"
        "f73950a2b0c0c10a08dbcd67e5b210bb"
        "21c58e2767d44f7dd4014e3966143bf7"
        "e3d66ff0c09be4c55f93b39994b8518d"
        "9c1d76d5b47374dea08f157d57d70634"
        "978f3856e0e5b481afbbdb5a3ac48d48"
        "4be92c93de229178354c2de526e9c65a"
        "31ede1ef68cb6398d7911684fec0babc"
        "3a781a66660783506974d0e14825101c"
        "3bfaea");
    vector<uint8_t> signature = wvcdm::a2b_hex(
        "927502b824afc42513ca6570de338b8a"
        "64c3a85eb828d3193624f27e8b1029c5"
        "5c119c9733b18f5849b3500918bcc005"
        "51d9a8fdf53a97749fa8dc480d6fe974"
        "2a5871f973926528972a1af49e3925b0"
        "adf14a842719b4a5a2d89fa9c0b6605d"
        "212bed1e6723b93406ad30e86829a5c7"
        "19b890b389306dc5506486ee2f36a8df"
        "e0a96af678c9cbd6aff397ca200e3edc"
        "1e36bd2f08b31d540c0cb282a9559e4a"
        "dd4fc9e6492eed0ccbd3a6982e5faa2d"
        "dd17be47417c80b4e5452d31f72401a0"
        "42325109544d954c01939079d409a5c3"
        "78d7512dfc2d2a71efcc3432a765d1c6"
        "a52cfce899cd79b15b4fc3723641ef6b"
        "d00acc10407e5df58dd1c3c5c559a506");
    TestSignature(kSign_PKCS1_Block1, message, signature);
  }
}

class DISABLED_GenericDRMTest : public DISABLED_TestKeybox {
 protected:
  static const size_t kBufferSize = 160;  // multiple of encryption block size.
  uint8_t clear_buffer_[kBufferSize];
  uint8_t encrypted_buffer_[kBufferSize];
  uint8_t iv_[wvcdm::KEY_IV_SIZE];

  void MakeFourKeys(Session* s, uint32_t duration = kDuration,
                    uint32_t control = 0, uint32_t nonce = 0,
                    const std::string& pst = "") {
    s->FillSimpleMessage(duration, control, nonce, pst);
    s->license().keys[0].control.control_bits |=
        htonl(wvoec_mock::kControlAllowEncrypt);
    s->license().keys[1].control.control_bits |=
        htonl(wvoec_mock::kControlAllowDecrypt);
    s->license().keys[2].control.control_bits |=
        htonl(wvoec_mock::kControlAllowSign);
    s->license().keys[3].control.control_bits |=
        htonl(wvoec_mock::kControlAllowVerify);

    s->license().keys[2].key_data_length = wvcdm::MAC_KEY_SIZE;
    s->license().keys[3].key_data_length = wvcdm::MAC_KEY_SIZE;

    for (size_t i = 0; i < kBufferSize; i++) {
      clear_buffer_[i] = 1 + i % 250;
    }
    for (size_t i = 0; i < wvcdm::KEY_IV_SIZE; i++) {
      iv_[i] = i;
    }
  }

  void EncryptBuffer(Session* s, unsigned int key_index,
                     const uint8_t* in_buffer, uint8_t* out_buffer) {
    AES_KEY aes_key;
    ASSERT_EQ(0, AES_set_encrypt_key(s->license().keys[key_index].key_data,
                                     AES_BLOCK_SIZE * 8, &aes_key));
    uint8_t iv_buffer[wvcdm::KEY_IV_SIZE];
    memcpy(iv_buffer, iv_, wvcdm::KEY_IV_SIZE);
    AES_cbc_encrypt(in_buffer, out_buffer, kBufferSize, &aes_key, iv_buffer,
                    AES_ENCRYPT);
  }

  // Sign the buffer with the specified key.
  void SignBuffer(Session* s, unsigned int key_index, const uint8_t* in_buffer,
                  uint8_t signature[SHA256_DIGEST_LENGTH]) {
    unsigned int md_len = SHA256_DIGEST_LENGTH;
    HMAC(EVP_sha256(), s->license().keys[key_index].key_data,
         wvcdm::MAC_KEY_SIZE, in_buffer, kBufferSize, signature, &md_len);
  }

  void BadEncrypt(unsigned int key_index, OEMCrypto_Algorithm algorithm,
                  size_t buffer_length) {
    OEMCryptoResult sts;
    InstallKeybox(kDefaultKeybox, true);
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    MakeFourKeys(&s);
    s.EncryptAndSign();
    s.LoadTestKeys();
    uint8_t expected_encrypted[kBufferSize];
    EncryptBuffer(&s, key_index, clear_buffer_, expected_encrypted);
    sts = OEMCrypto_SelectKey(
        s.session_id(), s.license().keys[key_index].key_id, kTestKeyIdLength);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    uint8_t encrypted[kBufferSize];
    sts = OEMCrypto_Generic_Encrypt(s.session_id(), clear_buffer_,
                                    buffer_length, iv_, algorithm, encrypted);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    ASSERT_NE(0, memcmp(encrypted, expected_encrypted, buffer_length));
  }

  void BadDecrypt(unsigned int key_index, OEMCrypto_Algorithm algorithm,
                  size_t buffer_length) {
    OEMCryptoResult sts;
    InstallKeybox(kDefaultKeybox, true);
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    MakeFourKeys(&s);
    s.EncryptAndSign();
    s.LoadTestKeys();
    uint8_t encrypted[kBufferSize];
    EncryptBuffer(&s, key_index, clear_buffer_, encrypted);
    sts = OEMCrypto_SelectKey(
        s.session_id(), s.license().keys[key_index].key_id, kTestKeyIdLength);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    uint8_t resultant[kBufferSize];
    sts = OEMCrypto_Generic_Decrypt(s.session_id(), encrypted, buffer_length,
                                    iv_, algorithm, resultant);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    ASSERT_NE(0, memcmp(clear_buffer_, resultant, buffer_length));
  }

  void BadSign(unsigned int key_index, OEMCrypto_Algorithm algorithm) {
    OEMCryptoResult sts;
    InstallKeybox(kDefaultKeybox, true);
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    MakeFourKeys(&s);
    s.EncryptAndSign();
    s.LoadTestKeys();
    uint8_t expected_signature[SHA256_DIGEST_LENGTH];
    SignBuffer(&s, key_index, clear_buffer_, expected_signature);

    sts = OEMCrypto_SelectKey(
        s.session_id(), s.license().keys[key_index].key_id, kTestKeyIdLength);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    size_t signature_length = (size_t)SHA256_DIGEST_LENGTH;
    uint8_t signature[SHA256_DIGEST_LENGTH];
    sts = OEMCrypto_Generic_Sign(s.session_id(), clear_buffer_, kBufferSize,
                                 algorithm, signature, &signature_length);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    ASSERT_NE(0, memcmp(signature, expected_signature, SHA256_DIGEST_LENGTH));
  }

  void BadVerify(unsigned int key_index, OEMCrypto_Algorithm algorithm,
                 size_t signature_size, bool alter_data) {
    OEMCryptoResult sts;
    InstallKeybox(kDefaultKeybox, true);
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    MakeFourKeys(&s);
    s.EncryptAndSign();
    s.LoadTestKeys();
    uint8_t signature[SHA256_DIGEST_LENGTH];
    SignBuffer(&s, key_index, clear_buffer_, signature);
    if (alter_data) {
      signature[0] ^= 42;
    }

    sts = OEMCrypto_SelectKey(
        s.session_id(), s.license().keys[key_index].key_id, kTestKeyIdLength);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    sts = OEMCrypto_Generic_Verify(s.session_id(), clear_buffer_, kBufferSize,
                                   algorithm, signature, signature_size);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
  }
};

TEST_F(DISABLED_GenericDRMTest, GenericKeyLoad) {
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s);
  s.EncryptAndSign();
  s.LoadTestKeys();
}

TEST_F(DISABLED_GenericDRMTest, GenericKeyEncrypt) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s);
  s.EncryptAndSign();
  s.LoadTestKeys();
  unsigned int key_index = 0;
  uint8_t expected_encrypted[kBufferSize];
  EncryptBuffer(&s, key_index, clear_buffer_, expected_encrypted);
  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  uint8_t encrypted[kBufferSize];
  sts =
      OEMCrypto_Generic_Encrypt(s.session_id(), clear_buffer_, kBufferSize, iv_,
                                OEMCrypto_AES_CBC_128_NO_PADDING, encrypted);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(encrypted, expected_encrypted, kBufferSize));
}

TEST_F(DISABLED_GenericDRMTest, GenericKeyBadEncrypt) {
  BadEncrypt(0, OEMCrypto_HMAC_SHA256, kBufferSize);
  BadEncrypt(0, OEMCrypto_AES_CBC_128_NO_PADDING, kBufferSize - 10);
  BadEncrypt(1, OEMCrypto_AES_CBC_128_NO_PADDING, kBufferSize);
  BadEncrypt(2, OEMCrypto_AES_CBC_128_NO_PADDING, kBufferSize);
  BadEncrypt(3, OEMCrypto_AES_CBC_128_NO_PADDING, kBufferSize);
}

TEST_F(DISABLED_GenericDRMTest, GenericKeyDecrypt) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s);
  s.EncryptAndSign();
  s.LoadTestKeys();
  unsigned int key_index = 1;
  uint8_t encrypted[kBufferSize];
  EncryptBuffer(&s, key_index, clear_buffer_, encrypted);
  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  uint8_t resultant[kBufferSize];
  sts = OEMCrypto_Generic_Decrypt(s.session_id(), encrypted, kBufferSize, iv_,
                                  OEMCrypto_AES_CBC_128_NO_PADDING, resultant);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(clear_buffer_, resultant, kBufferSize));
}

TEST_F(DISABLED_GenericDRMTest, GenericSecureToClear) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s);
  s.license().keys[1].control.control_bits |= htonl(
      wvoec_mock::kControlObserveDataPath | wvoec_mock::kControlDataPathSecure);
  s.EncryptAndSign();
  s.LoadTestKeys();
  unsigned int key_index = 1;
  uint8_t encrypted[kBufferSize];
  EncryptBuffer(&s, key_index, clear_buffer_, encrypted);
  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  uint8_t resultant[kBufferSize];
  sts = OEMCrypto_Generic_Decrypt(s.session_id(), encrypted, kBufferSize, iv_,
                                  OEMCrypto_AES_CBC_128_NO_PADDING, resultant);
  ASSERT_NE(OEMCrypto_SUCCESS, sts);
  ASSERT_NE(0, memcmp(clear_buffer_, resultant, kBufferSize));
}

TEST_F(DISABLED_GenericDRMTest, GenericKeyBadDecrypt) {
  BadDecrypt(1, OEMCrypto_HMAC_SHA256, kBufferSize);
  BadDecrypt(1, OEMCrypto_AES_CBC_128_NO_PADDING, kBufferSize - 10);
  BadDecrypt(0, OEMCrypto_AES_CBC_128_NO_PADDING, kBufferSize);
  BadDecrypt(2, OEMCrypto_AES_CBC_128_NO_PADDING, kBufferSize);
  BadDecrypt(3, OEMCrypto_AES_CBC_128_NO_PADDING, kBufferSize);
}

TEST_F(DISABLED_GenericDRMTest, GenericKeySign) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s);
  s.EncryptAndSign();
  s.LoadTestKeys();
  unsigned int key_index = 2;
  uint8_t expected_signature[SHA256_DIGEST_LENGTH];
  SignBuffer(&s, key_index, clear_buffer_, expected_signature);

  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  size_t gen_signature_length = 0;
  sts = OEMCrypto_Generic_Sign(s.session_id(), clear_buffer_, kBufferSize,
                               OEMCrypto_HMAC_SHA256, NULL,
                               &gen_signature_length);
  ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
  ASSERT_EQ(static_cast<size_t>(SHA256_DIGEST_LENGTH), gen_signature_length);
  uint8_t signature[SHA256_DIGEST_LENGTH];
  sts = OEMCrypto_Generic_Sign(s.session_id(), clear_buffer_, kBufferSize,
                               OEMCrypto_HMAC_SHA256, signature,
                               &gen_signature_length);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(signature, expected_signature, SHA256_DIGEST_LENGTH));
}

TEST_F(DISABLED_GenericDRMTest, GenericKeyBadSign) {
  BadSign(0, OEMCrypto_HMAC_SHA256);             // Can't sign with encrypt key.
  BadSign(1, OEMCrypto_HMAC_SHA256);             // Can't sign with decrypt key.
  BadSign(3, OEMCrypto_HMAC_SHA256);             // Can't sign with verify key.
  BadSign(2, OEMCrypto_AES_CBC_128_NO_PADDING);  // Bad signing algorithm.
}

TEST_F(DISABLED_GenericDRMTest, GenericKeyVerify) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s);
  s.EncryptAndSign();
  s.LoadTestKeys();
  unsigned int key_index = 3;
  uint8_t signature[SHA256_DIGEST_LENGTH];
  SignBuffer(&s, key_index, clear_buffer_, signature);

  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  sts = OEMCrypto_Generic_Verify(s.session_id(), clear_buffer_, kBufferSize,
                                 OEMCrypto_HMAC_SHA256, signature,
                                 SHA256_DIGEST_LENGTH);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
}

TEST_F(DISABLED_GenericDRMTest, GenericKeyBadVerify) {
  BadVerify(0, OEMCrypto_HMAC_SHA256, SHA256_DIGEST_LENGTH, false);
  BadVerify(1, OEMCrypto_HMAC_SHA256, SHA256_DIGEST_LENGTH, false);
  BadVerify(2, OEMCrypto_HMAC_SHA256, SHA256_DIGEST_LENGTH, false);
  BadVerify(3, OEMCrypto_HMAC_SHA256, SHA256_DIGEST_LENGTH, true);
  BadVerify(3, OEMCrypto_HMAC_SHA256, SHA256_DIGEST_LENGTH - 1, false);
  BadVerify(3, OEMCrypto_HMAC_SHA256, SHA256_DIGEST_LENGTH + 1, false);
  BadVerify(3, OEMCrypto_AES_CBC_128_NO_PADDING, SHA256_DIGEST_LENGTH, false);
}

TEST_F(DISABLED_GenericDRMTest, KeyDurationEncrypt) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s, kDuration);
  s.EncryptAndSign();
  s.LoadTestKeys();

  uint8_t expected_encrypted[kBufferSize];
  EncryptBuffer(&s, 0, clear_buffer_, expected_encrypted);
  unsigned int key_index = 0;
  uint8_t encrypted[kBufferSize];

  sleep(kShortSleep);  //  Should still be valid key.

  memset(encrypted, 0, kBufferSize);
  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  sts =
      OEMCrypto_Generic_Encrypt(s.session_id(), clear_buffer_, kBufferSize, iv_,
                                OEMCrypto_AES_CBC_128_NO_PADDING, encrypted);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(encrypted, expected_encrypted, kBufferSize));

  sleep(kLongSleep);  // Should be expired key.

  memset(encrypted, 0, kBufferSize);
  sts =
      OEMCrypto_Generic_Encrypt(s.session_id(), clear_buffer_, kBufferSize, iv_,
                                OEMCrypto_AES_CBC_128_NO_PADDING, encrypted);
  ASSERT_EQ(OEMCrypto_ERROR_KEY_EXPIRED, sts);
  ASSERT_NE(0, memcmp(encrypted, expected_encrypted, kBufferSize));
}

TEST_F(DISABLED_GenericDRMTest, KeyDurationDecrypt) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s, kDuration);
  s.EncryptAndSign();
  s.LoadTestKeys();

  unsigned int key_index = 1;
  uint8_t encrypted[kBufferSize];
  EncryptBuffer(&s, key_index, clear_buffer_, encrypted);
  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  uint8_t resultant[kBufferSize];
  sleep(kShortSleep);  //  Should still be valid key.

  memset(resultant, 0, kBufferSize);
  sts = OEMCrypto_Generic_Decrypt(s.session_id(), encrypted, kBufferSize, iv_,
                                  OEMCrypto_AES_CBC_128_NO_PADDING, resultant);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(clear_buffer_, resultant, kBufferSize));

  sleep(kLongSleep);  // Should be expired key.

  memset(resultant, 0, kBufferSize);
  sts = OEMCrypto_Generic_Decrypt(s.session_id(), encrypted, kBufferSize, iv_,
                                  OEMCrypto_AES_CBC_128_NO_PADDING, resultant);
  ASSERT_EQ(OEMCrypto_ERROR_KEY_EXPIRED, sts);
  ASSERT_NE(0, memcmp(clear_buffer_, resultant, kBufferSize));
}

TEST_F(DISABLED_GenericDRMTest, KeyDurationSign) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s, kDuration);
  s.EncryptAndSign();
  s.LoadTestKeys();

  unsigned int key_index = 2;
  uint8_t expected_signature[SHA256_DIGEST_LENGTH];
  uint8_t signature[SHA256_DIGEST_LENGTH];
  size_t signature_length = SHA256_DIGEST_LENGTH;
  SignBuffer(&s, key_index, clear_buffer_, expected_signature);

  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  sleep(kShortSleep);  //  Should still be valid key.

  memset(signature, 0, SHA256_DIGEST_LENGTH);
  sts = OEMCrypto_Generic_Sign(s.session_id(), clear_buffer_, kBufferSize,
                               OEMCrypto_HMAC_SHA256, signature,
                               &signature_length);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  ASSERT_EQ(0, memcmp(signature, expected_signature, SHA256_DIGEST_LENGTH));

  sleep(kLongSleep);  // Should be expired key.

  memset(signature, 0, SHA256_DIGEST_LENGTH);
  sts = OEMCrypto_Generic_Sign(s.session_id(), clear_buffer_, kBufferSize,
                               OEMCrypto_HMAC_SHA256, signature,
                               &signature_length);
  ASSERT_EQ(OEMCrypto_ERROR_KEY_EXPIRED, sts);
  ASSERT_NE(0, memcmp(signature, expected_signature, SHA256_DIGEST_LENGTH));
}

TEST_F(DISABLED_GenericDRMTest, KeyDurationVerify) {
  OEMCryptoResult sts;
  InstallKeybox(kDefaultKeybox, true);
  Session s;
  s.open();
  s.GenerateDerivedKeys();
  MakeFourKeys(&s, kDuration);
  s.EncryptAndSign();
  s.LoadTestKeys();

  unsigned int key_index = 3;
  uint8_t signature[SHA256_DIGEST_LENGTH];
  SignBuffer(&s, key_index, clear_buffer_, signature);

  sts = OEMCrypto_SelectKey(s.session_id(), s.license().keys[key_index].key_id,
                            kTestKeyIdLength);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  sleep(kShortSleep);  //  Should still be valid key.

  sts = OEMCrypto_Generic_Verify(s.session_id(), clear_buffer_, kBufferSize,
                                 OEMCrypto_HMAC_SHA256, signature,
                                 SHA256_DIGEST_LENGTH);
  ASSERT_EQ(OEMCrypto_SUCCESS, sts);

  sleep(kLongSleep);  // Should be expired key.

  sts = OEMCrypto_Generic_Verify(s.session_id(), clear_buffer_, kBufferSize,
                                 OEMCrypto_HMAC_SHA256, signature,
                                 SHA256_DIGEST_LENGTH);
  ASSERT_EQ(OEMCrypto_ERROR_KEY_EXPIRED, sts);
}

TEST_F(OEMCryptoClientTest, SupportsUsageTable) {
  if (OEMCrypto_SupportsUsageTable()) {
    EXPECT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    cout << "             OEMCrypto supports usage tables." << endl;
  } else {
    cout << "             OEMCrypto does not support usage tables." << endl;
    EXPECT_FALSE(wvcdm::Properties::oem_crypto_require_usage_tables());
  }
}

TEST_F(OEMCryptoClientTest, PSTReportSizes) {
  if (OEMCrypto_SupportsUsageTable()) {
    OEMCrypto_PST_Report report;
    uint8_t* location = reinterpret_cast<uint8_t*>(&report);
    EXPECT_EQ(48, sizeof(report));
    uint8_t *field;
    field = reinterpret_cast<uint8_t *>(&report.status);
    EXPECT_EQ(20, field - location);
    field = reinterpret_cast<uint8_t *>(&report.clock_security_level);
    EXPECT_EQ(21, field - location);
    field = reinterpret_cast<uint8_t *>(&report.pst_length);
    EXPECT_EQ(22, field - location);
    field = reinterpret_cast<uint8_t *>(&report.seconds_since_license_received);
    EXPECT_EQ(24, field - location);
    field = reinterpret_cast<uint8_t *>(&report.seconds_since_first_decrypt);
    EXPECT_EQ(32, field - location);
    field = reinterpret_cast<uint8_t *>(&report.seconds_since_last_decrypt);
    EXPECT_EQ(40, field - location);
    field = reinterpret_cast<uint8_t *>(&report.pst);
    EXPECT_EQ(48, field - location);
  }
}

class DISABLED_UsageTableTest : public DISABLED_GenericDRMTest,
                                public WithParamInterface<bool> {
 public:
  virtual void SetUp() {
    DISABLED_GenericDRMTest::SetUp();
    new_mac_keys_ = GetParam();
  }

  void DeactivatePST(const std::string& pst) {
    OEMCryptoResult sts = OEMCrypto_DeactivateUsageEntry(
        reinterpret_cast<const uint8_t*>(pst.c_str()), pst.length());
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
  }

  void LoadOfflineLicense(Session& s, const std::string& pst) {
    s.open();
    s.GenerateDerivedKeys();
    s.FillSimpleMessage(0, wvoec_mock::kControlNonceOrEntry, s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    s.GenerateReport(pst);
    s.GenerateReport(pst);
    EXPECT_EQ(kUnused, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    s.close();
  }

 protected:
  bool new_mac_keys_;
};

TEST_P(DISABLED_UsageTableTest, OnlineLicense) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);

    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    s.GenerateReport(pst);
    s.GenerateReport(pst);
    s.GenerateReport(pst);
    s.GenerateReport(pst);
    EXPECT_EQ(kUnused, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    s.TestDecryptCTR();
    s.GenerateReport(pst);
    EXPECT_EQ(kActive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_first_decrypt));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_last_decrypt));
    DeactivatePST(pst);
    s.GenerateReport(pst);
    EXPECT_EQ(kInactive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    s.TestDecryptCTR(false, OEMCrypto_ERROR_UNKNOWN_FAILURE);
  }
}

TEST_P(DISABLED_UsageTableTest, RepeatOnlineLicense) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);

    s.close();
    Session s2;
    s2.open();
    s2.GenerateDerivedKeys();
    uint8_t* pst_ptr = s.encrypted_license().pst;
    // Trying to reuse a PST is bad. We use session ID for s2, everything else
    // reused from s.
    ASSERT_NE(OEMCrypto_SUCCESS,
              OEMCrypto_LoadKeys(s2.session_id(), s.message_ptr(),
                                 sizeof(MessageData), &s.signature()[0],
                                 s.signature().size(),
                                 s.encrypted_license().mac_key_iv,
                                 s.encrypted_license().mac_keys, kNumKeys,
                                 s.key_array(), pst_ptr, pst.length()));
    s2.close();
  }
}

// A license with non-zero replay control bits needs a valid pst..
TEST_P(DISABLED_UsageTableTest, OnlineEmptyPST) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce());
    s.EncryptAndSign();
    OEMCryptoResult sts = OEMCrypto_LoadKeys(
        s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
        s.signature().size(), s.encrypted_license().mac_key_iv,
        s.encrypted_license().mac_keys, kNumKeys, s.key_array(),
        NULL, 0);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    s.close();
  }
}

TEST_P(DISABLED_UsageTableTest, EmptyTable) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    std::string pst = "no_pst";
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    s.GenerateReport(pst);
    s.close();
    OEMCrypto_DeleteUsageTable();
    Session s2;
    s2.open();
    s2.GenerateReport(pst, false);
    s2.close();
  }
}

TEST_P(DISABLED_UsageTableTest, FiftyEntries) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s1;
    s1.open();
    s1.GenerateDerivedKeys();
    std::string pst1 = "pst saved";
    s1.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s1.get_nonce(), pst1);
    s1.EncryptAndSign();
    s1.LoadTestKeys(pst1, new_mac_keys_);
    sleep(kShortSleep);

    const size_t ENTRY_COUNT = 49;// API says should hold at least 50 entries.
    Session sessions[ENTRY_COUNT];
    for (int i=0; i<ENTRY_COUNT; i++) {
      sessions[i].open();
      sessions[i].GenerateDerivedKeys();
      std::string pst = "pst ";
      char c = 'A' + i;
      pst = pst + c;
      sessions[i].FillSimpleMessage(
          0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
          sessions[i].get_nonce(), pst);
      sessions[i].EncryptAndSign();
      sessions[i].LoadTestKeys(pst, new_mac_keys_);
      sessions[i].GenerateReport(pst);
      sessions[i].close();
    }
    for (int i=0; i<ENTRY_COUNT; i++) {
      Session s;
      s.open();
      std::string pst = "pst ";
      char c = 'A' + i;
      pst = pst + c;
      s.GenerateReport(pst, true, &sessions[i]);
      EXPECT_EQ(kUnused, s.pst_report()->status);
      s.close();
    }
    sleep(kShortSleep);
    // If I add too many entries, it can delete the older ones first, except
    // it shouldn't delete the one attached to an open session. (s1)
    for (int i=0; i<ENTRY_COUNT; i++) {
      sessions[i].open();
      sessions[i].GenerateDerivedKeys();
      std::string pst = "newer pst ";
      char c = 'A' + i;
      pst = pst + c;
      sessions[i].FillSimpleMessage(
          0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
          sessions[i].get_nonce(), pst);
      sessions[i].EncryptAndSign();
      sessions[i].LoadTestKeys(pst, new_mac_keys_);
      sessions[i].GenerateReport(pst);
      sessions[i].close();
    }
    for (int i=0; i<49; i++) {
      Session s;
      s.open();
      std::string pst = "newer pst ";
      char c = 'A' + i;
      pst = pst + c;
      s.GenerateReport(pst, true, &sessions[i]);
      EXPECT_EQ(kUnused, s.pst_report()->status);
      s.close();
    }
    s1.close();
    s1.open();  // Make sure s1's entry is still in the table.
    s1.GenerateReport(pst1);
    EXPECT_EQ(kUnused, s1.pst_report()->status);
    s1.close();
  }
}

TEST_P(DISABLED_UsageTableTest, DeleteUnusedEntry) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    std::string pst = "my pst";
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    s.GenerateReport(pst);
    s.close();

    // New session should be able to generate report and copy mac keys.
    Session s2;
    s2.open();
    s2.GenerateReport(pst, true, &s);
    EXPECT_EQ(kUnused, s2.pst_report()->status);
    s2.DeleteEntry(pst);
    s2.close();

    // Now that session is deleted, we can't generate a report for it.
    Session s3;
    s3.open();
    s3.GenerateReport(pst, false);
    s3.close();
  }
}

TEST_P(DISABLED_UsageTableTest, DeleteActiveEntry) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    std::string pst = "my pst";
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    s.TestDecryptCTR();
    s.GenerateReport(pst);
    s.close();

    // New session should be able to generate report and copy mac keys.
    Session s2;
    s2.open();
    s2.GenerateReport(pst, true, &s);
    EXPECT_EQ(kActive, s2.pst_report()->status);
    s2.DeleteEntry(pst);
    s2.close();

    // Now that session is deleted, we can't generate a report for it.
    Session s3;
    s3.open();
    s3.GenerateReport(pst, false);
    s3.close();
  }
}

TEST_P(DISABLED_UsageTableTest, DeleteInactiveEntry) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    std::string pst = "my pst";
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    s.GenerateReport(pst);
    s.TestDecryptCTR();
    DeactivatePST(pst);
    s.close();

    // New session should be able to generate report and copy mac keys.
    Session s2;
    s2.open();
    s2.GenerateReport(pst, true, &s);
    EXPECT_EQ(kInactive, s2.pst_report()->status);
    s2.DeleteEntry(pst);
    s2.close();

    // Now that session is deleted, we can't generate a report for it.
    Session s3;
    s3.open();
    s3.GenerateReport(pst, false);
    s3.close();
  }
}

TEST_P(DISABLED_UsageTableTest, DeleteEntryBadSignature) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    std::string pst = "my pst";
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    s.GenerateReport(pst);
    s.close();

    // New session should be able to generate report and copy mac keys.
    Session s2;
    s2.open();
    s2.GenerateReport(pst, true, &s);
    uint8_t* pst_ptr = s2.encrypted_license().pst;
    memcpy(pst_ptr, pst.c_str(), min(sizeof(s2.license().pst), pst.length()));
    std::vector<uint8_t> signature(SHA256_DIGEST_LENGTH);
    // Cannot delete without correct signature.
    // ServerSignMessage(s2.encrypted_license(), &signature);
    ASSERT_NE(OEMCrypto_SUCCESS,
              OEMCrypto_DeleteUsageEntry(s2.session_id(), pst_ptr, pst.length(),
                                         s2.message_ptr(), sizeof(MessageData),
                                         &signature[0], signature.size()));
    s2.close();

    // The session is not deleted, we can still generate a report for it.
    Session s3;
    s3.open();
    s3.GenerateReport(pst, true, &s);
    EXPECT_EQ(kUnused, s3.pst_report()->status);
    s3.close();
  }
}

TEST_P(DISABLED_UsageTableTest, DeleteEntryWrongSession) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    std::string pst = "my pst";
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    s.GenerateReport(pst);
    s.close();

    // New session should not be able to delete without using GenerateReport
    // to load mac keys.
    Session s2;
    s2.open();
    // s2.GenerateReport(pst, true, &s);
    uint8_t* pst_ptr = s2.encrypted_license().pst;
    memcpy(pst_ptr, pst.c_str(), min(sizeof(s2.license().pst), pst.length()));
    std::vector<uint8_t> signature(SHA256_DIGEST_LENGTH);
    s2.ServerSignMessage(s2.encrypted_license(), &signature);
    ASSERT_NE(OEMCrypto_SUCCESS,
              OEMCrypto_DeleteUsageEntry(s2.session_id(), pst_ptr, pst.length(),
                                         s2.message_ptr(), sizeof(MessageData),
                                         &signature[0], signature.size()));
    s2.close();

    // The session is not deleted, we can still generate a report for it.
    Session s3;
    s3.open();
    s3.GenerateReport(pst, true, &s);
    EXPECT_EQ(kUnused, s3.pst_report()->status);
    s3.close();
  }
}

TEST_P(DISABLED_UsageTableTest, DeleteEntryBadRange) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    std::string pst = "my pst";
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceRequired,
        s.get_nonce(), pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    s.GenerateReport(pst);
    s.close();

    // New session should not be able to delete if pst doesn't point into
    // message.
    Session s2;
    s2.open();
    s2.GenerateReport(pst, true, &s);
    uint8_t* pst_ptr = s2.license().pst;
    memcpy(pst_ptr, pst.c_str(), min(sizeof(s2.license().pst), pst.length()));
    std::vector<uint8_t> signature(SHA256_DIGEST_LENGTH);
    s2.ServerSignMessage(s2.encrypted_license(), &signature);
    ASSERT_NE(OEMCrypto_SUCCESS,
              OEMCrypto_DeleteUsageEntry(s2.session_id(), pst_ptr, pst.length(),
                                         s2.message_ptr(), sizeof(MessageData),
                                         &signature[0], signature.size()));
    s2.close();

    // The session is not deleted, we can still generate a report for it.
    Session s3;
    s3.open();
    s3.GenerateReport(pst, true, &s);
    EXPECT_EQ(kUnused, s3.pst_report()->status);
    s3.close();
  }
}

TEST_P(DISABLED_UsageTableTest, DeactivateBadPST) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    std::string pst = "nonexistant pst";
    OEMCryptoResult sts = OEMCrypto_DeactivateUsageEntry(
        reinterpret_cast<const uint8_t*>(pst.c_str()), pst.length());
    EXPECT_EQ(OEMCrypto_ERROR_INVALID_CONTEXT, sts);
    std::string null_pst = "";
    sts = OEMCrypto_DeactivateUsageEntry(
        reinterpret_cast<const uint8_t*>(null_pst.c_str()), null_pst.length());
    EXPECT_EQ(OEMCrypto_ERROR_INVALID_CONTEXT, sts);
  }
}

TEST_P(DISABLED_UsageTableTest, GenericEncrypt) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "A PST";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    uint32_t nonce = s.get_nonce();
    MakeFourKeys(&s, 0, wvoec_mock::kControlNonceEnabled |
                            wvoec_mock::kControlNonceRequired,
                 nonce, pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    OEMCryptoResult sts;
    unsigned int key_index = 0;
    uint8_t expected_encrypted[kBufferSize];
    EncryptBuffer(&s, key_index, clear_buffer_, expected_encrypted);
    sts = OEMCrypto_SelectKey(
        s.session_id(), s.license().keys[key_index].key_id, kTestKeyIdLength);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    uint8_t encrypted[kBufferSize];
    sts = OEMCrypto_Generic_Encrypt(s.session_id(), clear_buffer_, kBufferSize,
                                    iv_, OEMCrypto_AES_CBC_128_NO_PADDING,
                                    encrypted);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    EXPECT_EQ(0, memcmp(encrypted, expected_encrypted, kBufferSize));
    s.GenerateReport(pst);
    EXPECT_EQ(kActive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_first_decrypt));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_last_decrypt));
    DeactivatePST(pst);
    s.GenerateReport(pst);
    EXPECT_EQ(kInactive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    memset(encrypted, 0, kBufferSize);
    sts = OEMCrypto_Generic_Encrypt(s.session_id(), clear_buffer_, kBufferSize,
                                    iv_, OEMCrypto_AES_CBC_128_NO_PADDING,
                                    encrypted);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    EXPECT_NE(0, memcmp(encrypted, expected_encrypted, kBufferSize));
  }
}

TEST_P(DISABLED_UsageTableTest, GenericDecrypt) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    uint32_t nonce = s.get_nonce();
    MakeFourKeys(&s, 0, wvoec_mock::kControlNonceEnabled |
                            wvoec_mock::kControlNonceRequired,
                 nonce, pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);
    OEMCryptoResult sts;
    unsigned int key_index = 1;
    uint8_t encrypted[kBufferSize];
    EncryptBuffer(&s, key_index, clear_buffer_, encrypted);
    sts = OEMCrypto_SelectKey(
        s.session_id(), s.license().keys[key_index].key_id, kTestKeyIdLength);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    uint8_t resultant[kBufferSize];
    sts =
        OEMCrypto_Generic_Decrypt(s.session_id(), encrypted, kBufferSize, iv_,
                                  OEMCrypto_AES_CBC_128_NO_PADDING, resultant);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    EXPECT_EQ(0, memcmp(clear_buffer_, resultant, kBufferSize));
    s.GenerateReport(pst);
    EXPECT_EQ(kActive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_first_decrypt));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_last_decrypt));
    DeactivatePST(pst);
    s.GenerateReport(pst);
    EXPECT_EQ(kInactive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    memset(resultant, 0, kBufferSize);
    sts =
        OEMCrypto_Generic_Decrypt(s.session_id(), encrypted, kBufferSize, iv_,
                                  OEMCrypto_AES_CBC_128_NO_PADDING, resultant);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    EXPECT_NE(0, memcmp(clear_buffer_, resultant, kBufferSize));
  }
}

TEST_P(DISABLED_UsageTableTest, GenericSign) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    uint32_t nonce = s.get_nonce();
    MakeFourKeys(&s, 0, wvoec_mock::kControlNonceEnabled |
                            wvoec_mock::kControlNonceRequired,
                 nonce, pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);

    OEMCryptoResult sts;
    unsigned int key_index = 2;
    uint8_t expected_signature[SHA256_DIGEST_LENGTH];
    SignBuffer(&s, key_index, clear_buffer_, expected_signature);

    sts = OEMCrypto_SelectKey(
        s.session_id(), s.license().keys[key_index].key_id, kTestKeyIdLength);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    size_t gen_signature_length = 0;
    sts = OEMCrypto_Generic_Sign(s.session_id(), clear_buffer_, kBufferSize,
                                 OEMCrypto_HMAC_SHA256, NULL,
                                 &gen_signature_length);
    ASSERT_EQ(OEMCrypto_ERROR_SHORT_BUFFER, sts);
    ASSERT_EQ(static_cast<size_t>(SHA256_DIGEST_LENGTH), gen_signature_length);
    uint8_t signature[SHA256_DIGEST_LENGTH];
    sts = OEMCrypto_Generic_Sign(s.session_id(), clear_buffer_, kBufferSize,
                                 OEMCrypto_HMAC_SHA256, signature,
                                 &gen_signature_length);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    ASSERT_EQ(0, memcmp(signature, expected_signature, SHA256_DIGEST_LENGTH));

    s.GenerateReport(pst);
    EXPECT_EQ(kActive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_first_decrypt));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_last_decrypt));
    DeactivatePST(pst);
    s.GenerateReport(pst);
    EXPECT_EQ(kInactive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));

    memset(signature, 0, SHA256_DIGEST_LENGTH);
    gen_signature_length = SHA256_DIGEST_LENGTH;
    sts = OEMCrypto_Generic_Sign(s.session_id(), clear_buffer_, kBufferSize,
                                 OEMCrypto_HMAC_SHA256, signature,
                                 &gen_signature_length);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    ASSERT_NE(0, memcmp(signature, expected_signature, SHA256_DIGEST_LENGTH));
  }
}

TEST_P(DISABLED_UsageTableTest, GenericVerify) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    uint32_t nonce = s.get_nonce();
    MakeFourKeys(&s, 0, wvoec_mock::kControlNonceEnabled |
                            wvoec_mock::kControlNonceRequired,
                 nonce, pst);
    s.EncryptAndSign();
    s.LoadTestKeys(pst, new_mac_keys_);

    OEMCryptoResult sts;
    unsigned int key_index = 3;
    uint8_t signature[SHA256_DIGEST_LENGTH];
    SignBuffer(&s, key_index, clear_buffer_, signature);

    sts = OEMCrypto_SelectKey(
        s.session_id(), s.license().keys[key_index].key_id, kTestKeyIdLength);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);
    sts = OEMCrypto_Generic_Verify(s.session_id(), clear_buffer_, kBufferSize,
                                   OEMCrypto_HMAC_SHA256, signature,
                                   SHA256_DIGEST_LENGTH);
    ASSERT_EQ(OEMCrypto_SUCCESS, sts);

    s.GenerateReport(pst);
    EXPECT_EQ(kActive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_first_decrypt));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_last_decrypt));
    DeactivatePST(pst);
    s.GenerateReport(pst);
    EXPECT_EQ(kInactive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));

    sts = OEMCrypto_Generic_Verify(s.session_id(), clear_buffer_, kBufferSize,
                                   OEMCrypto_HMAC_SHA256, signature,
                                   SHA256_DIGEST_LENGTH);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
  }
}

TEST_P(DISABLED_UsageTableTest, OfflineLicense) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    LoadOfflineLicense(s, pst);
  }
}

TEST_P(DISABLED_UsageTableTest, ReloadOfflineLicense) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    LoadOfflineLicense(s, pst);

    s.open();  // Offline license can be reused.
    s.GenerateDerivedKeys();
    // We will reuse the encrypted and signed message, so we don't call
    // FillSimpleMessage again.
    s.LoadTestKeys(pst, new_mac_keys_);
    s.GenerateReport(pst);
    s.GenerateReport(pst);
    EXPECT_EQ(kUnused, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    s.TestDecryptCTR();
    s.GenerateReport(pst);
    EXPECT_EQ(kActive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_first_decrypt));
    EXPECT_ALMOST(0,
                  wvcdm::htonll64(s.pst_report()->seconds_since_last_decrypt));
    s.close();
  }
}

TEST_P(DISABLED_UsageTableTest, BadReloadOfflineLicense) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    LoadOfflineLicense(s, pst);

    // Offline license with new mac keys should fail.
    Session s2;
    s2.open();
    s2.GenerateDerivedKeys();
    s2.FillSimpleMessage(0, wvoec_mock::kControlNonceOrEntry,
                         s2.get_nonce(), pst);
    s2.EncryptAndSign();
    uint8_t* pst_ptr = s2.encrypted_license().pst;
    ASSERT_NE(OEMCrypto_SUCCESS,
              OEMCrypto_LoadKeys(s2.session_id(), s2.message_ptr(),
                                 sizeof(MessageData), &s2.signature()[0],
                                 s2.signature().size(),
                                 s2.encrypted_license().mac_key_iv,
                                 s2.encrypted_license().mac_keys, kNumKeys,
                                 s2.key_array(), pst_ptr, pst.length()));
    s2.close();

    // Offline license with same mac keys should still be OK.
    s.open();
    s.GenerateDerivedKeys();
    s.LoadTestKeys(pst, new_mac_keys_);
    s.GenerateReport(pst);
    EXPECT_EQ(kUnused, s.pst_report()->status);
  }
}

// An offline license should not load on the first call if the nonce is bad.
TEST_P(DISABLED_UsageTableTest, OfflineBadNonce) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceOrEntry,
        42, pst);
    s.EncryptAndSign();
    uint8_t* pst_ptr = s.encrypted_license().pst;
    OEMCryptoResult sts = OEMCrypto_LoadKeys(
        s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
        s.signature().size(), s.encrypted_license().mac_key_iv,
        s.encrypted_license().mac_keys, kNumKeys, s.key_array(),
        pst_ptr, pst.length());
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    s.close();
  }
}

// An offline license needs a valid pst.
TEST_P(DISABLED_UsageTableTest, OfflineEmptyPST) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    s.FillSimpleMessage(
        0, wvoec_mock::kControlNonceEnabled | wvoec_mock::kControlNonceOrEntry,
        s.get_nonce());
    s.EncryptAndSign();
    OEMCryptoResult sts = OEMCrypto_LoadKeys(
        s.session_id(), s.message_ptr(), sizeof(MessageData), &s.signature()[0],
        s.signature().size(), s.encrypted_license().mac_key_iv,
        s.encrypted_license().mac_keys, kNumKeys, s.key_array(),
        NULL, 0);
    ASSERT_NE(OEMCrypto_SUCCESS, sts);
    s.close();
  }
}

TEST_P(DISABLED_UsageTableTest, DeactivateOfflineLicense) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    LoadOfflineLicense(s, pst);

    s.open();
    s.GenerateDerivedKeys();
    s.LoadTestKeys(pst, new_mac_keys_);  // Reload the license
    s.TestDecryptCTR();                  // Should be able to decrypt.
    DeactivatePST(pst);                  // Then deactivate.
    // After deactivate, should not be able to decrypt.
    s.TestDecryptCTR(false, OEMCrypto_ERROR_UNKNOWN_FAILURE);
    s.GenerateReport(pst);
    EXPECT_EQ(kInactive, s.pst_report()->status);
    EXPECT_ALMOST(
        0, wvcdm::htonll64(s.pst_report()->seconds_since_license_received));
    s.close();

    Session s2;
    s2.open();
    s2.GenerateDerivedKeys();
    // Offile license can not be reused if it has been deactivated.
    uint8_t* pst_ptr = s.encrypted_license().pst;
    EXPECT_NE(OEMCrypto_SUCCESS,
              OEMCrypto_LoadKeys(s2.session_id(), s.message_ptr(),
                                 sizeof(MessageData), &s.signature()[0],
                                 s.signature().size(),
                                 s.encrypted_license().mac_key_iv,
                                 s.encrypted_license().mac_keys, kNumKeys,
                                 s.key_array(), pst_ptr, pst.length()));
    // But we can still generate a report.
    Session s3;
    s3.open();
    s3.GenerateReport(pst, true, &s);
    EXPECT_EQ(kInactive, s3.pst_report()->status);
  }
}

TEST_P(DISABLED_UsageTableTest, BadRange) {
  if (OEMCrypto_SupportsUsageTable()) {
    std::string pst = "my_pst";
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    Session s;
    s.open();
    s.GenerateDerivedKeys();
    s.FillSimpleMessage(0, wvoec_mock::kControlNonceOrEntry,
                        s.get_nonce(), pst);
    s.EncryptAndSign();
    uint8_t* pst_ptr = s.license().pst;  // Bad: not in encrypted_license.
    ASSERT_NE(
        OEMCrypto_SUCCESS,
        OEMCrypto_LoadKeys(s.session_id(), s.message_ptr(), sizeof(MessageData),
                           &s.signature()[0], s.signature().size(),
                           s.encrypted_license().mac_key_iv,
                           s.encrypted_license().mac_keys, kNumKeys,
                           s.key_array(), pst_ptr, pst.length()));
  }
}

TEST_P(DISABLED_UsageTableTest, TimingTest) {
  if (OEMCrypto_SupportsUsageTable()) {
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_UpdateUsageTable());
    std::string pst1 = "my_pst_1";
    std::string pst2 = "my_pst_2";
    std::string pst3 = "my_pst_3";
    Session s1;
    Session s2;
    Session s3;
    LoadOfflineLicense(s1, pst1);
    time_t loaded1 = time(NULL);
    LoadOfflineLicense(s2, pst2);
    time_t loaded2 = time(NULL);
    LoadOfflineLicense(s3, pst3);
    time_t loaded3 = time(NULL);

    sleep(kLongSleep);
    s1.open();
    s1.GenerateDerivedKeys();
    s1.LoadTestKeys(pst1, new_mac_keys_);
    time_t first_decrypt1 = time(NULL);
    s1.TestDecryptCTR();

    s2.open();
    s2.GenerateDerivedKeys();
    s2.LoadTestKeys(pst2, new_mac_keys_);
    time_t first_decrypt2 = time(NULL);
    s2.TestDecryptCTR();

    sleep(kLongSleep);
    time_t second_decrypt = time(NULL);
    s1.TestDecryptCTR();
    s2.TestDecryptCTR();

    sleep(kLongSleep);
    DeactivatePST(pst1);
    s1.close();
    s2.close();

    sleep(kLongSleep);
    // This is as close to reboot as we can simulate in code.
    OEMCrypto_Terminate();
    sleep(kShortSleep);
    OEMCrypto_Initialize();
    InstallKeybox(kDefaultKeybox, true);

    // After a reboot, we should be able to reload keys, and generate reports.
    sleep(kLongSleep);
    time_t third_decrypt = time(NULL);
    s2.open();
    s2.GenerateDerivedKeys();
    s2.LoadTestKeys(pst2, new_mac_keys_);
    s2.TestDecryptCTR();
    s2.close();

    s1.open();
    s2.open();
    s3.open();
    sleep(kLongSleep);
    time_t report_generated1 = time(NULL);
    s1.GenerateReport(pst1);
    time_t report_generated2 = time(NULL);
    s2.GenerateReport(pst2);
    time_t report_generated3 = time(NULL);
    s3.GenerateReport(pst3);

    EXPECT_EQ(kInactive, s1.pst_report()->status);
    EXPECT_ALMOST(
        report_generated1 - loaded1,
        wvcdm::htonll64(s1.pst_report()->seconds_since_license_received));
    EXPECT_ALMOST(
        report_generated1 - first_decrypt1,
        wvcdm::htonll64(s1.pst_report()->seconds_since_first_decrypt));
    EXPECT_ALMOST(report_generated1 - second_decrypt,
                  wvcdm::htonll64(s1.pst_report()->seconds_since_last_decrypt));

    EXPECT_EQ(kActive, s2.pst_report()->status);
    EXPECT_ALMOST(
        report_generated2 - loaded2,
        wvcdm::htonll64(s2.pst_report()->seconds_since_license_received));
    EXPECT_ALMOST(
        report_generated2 - first_decrypt2,
        wvcdm::htonll64(s2.pst_report()->seconds_since_first_decrypt));
    EXPECT_ALMOST(report_generated2 - third_decrypt,
                  wvcdm::htonll64(s2.pst_report()->seconds_since_last_decrypt));

    EXPECT_EQ(kUnused, s3.pst_report()->status);
    EXPECT_ALMOST(
        report_generated3 - loaded3,
        wvcdm::htonll64(s3.pst_report()->seconds_since_license_received));
    // We don't expect first or last decrypt for unused report.
  }
}

INSTANTIATE_TEST_CASE_P(TestUsageTables, DISABLED_UsageTableTest,
                        Values(true, false));  // With and without new_mac_keys.

}  // namespace wvoec
