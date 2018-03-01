// Copyright 2012 Google Inc. All Rights Reserved.

#include <limits.h>

#include <algorithm>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "license.h"
#include "mock_clock.h"
#include "policy_engine.h"
#include "test_printers.h"
#include "wv_cdm_constants.h"

namespace {
const int64_t kDurationUnlimited = 0;
const int64_t kLicenseStartTime = 1413517500;   // ~ 01/01/2013
const int64_t kRentalDuration = 604800;         // 7 days
const int64_t kPlaybackDuration = 172800;       // 48 hours
const int64_t kStreamingLicenseDuration = 300;  // 5 minutes
const int64_t kOfflineLicenseDuration = kRentalDuration;
const int64_t kLicenseRenewalPeriod = 120;           // 2 minutes
const int64_t kLicenseRenewalRetryInterval = 30;     // 30 seconds
const int64_t kLicenseRenewalRecoveryDuration = 30;  // 30 seconds
const int64_t kLowDuration =
    std::min(std::min(std::min(kRentalDuration, kPlaybackDuration),
                      kStreamingLicenseDuration),
             kOfflineLicenseDuration);
const int64_t kHighDuration =
    std::max(std::max(std::max(kRentalDuration, kPlaybackDuration),
                      kStreamingLicenseDuration),
             kOfflineLicenseDuration);
const char* kRenewalServerUrl =
    "https://test.google.com/license/GetCencLicense";
const wvcdm::KeyId kKeyId = "357adc89f1673433c36c621f1b5c41ee";

int64_t GetLicenseRenewalDelay(int64_t license_duration) {
  return license_duration > kLicenseRenewalPeriod
             ? license_duration - kLicenseRenewalPeriod
             : 0;
}
}  // unnamed namespace

namespace wvcdm {

// protobuf generated classes.
using video_widevine_server::sdk::License;
using video_widevine_server::sdk::License_Policy;
using video_widevine_server::sdk::LicenseIdentification;
using video_widevine_server::sdk::STREAMING;
using video_widevine_server::sdk::OFFLINE;

// gmock methods
using ::testing::Return;
using ::testing::AtLeast;

class PolicyEngineTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    mock_clock_ = new MockClock();
    policy_engine_ = new PolicyEngine(NULL, mock_clock_);

    license_.set_license_start_time(kLicenseStartTime);

    LicenseIdentification* id = license_.mutable_id();
    id->set_version(1);
    id->set_type(STREAMING);

    License_Policy* policy = license_.mutable_policy();
    policy = license_.mutable_policy();
    policy->set_can_play(true);
    policy->set_can_persist(false);
    policy->set_can_renew(true);
    policy->set_rental_duration_seconds(kRentalDuration);
    policy->set_playback_duration_seconds(kPlaybackDuration);
    policy->set_license_duration_seconds(kStreamingLicenseDuration);
    policy->set_renewal_recovery_duration_seconds(
        kLicenseRenewalRecoveryDuration);

    policy->set_renewal_server_url(kRenewalServerUrl);
    policy->set_renewal_delay_seconds(
        GetLicenseRenewalDelay(kStreamingLicenseDuration));
    policy->set_renewal_retry_interval_seconds(kLicenseRenewalRetryInterval);
    policy->set_renew_with_usage(false);
  }

  virtual void TearDown() {
    delete policy_engine_;
    // Done by policy engine: delete mock_clock_;
    policy_engine_ = NULL;
    mock_clock_ = NULL;
  }

  int64_t GetMinOfRentalPlaybackLicenseDurations() {
    const License_Policy& policy = license_.policy();
    int64_t rental_duration = policy.rental_duration_seconds();
    int64_t playback_duration = policy.playback_duration_seconds();
    int64_t license_duration = policy.license_duration_seconds();
    if (rental_duration == kDurationUnlimited) rental_duration = LLONG_MAX;
    if (playback_duration == kDurationUnlimited) playback_duration = LLONG_MAX;
    if (license_duration == kDurationUnlimited) license_duration = LLONG_MAX;
    return std::min(std::min(rental_duration, playback_duration),
                    license_duration);
  }

  MockClock* mock_clock_;
  PolicyEngine* policy_engine_;
  License license_;
};

