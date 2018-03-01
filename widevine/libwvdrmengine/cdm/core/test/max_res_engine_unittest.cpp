// Copyright 2012 Google Inc. All Rights Reserved.

#include "crypto_session.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "license.h"
#include "max_res_engine.h"
#include "mock_clock.h"
#include "scoped_ptr.h"
#include "wv_cdm_types.h"

namespace wvcdm {

typedef ::video_widevine_server::sdk::License License;
typedef ::video_widevine_server::sdk::License::KeyContainer KeyContainer;
typedef ::video_widevine_server::sdk::License::KeyContainer::OutputProtection
    OutputProtection;
typedef ::video_widevine_server::sdk::License::KeyContainer::
    VideoResolutionConstraint VideoResolutionConstraint;
typedef ::google::protobuf::RepeatedPtrField<KeyContainer> KeyList;
typedef ::google::protobuf::RepeatedPtrField<VideoResolutionConstraint>
    ConstraintList;

using namespace testing;

namespace {

const KeyId kKeyId1 = "357adc89f1673433c36c621f1b5c41ee";
const KeyId kKeyId2 = "3d25f819250789ecfc9ed48cc99af164";
const KeyId kKeyId3 = "fe3cf6b69e76c9a1c877922e1a661707";
const KeyId kKeyId4 = "29a321b9886658078f916fdd41d6f570";
const KeyId kKeyId5 = "cc5b031bcde371031c06822d935b9a63";
const KeyId kKeyId6 = "90ac1332e4efc8acbaf929c8d321f50c";

const uint32_t kMinRes1 = 0;
const uint32_t kMaxRes1 = 2000;
const uint32_t kTargetRes1 = (kMinRes1 + kMaxRes1) / 2;
const uint32_t kMinRes2 = kMaxRes1;
const uint32_t kMaxRes2 = 4000;
const uint32_t kTargetRes2 = (kMinRes2 + kMaxRes2) / 2;
const uint32_t kTargetRes3 = kMaxRes2 + 1000;

const OutputProtection::HDCP kHdcpDefault = OutputProtection::HDCP_V2;
const OutputProtection::HDCP kHdcpConstraint = OutputProtection::HDCP_V2_1;

const int64_t kHdcpInterval = 10;

}  // namespace

class HdcpOnlyMockCryptoSession : public CryptoSession {
 public:
  MOCK_METHOD2(GetHdcpCapabilities,
               bool(OemCryptoHdcpVersion*, OemCryptoHdcpVersion*));
};

ACTION_P2(IncrementAndReturnPointee, p, a) {
  *p += a;
  return *p;
}

class MaxResEngineTest : public Test {
 protected:
  virtual void SetUp() {
    mock_clock_ = new NiceMock<MockClock>();
    current_time_ = 0;

    ON_CALL(*mock_clock_, GetCurrentTime())
        .WillByDefault(
            IncrementAndReturnPointee(&current_time_, kHdcpInterval));

    max_res_engine_.reset(new MaxResEngine(&crypto_session_, mock_clock_));

    KeyList* keys = license_.mutable_key();

    // Key 1 - Content key w/ ID, no HDCP, no constraints
    {
      KeyContainer* key1 = keys->Add();
      key1->set_type(KeyContainer::CONTENT);
      key1->set_id(kKeyId1);
    }

    // Key 2 - Content key w/ ID, HDCP, no constraints
    {
      KeyContainer* key2 = keys->Add();
      key2->set_type(KeyContainer::CONTENT);
      key2->set_id(kKeyId2);
      key2->mutable_required_protection()->set_hdcp(kHdcpDefault);
    }

    // Key 3 - Content key w/ ID, no HDCP, constraints
    {
      KeyContainer* key3 = keys->Add();
      key3->set_type(KeyContainer::CONTENT);
      key3->set_id(kKeyId3);
      AddConstraints(key3->mutable_video_resolution_constraints());
    }

    // Key 4 - Content key w/ ID, HDCP, constraints
    {
      KeyContainer* key4 = keys->Add();
      key4->set_type(KeyContainer::CONTENT);
      key4->set_id(kKeyId4);
      key4->mutable_required_protection()->set_hdcp(kHdcpDefault);
      AddConstraints(key4->mutable_video_resolution_constraints());
    }

    // Key 5 - Content key w/o ID, HDCP, constraints
    {
      KeyContainer* key5 = keys->Add();
      key5->set_type(KeyContainer::CONTENT);
      key5->mutable_required_protection()->set_hdcp(kHdcpDefault);
      AddConstraints(key5->mutable_video_resolution_constraints());
    }

    // Key 6 - Non-content key
    {
      KeyContainer* key6 = keys->Add();
      key6->set_type(KeyContainer::OPERATOR_SESSION);
    }
  }

