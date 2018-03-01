// Copyright 2013 Google Inc. All Rights Reserved.
// This file adds some print methods so that when unit tests fail, the
// will print the name of an enumeration instead of the numeric value.

#include "test_printers.h"

namespace wvcdm {

void PrintTo(const enum CdmResponseType& value, ::std::ostream* os) {
  switch(value) {
    case NO_ERROR: *os                      << "NO_ERROR";
      break;
    case UNKNOWN_ERROR: *os                 << "UNKNOWN_ERROR";
      break;
    case KEY_ADDED: *os                     << "KEY_ADDED";
      break;
    case KEY_ERROR: *os                     << "KEY_ERROR";
      break;
    case KEY_MESSAGE: *os                   << "KEY_MESSAGE";
      break;
    case NEED_KEY: *os                      << "NEED_KEY";
      break;
    case KEY_CANCELED: *os                  << "KEY_CANCELED";
      break;
    case NEED_PROVISIONING: *os             << "NEED_PROVISIONING";
      break;
    case DEVICE_REVOKED: *os                << "DEVICE_REVOKED";
      break;
    case INSUFFICIENT_CRYPTO_RESOURCES: *os << "INSUFFICIENT_CRYPTO_RESOURCES";
      break;
    default:
      *os                                   << "Unknown CdmResponseType";
      break;
  }
}

void PrintTo(const enum CdmEventType& value, ::std::ostream* os) {
  switch(value) {
    case LICENSE_EXPIRED_EVENT: *os         << "LICENSE_EXPIRED_EVENT";
      break;
    case LICENSE_RENEWAL_NEEDED_EVENT: *os  << "LICENSE_RENEWAL_NEEDED_EVENT";
      break;
    default:
      *os                                   << "Unknown  CdmEventType";
      break;
  }
};

void PrintTo(const enum CdmLicenseType& value, ::std::ostream* os) {
  switch(value) {
    case kLicenseTypeOffline: *os           << "kLicenseTypeOffline";
      break;
    case kLicenseTypeStreaming: *os         << "kLicenseTypeStreaming";
      break;
    case kLicenseTypeRelease: *os           << "kLicenseTypeRelease";
      break;
    default:
      *os                                   << "Unknown  CdmLicenseType";
      break;
  }
};

void PrintTo(const enum CdmSecurityLevel& value, ::std::ostream* os) {
  switch(value) {
    case kSecurityLevelUninitialized: *os   << "kSecurityLevelUninitialized";
      break;
    case kSecurityLevelL1: *os              << "kSecurityLevelL1";
      break;
    case kSecurityLevelL2: *os              << "kSecurityLevelL2";
      break;
    case kSecurityLevelL3: *os              << "kSecurityLevelL3";
      break;
    case kSecurityLevelUnknown: *os         << "kSecurityLevelUnknown";
      break;
    default:
      *os                                   << "Unknown CdmSecurityLevel";
      break;
  }
};

void PrintTo(const enum CdmCertificateType& value, ::std::ostream* os) {
  switch(value) {
    case kCertificateWidevine: *os          << "kCertificateWidevine";
      break;
    case kCertificateX509: *os              << "kCertificateX509";
      break;
    default:
      *os                                   << "Unknown CdmCertificateType";
      break;
  }
};

};  // namespace wvcdm
