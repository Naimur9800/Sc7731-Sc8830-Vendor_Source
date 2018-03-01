//
// Copyright 2013 Google Inc. All Rights Reserved.
//

//#define LOG_NDEBUG 0
#define LOG_TAG "WVCdm"
#include <utils/Log.h>

#include "WVDrmPlugin.h"

#include <endian.h>
#include <string.h>
#include <sstream>
#include <string>
#include <vector>

#include "mapErrors-inl.h"
#include "media/stagefright/MediaErrors.h"
#include "utils/Errors.h"
#include "wv_cdm_constants.h"

namespace {
  static const char* const kResetSecurityLevel = "";
  static const char* const kEnable = "enable";
  static const char* const kDisable = "disable";
  static const std::string kPsshTag = "pssh";
}

namespace wvdrm {

using namespace android;
using namespace std;
using namespace wvcdm;

WVDrmPlugin::WVDrmPlugin(WvContentDecryptionModule* cdm,
                         WVGenericCryptoInterface* crypto)
  : mCDM(cdm), mCrypto(crypto), mCryptoSessionsMutex(), mCryptoSessions() {}

WVDrmPlugin::~WVDrmPlugin() {
  typedef map<CdmSessionId, CryptoSession>::iterator mapIterator;
  Mutex::Autolock lock(mCryptoSessionsMutex);
  for (mapIterator iter = mCryptoSessions.begin();
       iter != mCryptoSessions.end();
       ++iter) {
    bool bRes = mCDM->DetachEventListener(iter->first, this);

    if (!bRes) {
      ALOGE("Received failure when trying to detach WVDrmPlugin as an event "
            "listener.");
    }

    CdmResponseType res = mCDM->CloseSession(iter->first);
    if (!isCdmResponseTypeSuccess(res)) {
      ALOGE("Failed to close session while destroying WVDrmPlugin");
    }
  }
  mCryptoSessions.clear();
}

status_t WVDrmPlugin::openSession(Vector<uint8_t>& sessionId) {
  CdmSessionId cdmSessionId;
  CdmResponseType res = mCDM->OpenSession("com.widevine", &mPropertySet,
                                          &cdmSessionId);

  if (!isCdmResponseTypeSuccess(res)) {
    return mapAndNotifyOfCdmResponseType(sessionId, res);
  }

  bool success = false;

  // Register for events
  bool listenerAttached = mCDM->AttachEventListener(cdmSessionId, this);

  if (listenerAttached) {
    // Construct a CryptoSession
    CdmQueryMap info;

    res = mCDM->QueryKeyControlInfo(cdmSessionId, &info);

    if (isCdmResponseTypeSuccess(res) &&
        info.count(QUERY_KEY_OEMCRYPTO_SESSION_ID)) {
      OEMCrypto_SESSION oecSessionId;
      istringstream(info[QUERY_KEY_OEMCRYPTO_SESSION_ID]) >> oecSessionId;

      {
        Mutex::Autolock lock(mCryptoSessionsMutex);
        mCryptoSessions[cdmSessionId] = CryptoSession(oecSessionId);
      }

      success = true;
    } else {
      ALOGE("Unable to query key control info.");
    }
  } else {
    ALOGE("Received failure when trying to attach WVDrmPlugin as an event"
          "listener.");
  }

  if (success) {
    // Marshal Session ID
    sessionId.clear();
    sessionId.appendArray(reinterpret_cast<const uint8_t*>(cdmSessionId.data()),
                          cdmSessionId.size());

    return android::OK;
  } else {
    if (listenerAttached) {
      mCDM->DetachEventListener(cdmSessionId, this);
    }

    mCDM->CloseSession(cdmSessionId);

    if (!isCdmResponseTypeSuccess(res)) {
      // We got an error code we can return.
      return mapAndNotifyOfCdmResponseType(sessionId, res);
    } else {
      // We got a failure that did not give us an error code, such as a failure
      // of AttachEventListener() or the key being missing from the map.
      return kErrorCDMGeneric;
    }
  }
}

status_t WVDrmPlugin::closeSession(const Vector<uint8_t>& sessionId) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  CdmResponseType res = mCDM->CloseSession(cdmSessionId);

  if (isCdmResponseTypeSuccess(res)) {
    Mutex::Autolock lock(mCryptoSessionsMutex);
    mCryptoSessions.erase(cdmSessionId);
  }

  return mapAndNotifyOfCdmResponseType(sessionId, res);
}