  void AddConstraints(ConstraintList* constraints) {
    // Constraint 1 - Low-res and no HDCP
    {
      VideoResolutionConstraint* constraint1 = constraints->Add();
      constraint1->set_min_resolution_pixels(kMinRes1);
      constraint1->set_max_resolution_pixels(kMaxRes1);
    }

    // Constraint 2 - High-res and stricter HDCP
    {
      VideoResolutionConstraint* constraint2 = constraints->Add();
      constraint2->set_min_resolution_pixels(kMinRes2);
      constraint2->set_max_resolution_pixels(kMaxRes2);
      constraint2->mutable_required_protection()->set_hdcp(kHdcpConstraint);
    }
  }

  MockClock* mock_clock_;
  int64_t current_time_;
  StrictMock<HdcpOnlyMockCryptoSession> crypto_session_;
  scoped_ptr<MaxResEngine> max_res_engine_;
  License license_;
};

TEST_F(MaxResEngineTest, IsPermissiveByDefault) {
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));

  max_res_engine_->OnTimerEvent();

  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));
}

TEST_F(MaxResEngineTest, IsPermissiveWithoutALicense) {
  max_res_engine_->SetResolution(1, kTargetRes1);
  max_res_engine_->OnTimerEvent();

  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));
}

TEST_F(MaxResEngineTest, IsPermissiveWithoutAResolution) {
  max_res_engine_->SetLicense(license_);
  max_res_engine_->OnTimerEvent();

  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));
}

TEST_F(MaxResEngineTest, HandlesResolutionsBasedOnConstraints) {
  EXPECT_CALL(crypto_session_, GetHdcpCapabilities(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<0>(CryptoSession::kOemCryptoNoHdcpDeviceAttached),
                Return(true)));

  max_res_engine_->SetLicense(license_);

  max_res_engine_->SetResolution(1, kTargetRes1);
  max_res_engine_->OnTimerEvent();
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));

  max_res_engine_->SetResolution(1, kTargetRes2);
  max_res_engine_->OnTimerEvent();
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));

  max_res_engine_->SetResolution(1, kTargetRes3);
  max_res_engine_->OnTimerEvent();
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_FALSE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_FALSE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));
}

TEST_F(MaxResEngineTest, RequestsHdcpImmediatelyAndOnlyAfterInterval) {
  int64_t start_time = current_time_;

  {
    InSequence calls;
    EXPECT_CALL(*mock_clock_, GetCurrentTime())
        .WillOnce(Return(start_time));
    EXPECT_CALL(crypto_session_, GetHdcpCapabilities(_, _))
        .WillOnce(
            DoAll(SetArgPointee<0>(CryptoSession::kOemCryptoHdcpVersion2_2),
                  Return(true)));
    EXPECT_CALL(*mock_clock_, GetCurrentTime())
        .WillOnce(Return(start_time + kHdcpInterval / 2))
        .WillOnce(Return(start_time + kHdcpInterval));
    EXPECT_CALL(crypto_session_, GetHdcpCapabilities(_, _))
        .WillOnce(
            DoAll(SetArgPointee<0>(CryptoSession::kOemCryptoHdcpVersion2_2),
                  Return(true)));
  }

  max_res_engine_->SetLicense(license_);
  max_res_engine_->SetResolution(1, kTargetRes1);
  max_res_engine_->OnTimerEvent();
  max_res_engine_->OnTimerEvent();
  max_res_engine_->OnTimerEvent();
}

TEST_F(MaxResEngineTest, DoesNotRequestHdcpWithoutALicense) {
  EXPECT_CALL(crypto_session_, GetHdcpCapabilities(_, _))
      .Times(0);

  max_res_engine_->OnTimerEvent();
}

TEST_F(MaxResEngineTest, HandlesConstraintOverridingHdcp) {
  EXPECT_CALL(crypto_session_, GetHdcpCapabilities(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<0>(CryptoSession::kOemCryptoHdcpVersion2),
                Return(true)));

  max_res_engine_->SetLicense(license_);

  max_res_engine_->SetResolution(1, kTargetRes1);
  max_res_engine_->OnTimerEvent();
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));

  max_res_engine_->SetResolution(1, kTargetRes2);
  max_res_engine_->OnTimerEvent();
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_FALSE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_FALSE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));
}

TEST_F(MaxResEngineTest, HandlesNoHdcp) {
  EXPECT_CALL(crypto_session_, GetHdcpCapabilities(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<0>(CryptoSession::kOemCryptoHdcpNotSupported),
                Return(true)));

  max_res_engine_->SetLicense(license_);

  max_res_engine_->SetResolution(1, kTargetRes1);
  max_res_engine_->OnTimerEvent();
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_FALSE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));

  max_res_engine_->SetResolution(1, kTargetRes2);
  max_res_engine_->OnTimerEvent();
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_FALSE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_FALSE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));
}

TEST_F(MaxResEngineTest, IgnoresHdcpWithoutAResolution) {
  EXPECT_CALL(crypto_session_, GetHdcpCapabilities(_, _))
      .Times(0);

  max_res_engine_->SetLicense(license_);
  max_res_engine_->OnTimerEvent();

  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId1));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId2));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId3));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId4));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId5));
  EXPECT_TRUE(max_res_engine_->CanDecrypt(kKeyId6));
}

}  // wvcdm
