// Copyright 2013 Google Inc. All Rights Reserved.
//
//  Mock implementation of OEMCrypto APIs
//
#include "oemcrypto_key_mock.h"

#include <cstring>
#include <vector>

#include "log.h"
#include "oemcrypto_logging.h"
#include "wv_cdm_constants.h"


namespace wvoec_mock {

#define FOURCC(c1, c2, c3, c4)                  \
  (c1 << 24 | c2 << 16 | c3 << 8 | c4)

bool KeyControlBlock::Validate() {
  if ((FOURCC('k', 'c', 't', 'l') != verification_) &&  // original verification
      (FOURCC('k', 'c', '0', '9') != verification_)) {  // add in version 9 api.
    LOGE("KCB: BAD verification string: %08X (not %08X or %08X)", verification_,
         0x6b63746c, 0x6b633039);
    return false;
  }
  valid_ = true;
  return valid_;
}

// This extracts 4 bytes in network byte order to a 32 bit integer in
// host byte order.
uint32_t KeyControlBlock::ExtractField(const std::vector<uint8_t>& str,
                                       int idx) {
  int bidx = idx * 4;
  uint32_t t = static_cast<unsigned char>(str[bidx]) << 24;
  t |= static_cast<unsigned char>(str[bidx + 1]) << 16;
  t |= static_cast<unsigned char>(str[bidx + 2]) << 8;
  t |= static_cast<unsigned char>(str[bidx + 3]);
  return t;
}

KeyControlBlock::KeyControlBlock(
    const std::vector<uint8_t>& key_control_string) {
  if (key_control_string.size() < wvcdm::KEY_CONTROL_SIZE) {
    LOGE("KCB: BAD Size: %d (not %d)", key_control_string.size(),
         wvcdm::KEY_CONTROL_SIZE);
    return;
  }

  verification_ = ExtractField(key_control_string, 0);
  duration_     = ExtractField(key_control_string, 1);
  nonce_        = ExtractField(key_control_string, 2);
  control_bits_ = ExtractField(key_control_string, 3);
  if (LogCategoryEnabled(kLoggingDumpKeyControlBlocks)) {
    LOGD("KCB:");
    LOGD("  valid:    %d", valid());
    LOGD("  duration: %d", duration());
    LOGD("  nonce:    %08X", nonce());
    LOGD("  magic:    %08X", verification());
    LOGD("  bits:     %08X", control_bits());
    switch (control_bits() & kControlReplayMask) {
      case kControlNonceRequired:
        LOGD("  bits kControlReplay kControlNonceRequired.");
        break;
      case kControlNonceOrEntry:
        LOGD("  bits kControlReplay kControlNonceOrEntry.");
        break;
      default:
        LOGD("  bits kControlReplay unset.");
        break;
    }
    LOGD("  bits kControlKDCPVersion 0x%02x.",
         (control_bits() & kControlHDCPVersionMask)
         >> kControlHDCPVersionShift);
    LOGD("  bit kControlAllowEncrypt %s.",
         (control_bits() & kControlAllowEncrypt) ? "set" : "unset");
    LOGD("  bit kControlAllowDecrypt %s.",
         (control_bits() & kControlAllowDecrypt) ? "set" : "unset");
    LOGD("  bit kControlAllowSign %s.",
         (control_bits() & kControlAllowSign) ? "set" : "unset");
    LOGD("  bit kControlAllowVerify %s.",
         (control_bits() & kControlAllowVerify) ? "set" : "unset");
    LOGD("  bit kControlObserveDataPath %s.",
         (control_bits() & kControlObserveDataPath) ? "set" : "unset");
    LOGD("  bit kControlObserveHDCP %s.",
         (control_bits() & kControlObserveHDCP) ? "set" : "unset");
    LOGD("  bit kControlObserveCGMS %s.",
         (control_bits() & kControlObserveCGMS) ? "set" : "unset");
    LOGD("  bit kControlDataPathSecure %s.",
         (control_bits() & kControlDataPathSecure) ? "set" : "unset");
    LOGD("  bit kControlNonceEnabled %s.",
         (control_bits() & kControlNonceEnabled) ? "set" : "unset");
    LOGD("  bit kControlHDCPRequired %s.",
         (control_bits() & kControlHDCPRequired) ? "set" : "unset");
    uint32_t cgms_bits = control_bits() & 0x3;
    const char* cgms_values[4] = {"free", "BAD", "once", "never"};
    LOGD("    CGMS = %s", cgms_values[cgms_bits]);
  }
  Validate();
}

Key::Key(const std::vector<uint8_t>& key_string, const KeyControlBlock& control)
    : value_(key_string), control_(control) {}

void Key::UpdateDuration(const KeyControlBlock& control) {
  control_.set_duration(control.duration());
}

};  // namespace wvoec_mock