status_t WVDrmPlugin::getKeyRequest(
    const Vector<uint8_t>& scope,
    const Vector<uint8_t>& initData,
    const String8& initDataType,
    KeyType keyType,
    const KeyedVector<String8, String8>& optionalParameters,
    Vector<uint8_t>& request,
    String8& defaultUrl) {
  CdmLicenseType cdmLicenseType;
  CdmSessionId cdmSessionId;
  CdmKeySetId cdmKeySetId;
  if (keyType == kKeyType_Offline) {
    cdmLicenseType = kLicenseTypeOffline;
    cdmSessionId.assign(scope.begin(), scope.end());
  } else if (keyType == kKeyType_Streaming) {
    cdmLicenseType = kLicenseTypeStreaming;
    cdmSessionId.assign(scope.begin(), scope.end());
  } else if (keyType == kKeyType_Release) {
    cdmLicenseType = kLicenseTypeRelease;
    cdmKeySetId.assign(scope.begin(), scope.end());
  } else {
    return android::ERROR_DRM_CANNOT_HANDLE;
  }

  string cdmInitDataType = initDataType.string();
  // Provide backwards-compatibility for apps that pass non-EME-compatible MIME
  // types.
  if (!WvContentDecryptionModule::IsSupported(cdmInitDataType)) {
    cdmInitDataType = wvcdm::ISO_BMFF_VIDEO_MIME_TYPE;
  }

  CdmInitData processedInitData;
  if (initData.size() > 0 &&
      WvContentDecryptionModule::IsCenc(cdmInitDataType) &&
      !InitDataResemblesPSSH(initData)) {
    // This data was passed in the old format, pre-unwrapped. We need to wrap
    // the init data in a new PSSH header.
    static const char psshPrefix[] = {
      0, 0, 0, 0,                                     // Total size
      'p', 's', 's', 'h',                             // "PSSH"
      0, 0, 0, 0,                                     // Flags - must be zero
      0xED, 0xEF, 0x8B, 0xA9, 0x79, 0xD6, 0x4A, 0xCE, // Widevine UUID
      0xA3, 0xC8, 0x27, 0xDC, 0xD5, 0x1D, 0x21, 0xED,
      0, 0, 0, 0                                      // Size of initData
    };
    processedInitData.assign(psshPrefix, sizeof(psshPrefix) / sizeof(char));
    processedInitData.append(reinterpret_cast<const char*>(initData.array()),
                             initData.size());
    const size_t kPsshBoxSizeLocation = 0;
    const size_t kInitDataSizeLocation =
        sizeof(psshPrefix) - sizeof(uint32_t);
    uint32_t psshBoxSize = htonl(processedInitData.size());
    uint32_t initDataSize = htonl(initData.size());
    memcpy(&processedInitData[kPsshBoxSizeLocation], &psshBoxSize,
           sizeof(uint32_t));
    memcpy(&processedInitData[kInitDataSizeLocation], &initDataSize,
           sizeof(uint32_t));
  } else {
    // For other formats, we can pass the init data through unmodified.
    processedInitData.assign(reinterpret_cast<const char*>(initData.array()),
                             initData.size());
  }

  CdmAppParameterMap cdmParameters;
  for (size_t i = 0; i < optionalParameters.size(); ++i) {
    const String8& key = optionalParameters.keyAt(i);
    const String8& value = optionalParameters.valueAt(i);

    string cdmKey(key.string(), key.size());
    string cdmValue(value.string(), value.size());

    cdmParameters[cdmKey] = cdmValue;
  }

  CdmKeyMessage keyRequest;
  string cdmDefaultUrl;
  CdmResponseType res = mCDM->GenerateKeyRequest(cdmSessionId, cdmKeySetId,
                                                 cdmInitDataType,
                                                 processedInitData,
                                                 cdmLicenseType, cdmParameters,
                                                 &mPropertySet, &keyRequest,
                                                 &cdmDefaultUrl);

  if (isCdmResponseTypeSuccess(res)) {
    defaultUrl.clear();
    defaultUrl.setTo(cdmDefaultUrl.data(), cdmDefaultUrl.size());

    request.clear();
    request.appendArray(reinterpret_cast<const uint8_t*>(keyRequest.data()),
                        keyRequest.size());
  }

  if (keyType == kKeyType_Release) {
    // When releasing keys, we do not have a session ID.
    return mapCdmResponseType(res);
  } else {
    // For all other requests, we have a session ID.
    return mapAndNotifyOfCdmResponseType(scope, res);
  }
}

status_t WVDrmPlugin::provideKeyResponse(
    const Vector<uint8_t>& scope,
    const Vector<uint8_t>& response,
    Vector<uint8_t>& keySetId) {
  CdmSessionId cdmSessionId;
  CdmKeyResponse cdmResponse(response.begin(), response.end());
  CdmKeySetId cdmKeySetId;

  bool isRequest = (memcmp(scope.array(), SESSION_ID_PREFIX,
                           sizeof(SESSION_ID_PREFIX) - 1) == 0);
  bool isRelease = (memcmp(scope.array(), KEY_SET_ID_PREFIX,
                           sizeof(KEY_SET_ID_PREFIX) - 1) == 0);

  if (isRequest) {
    cdmSessionId.assign(scope.begin(), scope.end());
  } else if (isRelease) {
    cdmKeySetId.assign(scope.begin(), scope.end());
  } else {
    return android::ERROR_DRM_CANNOT_HANDLE;
  }

  CdmResponseType res = mCDM->AddKey(cdmSessionId, cdmResponse, &cdmKeySetId);

  if (isRequest && isCdmResponseTypeSuccess(res)) {
    keySetId.clear();
    keySetId.appendArray(reinterpret_cast<const uint8_t*>(cdmKeySetId.data()),
                         cdmKeySetId.size());
  }

  if (isRelease) {
    // When releasing keys, we do not have a session ID.
    return mapCdmResponseType(res);
  } else {
    // For all other requests, we have a session ID.
    status_t status = mapAndNotifyOfCdmResponseType(scope, res);
    // For "NEED_KEY," we still want to send the notifcation, but then we don't
    // return the error.  This is because "NEED_KEY" from AddKey() is an
    // expected behavior when sending a privacy certificate.
    if (res == wvcdm::NEED_KEY && mPropertySet.use_privacy_mode()) {
      status = android::OK;
    }
    return status;
  }
}

