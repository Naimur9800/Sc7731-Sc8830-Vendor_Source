// Copyright 2014 Google Inc. All Rights Reserved.

#include "max_res_engine.h"

#include "clock.h"
#include "log.h"

namespace {

typedef std::map<wvcdm::KeyId, wvcdm::MaxResEngine::KeyStatus*>::const_iterator
    KeyIterator;

const int64_t kHdcpCheckInterval = 10;
const uint32_t kNoResolution = 0;

}  // namespace

namespace wvcdm {

MaxResEngine::MaxResEngine(CryptoSession* crypto_session) : status_lock_() {
  Init(crypto_session, new Clock());
}

MaxResEngine::MaxResEngine(CryptoSession* crypto_session, Clock* clock)
    : status_lock_() {
  Init(crypto_session, clock);
}

MaxResEngine::~MaxResEngine() {
  AutoLock lock(status_lock_);
  DeleteAllKeys();
}

bool MaxResEngine::CanDecrypt(const KeyId& key_id) {
  AutoLock lock(status_lock_);
  if (keys_.count(key_id) > 0) {
    return keys_[key_id]->can_decrypt();
  } else {
    // If a Key ID is unknown to us, we don't know of any constraints for it,
    // so never block decryption.
    return true;
  }
}

void MaxResEngine::Init(CryptoSession* crypto_session, Clock* clock) {
  AutoLock lock(status_lock_);
  current_resolution_ = kNoResolution;
  clock_.reset(clock);
  next_check_time_ = clock_->GetCurrentTime();
  crypto_session_ = crypto_session;
}

void MaxResEngine::SetLicense(
    const video_widevine_server::sdk::License& license) {
  AutoLock lock(status_lock_);
  DeleteAllKeys();
  for (int32_t key_iter = 0; key_iter < license.key_size(); ++key_iter) {
    const KeyContainer& key = license.key(key_iter);
    if (key.type() == KeyContainer::CONTENT && key.has_id() &&
        key.video_resolution_constraints_size() > 0) {
      const ConstraintList& constraints = key.video_resolution_constraints();
      const KeyId& key_id = key.id();
      if (key.has_required_protection()) {
        keys_[key_id] =
            new KeyStatus(constraints, key.required_protection().hdcp());
      } else {
        keys_[key_id] = new KeyStatus(constraints);
      }
    }
  }
}

void MaxResEngine::SetResolution(uint32_t width, uint32_t height) {
  AutoLock lock(status_lock_);
  current_resolution_ = width * height;
}

void MaxResEngine::OnTimerEvent() {
  AutoLock lock(status_lock_);
  int64_t current_time = clock_->GetCurrentTime();
  if (!keys_.empty() && current_resolution_ != kNoResolution &&
      current_time >= next_check_time_) {
    CryptoSession::OemCryptoHdcpVersion current_hdcp_level;
    CryptoSession::OemCryptoHdcpVersion ignored;
    if (!crypto_session_->GetHdcpCapabilities(&current_hdcp_level, &ignored)) {
      current_hdcp_level = CryptoSession::kOemCryptoHdcpNotSupported;
    }
    for (KeyIterator i = keys_.begin(); i != keys_.end(); ++i) {
      i->second->Update(current_resolution_, current_hdcp_level);
    }
    next_check_time_ = current_time + kHdcpCheckInterval;
  }
}

void MaxResEngine::DeleteAllKeys() {
  // This helper method assumes that status_lock_ is already held.
  for (KeyIterator i = keys_.begin(); i != keys_.end(); ++i) delete i->second;
  keys_.clear();
}

MaxResEngine::KeyStatus::KeyStatus(const ConstraintList& constraints)
    : default_hdcp_level_(NULL) {
  Init(constraints);
}

MaxResEngine::KeyStatus::KeyStatus(
    const ConstraintList& constraints,
    const OutputProtection::HDCP& default_hdcp_level) {
  default_hdcp_level_.reset(new CryptoSession::OemCryptoHdcpVersion(
      ProtobufHdcpToOemCryptoHdcp(default_hdcp_level)));
  Init(constraints);
}

void MaxResEngine::KeyStatus::Init(const ConstraintList& constraints) {
  constraints_.Clear();
  constraints_.MergeFrom(constraints);
  can_decrypt_ = true;
}

void MaxResEngine::KeyStatus::Update(
    uint32_t res, CryptoSession::OemCryptoHdcpVersion current_hdcp_level) {
  VideoResolutionConstraint* current_constraint = GetConstraintForRes(res);

  if (current_constraint == NULL) {
    can_decrypt_ = false;
    return;
  }

  CryptoSession::OemCryptoHdcpVersion desired_hdcp_level;
  if (current_constraint->has_required_protection()) {
    desired_hdcp_level = ProtobufHdcpToOemCryptoHdcp(
        current_constraint->required_protection().hdcp());
  } else if (default_hdcp_level_.get() != NULL) {
    desired_hdcp_level = *default_hdcp_level_;
  } else {
    // No constraint value and no default means there's nothing to enforce.
    can_decrypt_ = true;
    return;
  }

  can_decrypt_ = (current_hdcp_level >= desired_hdcp_level);
}

MaxResEngine::VideoResolutionConstraint*
MaxResEngine::KeyStatus::GetConstraintForRes(uint32_t res) {
  typedef ConstraintList::pointer_iterator Iterator;
  for (Iterator i = constraints_.pointer_begin();
       i != constraints_.pointer_end(); ++i) {
    VideoResolutionConstraint* constraint = *i;
    if (constraint->has_min_resolution_pixels() &&
        constraint->has_max_resolution_pixels() &&
        res >= constraint->min_resolution_pixels() &&
        res <= constraint->max_resolution_pixels()) {
      return constraint;
    }
  }
  return NULL;
}

CryptoSession::OemCryptoHdcpVersion
MaxResEngine::KeyStatus::ProtobufHdcpToOemCryptoHdcp(
    const OutputProtection::HDCP& input) {
  switch (input) {
    case OutputProtection::HDCP_NONE:
      return CryptoSession::kOemCryptoHdcpNotSupported;
    case OutputProtection::HDCP_V1:
      return CryptoSession::kOemCryptoHdcpVersion1;
    case OutputProtection::HDCP_V2:
      return CryptoSession::kOemCryptoHdcpVersion2;
    case OutputProtection::HDCP_V2_1:
      return CryptoSession::kOemCryptoHdcpVersion2_1;
    case OutputProtection::HDCP_V2_2:
      return CryptoSession::kOemCryptoHdcpVersion2_2;
    default:
      LOGE(
          "MaxResEngine::KeyStatus::ProtobufHdcpToOemCryptoHdcp: "
          "Unknown HDCP Level");
      return CryptoSession::kOemCryptoNoHdcpDeviceAttached;
  }
}

}  // wvcdm
