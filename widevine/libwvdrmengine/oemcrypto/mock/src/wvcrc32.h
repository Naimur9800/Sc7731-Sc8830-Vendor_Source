// Copyright 2013 Google Inc. All Rights Reserved.
//
// Compute CRC32 Checksum. Needed for verification of WV Keybox.
//
#ifndef WVOEC_MOCK_WVCRC32_H_
#define WVOEC_MOCK_WVCRC32_H_

#include <stdint.h>

uint32_t wvcrc32(const uint8_t* p_begin, int i_count);

// Convert to network byte order
uint32_t wvcrc32n(const uint8_t* p_begin, int i_count);

#endif // WVOEC_MOCK_WVCRC32_H_
