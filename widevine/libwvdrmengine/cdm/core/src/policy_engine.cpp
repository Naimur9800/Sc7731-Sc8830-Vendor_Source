// Copyright 2013 Google Inc. All Rights Reserved.

#include "policy_engine.h"

#include <limits.h>

#include <sstream>
#include <string>
#include <vector>

#include "clock.h"
#include "log.h"
#include "properties.h"
#include "string_conversions.h"
#include "wv_cdm_constants.h"

namespace wvcdm {

PolicyEngine::PolicyEngine(CryptoSession* crypto_session)
    : max_res_engine_(crypto_session) {
  Init(new Clock());
}

PolicyEngine::PolicyEngine(CryptoSession* crypto_session, Clock* clock)
    : max_res_engine_(crypto_session) {
  Init(clock);
}

PolicyEngine::~PolicyEngine() {
  if (clock_)
    delete clock_;
}

void PolicyEngine::Init(Clock* clock) {
  license_state_ = kLicenseStateInitial;
  can_decrypt_ = false;
  license_start_time_ = 0;
  playback_start_time_ = 0;
  last_playback_time_ = 0;
  next_renewal_time_ = 0;
  policy_max_duration_seconds_ = 0;
  clock_ = clock;
}

bool PolicyEngine::CanDecrypt(const KeyId& key_id) {
  return can_decrypt_ && max_res_engine_.CanDecrypt(key_id);
}

void PolicyEngine::OnTimerEvent(bool* event_occurred, CdmEventType* event) {
  *event_occurred = false;
  int64_t current_time = clock_->GetCurrentTime();

  // License expiration trumps all.
  if ((IsLicenseDurationExpired(current_time) ||
      IsPlaybackDurationExpired(current_time)) &&
      license_state_ != kLicenseStateExpired) {
    license_state_ = kLicenseStateExpired;
    can_decrypt_ = false;
    *event = LICENSE_EXPIRED_EVENT;
    *event_occurred = true;
    return;
  }

  bool renewal_needed = false;

  // Test to determine if renewal should be attempted.
  switch (license_state_) {
    case kLicenseStateCanPlay: {
      if (IsRenewalDelayExpired(current_time))
        renewal_needed = true;
      break;
    }

    case kLicenseStateNeedRenewal: {
      renewal_needed = true;
      break;
    }

    case kLicenseStateWaitingLicenseUpdate: {
      if (IsRenewalRetryIntervalExpired(current_time))
        renewal_needed = true;
      break;
    }

    case kLicenseStatePending: {
      if (current_time >= license_start_time_) {
        license_state_ = kLicenseStateCanPlay;
        can_decrypt_ = true;
      }
      break;
    }

    case kLicenseStateInitial:
    case kLicenseStateExpired: {
      break;
    }

    default: {
      license_state_ = kLicenseStateExpired;
      can_decrypt_ = false;
      break;
    }
  }

  if (renewal_needed) {
    UpdateRenewalRequest(current_time);
    *event = LICENSE_RENEWAL_NEEDED_EVENT;
    *event_occurred = true;
  }

  max_res_engine_.OnTimerEvent();
}

void PolicyEngine::SetLicense(
    const video_widevine_server::sdk::License& license) {
  license_id_.Clear();
  license_id_.CopyFrom(license.id());
  policy_.Clear();
  UpdateLicense(license);
  max_res_engine_.SetLicense(license);
}

void PolicyEngine::UpdateLicense(
    const video_widevine_server::sdk::License& license) {
  if (!license.has_policy())
    return;

  if (kLicenseStateExpired == license_state_) {
    LOGD("PolicyEngine::UpdateLicense: updating an expired license");
  }

  policy_.MergeFrom(license.policy());

  // some basic license validation
  // license start time needs to be specified in the initial response
  if (!license.has_license_start_time()) return;

  // if renewal, discard license if version has not been updated
  if (license_state_ != kLicenseStateInitial) {
    if (license.id().version() > license_id_.version())
      license_id_.CopyFrom(license.id());
    else
      return;
  }

  // Update time information
  license_start_time_ = license.license_start_time();
  next_renewal_time_ = license_start_time_ + policy_.renewal_delay_seconds();

  // Calculate policy_max_duration_seconds_. policy_max_duration_seconds_
  // will be set to the minimum of the following policies :
  // rental_duration_seconds and license_duration_seconds.
  // The value is used to determine when the license expires.
  policy_max_duration_seconds_ = 0;

  if (policy_.has_rental_duration_seconds())
    policy_max_duration_seconds_ = policy_.rental_duration_seconds();

  if ((policy_.license_duration_seconds() > 0) &&
      ((policy_.license_duration_seconds() <
       policy_max_duration_seconds_) ||
       policy_max_duration_seconds_ == 0)) {
    policy_max_duration_seconds_ = policy_.license_duration_seconds();
  }

  int64_t current_time = clock_->GetCurrentTime();
  if (!policy_.can_play() ||
      IsLicenseDurationExpired(current_time) ||
      IsPlaybackDurationExpired(current_time)) {
    license_state_ = kLicenseStateExpired;
    return;
  }

  // Update state
  if (current_time >= license_start_time_) {
    license_state_ = kLicenseStateCanPlay;
    can_decrypt_ = true;
  } else {
    license_state_ = kLicenseStatePending;
    can_decrypt_ = false;
  }
}

void PolicyEngine::BeginDecryption() {
  if (playback_start_time_ == 0) {
    switch (license_state_) {
      case kLicenseStateCanPlay:
      case kLicenseStateNeedRenewal:
      case kLicenseStateWaitingLicenseUpdate:
        playback_start_time_ = clock_->GetCurrentTime();
        last_playback_time_ = playback_start_time_;

        if (policy_.renew_with_usage()) {
          license_state_ = kLicenseStateNeedRenewal;
        }
        break;
      case kLicenseStateInitial:
      case kLicenseStatePending:
      case kLicenseStateExpired:
      default:
        break;
    }
  }
}

void PolicyEngine::DecryptionEvent() {
  last_playback_time_ = clock_->GetCurrentTime();
}

void PolicyEngine::NotifyResolution(uint32_t width, uint32_t height) {
  max_res_engine_.SetResolution(width, height);
}

CdmResponseType PolicyEngine::Query(CdmQueryMap* key_info) {
  std::stringstream ss;
  int64_t current_time = clock_->GetCurrentTime();

  if (license_state_ == kLicenseStateInitial) {
    key_info->clear();
    return NO_ERROR;
  }

  (*key_info)[QUERY_KEY_LICENSE_TYPE] =
    license_id_.type() == video_widevine_server::sdk::STREAMING ?
    QUERY_VALUE_STREAMING : QUERY_VALUE_OFFLINE;
  (*key_info)[QUERY_KEY_PLAY_ALLOWED] = policy_.can_play() ?
    QUERY_VALUE_TRUE : QUERY_VALUE_FALSE;
  (*key_info)[QUERY_KEY_PERSIST_ALLOWED] = policy_.can_persist() ?
    QUERY_VALUE_TRUE : QUERY_VALUE_FALSE;
  (*key_info)[QUERY_KEY_RENEW_ALLOWED] = policy_.can_renew() ?
    QUERY_VALUE_TRUE : QUERY_VALUE_FALSE;
  ss << GetLicenseDurationRemaining(current_time);
  (*key_info)[QUERY_KEY_LICENSE_DURATION_REMAINING] = ss.str();
  ss.str("");
  ss << GetPlaybackDurationRemaining(current_time);
  (*key_info)[QUERY_KEY_PLAYBACK_DURATION_REMAINING] = ss.str();
  (*key_info)[QUERY_KEY_RENEWAL_SERVER_URL] = policy_.renewal_server_url();

  return NO_ERROR;
}

bool PolicyEngine::GetSecondsSinceStarted(int64_t* seconds_since_started) {
  if (playback_start_time_ == 0) return false;

  *seconds_since_started = clock_->GetCurrentTime() - playback_start_time_;
  return (*seconds_since_started >= 0) ? true : false;
}

bool PolicyEngine::GetSecondsSinceLastPlayed(
    int64_t* seconds_since_last_played) {
  if (last_playback_time_ == 0) return false;

  *seconds_since_last_played = clock_->GetCurrentTime() - last_playback_time_;
  return (*seconds_since_last_played >= 0) ? true : false;
}

void PolicyEngine::RestorePlaybackTimes(int64_t playback_start_time,
                                        int64_t last_playback_time) {
  playback_start_time_ = (playback_start_time > 0) ? playback_start_time : 0;
  last_playback_time_ = (last_playback_time > 0) ? last_playback_time : 0;
}

void PolicyEngine::UpdateRenewalRequest(int64_t current_time) {
  license_state_ = kLicenseStateWaitingLicenseUpdate;
  next_renewal_time_ = current_time + policy_.renewal_retry_interval_seconds();
}

// For the policy time fields checked in the following methods, a value of 0
// indicates that there is no limit to the duration. These methods
// will always return false if the value is 0.
bool PolicyEngine::IsLicenseDurationExpired(int64_t current_time) {
  return policy_max_duration_seconds_ &&
      license_start_time_ + policy_max_duration_seconds_ <=
      current_time;
}

int64_t PolicyEngine::GetLicenseDurationRemaining(int64_t current_time) {
  if (0 == policy_max_duration_seconds_) return LLONG_MAX;

  int64_t remaining_time = policy_max_duration_seconds_
      + license_start_time_ - current_time;

  if (remaining_time < 0)
    remaining_time = 0;
  else if (remaining_time > policy_max_duration_seconds_)
    remaining_time = policy_max_duration_seconds_;
  return remaining_time;
}

bool PolicyEngine::IsPlaybackDurationExpired(int64_t current_time) {
  return (policy_.playback_duration_seconds() > 0) &&
      playback_start_time_ &&
      playback_start_time_ + policy_.playback_duration_seconds() <=
      current_time;
}

int64_t PolicyEngine::GetPlaybackDurationRemaining(int64_t current_time) {
  if (0 == policy_.playback_duration_seconds()) return LLONG_MAX;
  if (0 == playback_start_time_) return policy_.playback_duration_seconds();

  int64_t remaining_time = policy_.playback_duration_seconds()
      + playback_start_time_ - current_time;

  if (remaining_time < 0) remaining_time = 0;
  return remaining_time;
}

bool PolicyEngine::IsRenewalDelayExpired(int64_t current_time) {
  return policy_.can_renew() &&
      (policy_.renewal_delay_seconds() > 0) &&
      license_start_time_ + policy_.renewal_delay_seconds() <=
      current_time;
}

bool PolicyEngine::IsRenewalRecoveryDurationExpired(
    int64_t current_time) {
// NOTE: Renewal Recovery Duration is currently not used.
  return (policy_.renewal_recovery_duration_seconds() > 0) &&
      license_start_time_ + policy_.renewal_recovery_duration_seconds() <=
      current_time;
}

bool PolicyEngine::IsRenewalRetryIntervalExpired(
    int64_t current_time) {
  return policy_.can_renew() &&
      (policy_.renewal_retry_interval_seconds() > 0) &&
      next_renewal_time_ <= current_time;
}

}  // wvcdm