TEST_F(PolicyEngineTest, NoLicense) {
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackSuccess) {
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + 10));

  policy_engine_->SetLicense(license_);

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackFailed_CanPlayFalse) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_can_play(false);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5));

  policy_engine_->SetLicense(license_);
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->BeginDecryption();
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackFails_RentalDurationExpired) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_rental_duration_seconds(kLowDuration);
  policy->set_license_duration_seconds(kHighDuration);
  policy->set_renewal_delay_seconds(GetLicenseRenewalDelay(kHighDuration));
  int64_t min_duration = GetMinOfRentalPlaybackLicenseDurations();

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + min_duration - 1))
      .WillOnce(Return(kLicenseStartTime + min_duration));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackFails_PlaybackDurationExpired) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_license_duration_seconds(kHighDuration);
  policy->set_renewal_delay_seconds(GetLicenseRenewalDelay(kHighDuration));
  int64_t playback_start_time = kLicenseStartTime + 10000;

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(playback_start_time))
      .WillOnce(Return(playback_start_time + kPlaybackDuration - 2))
      .WillOnce(Return(playback_start_time + kPlaybackDuration + 2));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackFails_LicenseDurationExpired) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_can_renew(false);
  int64_t min_duration = GetMinOfRentalPlaybackLicenseDurations();

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + min_duration - 1))
      .WillOnce(Return(kLicenseStartTime + min_duration));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackFails_ExpiryBeforeRenewalDelay) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_renewal_delay_seconds(kStreamingLicenseDuration + 10);
  int64_t min_duration = GetMinOfRentalPlaybackLicenseDurations();

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + min_duration - 1))
      .WillOnce(Return(kLicenseStartTime + min_duration));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_RentalDuration0) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_rental_duration_seconds(kDurationUnlimited);
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 1))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration - 1))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_PlaybackDuration0) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_playback_duration_seconds(kDurationUnlimited);
  policy->set_license_duration_seconds(kHighDuration);
  int64_t license_renewal_delay = GetLicenseRenewalDelay(kHighDuration);
  policy->set_renewal_delay_seconds(license_renewal_delay);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 2))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 2))
      .WillOnce(Return(kLicenseStartTime + kHighDuration - 2))
      .WillOnce(Return(kLicenseStartTime + kHighDuration + 2));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_LicenseDuration0) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_license_duration_seconds(kDurationUnlimited);
  policy->set_rental_duration_seconds(kLowDuration);
  policy->set_renewal_delay_seconds(GetLicenseRenewalDelay(kHighDuration));
  int64_t min_duration = GetMinOfRentalPlaybackLicenseDurations();

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + min_duration - 1))
      .WillOnce(Return(kLicenseStartTime + min_duration));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_Durations0) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_rental_duration_seconds(kDurationUnlimited);
  policy->set_playback_duration_seconds(kDurationUnlimited);
  policy->set_license_duration_seconds(kDurationUnlimited);
  policy->set_renewal_delay_seconds(kHighDuration + 10);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + kHighDuration - 1))
      .WillOnce(Return(kLicenseStartTime + kHighDuration));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_LicenseWithFutureStartTime) {
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime - 100))
      .WillOnce(Return(kLicenseStartTime - 50))
      .WillOnce(Return(kLicenseStartTime))
      .WillOnce(Return(kLicenseStartTime + 10));

  policy_engine_->SetLicense(license_);

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackFailed_CanRenewFalse) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_can_renew(false);
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 10));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_RenewSuccess) {
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 15))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 20))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay +
                       kLicenseRenewalRetryInterval + 10));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  license_.set_license_start_time(kLicenseStartTime + license_renewal_delay +
                                  15);
  LicenseIdentification* id = license_.mutable_id();
  id->set_version(2);
  policy_engine_->UpdateLicense(license_);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_RenewSuccess_WithFutureStartTime) {
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 15))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 20))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 30))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 60));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  license_.set_license_start_time(kLicenseStartTime + license_renewal_delay +
                                  50);
  LicenseIdentification* id = license_.mutable_id();
  id->set_version(2);
  policy_engine_->UpdateLicense(license_);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackFailed_RenewFailedVersionNotUpdated) {
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 40))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 10));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  license_.set_license_start_time(kLicenseStartTime + license_renewal_delay +
                                  15);
  policy_engine_->UpdateLicense(license_);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackFailed_RepeatedRenewFailures) {
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 20))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 40))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 50))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 70))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 80))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 15));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_RenewSuccessAfterExpiry) {
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 20))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 40))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 50))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 70))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 80))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 10))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 30))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 40));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  license_.set_license_start_time(kLicenseStartTime +
                                  kStreamingLicenseDuration + 20);
  LicenseIdentification* id = license_.mutable_id();
  id->set_version(2);
  License_Policy* policy = license_.mutable_policy();
  policy = license_.mutable_policy();
  policy->set_playback_duration_seconds(kPlaybackDuration + 100);
  policy->set_license_duration_seconds(kStreamingLicenseDuration + 100);

  policy_engine_->UpdateLicense(license_);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_RenewSuccessAfterFailures) {
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 20))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 40))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 50))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 55))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 67))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 200));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  license_.set_license_start_time(kLicenseStartTime + license_renewal_delay +
                                  55);
  LicenseIdentification* id = license_.mutable_id();
  id->set_version(2);
  policy_engine_->UpdateLicense(license_);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, PlaybackOk_RenewedWithUsage) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_renew_with_usage(true);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + 10))
      .WillOnce(Return(kLicenseStartTime + 20))
      .WillOnce(Return(kLicenseStartTime + 40))
      .WillOnce(Return(kLicenseStartTime + 50));

  policy_engine_->SetLicense(license_);

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  license_.set_license_start_time(kLicenseStartTime + 30);
  policy->set_renew_with_usage(false);
  LicenseIdentification* id = license_.mutable_id();
  id->set_version(2);
  policy_engine_->UpdateLicense(license_);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
}

