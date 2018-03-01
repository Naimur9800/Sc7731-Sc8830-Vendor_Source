// Copyright 2014 Google Inc. All Rights Reserved.

#ifndef WVCDM_CORE_MAX_RES_ENGINE_H_
#define WVCDM_CORE_MAX_RES_ENGINE_H_

#include <map>

#include "crypto_session.h"
#include "license_protocol.pb.h"
#include "lock.h"
#include "scoped_ptr.h"
#include "wv_cdm_types.h"

namespace wvcdm {

class Clock;
class MaxResEngineTest;

// Similar to the Policy Engine, this acts as an oracle that basically says
// "Yes(true) you may still decrypt or no(false) you may not decrypt this data
// anymore."
class MaxResEngine {
 public:
  explicit MaxResEngine(CryptoSession* crypto_session);
  virtual ~MaxResEngine();

  // The value returned is computed during the last call to SetLicense/
  // SetResolution/OnTimerEvent and may be out of sync depending on the amount
  // of time elapsed. The current decryption status is not calculated when this
  // function is called to avoid overhead in the decryption path.
  virtual bool CanDecrypt(const KeyId& key_id);

  // SetLicense is used in handling the initial license response. It stores
  // an exact copy of the key constraints from the license.
  virtual void SetLicense(const video_widevine_server::sdk::License& license);

  // SetResolution is called when the current output resolution is updated by
  // the decoder. The max-res engine will recalculate the current resolution
  // constraints, (if any) which may affect the results for CanDecrypt().
  virtual void SetResolution(uint32_t width, uint32_t height);

  // OnTimerEvent is called when a timer fires. The max-res engine may check the
  // current HDCP level using the crypto session, which may affect the results
  // for CanDecrypt().
  virtual void OnTimerEvent();

 private:
  typedef ::video_widevine_server::sdk::License::KeyContainer KeyContainer;
  typedef ::video_widevine_server::sdk::License::KeyContainer::OutputProtection
      OutputProtection;
  typedef ::video_widevine_server::sdk::License::KeyContainer::
      VideoResolutionConstraint VideoResolutionConstraint;
  typedef ::google::protobuf::RepeatedPtrField<VideoResolutionConstraint>
      ConstraintList;

  class KeyStatus {
   public:
    explicit KeyStatus(const ConstraintList& constraints);
    KeyStatus(const ConstraintList& constraints,
              const OutputProtection::HDCP& default_hdcp_level);

    bool can_decrypt() const { return can_decrypt_; }

    void Update(uint32_t res,
                CryptoSession::OemCryptoHdcpVersion current_hdcp_level);

   private:
    void Init(const ConstraintList& constraints);

    VideoResolutionConstraint* GetConstraintForRes(uint32_t res);

    static CryptoSession::OemCryptoHdcpVersion ProtobufHdcpToOemCryptoHdcp(
        const OutputProtection::HDCP& input);

    bool can_decrypt_;

    scoped_ptr<CryptoSession::OemCryptoHdcpVersion> default_hdcp_level_;
    ConstraintList constraints_;
  };

  void Init(CryptoSession* crypto_session, Clock* clock);

  void DeleteAllKeys();

  Lock status_lock_;
  std::map<KeyId, KeyStatus*> keys_;
  uint32_t current_resolution_;
  int64_t next_check_time_;

  scoped_ptr<Clock> clock_;
  CryptoSession* crypto_session_;

  // For testing
  friend class MaxResEngineTest;
  MaxResEngine(CryptoSession* crypto_session, Clock* clock);

  CORE_DISALLOW_COPY_AND_ASSIGN(MaxResEngine);
};

}  // wvcdm

#endif  // WVCDM_CORE_MAX_RES_ENGINE_H_
