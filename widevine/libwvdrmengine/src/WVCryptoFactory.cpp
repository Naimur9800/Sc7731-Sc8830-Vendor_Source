//
// Copyright 2013 Google Inc. All Rights Reserved.
//

//#define LOG_NDEBUG 0
#define LOG_TAG "WVCdm"
#include <utils/Log.h>

#include "WVCryptoFactory.h"

#include <dlfcn.h>

#include "utils/Errors.h"
#include "WVCDMSingleton.h"
#include "WVCryptoPlugin.h"
#include "WVUUID.h"

namespace wvdrm {

using namespace android;

WVCryptoFactory::WVCryptoFactory()
    : mLegacyLibraryHandle(NULL),
      mLegacyFactory(NULL) {
  mLegacyLibraryHandle = dlopen("libdrmdecrypt.so", RTLD_NOW);

  if (mLegacyLibraryHandle == NULL) {
    ALOGW("Unable to locate libdrmdecrypt.so");
  } else {
    typedef CryptoFactory* (*CreateCryptoFactoryFunc)();

    CreateCryptoFactoryFunc legacyCreateCryptoFactory =
        (CreateCryptoFactoryFunc)dlsym(mLegacyLibraryHandle,
                                       "createCryptoFactory");

    if (legacyCreateCryptoFactory == NULL) {
      ALOGW("Unable to find legacy symbol 'createCryptoFactory'.");
      dlclose(mLegacyLibraryHandle);
      mLegacyLibraryHandle = NULL;
    } else {
      mLegacyFactory = legacyCreateCryptoFactory();
      if (mLegacyFactory == NULL) {
        ALOGW("Legacy createCryptoFactory() failed.");
        dlclose(mLegacyLibraryHandle);
        mLegacyLibraryHandle = NULL;
      }
    }
  }
}

WVCryptoFactory::~WVCryptoFactory() {
  if (mLegacyFactory != NULL) {
    delete mLegacyFactory;
    mLegacyFactory = NULL;
  }

  if (mLegacyLibraryHandle != NULL) {
    dlclose(mLegacyLibraryHandle);
    mLegacyLibraryHandle = NULL;
  }
}

bool WVCryptoFactory::isCryptoSchemeSupported(const uint8_t uuid[16]) const {
  return isWidevineUUID(uuid);
}

status_t WVCryptoFactory::createPlugin(const uint8_t uuid[16], const void* data,
                                       size_t size, CryptoPlugin** plugin) {
  if (!isCryptoSchemeSupported(uuid)) {
    *plugin = NULL;
    return BAD_VALUE;
  }

  // 0 size means they want an old-style Widevine plugin
  if (size == 0) {
    if (mLegacyFactory == NULL) {
      ALOGE("Legacy crypto factory loading failed. Cannot create legacy "
            "plugin. Look for warnings about this that were logged earlier, "
            "during CryptoFactory instantiation.");
      return -EINVAL;
    }

    return mLegacyFactory->createPlugin(uuid, data, size, plugin);
  }

  // We received a session ID, so create a CENC plugin.
  *plugin = new WVCryptoPlugin(data, size, getCDM());

  return OK;
}

} // namespace wvdrm
