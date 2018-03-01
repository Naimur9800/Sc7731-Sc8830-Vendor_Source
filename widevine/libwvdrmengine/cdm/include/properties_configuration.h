// Copyright 2013 Google Inc. All Rights Reserved.

#ifndef CDM_BASE_PROPERTIES_CONFIGURATION_H_
#define CDM_BASE_PROPERTIES_CONFIGURATION_H_

#include "wv_cdm_constants.h"
#include "properties.h"

namespace wvcdm {

// Set only one of the three below to true. If secure buffer
// is selected, fallback to userspace buffers may occur
// if L1/L2 OEMCrypto APIs fail
const bool kPropertyOemCryptoUseSecureBuffers = true;
const bool kPropertyOemCryptoUseFifo = false;
const bool kPropertyOemCryptoUseUserSpaceBuffers = false;

// If true, the unit tests require OEMCrypto to support usage tables.
const bool kPropertyOemCryptoRequireUsageTable = true;

// If false, keyboxes will be used as client identification
// and passed as the token in the license request
const bool kPropertyUseCertificatesAsIdentification = true;

// If true, device files will be moved to the directory specified by
// Properties::GetDeviceFilesBasePath
const bool kSecurityLevelPathBackwardCompatibilitySupport = true;

} // namespace wvcdm

#endif  // CDM_BASE_WV_PROPERTIES_CONFIGURATION_H_
