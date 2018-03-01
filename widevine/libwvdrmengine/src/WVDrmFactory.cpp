//
// Copyright 2013 Google Inc. All Rights Reserved.
//

//#define LOG_NDEBUG 0
#define LOG_TAG "WVCdm"
#include <utils/Log.h>

#include "WVDrmFactory.h"

#include "utils/Errors.h"
#include "wv_cdm_constants.h"
#include "WVCDMSingleton.h"
#include "wv_content_decryption_module.h"
#include "WVDrmPlugin.h"
#include "WVUUID.h"

namespace wvdrm {

using namespace android;

WVGenericCryptoInterface WVDrmFactory::sOemCryptoInterface;

bool WVDrmFactory::isCryptoSchemeSupported(const uint8_t uuid[16]) {
  return isWidevineUUID(uuid);
}

bool WVDrmFactory::isContentTypeSupported(const String8 &initDataType) {
  return wvcdm::WvContentDecryptionModule::IsSupported(initDataType.string());
}

status_t WVDrmFactory::createDrmPlugin(const uint8_t uuid[16],
                                          DrmPlugin** plugin) {
  if (!isCryptoSchemeSupported(uuid)) {
    *plugin = NULL;
    return BAD_VALUE;
  }

  *plugin = new WVDrmPlugin(getCDM(), &sOemCryptoInterface);

  return OK;
}

} // namespace wvdrm