status_t WVDrmPlugin::removeKeys(const Vector<uint8_t>& sessionId) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());

  CdmResponseType res = mCDM->CancelKeyRequest(cdmSessionId);

  return mapAndNotifyOfCdmResponseType(sessionId, res);
}

status_t WVDrmPlugin::restoreKeys(const Vector<uint8_t>& sessionId,
                                  const Vector<uint8_t>& keySetId) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  CdmKeySetId cdmKeySetId(keySetId.begin(), keySetId.end());

  CdmResponseType res = mCDM->RestoreKey(cdmSessionId, cdmKeySetId);

  return mapAndNotifyOfCdmResponseType(sessionId, res);
}

status_t WVDrmPlugin::queryKeyStatus(
    const Vector<uint8_t>& sessionId,
    KeyedVector<String8, String8>& infoMap) const {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  CdmQueryMap cdmLicenseInfo;

  CdmResponseType res = mCDM->QueryKeyStatus(cdmSessionId, &cdmLicenseInfo);

  if (isCdmResponseTypeSuccess(res)) {
    infoMap.clear();
    for (CdmQueryMap::const_iterator iter = cdmLicenseInfo.begin();
         iter != cdmLicenseInfo.end();
         ++iter) {
      const string& cdmKey = iter->first;
      const string& cdmValue = iter->second;

      String8 key(cdmKey.data(), cdmKey.size());
      String8 value(cdmValue.data(), cdmValue.size());

      infoMap.add(key, value);
    }
  }

  return mapCdmResponseType(res);
}

status_t WVDrmPlugin::getProvisionRequest(const String8& cert_type,
                                          const String8& cert_authority,
                                          Vector<uint8_t>& request,
                                          String8& defaultUrl) {
  CdmProvisioningRequest cdmProvisionRequest;
  string cdmDefaultUrl;

  CdmCertificateType cdmCertType = kCertificateWidevine;
  if (cert_type == "X.509") {
    cdmCertType = kCertificateX509;
  }

  string cdmCertAuthority = cert_authority.string();

  CdmResponseType res = mCDM->GetProvisioningRequest(cdmCertType,
                                                     cdmCertAuthority,
                                                     &cdmProvisionRequest,
                                                     &cdmDefaultUrl);

  if (isCdmResponseTypeSuccess(res)) {
    request.clear();
    request.appendArray(reinterpret_cast<const uint8_t*>(
                            cdmProvisionRequest.data()),
                        cdmProvisionRequest.size());

    defaultUrl.clear();
    defaultUrl.setTo(cdmDefaultUrl.data(), cdmDefaultUrl.size());
  }

  return mapCdmResponseType(res);
}

status_t WVDrmPlugin::provideProvisionResponse(
    const Vector<uint8_t>& response,
    Vector<uint8_t>& certificate,
    Vector<uint8_t>& wrapped_key) {
  CdmProvisioningResponse cdmResponse(response.begin(), response.end());
  string cdmCertificate;
  string cdmWrappedKey;
  CdmResponseType res = mCDM->HandleProvisioningResponse(cdmResponse,
                                                         &cdmCertificate,
                                                         &cdmWrappedKey);
  if (isCdmResponseTypeSuccess(res)) {
    certificate.clear();
    certificate.appendArray(
        reinterpret_cast<const uint8_t*>(cdmCertificate.data()),
        cdmCertificate.size());

    wrapped_key.clear();
    wrapped_key.appendArray(
        reinterpret_cast<const uint8_t*>(cdmWrappedKey.data()),
        cdmWrappedKey.size());
  }

  return mapCdmResponseType(res);
}

status_t WVDrmPlugin::unprovisionDevice() {
  CdmResponseType res1 = mCDM->Unprovision(kSecurityLevelL1);
  CdmResponseType res3 = mCDM->Unprovision(kSecurityLevelL3);
  if (!isCdmResponseTypeSuccess(res1))
  {
    return mapCdmResponseType(res1);
  }
  else
  {
    return mapCdmResponseType(res3);
  }
}