TEST_F(PolicyEngineTest, QuerySuccess_LicenseNotReceived) {
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(0, query_info.size());
}

TEST_F(PolicyEngineTest, QuerySuccess_LicenseStartTimeNotSet) {
  license_.clear_license_start_time();

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1));

  policy_engine_->SetLicense(license_);

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(0, query_info.size());
}

TEST_F(PolicyEngineTest, QuerySuccess) {
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 100));

  policy_engine_->SetLicense(license_);

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  ss.clear();
  EXPECT_EQ(kStreamingLicenseDuration - 100, remaining_time);
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_PlaybackNotBegun) {
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 100))
      .WillOnce(Return(kLicenseStartTime + 200));

  policy_engine_->SetLicense(license_);

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration - 100, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);

  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  ss.clear();
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration - 200, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_PlaybackBegun) {
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 50))
      .WillOnce(Return(kLicenseStartTime + 100))
      .WillOnce(Return(kLicenseStartTime + 150))
      .WillOnce(Return(kLicenseStartTime + 200));

  policy_engine_->SetLicense(license_);

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration - 50, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  ss.clear();
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration - 200, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration - 100, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_Offline) {
  LicenseIdentification* id = license_.mutable_id();
  id->set_type(OFFLINE);

  License_Policy* policy = license_.mutable_policy();
  policy->set_can_persist(true);
  policy->set_can_renew(false);
  policy->set_license_duration_seconds(kOfflineLicenseDuration);
  policy->set_renewal_delay_seconds(
      GetLicenseRenewalDelay(kOfflineLicenseDuration));

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 100))
      .WillOnce(Return(kLicenseStartTime + 200))
      .WillOnce(Return(kLicenseStartTime + 300));

  policy_engine_->SetLicense(license_);

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_OFFLINE, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kOfflineLicenseDuration - 300, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration - 100, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_InitialRentalDurationExpired) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_rental_duration_seconds(kLowDuration);
  policy->set_license_duration_seconds(kHighDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + kLowDuration + 1))
      .WillOnce(Return(kLicenseStartTime + kLowDuration + 5));

  policy_engine_->SetLicense(license_);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(0, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_InitialLicenseDurationExpired) {
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 1))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 5));

  policy_engine_->SetLicense(license_);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(0, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_CanPlayFalse) {
  LicenseIdentification* id = license_.mutable_id();
  id->set_type(OFFLINE);

  License_Policy* policy = license_.mutable_policy();
  policy->set_can_play(false);
  policy->set_can_persist(true);
  policy->set_license_duration_seconds(kOfflineLicenseDuration);
  policy->set_renewal_delay_seconds(
      GetLicenseRenewalDelay(kOfflineLicenseDuration));

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + 100));

  policy_engine_->SetLicense(license_);
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->BeginDecryption();
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_OFFLINE, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kOfflineLicenseDuration - 100, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_RentalDurationExpired) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_rental_duration_seconds(kLowDuration);
  policy->set_license_duration_seconds(kHighDuration);
  policy->set_renewal_delay_seconds(GetLicenseRenewalDelay(kHighDuration));

  int64_t min_duration = GetMinOfRentalPlaybackLicenseDurations();
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + min_duration - 1))
      .WillOnce(Return(kLicenseStartTime + min_duration))
      .WillOnce(Return(kLicenseStartTime + min_duration + 5));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(0, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration - min_duration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_PlaybackDurationExpired) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_playback_duration_seconds(kLowDuration);
  policy->set_license_duration_seconds(kHighDuration);
  policy->set_renewal_delay_seconds(GetLicenseRenewalDelay(kHighDuration));

  int64_t min_duration = GetMinOfRentalPlaybackLicenseDurations();
  int64_t playback_start_time = kLicenseStartTime + 10000;
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(playback_start_time))
      .WillOnce(Return(playback_start_time - 2 + min_duration))
      .WillOnce(Return(playback_start_time + 2 + min_duration))
      .WillOnce(Return(playback_start_time + 5 + min_duration));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kHighDuration - 10005 - min_duration, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(0, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_LicenseDurationExpired) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_can_renew(false);
  int64_t min_duration = GetMinOfRentalPlaybackLicenseDurations();

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + min_duration - 1))
      .WillOnce(Return(kLicenseStartTime + min_duration))
      .WillOnce(Return(kLicenseStartTime + min_duration + 5));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(0, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration - min_duration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_RentalDuration0) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_rental_duration_seconds(kDurationUnlimited);
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 1))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration - 1))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration))
      .WillOnce(Return(kLicenseStartTime + kStreamingLicenseDuration + 5));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(0, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration - kStreamingLicenseDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_PlaybackDuration0) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_playback_duration_seconds(kDurationUnlimited);
  policy->set_license_duration_seconds(kHighDuration);
  int64_t license_renewal_delay = GetLicenseRenewalDelay(kHighDuration);
  policy->set_renewal_delay_seconds(license_renewal_delay);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 2))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 2))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 5))
      .WillOnce(Return(kLicenseStartTime + kHighDuration - 2))
      .WillOnce(Return(kLicenseStartTime + kHighDuration + 2))
      .WillOnce(Return(kLicenseStartTime + kHighDuration + 5));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kHighDuration - license_renewal_delay - 5, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(LLONG_MAX, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  ss.clear();
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(0, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(LLONG_MAX, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_LicenseDuration0) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_license_duration_seconds(kDurationUnlimited);
  policy->set_rental_duration_seconds(kStreamingLicenseDuration);
  policy->set_renewal_delay_seconds(GetLicenseRenewalDelay(kHighDuration));
  int64_t min_duration = GetMinOfRentalPlaybackLicenseDurations();

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + min_duration - 1))
      .WillOnce(Return(kLicenseStartTime + min_duration))
      .WillOnce(Return(kLicenseStartTime + min_duration + 5));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_EXPIRED_EVENT, event);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(0, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration - min_duration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_Durations0) {
  License_Policy* policy = license_.mutable_policy();
  policy->set_rental_duration_seconds(kDurationUnlimited);
  policy->set_playback_duration_seconds(kDurationUnlimited);
  policy->set_license_duration_seconds(kDurationUnlimited);
  policy->set_renewal_delay_seconds(kHighDuration + 10);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + kHighDuration))
      .WillOnce(Return(kLicenseStartTime + kHighDuration + 9))
      .WillOnce(Return(kLicenseStartTime + kHighDuration + 15));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(LLONG_MAX, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(LLONG_MAX, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_LicenseWithFutureStartTime) {
  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime - 100))
      .WillOnce(Return(kLicenseStartTime - 50))
      .WillOnce(Return(kLicenseStartTime - 10))
      .WillOnce(Return(kLicenseStartTime))
      .WillOnce(Return(kLicenseStartTime + 10))
      .WillOnce(Return(kLicenseStartTime + 25));

  policy_engine_->SetLicense(license_);

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);

  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));
  policy_engine_->BeginDecryption();

  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  ss.clear();
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration - 25, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration - 15, remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_Renew) {
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 25))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 20))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay +
                       kLicenseRenewalRetryInterval + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay +
                       kLicenseRenewalRetryInterval + 15));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  license_.set_license_start_time(kLicenseStartTime + license_renewal_delay +
                                  15);
  LicenseIdentification* id = license_.mutable_id();
  id->set_version(2);
  policy_engine_->UpdateLicense(license_);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration - kLicenseRenewalRetryInterval,
            remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration + 5 - license_renewal_delay -
                kLicenseRenewalRetryInterval - 15,
            remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

