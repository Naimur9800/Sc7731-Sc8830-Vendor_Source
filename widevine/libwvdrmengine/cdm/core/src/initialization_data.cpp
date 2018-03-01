// Copyright 2014 Google Inc. All Rights Reserved.

#include "initialization_data.h"

#include <string.h>

#include "buffer_reader.h"
#include "log.h"
#include "properties.h"
#include "wv_cdm_constants.h"

namespace wvcdm {

InitializationData::InitializationData(const std::string& type,
                                       const CdmInitData& data)
    : type_(type), is_cenc_(false), is_webm_(false) {
  if (type == ISO_BMFF_VIDEO_MIME_TYPE ||
      type == ISO_BMFF_AUDIO_MIME_TYPE ||
      type == CENC_INIT_DATA_FORMAT) {
    is_cenc_ = true;
  } else if (type == WEBM_VIDEO_MIME_TYPE ||
             type == WEBM_AUDIO_MIME_TYPE ||
             type == WEBM_INIT_DATA_FORMAT) {
    is_webm_ = true;
  }

  if (is_supported()) {
    if (is_cenc()) {
      ExtractWidevinePssh(data, &data_);
    } else {
      data_ = data;
    }
  }
}

// Parse a blob of multiple concatenated PSSH atoms to extract the first
// Widevine PSSH.
bool InitializationData::ExtractWidevinePssh(
    const CdmInitData& init_data, CdmInitData* output) {
  BufferReader reader(
      reinterpret_cast<const uint8_t*>(init_data.data()), init_data.length());

  // TODO(kqyang): Extracted from an actual init_data;
  // Need to find out where it comes from.
  static const uint8_t kWidevineSystemId[] = {
      0xED, 0xEF, 0x8B, 0xA9, 0x79, 0xD6, 0x4A, 0xCE,
      0xA3, 0xC8, 0x27, 0xDC, 0xD5, 0x1D, 0x21, 0xED,
  };

  // one PSSH blob consists of:
  // 4 byte size of the PSSH atom, inclusive
  // "pssh"
  // 4 byte flags, value 0
  // 16 byte system id
  // 4 byte size of PSSH data, exclusive
  while (1) {
    // size of PSSH atom, used for skipping
    uint32_t size;
    if (!reader.Read4(&size)) {
      LOGW("CdmEngine::ExtractWidevinePssh: Unable to read PSSH atom size");
      return false;
    }

    // "pssh"
    std::vector<uint8_t> pssh;
    if (!reader.ReadVec(&pssh, 4)) {
      LOGW("CdmEngine::ExtractWidevinePssh: Unable to read PSSH literal");
      return false;
    }
    if (memcmp(&pssh[0], "pssh", 4)) {
      LOGW("CdmEngine::ExtractWidevinePssh: PSSH literal not present");
      return false;
    }

    // flags
    uint32_t flags;
    if (!reader.Read4(&flags)) {
      LOGW("CdmEngine::ExtractWidevinePssh: Unable to read PSSH flags");
      return false;
    }
    if (flags != 0) {
      LOGW("CdmEngine::ExtractWidevinePssh: PSSH flags not zero");
      return false;
    }

    // system id
    std::vector<uint8_t> system_id;
    if (!reader.ReadVec(&system_id, sizeof(kWidevineSystemId))) {
      LOGW("CdmEngine::ExtractWidevinePssh: Unable to read system ID");
      return false;
    }

    if (memcmp(&system_id[0], kWidevineSystemId,
               sizeof(kWidevineSystemId))) {
      // skip the remaining contents of the atom,
      // after size field, atom name, flags and system id
      if (!reader.SkipBytes(
          size - 4 - 4 - 4 - sizeof(kWidevineSystemId))) {
        LOGW("CdmEngine::ExtractWidevinePssh: Unable to rest of PSSH atom");
        return false;
      }
      continue;
    }

    // size of PSSH box
    uint32_t pssh_length;
    if (!reader.Read4(&pssh_length)) {
      LOGW("CdmEngine::ExtractWidevinePssh: Unable to read PSSH box size");
      return false;
    }

    output->clear();
    if (!reader.ReadString(output, pssh_length)) {
      LOGW("CdmEngine::ExtractWidevinePssh: Unable to read PSSH");
      return false;
    }

    return true;
  }

  // we did not find a matching record
  return false;
}

}  // namespace wvcdm