status_t WVDrmPlugin::getSecureStop(const Vector<uint8_t>& ssid,
                                    Vector<uint8_t>& secureStop) {
  CdmUsageInfo cdmUsageInfo;
  CdmSecureStopId cdmSsid(ssid.begin(), ssid.end());
  CdmResponseType res = mCDM->GetUsageInfo(
      mPropertySet.app_id(), cdmSsid, &cdmUsageInfo);
  if (isCdmResponseTypeSuccess(res)) {
    secureStop.clear();
    for (CdmUsageInfo::const_iterator iter = cdmUsageInfo.begin();
         iter != cdmUsageInfo.end();
         ++iter) {
      const string& cdmStop = *iter;

      secureStop.appendArray(reinterpret_cast<const uint8_t*>(cdmStop.data()),
                       cdmStop.size());
    }
  }
  return mapCdmResponseType(res);
}

status_t WVDrmPlugin::getSecureStops(List<Vector<uint8_t> >& secureStops) {
  CdmUsageInfo cdmUsageInfo;
  CdmResponseType res =
      mCDM->GetUsageInfo(mPropertySet.app_id(), &cdmUsageInfo);
  if (isCdmResponseTypeSuccess(res)) {
    secureStops.clear();
    for (CdmUsageInfo::const_iterator iter = cdmUsageInfo.begin();
         iter != cdmUsageInfo.end();
         ++iter) {
      const string& cdmStop = *iter;

      Vector<uint8_t> stop;
      stop.appendArray(reinterpret_cast<const uint8_t*>(cdmStop.data()),
                       cdmStop.size());

      secureStops.push_back(stop);
    }
  }
  return mapCdmResponseType(res);
}

status_t WVDrmPlugin::releaseAllSecureStops() {
  CdmResponseType res = mCDM->ReleaseAllUsageInfo(mPropertySet.app_id());
  return mapCdmResponseType(res);
}

status_t WVDrmPlugin::releaseSecureStops(const Vector<uint8_t>& ssRelease) {
  CdmUsageInfoReleaseMessage cdmMessage(ssRelease.begin(), ssRelease.end());
  CdmResponseType res = mCDM->ReleaseUsageInfo(cdmMessage);
  return mapCdmResponseType(res);
}

status_t WVDrmPlugin::getPropertyString(const String8& name,
                                        String8& value) const {
  if (name == "vendor") {
    value = "Google";
  } else if (name == "version") {
    value = "1.0";
  } else if (name == "description") {
    value = "Widevine CDM";
  } else if (name == "algorithms") {
    value = "AES/CBC/NoPadding,HmacSHA256";
  } else if (name == "securityLevel") {
    string requestedLevel = mPropertySet.security_level();

    if (requestedLevel.length() > 0) {
      value = requestedLevel.c_str();
    } else {
      CdmQueryMap status;
      CdmResponseType res = mCDM->QueryStatus(&status);
      if (!isCdmResponseTypeSuccess(res)) {
        ALOGE("Error querying CDM status: %u", res);
        return mapCdmResponseType(res);
      } else if (!status.count(QUERY_KEY_SECURITY_LEVEL)) {
        ALOGE("CDM did not report a security level");
        return kErrorCDMGeneric;
      }
      value = status[QUERY_KEY_SECURITY_LEVEL].c_str();
    }
  } else if (name == "systemId") {
    CdmQueryMap status;
    CdmResponseType res = mCDM->QueryStatus(&status);
    if (res != wvcdm::NO_ERROR) {
      ALOGE("Error querying CDM status: %u", res);
      return mapCdmResponseType(res);
    } else if (!status.count(QUERY_KEY_SYSTEM_ID)) {
      ALOGE("CDM did not report a system ID");
      return kErrorCDMGeneric;
    }
    value = status[QUERY_KEY_SYSTEM_ID].c_str();
  } else if (name == "privacyMode") {
    if (mPropertySet.use_privacy_mode()) {
      value = kEnable;
    } else {
      value = kDisable;
    }
  } else if (name == "sessionSharing") {
    if (mPropertySet.is_session_sharing_enabled()) {
      value = kEnable;
    } else {
      value = kDisable;
    }
  } else if (name == "hdcpLevel") {
    CdmQueryMap status;
    CdmResponseType res = mCDM->QueryStatus(&status);
    if (res != wvcdm::NO_ERROR) {
      ALOGE("Error querying CDM status: %u", res);
      return mapCdmResponseType(res);
    } else if (!status.count(QUERY_KEY_CURRENT_HDCP_LEVEL)) {
      ALOGE("CDM did not report a current HDCP level");
      return kErrorCDMGeneric;
    }
    value = status[QUERY_KEY_CURRENT_HDCP_LEVEL].c_str();
  } else if (name == "maxHdcpLevel") {
    CdmQueryMap status;
    CdmResponseType res = mCDM->QueryStatus(&status);
    if (res != wvcdm::NO_ERROR) {
      ALOGE("Error querying CDM status: %u", res);
      return mapCdmResponseType(res);
    } else if (!status.count(QUERY_KEY_MAX_HDCP_LEVEL)) {
      ALOGE("CDM did not report a maximum HDCP level");
      return kErrorCDMGeneric;
    }
    value = status[QUERY_KEY_MAX_HDCP_LEVEL].c_str();
  } else if (name == "usageReportingSupport") {
    CdmQueryMap status;
    CdmResponseType res = mCDM->QueryStatus(&status);
    if (res != wvcdm::NO_ERROR) {
      ALOGE("Error querying CDM status: %u", res);
      return mapCdmResponseType(res);
    } else if (!status.count(QUERY_KEY_USAGE_SUPPORT)) {
      ALOGE("CDM did not report whether it supports usage reporting");
      return kErrorCDMGeneric;
    }
    value = status[QUERY_KEY_USAGE_SUPPORT].c_str();
  } else {
    ALOGE("App requested unknown string property %s", name.string());
    return android::ERROR_DRM_CANNOT_HANDLE;
  }

  return android::OK;
}