TEST_F(PolicyEngineTest, QuerySuccess_RenewWithFutureStartTime) {
  int64_t license_renewal_delay =
      GetLicenseRenewalDelay(kStreamingLicenseDuration);

  EXPECT_CALL(*mock_clock_, GetCurrentTime())
      .WillOnce(Return(kLicenseStartTime + 1))
      .WillOnce(Return(kLicenseStartTime + 5))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay - 25))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay + 20))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay +
                       kLicenseRenewalRetryInterval + 10))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay +
                       kLicenseRenewalRetryInterval + 20))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay +
                       kLicenseRenewalRetryInterval + 30))
      .WillOnce(Return(kLicenseStartTime + license_renewal_delay +
                       kLicenseRenewalRetryInterval + 40));

  policy_engine_->SetLicense(license_);

  policy_engine_->BeginDecryption();
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  bool event_occurred;
  CdmEventType event;
  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_TRUE(event_occurred);
  EXPECT_EQ(LICENSE_RENEWAL_NEEDED_EVENT, event);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  license_.set_license_start_time(kLicenseStartTime + license_renewal_delay +
                                  kLicenseRenewalRetryInterval + 20);
  LicenseIdentification* id = license_.mutable_id();
  id->set_version(2);
  policy_engine_->UpdateLicense(license_);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_FALSE(policy_engine_->CanDecrypt(kKeyId));

  CdmQueryMap query_info;
  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  int64_t remaining_time;
  std::istringstream ss;
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration + 5 - license_renewal_delay -
                kLicenseRenewalRetryInterval - 20,
            remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);

  policy_engine_->OnTimerEvent(&event_occurred, &event);
  EXPECT_FALSE(event_occurred);
  EXPECT_TRUE(policy_engine_->CanDecrypt(kKeyId));

  EXPECT_EQ(NO_ERROR, policy_engine_->Query(&query_info));
  EXPECT_EQ(QUERY_VALUE_STREAMING, query_info[QUERY_KEY_LICENSE_TYPE]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_PLAY_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_FALSE, query_info[QUERY_KEY_PERSIST_ALLOWED]);
  EXPECT_EQ(QUERY_VALUE_TRUE, query_info[QUERY_KEY_RENEW_ALLOWED]);

  ss.clear();
  ss.str(query_info[QUERY_KEY_LICENSE_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kStreamingLicenseDuration - 20, remaining_time);
  ss.clear();
  ss.str(query_info[QUERY_KEY_PLAYBACK_DURATION_REMAINING]);
  ss >> remaining_time;
  EXPECT_EQ(kPlaybackDuration + 5 - license_renewal_delay -
                kLicenseRenewalRetryInterval - 40,
            remaining_time);
  EXPECT_EQ(kRenewalServerUrl, query_info[QUERY_KEY_RENEWAL_SERVER_URL]);
}

}  // wvcdm