status_t WVDrmPlugin::getPropertyByteArray(const String8& name,
                                           Vector<uint8_t>& value) const {
  if (name == "deviceUniqueId") {
    CdmQueryMap status;

    CdmResponseType res = mCDM->QueryStatus(&status);

    if (!isCdmResponseTypeSuccess(res)) {
      ALOGE("Error querying CDM status: %u", res);
      return mapCdmResponseType(res);
    } else if (!status.count(QUERY_KEY_DEVICE_ID)) {
      ALOGE("CDM did not report a device unique ID");
      return kErrorCDMGeneric;
    }

    const string& uniqueId = status[QUERY_KEY_DEVICE_ID];

    value.clear();
    value.appendArray(reinterpret_cast<const uint8_t*>(uniqueId.data()),
                      uniqueId.size());
  } else if (name == "provisioningUniqueId") {
    CdmQueryMap status;

    CdmResponseType res = mCDM->QueryStatus(&status);

    if (!isCdmResponseTypeSuccess(res)) {
      ALOGE("Error querying CDM status: %u", res);
      return mapCdmResponseType(res);
    } else if (!status.count(QUERY_KEY_PROVISIONING_ID)) {
      ALOGE("CDM did not report a provisioning unique ID");
      return kErrorCDMGeneric;
    }

    const string& uniqueId = status[QUERY_KEY_PROVISIONING_ID];

    value.clear();
    value.appendArray(reinterpret_cast<const uint8_t*>(uniqueId.data()),
                      uniqueId.size());
  } else if (name == "serviceCertificate") {
    std::string cert = mPropertySet.service_certificate();
    value.clear();
    value.appendArray(reinterpret_cast<const uint8_t*>(&cert[0]), cert.size());
  } else {
    ALOGE("App requested unknown byte array property %s", name.string());
    return android::ERROR_DRM_CANNOT_HANDLE;
  }

  return android::OK;
}

status_t WVDrmPlugin::setPropertyString(const String8& name,
                                        const String8& value) {
  if (name == "securityLevel") {
    size_t sessionCount = 0;
    {
      Mutex::Autolock lock(mCryptoSessionsMutex);
      sessionCount = mCryptoSessions.size();
    }
    if (sessionCount == 0) {
      if (value == QUERY_VALUE_SECURITY_LEVEL_L3.c_str()) {
        mPropertySet.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L3);
      } else if (value == QUERY_VALUE_SECURITY_LEVEL_L1.c_str()) {
        // We must be sure we CAN set the security level to L1.
        CdmQueryMap status;
        CdmResponseType res = mCDM->QueryStatus(&status);
        if (!isCdmResponseTypeSuccess(res)) {
          ALOGE("Error querying CDM status: %u", res);
          return mapCdmResponseType(res);
        } else if (!status.count(QUERY_KEY_SECURITY_LEVEL)) {
          ALOGE("CDM did not report a security level");
          return kErrorCDMGeneric;
        }

        if (status[QUERY_KEY_SECURITY_LEVEL] != QUERY_VALUE_SECURITY_LEVEL_L1) {
          ALOGE("App requested L1 security on a non-L1 device.");
          return android::BAD_VALUE;
        } else {
          mPropertySet.set_security_level(kResetSecurityLevel);
        }
      } else if (value == kResetSecurityLevel) {
        mPropertySet.set_security_level(kResetSecurityLevel);
      } else {
        ALOGE("App requested invalid security level %s", value.string());
        return android::BAD_VALUE;
      }
    } else {
      ALOGE("App tried to change security level while sessions are open.");
      return kErrorSessionIsOpen;
    }
  } else if (name == "privacyMode") {
    if (value == kEnable) {
      mPropertySet.set_use_privacy_mode(true);
    } else if (value == kDisable) {
      mPropertySet.set_use_privacy_mode(false);
    } else {
      ALOGE("App requested unknown privacy mode %s", value.string());
      return android::BAD_VALUE;
    }
  } else if (name == "sessionSharing") {
    size_t sessionCount = 0;
    {
      Mutex::Autolock lock(mCryptoSessionsMutex);
      sessionCount = mCryptoSessions.size();
    }
    if (sessionCount == 0) {
      if (value == kEnable) {
        mPropertySet.set_is_session_sharing_enabled(true);
      } else if (value == kDisable) {
        mPropertySet.set_is_session_sharing_enabled(false);
      } else {
        ALOGE("App requested unknown sharing type %s", value.string());
        return android::BAD_VALUE;
      }
    } else {
      ALOGE("App tried to change key sharing while sessions are open.");
      return kErrorSessionIsOpen;
    }
  } else if (name == "appId") {
    size_t sessionCount = 0;
    {
      Mutex::Autolock lock(mCryptoSessionsMutex);
      sessionCount = mCryptoSessions.size();
    }
    if (sessionCount == 0) {
      mPropertySet.set_app_id(value.string());
    } else {
      ALOGE("App tried to set the application id while sessions are opened.");
      return kErrorSessionIsOpen;
    }
  } else {
    ALOGE("App set unknown string property %s", name.string());
    return android::ERROR_DRM_CANNOT_HANDLE;
  }

  return android::OK;
}

status_t WVDrmPlugin::setPropertyByteArray(const String8& name,
                                           const Vector<uint8_t>& value) {
  if (name == "serviceCertificate") {
    std::string cert(value.begin(), value.end());
    mPropertySet.set_service_certificate(cert);
  } else {
    ALOGE("App set unknown byte array property %s", name.string());
    return android::ERROR_DRM_CANNOT_HANDLE;
  }

  return android::OK;
}

status_t WVDrmPlugin::setCipherAlgorithm(const Vector<uint8_t>& sessionId,
                                         const String8& algorithm) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  Mutex::Autolock lock(mCryptoSessionsMutex);

  if (!mCryptoSessions.count(cdmSessionId)) {
    return android::ERROR_DRM_SESSION_NOT_OPENED;
  }

  CryptoSession& cryptoSession = mCryptoSessions[cdmSessionId];

  if (algorithm == "AES/CBC/NoPadding") {
    cryptoSession.setCipherAlgorithm(OEMCrypto_AES_CBC_128_NO_PADDING);
  } else {
    return android::ERROR_DRM_CANNOT_HANDLE;
  }

  return android::OK;
}

status_t WVDrmPlugin::setMacAlgorithm(const Vector<uint8_t>& sessionId,
                                      const String8& algorithm) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  Mutex::Autolock lock(mCryptoSessionsMutex);

  if (!mCryptoSessions.count(cdmSessionId)) {
    return android::ERROR_DRM_SESSION_NOT_OPENED;
  }

  CryptoSession& cryptoSession = mCryptoSessions[cdmSessionId];

  if (algorithm == "HmacSHA256") {
    cryptoSession.setMacAlgorithm(OEMCrypto_HMAC_SHA256);
  } else {
    return android::ERROR_DRM_CANNOT_HANDLE;
  }

  return android::OK;
}

status_t WVDrmPlugin::encrypt(const Vector<uint8_t>& sessionId,
                              const Vector<uint8_t>& keyId,
                              const Vector<uint8_t>& input,
                              const Vector<uint8_t>& iv,
                              Vector<uint8_t>& output) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  Mutex::Autolock lock(mCryptoSessionsMutex);

  if (!mCryptoSessions.count(cdmSessionId)) {
    return android::ERROR_DRM_SESSION_NOT_OPENED;
  }

  const CryptoSession& cryptoSession = mCryptoSessions[cdmSessionId];

  if (cryptoSession.cipherAlgorithm() == kInvalidCrytpoAlgorithm) {
    return android::NO_INIT;
  }

  OEMCryptoResult res = mCrypto->selectKey(cryptoSession.oecSessionId(),
                                           keyId.array(), keyId.size());

  if (res != OEMCrypto_SUCCESS) {
    ALOGE("OEMCrypto_SelectKey failed with %u", res);
    return mapAndNotifyOfOEMCryptoResult(sessionId, res);
  }

  output.resize(input.size());

  res = mCrypto->encrypt(cryptoSession.oecSessionId(), input.array(),
                         input.size(), iv.array(),
                         cryptoSession.cipherAlgorithm(), output.editArray());

  if (res == OEMCrypto_SUCCESS) {
    return android::OK;
  } else {
    ALOGE("OEMCrypto_Generic_Encrypt failed with %u", res);
    return mapAndNotifyOfOEMCryptoResult(sessionId, res);
  }
}

status_t WVDrmPlugin::decrypt(const Vector<uint8_t>& sessionId,
                              const Vector<uint8_t>& keyId,
                              const Vector<uint8_t>& input,
                              const Vector<uint8_t>& iv,
                              Vector<uint8_t>& output) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  Mutex::Autolock lock(mCryptoSessionsMutex);

  if (!mCryptoSessions.count(cdmSessionId)) {
    return android::ERROR_DRM_SESSION_NOT_OPENED;
  }

  const CryptoSession& cryptoSession = mCryptoSessions[cdmSessionId];

  if (cryptoSession.cipherAlgorithm() == kInvalidCrytpoAlgorithm) {
    return android::NO_INIT;
  }

  OEMCryptoResult res = mCrypto->selectKey(cryptoSession.oecSessionId(),
                                           keyId.array(), keyId.size());

  if (res != OEMCrypto_SUCCESS) {
    ALOGE("OEMCrypto_SelectKey failed with %u", res);
    return mapAndNotifyOfOEMCryptoResult(sessionId, res);
  }

  output.resize(input.size());

  res = mCrypto->decrypt(cryptoSession.oecSessionId(), input.array(),
                         input.size(), iv.array(),
                         cryptoSession.cipherAlgorithm(), output.editArray());

  if (res == OEMCrypto_SUCCESS) {
    return android::OK;
  } else {
    ALOGE("OEMCrypto_Generic_Decrypt failed with %u", res);
    return mapAndNotifyOfOEMCryptoResult(sessionId, res);
  }
}

status_t WVDrmPlugin::sign(const Vector<uint8_t>& sessionId,
                           const Vector<uint8_t>& keyId,
                           const Vector<uint8_t>& message,
                           Vector<uint8_t>& signature) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  Mutex::Autolock lock(mCryptoSessionsMutex);

  if (!mCryptoSessions.count(cdmSessionId)) {
    return android::ERROR_DRM_SESSION_NOT_OPENED;
  }

  const CryptoSession& cryptoSession = mCryptoSessions[cdmSessionId];

  if (cryptoSession.macAlgorithm() == kInvalidCrytpoAlgorithm) {
    return android::NO_INIT;
  }

  OEMCryptoResult res = mCrypto->selectKey(cryptoSession.oecSessionId(),
                                           keyId.array(), keyId.size());

  if (res != OEMCrypto_SUCCESS) {
    ALOGE("OEMCrypto_SelectKey failed with %u", res);
    return mapAndNotifyOfOEMCryptoResult(sessionId, res);
  }

  size_t signatureSize = 0;

  res = mCrypto->sign(cryptoSession.oecSessionId(), message.array(),
                      message.size(), cryptoSession.macAlgorithm(),
                      NULL, &signatureSize);

  if (res != OEMCrypto_ERROR_SHORT_BUFFER) {
    ALOGE("OEMCrypto_Generic_Sign failed with %u when requesting signature "
          "size", res);
    if (res != OEMCrypto_SUCCESS) {
      return mapAndNotifyOfOEMCryptoResult(sessionId, res);
    } else {
      return android::ERROR_DRM_UNKNOWN;
    }
  }

  signature.resize(signatureSize);

  res = mCrypto->sign(cryptoSession.oecSessionId(), message.array(),
                      message.size(), cryptoSession.macAlgorithm(),
                      signature.editArray(), &signatureSize);

  if (res == OEMCrypto_SUCCESS) {
    return android::OK;
  } else {
    ALOGE("OEMCrypto_Generic_Sign failed with %u", res);
    return mapAndNotifyOfOEMCryptoResult(sessionId, res);
  }
}

status_t WVDrmPlugin::verify(const Vector<uint8_t>& sessionId,
                             const Vector<uint8_t>& keyId,
                             const Vector<uint8_t>& message,
                             const Vector<uint8_t>& signature,
                             bool& match) {
  CdmSessionId cdmSessionId(sessionId.begin(), sessionId.end());
  Mutex::Autolock lock(mCryptoSessionsMutex);

  if (!mCryptoSessions.count(cdmSessionId)) {
    return android::ERROR_DRM_SESSION_NOT_OPENED;
  }

  const CryptoSession& cryptoSession = mCryptoSessions[cdmSessionId];

  if (cryptoSession.macAlgorithm() == kInvalidCrytpoAlgorithm) {
    return android::NO_INIT;
  }

  OEMCryptoResult res = mCrypto->selectKey(cryptoSession.oecSessionId(),
                                           keyId.array(), keyId.size());

  if (res != OEMCrypto_SUCCESS) {
    ALOGE("OEMCrypto_SelectKey failed with %u", res);
    return mapAndNotifyOfOEMCryptoResult(sessionId, res);
  }

  res = mCrypto->verify(cryptoSession.oecSessionId(), message.array(),
                        message.size(), cryptoSession.macAlgorithm(),
                        signature.array(), signature.size());

  if (res == OEMCrypto_SUCCESS) {
    match = true;
    return android::OK;
  } else if (res == OEMCrypto_ERROR_SIGNATURE_FAILURE) {
    match = false;
    return android::OK;
  } else {
    ALOGE("OEMCrypto_Generic_Verify failed with %u", res);
    return mapAndNotifyOfOEMCryptoResult(sessionId, res);
  }
}

status_t WVDrmPlugin::signRSA(const Vector<uint8_t>& sessionId,
                              const String8& algorithm,
                              const Vector<uint8_t>& message,
                              const Vector<uint8_t>& wrappedKey,
                              Vector<uint8_t>& signature) {
  RSA_Padding_Scheme padding_scheme;
  if (algorithm == "RSASSA-PSS-SHA1") {
    padding_scheme = kSign_RSASSA_PSS;
  } else if (algorithm == "PKCS1-BlockType1") {
    padding_scheme = kSign_PKCS1_Block1;
  } else {
    ALOGE("Unknown RSA Algorithm %s", algorithm.string());
    return android::ERROR_DRM_CANNOT_HANDLE;
  }
  OEMCryptoResult res = mCrypto->signRSA(wrappedKey.array(),
                                         wrappedKey.size(),
                                         message.array(), message.size(),
                                         signature,
                                         padding_scheme);

  if (res != OEMCrypto_SUCCESS) {
    ALOGE("OEMCrypto_GenerateRSASignature failed with %u", res);
    return mapOEMCryptoResult(res);
  }

  return android::OK;
}

void WVDrmPlugin::OnEvent(const CdmSessionId& cdmSessionId,
                          CdmEventType cdmEventType) {
  Vector<uint8_t> sessionId;
  EventType eventType = kDrmPluginEventVendorDefined;

  switch (cdmEventType) {
    case LICENSE_EXPIRED_EVENT:
      eventType = kDrmPluginEventKeyExpired;
      break;
    case LICENSE_RENEWAL_NEEDED_EVENT:
      eventType = kDrmPluginEventKeyNeeded;
      break;
  }

  sessionId.appendArray(reinterpret_cast<const uint8_t*>(cdmSessionId.data()),
                        cdmSessionId.size());

  // Call base-class method with translated event.
  sendEvent(eventType, 0, &sessionId, NULL);
}

status_t WVDrmPlugin::mapAndNotifyOfCdmResponseType(
    const Vector<uint8_t>& sessionId,
    CdmResponseType res) {
  if (res == wvcdm::NEED_PROVISIONING) {
    sendEvent(kDrmPluginEventProvisionRequired, 0, &sessionId, NULL);
  } else if (res == wvcdm::NEED_KEY) {
    sendEvent(kDrmPluginEventKeyNeeded, 0, &sessionId, NULL);
  }

  return mapCdmResponseType(res);
}

status_t WVDrmPlugin::mapAndNotifyOfOEMCryptoResult(
    const Vector<uint8_t>& sessionId,
    OEMCryptoResult res) {
  if (res == OEMCrypto_ERROR_NO_DEVICE_KEY) {
    sendEvent(kDrmPluginEventProvisionRequired, 0, &sessionId, NULL);
  }
  return mapOEMCryptoResult(res);
}

status_t WVDrmPlugin::mapOEMCryptoResult(OEMCryptoResult res) {
  switch (res) {
    case OEMCrypto_SUCCESS:
      return android::OK;
    case OEMCrypto_ERROR_SIGNATURE_FAILURE:
      return android::ERROR_DRM_TAMPER_DETECTED;
    case OEMCrypto_ERROR_SHORT_BUFFER:
      return kErrorIncorrectBufferSize;
    case OEMCrypto_ERROR_NO_DEVICE_KEY:
      return android::ERROR_DRM_NOT_PROVISIONED;
    case OEMCrypto_ERROR_INVALID_SESSION:
      return android::ERROR_DRM_SESSION_NOT_OPENED;
    case OEMCrypto_ERROR_TOO_MANY_SESSIONS:
      return kErrorTooManySessions;
    case OEMCrypto_ERROR_INVALID_RSA_KEY:
      return kErrorInvalidKey;
    case OEMCrypto_ERROR_INSUFFICIENT_RESOURCES:
      return android::ERROR_DRM_RESOURCE_BUSY;
    case OEMCrypto_ERROR_NOT_IMPLEMENTED:
      return android::ERROR_DRM_CANNOT_HANDLE;
    case OEMCrypto_ERROR_UNKNOWN_FAILURE:
    case OEMCrypto_ERROR_OPEN_SESSION_FAILED:
      return android::ERROR_DRM_UNKNOWN;
    default:
      return android::UNKNOWN_ERROR;
  }
}

bool WVDrmPlugin::InitDataResemblesPSSH(const Vector<uint8_t>& initData) {
  const uint8_t* const initDataArray = initData.array();

  // Extract the size field
  const uint8_t* const sizeField = &initDataArray[0];
  uint32_t nboSize;
  memcpy(&nboSize, sizeField, sizeof(nboSize));
  uint32_t size = ntohl(nboSize);

  if (size > initData.size()) {
    return false;
  }

  // Extract the ID field
  const char* const idField =
      reinterpret_cast<const char* const>(&initDataArray[sizeof(nboSize)]);
  string id(idField, kPsshTag.size());
  return id == kPsshTag;
}

}  // namespace wvdrm
