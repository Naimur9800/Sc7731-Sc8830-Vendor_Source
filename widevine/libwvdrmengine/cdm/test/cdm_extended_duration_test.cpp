// Copyright 2013 Google Inc. All Rights Reserved.

#include <errno.h>
#include <getopt.h>
#include <sstream>

#include "clock.h"
#include "config_test_env.h"
#include "device_files.h"
#include "file_store.h"
#include "license_request.h"
#include "license_protocol.pb.h"
#include "log.h"
#include "gtest/gtest.h"
#include "OEMCryptoCENC.h"
#include "oemcrypto_adapter.h"
#include "properties.h"
#include "string_conversions.h"
#include "url_request.h"
#include "wv_cdm_constants.h"
#include "wv_cdm_event_listener.h"
#include "wv_content_decryption_module.h"

namespace {

// HTTP response codes.
const int kHttpOk = 200;
const int kHttpBadRequest = 400;
const int kHttpInternalServerError = 500;

const uint32_t kMinute = 60;
const uint32_t kClockTolerance = 5;
const uint32_t kTwoMinutes = 120;

const uint32_t kMaxUsageTableSize = 50;

// Default license server, can be configured using --server command line option
// Default key id (pssh), can be configured using --keyid command line option
std::string g_client_auth;
wvcdm::ConfigTestEnv* g_config = NULL;
wvcdm::KeyId g_key_id;
wvcdm::CdmKeySystem g_key_system;
std::string g_license_server;
wvcdm::KeyId g_wrong_key_id;
wvcdm::LicenseServerId g_license_server_id = wvcdm::kContentProtectionServer;

std::string kServiceCertificate =
    "0803121028703454C008F63618ADE7443DB6C4C8188BE7F99005228E023082010"
    "A0282010100B52112B8D05D023FCC5D95E2C251C1C649B4177CD8D2BEEF355BB0"
    "6743DE661E3D2ABC3182B79946D55FDC08DFE95407815E9A6274B322A2C7F5E06"
    "7BB5F0AC07A89D45AEA94B2516F075B66EF811D0D26E1B9A6B894F2B9857962AA"
    "171C4F66630D3E4C602718897F5E1EF9B6AAF5AD4DBA2A7E14176DF134A1D3185"
    "B5A218AC05A4C41F081EFFF80A3A040C50B09BBC740EEDCD8F14D675A91980F92"
    "CA7DDC646A06ADAD5101F74A0E498CC01F00532BAC217850BD905E90923656B7D"
    "FEFEF42486767F33EF6283D4F4254AB72589390BEE55808F1D668080D45D893C2"
    "BCA2F74D60A0C0D0A0993CEF01604703334C3638139486BC9DAF24FD67A07F9AD"
    "94302030100013A1273746167696E672E676F6F676C652E636F6D";

// TODO(rfrias): refactor to print out the decryption test names
struct SubSampleInfo {
  bool retrieve_key;
  size_t num_of_subsamples;
  bool validate_key_id;
  bool is_encrypted;
  bool is_secure;
  wvcdm::KeyId key_id;
  std::vector<uint8_t> encrypt_data;
  std::vector<uint8_t> decrypt_data;
  std::vector<uint8_t> iv;
  size_t block_offset;
  uint8_t subsample_flags;
};

SubSampleInfo kEncryptedStreamingNoPstSubSample = {
    // key SD, encrypted, 256b
    true, 1, true, true, false,
    wvcdm::a2bs_hex("371EA35E1A985D75D198A7F41020DC23"),
    wvcdm::a2b_hex(
        "64ab17b3e3dfab47245c7cce4543d4fc7a26dcf248f19f9b59f3c92601440b36"
        "17c8ed0c96c656549e461f38708cd47a434066f8df28ccc28b79252eee3f9c2d"
        "7f6c68ebe40141fe818fe082ca523c03d69ddaf183a93c022327fedc5582c5ab"
        "ca9d342b71263a67f9cb2336f12108aaaef464f17177e44e9b0c4e56e61da53c"
        "2150b4405cc82d994dfd9bf4087c761956d6688a9705db4cf350381085f383c4"
        "9666d4aed135c519c1f0b5cba06e287feea96ea367bf54e7368dcf998276c6e4"
        "6497e0c50e20fef74e42cb518fe7f22ef27202428688f86404e8278587017012"
        "c1d65537c6cbd7dde04aae338d68115a9f430afc100ab83cdadf45dca39db685"),
    wvcdm::a2b_hex(
        "217ce9bde99bd91e9733a1a00b9b557ac3a433dc92633546156817fae26b6e1c"
        "942ac20a89ff79f4c2f25fba99d6a44618a8c0420b27d54e3da17b77c9d43cca"
        "595d259a1e4a8b6d7744cd98c5d3f921adc252eb7d8af6b916044b676a574747"
        "8df21fdc42f166880d97a2225cd5c9ea5e7b752f4cf81bbdbe98e542ee10e1c6"
        "ad868a6ac55c10d564fc23b8acff407daaf4ed2743520e02cda9680d9ea88e91"
        "029359c4cf5906b6ab5bf60fbb3f1a1c7c59acfc7e4fb4ad8e623c04d503a3dd"
        "4884604c8da8a53ce33db9ff8f1c5bb6bb97f37b39906bf41596555c1bcce9ed"
        "08a899cd760ff0899a1170c2f224b9c52997a0785b7fe170805fd3e8b1127659"),
    wvcdm::a2b_hex("f6f4b1e600a5b67813ed2bded913ba9f"), 0, 3};

SubSampleInfo kEncryptedStreamingClip1SubSample = {
    true, 1, true, true, false,
    wvcdm::a2bs_hex("E82DDD1D07CBB31CDD31EBAAE0894609"),
    wvcdm::a2b_hex(
        "fe8670a01c86906c056b4bf85ad278464c4eb79c60de1da8480e66e78561350e"
        "a25ae19a001f834c43aaeadf900b3c5a6745e885a4d1d1ae5bafac08dc1d60e5"
        "f3465da303909ec4b09023490471f670b615d77db844192854fdab52b7806203"
        "89b374594bbb6a2f2fcc31036d7cb8a3f80c0e27637b58a7650028fbf2470d68"
        "1bbd77934af165d215ef325c74438c9d99a20fc628792db28c05ed5deff7d9d4"
        "dba02ddb6cf11dc6e78cb5200940af9a2321c3a7c4c79be67b54a744dae1209c"
        "fa02fc250ce18d30c7da9c3a4a6c9619bf8561a42ff1e55a7b14fa3c8de69196"
        "c2b8e3ff672fc37003b479da5d567b7199917dbe5aa402890ebb066bce140b33"),
    wvcdm::a2b_hex(
        "d08733bd0ef671f467906b50ff8322091400f86fd6f016fea2b86e33923775b3"
        "ebb4c8c6f3ba8b78dd200a74d3872a40264ab99e1d422e4f819abb7f249114aa"
        "b334420b37c86ce81938615ab9d3a6b2de8db545cd88e35091031e73016fb386"
        "1b754298329b52dbe483de3a532277815e659f3e05e89257333225b933d92e15"
        "ef2deff287a192d2c8fc942a29a5f3a1d54440ac6385de7b34bb650b889e4ae9"
        "58c957b5f5ff268f445c0a6b825fcad55290cb7b5c9814bc4c72984dcf4c8fd7"
        "5f511c173b2e0a3163b18a1eac58539e5c188aeb0751b946ad4dcd08ea777a7f"
        "37326df26fa509343faa98dff667629f557873f1284903202e451227ef465a62"),
    wvcdm::a2b_hex("7362b5140c4ce0cd5f863858668d3f1a"), 0, 3};

SubSampleInfo kEncryptedStreamingClip5SubSample = {
    true, 1, true, true, false,
    wvcdm::a2bs_hex("3AE243D83B93B3311A1D777FF5FBE01A"),
    wvcdm::a2b_hex(
        "934997779aa1aeb45d6ba8845f13786575d0adf85a5e93674d9597f8d4286ed7"
        "dcce02f306e502bbd9f1cadf502f354038ca921276d158d911bdf3171d335b18"
        "0ae0f9abece16ff31ee263228354f724da2f3723b19caa38ea02bd6563b01208"
        "fb5bf57854ac0fe38d5883197ef90324b2721ff20fdcf9a53819515e6daa096e"
        "70f6f5c1d29a4a13dafd127e2e1f761ea0e28fd451607552ecbaef5da3c780bc"
        "aaf2667b4cc4f858f01d480cac9e32c3fbb5705e5d2adcceebefc2535c117208"
        "e65f604799fc3d7223e16908550f287a4bea687008cb0064cf14d3aeedb8c705"
        "09ebc5c2b8b5315f43c04d78d2f55f4b32c7d33e157114362106395cc0bb6d93"),
    wvcdm::a2b_hex(
        "2dd54eee1307753508e1f250d637044d6e8f5abf057dab73e9e95f83910e4efc"
        "191c9bac63950f13fd51833dd94a4d03f2b64fb5c721970c418fe53fa6f74ad5"
        "a6e16477a35c7aa6e28909b069cd25770ef80da20918fc30fe95fd5c87fd3522"
        "1649de17ca2c7b3dc31f936f0cbdf97c7b1c15de3a86b279dc4b4de64943914a"
        "99734556c4b7a1a0b022c1933cb0786068fc18d49fed2f2b49f3ac6d01c32d07"
        "92175ce2844eaf9064e6a3fcffade038d690cbed81659351163a22432f0d0545"
        "037e1c805d8e92a1272b4196ad0ce22f26bb80063137a8e454d3b97e2414283d"
        "ed0716cd8bceb80cf59166a217006bd147c51b04dfb183088ce3f51e9b9f759e"),
    wvcdm::a2b_hex("b358ab21ac90455bbf60490daad457e3"), 0, 3};

SubSampleInfo kEncryptedOfflineClip2SubSample = {
    true, 1, true, true, false,
    wvcdm::a2bs_hex("3260F39E12CCF653529990168A3583FF"),
    wvcdm::a2b_hex(
        "3b2cbde084973539329bd5656da22d20396249bf4a18a51c38c4743360cc9fea"
        "a1c78d53de1bd7e14dc5d256fd20a57178a98b83804258c239acd7aa38f2d7d2"
        "eca614965b3d22049e19e236fc1800e60965d8b36415677bf2f843d50a6943c4"
        "683c07c114a32f5e5fbc9939c483c3a1b2ecd3d82b554d649798866191724283"
        "f0ab082eba2da79aaca5c4eaf186f9ee9a0c568f621f705a578f30e4e2ef7b96"
        "5e14cc046ce6dbf272ee5558b098f332333e95fc879dea6c29bf34acdb649650"
        "f08201b9e649960f2493fd7677cc3abf5ae70e5445845c947ba544456b431646"
        "d95a133bff5f57614dda5e4446cd8837901d074149dadf4b775b5b07bb88ca20"),
    wvcdm::a2b_hex(
        "D3EE543581F16AB2EABFA13468133314D19CB6A14A42229BE83543828D801475"
        "FAE1C2C5D193DA8445B9C4B1598E8FCBDF42EFF1FBB54EBC6A4815E2836C2848"
        "15094DEDE76FE4658A2D6EA3E775A872CA71835CF274676C18556C665EC7F32A"
        "4DBB04C10BA988B42758E37DCEFD99D9CE3AFFB1E816C412B4013890E1A31E25"
        "64EBF2125BC54D66FECDF8A31956830121546DC183B3DAC103E223778875B590"
        "3961448C287B367C585E510DA43BF9242B8E9A27B9F6F3EC7E4B5A0A74A1742B"
        "F5CD65EA66D7D9E79C02C7E7D5CD02DB182DDD8EAC3525B0834F1F2822AD0006"
        "944B5080B998BB0FE6E566AAFAE2328B37FD189F1920A964434ECF18E11AC81E"),
    wvcdm::a2b_hex("7362b5140c4ce0cd5f863858668d3f1a"), 0, 3};

std::string kStreamingClip1PstInitData = wvcdm::a2bs_hex(
    "000000427073736800000000"                  // blob size and pssh
    "EDEF8BA979D64ACEA3C827DCD51D21ED00000022"  // Widevine system id
    "08011a0d7769646576696e655f74657374220f73"  // pssh data
    "747265616d696e675f636c697033");

std::string kOfflineClip2PstInitData = wvcdm::a2bs_hex(
    "000000427073736800000000"                  // blob size and pssh
    "EDEF8BA979D64ACEA3C827DCD51D21ED00000020"  // Widevine system id
    "08011a0d7769646576696e655f74657374220d6f"  // pssh data
    "66666c696e655f636c697032");

}  // namespace

namespace wvcdm {
// Protobuf generated classes
using video_widevine_server::sdk::LicenseIdentification;
using video_widevine_server::sdk::LicenseRequest_ContentIdentification;
using video_widevine_server::sdk::ClientIdentification;
using video_widevine_server::sdk::SignedMessage;

class TestWvCdmClientPropertySet : public CdmClientPropertySet {
 public:
  TestWvCdmClientPropertySet()
      : use_privacy_mode_(false),
        is_session_sharing_enabled_(false),
        session_sharing_id_(0) {}
  virtual ~TestWvCdmClientPropertySet() {}

  virtual const std::string& app_id() const { return app_id_; }
  virtual const std::string& security_level() const { return security_level_; }
  virtual const std::string& service_certificate() const {
    return service_certificate_;
  }
  virtual bool use_privacy_mode() const { return use_privacy_mode_; }
  virtual bool is_session_sharing_enabled() const {
    return is_session_sharing_enabled_;
  }
  virtual uint32_t session_sharing_id() const { return session_sharing_id_; }

  void set_app_id(const std::string& app_id) { app_id_ = app_id; }
  void set_security_level(const std::string& security_level) {
    if (!security_level.compare(QUERY_VALUE_SECURITY_LEVEL_L1) ||
        !security_level.compare(QUERY_VALUE_SECURITY_LEVEL_L3)) {
      security_level_ = security_level;
    }
  }
  void set_service_certificate(const std::string& service_certificate) {
    service_certificate_ = service_certificate;
  }
  void set_use_privacy_mode(bool use_privacy_mode) {
    use_privacy_mode_ = use_privacy_mode;
  }
  void set_session_sharing_mode(bool enable) {
    is_session_sharing_enabled_ = enable;
  }
  void set_session_sharing_id(uint32_t id) { session_sharing_id_ = id; }

 private:
  std::string app_id_;
  std::string security_level_;
  std::string service_certificate_;
  bool use_privacy_mode_;
  bool is_session_sharing_enabled_;
  uint32_t session_sharing_id_;
};

class TestWvCdmEventListener : public WvCdmEventListener {
 public:
  TestWvCdmEventListener() : WvCdmEventListener() {}
  virtual void OnEvent(const CdmSessionId& id, CdmEventType event) {
    session_id_ = id;
    event_type_ = event;
  }
  CdmSessionId session_id() { return session_id_; }
  CdmEventType event_type() { return event_type_; }

 private:
  CdmSessionId session_id_;
  CdmEventType event_type_;
};

class WvCdmExtendedDurationTest : public testing::Test {
 public:
  WvCdmExtendedDurationTest() {}
  ~WvCdmExtendedDurationTest() {}

 protected:
  void GetOfflineConfiguration(std::string* key_id, std::string* client_auth) {
    ConfigTestEnv config(g_license_server_id, false);
    if (g_key_id.compare(a2bs_hex(g_config->key_id())) == 0)
      key_id->assign(wvcdm::a2bs_hex(config.key_id()));
    else
      key_id->assign(g_key_id);

    if (g_client_auth.compare(g_config->client_auth()) == 0)
      client_auth->assign(config.client_auth());
    else
      client_auth->assign(g_client_auth);
  }

  void GenerateKeyRequest(const std::string& init_data,
                          CdmLicenseType license_type) {
    wvcdm::CdmAppParameterMap app_parameters;
    std::string server_url;
    EXPECT_EQ(wvcdm::KEY_MESSAGE,
              decryptor_.GenerateKeyRequest(
                  session_id_, key_set_id_, "video/mp4", init_data,
                  license_type, app_parameters, NULL, &key_msg_, &server_url));
    EXPECT_EQ(0u, server_url.size());
  }

  void GenerateRenewalRequest(CdmLicenseType license_type,
                              std::string* server_url) {
    // TODO application makes a license request, CDM will renew the license
    // when appropriate.
    std::string init_data;
    wvcdm::CdmAppParameterMap app_parameters;
    EXPECT_EQ(wvcdm::KEY_MESSAGE,
              decryptor_.GenerateKeyRequest(
                  session_id_, key_set_id_, "video/mp4", init_data,
                  license_type, app_parameters, NULL, &key_msg_, server_url));
    // TODO(edwinwong, rfrias): Add tests cases for when license server url
    // is empty on renewal. Need appropriate key id at the server.
    EXPECT_NE(0u, server_url->size());
  }

  void GenerateKeyRelease(CdmKeySetId key_set_id) {
    CdmSessionId session_id;
    CdmInitData init_data;
    wvcdm::CdmAppParameterMap app_parameters;
    std::string server_url;
    EXPECT_EQ(wvcdm::KEY_MESSAGE,
              decryptor_.GenerateKeyRequest(
                  session_id, key_set_id, "video/mp4", init_data,
                  kLicenseTypeRelease, app_parameters, NULL, &key_msg_,
                  &server_url));
  }

  // Post a request and extract the drm message from the response
  std::string GetKeyRequestResponse(const std::string& server_url,
                                    const std::string& client_auth) {
    // Use secure connection and chunk transfer coding.
    UrlRequest url_request(server_url + client_auth);
    if (!url_request.is_connected()) {
      return "";
    }
    url_request.PostRequest(key_msg_);
    std::string message;
    int resp_bytes = url_request.GetResponse(&message);

    int status_code = url_request.GetStatusCode(message);
    EXPECT_EQ(kHttpOk, status_code);

    std::string drm_msg;
    if (kHttpOk == status_code) {
      LicenseRequest lic_request;
      lic_request.GetDrmMessage(message, drm_msg);
      LOGV("HTTP response body: (%u bytes)", drm_msg.size());
    }
    key_response_ = drm_msg;
    return drm_msg;
  }

  // Post a request and extract the signed provisioning message from
  // the HTTP response.
  std::string GetCertRequestResponse(const std::string& server_url) {
    // Use secure connection and chunk transfer coding.
    UrlRequest url_request(server_url);
    if (!url_request.is_connected()) {
      return "";
    }

    url_request.PostCertRequestInQueryString(key_msg_);
    std::string message;
    int resp_bytes = url_request.GetResponse(&message);
    LOGD("end %d bytes response dump", resp_bytes);

    int status_code = url_request.GetStatusCode(message);
    EXPECT_EQ(kHttpOk, status_code);
    return message;
  }

  // Post a request and extract the signed provisioning message from
  // the HTTP response.
  std::string GetUsageInfoResponse(const std::string& server_url,
                                   const std::string& client_auth,
                                   const std::string& usage_info_request) {
    // Use secure connection and chunk transfer coding.
    UrlRequest url_request(server_url + client_auth);
    if (!url_request.is_connected()) {
      return "";
    }
    url_request.PostRequest(usage_info_request);
    std::string message;
    int resp_bytes = url_request.GetResponse(&message);

    int status_code = url_request.GetStatusCode(message);
    EXPECT_EQ(kHttpOk, status_code);

    std::string usage_info;
    if (kHttpOk == status_code) {
      LicenseRequest license;
      license.GetDrmMessage(message, usage_info);
      LOGV("HTTP response body: (%u bytes)", usage_info.size());
    }
    return usage_info;
  }

  void VerifyKeyRequestResponse(const std::string& server_url,
                                const std::string& client_auth,
                                bool is_renewal) {
    std::string resp = GetKeyRequestResponse(server_url, client_auth);

    if (is_renewal) {
      // TODO application makes a license request, CDM will renew the license
      // when appropriate
      EXPECT_EQ(decryptor_.AddKey(session_id_, resp, &key_set_id_),
                wvcdm::KEY_ADDED);
    } else {
      EXPECT_EQ(decryptor_.AddKey(session_id_, resp, &key_set_id_),
                wvcdm::KEY_ADDED);
    }
  }

  void Unprovision() {
    EXPECT_EQ(NO_ERROR, decryptor_.Unprovision(kSecurityLevelL1));
    EXPECT_EQ(NO_ERROR, decryptor_.Unprovision(kSecurityLevelL3));
  }

  void Provision() {
    CdmResponseType status =
        decryptor_.OpenSession(g_key_system, NULL, &session_id_);
    switch (status) {
      case NO_ERROR:
        decryptor_.CloseSession(session_id_);
        return;
      case NEED_PROVISIONING:
        break;
      default:
        EXPECT_EQ(NO_ERROR, status);
        return;
    }

    std::string provisioning_server_url;
    CdmCertificateType cert_type = kCertificateWidevine;
    std::string cert_authority, cert, wrapped_key;

    status = decryptor_.GetProvisioningRequest(
        cert_type, cert_authority, &key_msg_, &provisioning_server_url);
    EXPECT_EQ(wvcdm::NO_ERROR, status);
    if (NO_ERROR != status) return;
    EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());

    std::string response =
        GetCertRequestResponse(g_config->provisioning_server_url());
    EXPECT_NE(0, static_cast<int>(response.size()));
    EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.HandleProvisioningResponse(
                                   response, &cert, &wrapped_key));
    EXPECT_EQ(0, static_cast<int>(cert.size()));
    EXPECT_EQ(0, static_cast<int>(wrapped_key.size()));
    decryptor_.CloseSession(session_id_);
    return;
  }

  void ValidateResponse(video_widevine_server::sdk::LicenseType license_type,
                        bool has_provider_session_token) {
    // Validate signed response
    SignedMessage signed_message;
    EXPECT_TRUE(signed_message.ParseFromString(key_response_));
    EXPECT_EQ(SignedMessage::LICENSE, signed_message.type());
    EXPECT_TRUE(signed_message.has_signature());
    EXPECT_TRUE(!signed_message.msg().empty());

    // Verify license
    video_widevine_server::sdk::License license;
    EXPECT_TRUE(license.ParseFromString(signed_message.msg()));

    // Verify license identification
    license_id_ = license.id();
    EXPECT_EQ(license_type, license_id_.type());
    EXPECT_EQ(has_provider_session_token,
              license_id_.has_provider_session_token());
  }

  void ValidateRenewalRequest(int64_t expected_seconds_since_started,
                              int64_t expected_seconds_since_last_played,
                              bool has_provider_session_token) {
    // Validate signed renewal request
    SignedMessage signed_message;
    EXPECT_TRUE(signed_message.ParseFromString(key_msg_));
    EXPECT_EQ(SignedMessage::LICENSE_REQUEST, signed_message.type());
    EXPECT_TRUE(signed_message.has_signature());
    EXPECT_TRUE(!signed_message.msg().empty());

    // Verify license request
    video_widevine_server::sdk::LicenseRequest license_renewal;
    EXPECT_TRUE(license_renewal.ParseFromString(signed_message.msg()));

    // Verify Content Identification
    const LicenseRequest_ContentIdentification& content_id =
        license_renewal.content_id();
    EXPECT_FALSE(content_id.has_cenc_id());
    EXPECT_FALSE(content_id.has_webm_id());
    EXPECT_TRUE(content_id.has_license());

    const ::video_widevine_server::sdk::
        LicenseRequest_ContentIdentification_ExistingLicense& existing_license =
            content_id.license();

    const LicenseIdentification& id = existing_license.license_id();
    EXPECT_TRUE(std::equal(id.request_id().begin(), id.request_id().end(),
                           license_id_.request_id().begin()));
    EXPECT_TRUE(std::equal(id.session_id().begin(), id.session_id().end(),
                           license_id_.session_id().begin()));
    EXPECT_TRUE(std::equal(id.purchase_id().begin(), id.purchase_id().end(),
                           license_id_.purchase_id().begin()));
    EXPECT_EQ(license_id_.type(), id.type());
    EXPECT_EQ(license_id_.version(), id.version());
    EXPECT_EQ(has_provider_session_token, !id.provider_session_token().empty());

    EXPECT_LT(expected_seconds_since_started - kClockTolerance,
              existing_license.seconds_since_started());
    EXPECT_LT(existing_license.seconds_since_started(),
              expected_seconds_since_started + kClockTolerance);
    EXPECT_LT(expected_seconds_since_last_played - kClockTolerance,
              existing_license.seconds_since_last_played());
    EXPECT_LT(existing_license.seconds_since_last_played(),
              expected_seconds_since_last_played + kClockTolerance);
    EXPECT_TRUE(existing_license.session_usage_table_entry().empty());

    EXPECT_EQ(::video_widevine_server::sdk::LicenseRequest_RequestType_RENEWAL,
              license_renewal.type());
    EXPECT_LT(0, license_renewal.request_time());
    EXPECT_EQ(video_widevine_server::sdk::VERSION_2_1,
              license_renewal.protocol_version());
    EXPECT_TRUE(license_renewal.has_key_control_nonce());
  }

  void ValidateReleaseRequest(std::string& usage_msg,
                              int64_t expected_seconds_since_license_received,
                              int64_t expected_seconds_since_first_playback,
                              int64_t expected_seconds_since_last_playback) {
    // Validate signed renewal request
    SignedMessage signed_message;
    EXPECT_TRUE(signed_message.ParseFromString(usage_msg));
    EXPECT_EQ(SignedMessage::LICENSE_REQUEST, signed_message.type());
    EXPECT_TRUE(signed_message.has_signature());
    EXPECT_TRUE(!signed_message.msg().empty());

    // Verify license request
    video_widevine_server::sdk::LicenseRequest license_renewal;
    EXPECT_TRUE(license_renewal.ParseFromString(signed_message.msg()));

    // Verify Content Identification
    const LicenseRequest_ContentIdentification& content_id =
        license_renewal.content_id();
    EXPECT_FALSE(content_id.has_cenc_id());
    EXPECT_FALSE(content_id.has_webm_id());
    EXPECT_TRUE(content_id.has_license());

    const ::video_widevine_server::sdk::
        LicenseRequest_ContentIdentification_ExistingLicense& existing_license =
            content_id.license();

    const LicenseIdentification& id = existing_license.license_id();
    EXPECT_TRUE(std::equal(id.request_id().begin(), id.request_id().end(),
                           license_id_.request_id().begin()));
    EXPECT_TRUE(std::equal(id.session_id().begin(), id.session_id().end(),
                           license_id_.session_id().begin()));
    EXPECT_TRUE(std::equal(id.purchase_id().begin(), id.purchase_id().end(),
                           license_id_.purchase_id().begin()));
    EXPECT_EQ(license_id_.type(), id.type());
    EXPECT_EQ(license_id_.version(), id.version());
    EXPECT_TRUE(!id.provider_session_token().empty());

    EXPECT_LT(expected_seconds_since_first_playback - kClockTolerance,
              existing_license.seconds_since_started());
    EXPECT_LT(existing_license.seconds_since_started(),
              expected_seconds_since_first_playback + kClockTolerance);
    EXPECT_LT(expected_seconds_since_last_playback - kClockTolerance,
              existing_license.seconds_since_last_played());
    EXPECT_LT(existing_license.seconds_since_last_played(),
              expected_seconds_since_last_playback + kClockTolerance);
    EXPECT_TRUE(!existing_license.session_usage_table_entry().empty());

    // Verify usage report
    OEMCrypto_PST_Report usage_report;
    memcpy(&usage_report, existing_license.session_usage_table_entry().data(),
           sizeof(usage_report));
    EXPECT_EQ(sizeof(usage_report) + usage_report.pst_length,
              existing_license.session_usage_table_entry().size());
    EXPECT_EQ(kInactive, usage_report.status);
    EXPECT_EQ(id.provider_session_token().size(), usage_report.pst_length);
    std::string pst(existing_license.session_usage_table_entry().data() +
                        sizeof(OEMCrypto_PST_Report),
                    usage_report.pst_length);
    EXPECT_EQ(id.provider_session_token(), pst);
    EXPECT_LE(kInsecureClock, usage_report.clock_security_level);

    int64_t seconds_since_license_received =
        htonll64(usage_report.seconds_since_license_received);
    int64_t seconds_since_first_decrypt =
        htonll64(usage_report.seconds_since_first_decrypt);
    int64_t seconds_since_last_decrypt =
        htonll64(usage_report.seconds_since_last_decrypt);
    // Detect licenses that were never used
    if (seconds_since_first_decrypt < 0 ||
        seconds_since_first_decrypt > seconds_since_license_received) {
      seconds_since_first_decrypt = 0;
      seconds_since_last_decrypt = 0;
    }
    EXPECT_LT(expected_seconds_since_license_received - kClockTolerance,
              seconds_since_license_received);
    EXPECT_LT(seconds_since_license_received,
              expected_seconds_since_license_received + kClockTolerance);
    EXPECT_LT(expected_seconds_since_first_playback - kClockTolerance,
              seconds_since_first_decrypt);
    EXPECT_LT(seconds_since_first_decrypt,
              expected_seconds_since_first_playback + kClockTolerance);
    EXPECT_LT(expected_seconds_since_last_playback - kClockTolerance,
              seconds_since_last_decrypt);
    EXPECT_LT(seconds_since_last_decrypt,
              expected_seconds_since_last_playback + kClockTolerance);

    EXPECT_EQ(::video_widevine_server::sdk::LicenseRequest_RequestType_RELEASE,
              license_renewal.type());
    EXPECT_LT(0, license_renewal.request_time());
    EXPECT_EQ(video_widevine_server::sdk::VERSION_2_1,
              license_renewal.protocol_version());
    EXPECT_TRUE(license_renewal.has_key_control_nonce());
  }

  void QueryKeyStatus(bool streaming, int64_t* license_duration_remaining,
                      int64_t* playback_duration_remaining) {
    CdmQueryMap query_info;
    CdmQueryMap::iterator itr;
    EXPECT_EQ(wvcdm::NO_ERROR,
              decryptor_.QueryKeyStatus(session_id_, &query_info));

    itr = query_info.find(wvcdm::QUERY_KEY_LICENSE_TYPE);
    ASSERT_TRUE(itr != query_info.end());
    if (streaming) {
      EXPECT_EQ(wvcdm::QUERY_VALUE_STREAMING, itr->second);
    } else {
      EXPECT_EQ(wvcdm::QUERY_VALUE_OFFLINE, itr->second);
    }
    itr = query_info.find(wvcdm::QUERY_KEY_PLAY_ALLOWED);
    ASSERT_TRUE(itr != query_info.end());
    EXPECT_EQ(wvcdm::QUERY_VALUE_TRUE, itr->second);
    itr = query_info.find(wvcdm::QUERY_KEY_PERSIST_ALLOWED);
    ASSERT_TRUE(itr != query_info.end());
    if (streaming) {
      EXPECT_EQ(wvcdm::QUERY_VALUE_FALSE, itr->second);
    } else {
      EXPECT_EQ(wvcdm::QUERY_VALUE_TRUE, itr->second);
    }
    itr = query_info.find(wvcdm::QUERY_KEY_RENEW_ALLOWED);
    ASSERT_TRUE(itr != query_info.end());
    EXPECT_EQ(wvcdm::QUERY_VALUE_TRUE, itr->second);

    std::istringstream ss;
    itr = query_info.find(wvcdm::QUERY_KEY_LICENSE_DURATION_REMAINING);
    ASSERT_TRUE(itr != query_info.end());
    ss.str(itr->second);
    ASSERT_TRUE(ss >> *license_duration_remaining);
    EXPECT_LT(0, *license_duration_remaining);
    itr = query_info.find(wvcdm::QUERY_KEY_PLAYBACK_DURATION_REMAINING);
    ASSERT_TRUE(itr != query_info.end());
    ss.clear();
    ss.str(itr->second);
    ASSERT_TRUE(ss >> *playback_duration_remaining);
    EXPECT_LT(0, *playback_duration_remaining);

    itr = query_info.find(wvcdm::QUERY_KEY_RENEWAL_SERVER_URL);
    ASSERT_TRUE(itr != query_info.end());
    EXPECT_LT(0u, itr->second.size());
  }

  std::string GetSecurityLevel(TestWvCdmClientPropertySet* property_set) {
    decryptor_.OpenSession(g_key_system, property_set, &session_id_);
    CdmQueryMap query_info;
    EXPECT_EQ(wvcdm::NO_ERROR,
              decryptor_.QuerySessionStatus(session_id_, &query_info));
    CdmQueryMap::iterator itr =
        query_info.find(wvcdm::QUERY_KEY_SECURITY_LEVEL);
    EXPECT_TRUE(itr != query_info.end());
    decryptor_.CloseSession(session_id_);
    return itr->second;
  }

  CdmSecurityLevel GetDefaultSecurityLevel() {
    std::string level = GetSecurityLevel(NULL).c_str();
    CdmSecurityLevel security_level = kSecurityLevelUninitialized;
    if (level.compare(wvcdm::QUERY_VALUE_SECURITY_LEVEL_L1) == 0) {
      security_level = kSecurityLevelL1;
    } else if (level.compare(wvcdm::QUERY_VALUE_SECURITY_LEVEL_L3) == 0) {
      security_level = kSecurityLevelL3;
    } else {
      EXPECT_TRUE(false);
    }
    return security_level;
  }

  wvcdm::WvContentDecryptionModule decryptor_;
  CdmKeyMessage key_msg_;
  CdmKeyResponse key_response_;
  CdmSessionId session_id_;
  CdmKeySetId key_set_id_;
  video_widevine_server::sdk::LicenseIdentification license_id_;
};

TEST_F(WvCdmExtendedDurationTest, VerifyLicenseRequestTest) {
  Provision();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);

  EXPECT_TRUE(!key_msg_.empty());

  // Validate signed request
  SignedMessage signed_message;
  EXPECT_TRUE(signed_message.ParseFromString(key_msg_));
  EXPECT_EQ(SignedMessage::LICENSE_REQUEST, signed_message.type());
  EXPECT_TRUE(signed_message.has_signature());
  EXPECT_TRUE(!signed_message.msg().empty());

  // Verify license request
  video_widevine_server::sdk::LicenseRequest license_request;
  EXPECT_TRUE(license_request.ParseFromString(signed_message.msg()));

  // Verify Client Identification
  const ClientIdentification& client_id = license_request.client_id();
  EXPECT_EQ(video_widevine_server::sdk::
                ClientIdentification_TokenType_DEVICE_CERTIFICATE,
            client_id.type());

  EXPECT_LT(0, client_id.client_info_size());
  for (int i = 0; i < client_id.client_info_size(); ++i) {
    const ::video_widevine_server::sdk::ClientIdentification_NameValue&
        name_value = client_id.client_info(i);
    EXPECT_TRUE(!name_value.name().empty());
    EXPECT_TRUE(!name_value.value().empty());
  }

  EXPECT_FALSE(client_id.has_provider_client_token());
  EXPECT_FALSE(client_id.has_license_counter());

  const ::video_widevine_server::sdk::ClientIdentification_ClientCapabilities&
      client_capabilities = client_id.client_capabilities();
  EXPECT_FALSE(client_capabilities.has_client_token());
  EXPECT_TRUE(client_capabilities.has_session_token());
  EXPECT_FALSE(client_capabilities.video_resolution_constraints());
  EXPECT_TRUE(client_capabilities.has_max_hdcp_version());
  EXPECT_TRUE(client_capabilities.has_oem_crypto_api_version());

  // Verify Content Identification
  const LicenseRequest_ContentIdentification& content_id =
      license_request.content_id();
  EXPECT_TRUE(content_id.has_cenc_id());
  EXPECT_FALSE(content_id.has_webm_id());
  EXPECT_FALSE(content_id.has_license());

  const ::video_widevine_server::sdk::LicenseRequest_ContentIdentification_CENC&
      cenc_id = content_id.cenc_id();
  EXPECT_TRUE(std::equal(cenc_id.pssh(0).begin(), cenc_id.pssh(0).end(),
                         g_key_id.begin() + 32));
  EXPECT_EQ(video_widevine_server::sdk::STREAMING, cenc_id.license_type());
  EXPECT_TRUE(cenc_id.has_request_id());

  // Verify other license request fields
  EXPECT_EQ(::video_widevine_server::sdk::LicenseRequest_RequestType_NEW,
            license_request.type());
  EXPECT_LT(0, license_request.request_time());
  EXPECT_EQ(video_widevine_server::sdk::VERSION_2_1,
            license_request.protocol_version());
  EXPECT_TRUE(license_request.has_key_control_nonce());

  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmExtendedDurationTest, VerifyLicenseRenewalTest) {
  Provision();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  // Validate signed response
  SignedMessage signed_message;
  EXPECT_TRUE(signed_message.ParseFromString(key_response_));
  EXPECT_EQ(SignedMessage::LICENSE, signed_message.type());
  EXPECT_TRUE(signed_message.has_signature());
  EXPECT_TRUE(!signed_message.msg().empty());

  // Verify license
  video_widevine_server::sdk::License license;
  EXPECT_TRUE(license.ParseFromString(signed_message.msg()));

  // Verify license identification
  video_widevine_server::sdk::LicenseIdentification license_id = license.id();
  EXPECT_LT(0, license_id.request_id().size());
  EXPECT_LT(0, license_id.session_id().size());
  EXPECT_EQ(video_widevine_server::sdk::STREAMING, license_id.type());
  EXPECT_FALSE(license_id.has_provider_session_token());

  // Create renewal request
  std::string license_server;
  GenerateRenewalRequest(kLicenseTypeStreaming, &license_server);
  EXPECT_TRUE(!license_server.empty());
  EXPECT_TRUE(!key_msg_.empty());

  // Validate signed renewal request
  signed_message.Clear();
  EXPECT_TRUE(signed_message.ParseFromString(key_msg_));
  EXPECT_EQ(SignedMessage::LICENSE_REQUEST, signed_message.type());
  EXPECT_TRUE(signed_message.has_signature());
  EXPECT_TRUE(!signed_message.msg().empty());

  // Verify license request
  video_widevine_server::sdk::LicenseRequest license_renewal;
  EXPECT_TRUE(license_renewal.ParseFromString(signed_message.msg()));

  // Client Identification not filled in in renewal

  // Verify Content Identification
  const LicenseRequest_ContentIdentification& content_id =
      license_renewal.content_id();
  EXPECT_FALSE(content_id.has_cenc_id());
  EXPECT_FALSE(content_id.has_webm_id());
  EXPECT_TRUE(content_id.has_license());

  const ::video_widevine_server::sdk::
      LicenseRequest_ContentIdentification_ExistingLicense& existing_license =
          content_id.license();

  const LicenseIdentification& id = existing_license.license_id();
  EXPECT_TRUE(std::equal(id.request_id().begin(), id.request_id().end(),
                         license_id.request_id().begin()));
  EXPECT_TRUE(std::equal(id.session_id().begin(), id.session_id().end(),
                         license_id.session_id().begin()));
  EXPECT_TRUE(std::equal(id.purchase_id().begin(), id.purchase_id().end(),
                         license_id.purchase_id().begin()));
  EXPECT_EQ(license_id.type(), id.type());
  EXPECT_EQ(license_id.version(), id.version());
  EXPECT_TRUE(id.provider_session_token().empty());

  EXPECT_EQ(0, existing_license.seconds_since_started());
  EXPECT_EQ(0, existing_license.seconds_since_last_played());
  EXPECT_TRUE(existing_license.session_usage_table_entry().empty());

  EXPECT_EQ(::video_widevine_server::sdk::LicenseRequest_RequestType_RENEWAL,
            license_renewal.type());
  EXPECT_LT(0, license_renewal.request_time());
  EXPECT_EQ(video_widevine_server::sdk::VERSION_2_1,
            license_renewal.protocol_version());
  EXPECT_TRUE(license_renewal.has_key_control_nonce());

  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmExtendedDurationTest, UsageOverflowTest) {
  Provision();
  SubSampleInfo* data = &kEncryptedStreamingClip5SubSample;
  TestWvCdmClientPropertySet client_property_set;
  TestWvCdmClientPropertySet* property_set = NULL;

  CdmSecurityLevel security_level = GetDefaultSecurityLevel();
  DeviceFiles handle;
  EXPECT_TRUE(handle.Init(security_level));
  File file;
  handle.SetTestFile(&file);
  EXPECT_TRUE(handle.DeleteAllUsageInfoForApp(""));

  for (size_t i = 0; i < kMaxUsageTableSize + 100; ++i) {
    decryptor_.OpenSession(g_key_system, property_set, &session_id_);
    std::string key_id = a2bs_hex(
        "000000427073736800000000"                  // blob size and pssh
        "EDEF8BA979D64ACEA3C827DCD51D21ED00000022"  // Widevine system id
        "08011a0d7769646576696e655f74657374220f73"  // pssh data
        "747265616d696e675f636c697035");

    GenerateKeyRequest(key_id, kLicenseTypeStreaming);
    VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
    decryptor_.CloseSession(session_id_);
  }

  uint32_t num_usage_info = 0;
  CdmUsageInfo usage_info;
  CdmUsageInfoReleaseMessage release_msg;
  CdmResponseType status = decryptor_.GetUsageInfo("", &usage_info);
  EXPECT_EQ(usage_info.empty() ? NO_ERROR : KEY_MESSAGE, status);
  while (usage_info.size() > 0) {
    for (size_t i = 0; i < usage_info.size(); ++i) {
      release_msg =
          GetUsageInfoResponse(g_license_server, g_client_auth, usage_info[i]);
      EXPECT_EQ(NO_ERROR, decryptor_.ReleaseUsageInfo(release_msg));
    }
    status = decryptor_.GetUsageInfo("", &usage_info);
    switch (status) {
      case KEY_MESSAGE: EXPECT_FALSE(usage_info.empty()); break;
      case NO_ERROR: EXPECT_TRUE(usage_info.empty()); break;
      default: FAIL() << "GetUsageInfo failed with error "
                    << static_cast<int>(status) ; break;
    }
  }
}

class WvCdmStreamingNoPstTest : public WvCdmExtendedDurationTest,
                                public ::testing::WithParamInterface<size_t> {};

TEST_P(WvCdmStreamingNoPstTest, UsageTest) {
  Unprovision();
  Provision();

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  ValidateResponse(video_widevine_server::sdk::STREAMING, false);

  int64_t initial_license_duration_remaining = 0;
  int64_t initial_playback_duration_remaining = 0;
  QueryKeyStatus(true, &initial_license_duration_remaining,
                 &initial_playback_duration_remaining);

  sleep(kMinute);
  int64_t expected_seconds_since_license_received = kMinute;
  int64_t expected_seconds_since_initial_playback = 0;
  int64_t expected_seconds_since_last_playback = 0;

  for (size_t i = 0; i < GetParam(); ++i) {
    // Decrypt data
    SubSampleInfo* data = &kEncryptedStreamingNoPstSubSample;
    for (size_t i = 0; i < data->num_of_subsamples; i++) {
      std::vector<uint8_t> decrypt_buffer((data + i)->encrypt_data.size());
      CdmDecryptionParameters decryption_parameters(
          &(data + i)->key_id, &(data + i)->encrypt_data.front(),
          (data + i)->encrypt_data.size(), &(data + i)->iv,
          (data + i)->block_offset, &decrypt_buffer[0]);
      decryption_parameters.is_encrypted = (data + i)->is_encrypted;
      decryption_parameters.is_secure = (data + i)->is_secure;
      decryption_parameters.subsample_flags = (data + i)->subsample_flags;
      EXPECT_EQ(NO_ERROR,
                decryptor_.Decrypt(session_id_, (data + i)->validate_key_id,
                                   decryption_parameters));

      EXPECT_TRUE(std::equal((data + i)->decrypt_data.begin(),
                             (data + i)->decrypt_data.end(),
                             decrypt_buffer.begin()));
    }

    sleep(kMinute);
    expected_seconds_since_license_received += kMinute;
    expected_seconds_since_initial_playback += kMinute;
    expected_seconds_since_last_playback = kMinute;
  }

  // Create renewal request and validate
  std::string license_server;
  GenerateRenewalRequest(kLicenseTypeStreaming, &license_server);
  EXPECT_TRUE(!license_server.empty());
  EXPECT_TRUE(!key_msg_.empty());

  ValidateRenewalRequest(expected_seconds_since_initial_playback,
                         expected_seconds_since_last_playback, false);

  // Query and validate usage information
  int64_t license_duration_remaining = 0;
  int64_t playback_duration_remaining = 0;
  QueryKeyStatus(true, &license_duration_remaining,
                 &playback_duration_remaining);

  int64_t delta =
      initial_license_duration_remaining - license_duration_remaining;
  EXPECT_LT(expected_seconds_since_license_received - kClockTolerance, delta);
  EXPECT_LT(delta, expected_seconds_since_license_received + kClockTolerance);
  delta = initial_playback_duration_remaining - playback_duration_remaining;
  EXPECT_LT(expected_seconds_since_initial_playback - kClockTolerance, delta);
  EXPECT_LT(delta, expected_seconds_since_initial_playback + kClockTolerance);

  decryptor_.CloseSession(session_id_);
}

INSTANTIATE_TEST_CASE_P(Cdm, WvCdmStreamingNoPstTest,
                        ::testing::Values(0, 1, 2));

class WvCdmStreamingPstTest : public WvCdmExtendedDurationTest,
                              public ::testing::WithParamInterface<size_t> {};

TEST_P(WvCdmStreamingPstTest, UsageTest) {
  Unprovision();
  Provision();

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(kStreamingClip1PstInitData, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  ValidateResponse(video_widevine_server::sdk::STREAMING, true);

  int64_t initial_license_duration_remaining = 0;
  int64_t initial_playback_duration_remaining = 0;
  QueryKeyStatus(true, &initial_license_duration_remaining,
                 &initial_playback_duration_remaining);

  sleep(kMinute);
  int64_t expected_seconds_since_license_received = kMinute;
  int64_t expected_seconds_since_initial_playback = 0;
  int64_t expected_seconds_since_last_playback = 0;

  for (size_t i = 0; i < GetParam(); ++i) {
    // Decrypt data
    SubSampleInfo* data = &kEncryptedStreamingClip1SubSample;
    for (size_t i = 0; i < data->num_of_subsamples; i++) {
      std::vector<uint8_t> decrypt_buffer((data + i)->encrypt_data.size());
      CdmDecryptionParameters decryption_parameters(
          &(data + i)->key_id, &(data + i)->encrypt_data.front(),
          (data + i)->encrypt_data.size(), &(data + i)->iv,
          (data + i)->block_offset, &decrypt_buffer[0]);
      decryption_parameters.is_encrypted = (data + i)->is_encrypted;
      decryption_parameters.is_secure = (data + i)->is_secure;
      decryption_parameters.subsample_flags = (data + i)->subsample_flags;
      EXPECT_EQ(NO_ERROR,
                decryptor_.Decrypt(session_id_, (data + i)->validate_key_id,
                                   decryption_parameters));

      EXPECT_TRUE(std::equal((data + i)->decrypt_data.begin(),
                             (data + i)->decrypt_data.end(),
                             decrypt_buffer.begin()));
    }

    sleep(kMinute);
    expected_seconds_since_license_received += kMinute;
    expected_seconds_since_initial_playback += kMinute;
    expected_seconds_since_last_playback = kMinute;
  }

  // Create renewal request and validate
  std::string license_server;
  GenerateRenewalRequest(kLicenseTypeStreaming, &license_server);
  EXPECT_TRUE(!license_server.empty());
  EXPECT_TRUE(!key_msg_.empty());

  ValidateRenewalRequest(expected_seconds_since_initial_playback,
                         expected_seconds_since_last_playback, true);

  // Query and validate usage information
  int64_t license_duration_remaining = 0;
  int64_t playback_duration_remaining = 0;
  QueryKeyStatus(true, &license_duration_remaining,
                 &playback_duration_remaining);

  int64_t delta =
      initial_license_duration_remaining - license_duration_remaining;
  EXPECT_LT(expected_seconds_since_license_received - kClockTolerance, delta);
  EXPECT_LT(delta, expected_seconds_since_license_received + kClockTolerance);
  delta = initial_playback_duration_remaining - playback_duration_remaining;
  EXPECT_LT(expected_seconds_since_initial_playback - kClockTolerance, delta);
  EXPECT_LT(delta, expected_seconds_since_initial_playback + kClockTolerance);

  decryptor_.CloseSession(session_id_);
}

INSTANTIATE_TEST_CASE_P(Cdm, WvCdmStreamingPstTest, ::testing::Values(0, 1, 2));

class WvCdmStreamingUsageReportTest
    : public WvCdmExtendedDurationTest,
      public ::testing::WithParamInterface<size_t> {};

TEST_P(WvCdmStreamingUsageReportTest, UsageTest) {
  Unprovision();
  Provision();

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(kStreamingClip1PstInitData, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  ValidateResponse(video_widevine_server::sdk::STREAMING, true);

  int64_t initial_license_duration_remaining = 0;
  int64_t initial_playback_duration_remaining = 0;
  QueryKeyStatus(true, &initial_license_duration_remaining,
                 &initial_playback_duration_remaining);

  sleep(kMinute);
  int64_t expected_seconds_since_license_received = kMinute;
  int64_t expected_seconds_since_initial_playback = 0;
  int64_t expected_seconds_since_last_playback = 0;

  for (size_t i = 0; i < GetParam(); ++i) {
    // Decrypt data
    SubSampleInfo* data = &kEncryptedStreamingClip1SubSample;
    for (size_t i = 0; i < data->num_of_subsamples; i++) {
      std::vector<uint8_t> decrypt_buffer((data + i)->encrypt_data.size());
      CdmDecryptionParameters decryption_parameters(
          &(data + i)->key_id, &(data + i)->encrypt_data.front(),
          (data + i)->encrypt_data.size(), &(data + i)->iv,
          (data + i)->block_offset, &decrypt_buffer[0]);
      decryption_parameters.is_encrypted = (data + i)->is_encrypted;
      decryption_parameters.is_secure = (data + i)->is_secure;
      decryption_parameters.subsample_flags = (data + i)->subsample_flags;
      EXPECT_EQ(NO_ERROR,
                decryptor_.Decrypt(session_id_, (data + i)->validate_key_id,
                                   decryption_parameters));

      EXPECT_TRUE(std::equal((data + i)->decrypt_data.begin(),
                             (data + i)->decrypt_data.end(),
                             decrypt_buffer.begin()));
    }

    sleep(kMinute);
    expected_seconds_since_license_received += kMinute;
    expected_seconds_since_initial_playback += kMinute;
    expected_seconds_since_last_playback = kMinute;
  }

  // Query and validate usage information
  int64_t license_duration_remaining = 0;
  int64_t playback_duration_remaining = 0;
  QueryKeyStatus(true, &license_duration_remaining,
                 &playback_duration_remaining);

  int64_t delta =
      initial_license_duration_remaining - license_duration_remaining;
  EXPECT_LT(expected_seconds_since_license_received - kClockTolerance, delta);
  EXPECT_LT(delta, expected_seconds_since_license_received + kClockTolerance);
  delta = initial_playback_duration_remaining - playback_duration_remaining;
  EXPECT_LT(expected_seconds_since_initial_playback - kClockTolerance, delta);
  EXPECT_LT(delta, expected_seconds_since_initial_playback + kClockTolerance);

  decryptor_.CloseSession(session_id_);

  // Create usage report and validate
  uint32_t num_usage_info = 0;
  CdmUsageInfo usage_info;
  CdmUsageInfoReleaseMessage release_msg;
  std::string app_id;
  CdmResponseType status = decryptor_.GetUsageInfo(app_id, &usage_info);
  EXPECT_EQ(usage_info.empty() ? NO_ERROR : KEY_MESSAGE, status);
  while (usage_info.size() > 0) {
    for (size_t i = 0; i < usage_info.size(); ++i) {
      ValidateReleaseRequest(usage_info[i],
                             expected_seconds_since_license_received,
                             expected_seconds_since_initial_playback,
                             expected_seconds_since_last_playback);
      release_msg =
          GetUsageInfoResponse(g_license_server, g_client_auth, usage_info[i]);
      EXPECT_EQ(NO_ERROR, decryptor_.ReleaseUsageInfo(release_msg));
    }
    status = decryptor_.GetUsageInfo(app_id, &usage_info);
    switch (status) {
      case KEY_MESSAGE:
        EXPECT_FALSE(usage_info.empty());
        break;
      case NO_ERROR:
        EXPECT_TRUE(usage_info.empty());
        break;
      default:
        FAIL() << "GetUsageInfo failed with error " << static_cast<int>(status);
        break;
    }
  }
}

INSTANTIATE_TEST_CASE_P(Cdm, WvCdmStreamingUsageReportTest,
                        ::testing::Values(0, 1, 2));

class WvCdmOfflineUsageReportTest
    : public WvCdmExtendedDurationTest,
      public ::testing::WithParamInterface<size_t> {};

TEST_P(WvCdmOfflineUsageReportTest, UsageTest) {
  Unprovision();
  Provision();

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(kOfflineClip2PstInitData, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  ValidateResponse(video_widevine_server::sdk::OFFLINE, true);

  CdmKeySetId key_set_id = key_set_id_;
  EXPECT_FALSE(key_set_id_.empty());

  int64_t initial_license_duration_remaining = 0;
  int64_t initial_playback_duration_remaining = 0;
  QueryKeyStatus(false, &initial_license_duration_remaining,
                 &initial_playback_duration_remaining);

  decryptor_.CloseSession(session_id_);

  sleep(kMinute);
  int64_t expected_seconds_since_license_received = kMinute;
  int64_t expected_seconds_since_initial_playback = 0;
  int64_t expected_seconds_since_last_playback = 0;

  for (size_t i = 0; i < GetParam(); ++i) {
    session_id_.clear();
    decryptor_.OpenSession(g_key_system, NULL, &session_id_);
    EXPECT_EQ(wvcdm::KEY_ADDED, decryptor_.RestoreKey(session_id_, key_set_id));

    // Query and validate usage information
    int64_t license_duration_remaining = 0;
    int64_t playback_duration_remaining = 0;
    QueryKeyStatus(false, &license_duration_remaining,
                   &playback_duration_remaining);

    int64_t delta =
        initial_license_duration_remaining - license_duration_remaining;
    EXPECT_LT(expected_seconds_since_license_received - kClockTolerance, delta);
    EXPECT_LT(delta, expected_seconds_since_license_received + kClockTolerance);
    delta = initial_playback_duration_remaining - playback_duration_remaining;
    EXPECT_LT(expected_seconds_since_initial_playback - kClockTolerance, delta);
    EXPECT_LT(delta, expected_seconds_since_initial_playback + kClockTolerance);

    // Decrypt data
    SubSampleInfo* data = &kEncryptedOfflineClip2SubSample;
    for (size_t i = 0; i < data->num_of_subsamples; i++) {
      std::vector<uint8_t> decrypt_buffer((data + i)->encrypt_data.size());
      CdmDecryptionParameters decryption_parameters(
          &(data + i)->key_id, &(data + i)->encrypt_data.front(),
          (data + i)->encrypt_data.size(), &(data + i)->iv,
          (data + i)->block_offset, &decrypt_buffer[0]);
      decryption_parameters.is_encrypted = (data + i)->is_encrypted;
      decryption_parameters.is_secure = (data + i)->is_secure;
      decryption_parameters.subsample_flags = (data + i)->subsample_flags;
      EXPECT_EQ(NO_ERROR,
                decryptor_.Decrypt(session_id_, (data + i)->validate_key_id,
                                   decryption_parameters));

      EXPECT_TRUE(std::equal((data + i)->decrypt_data.begin(),
                             (data + i)->decrypt_data.end(),
                             decrypt_buffer.begin()));
    }

    sleep(10);
    decryptor_.CloseSession(session_id_);

    sleep(kMinute - 10);
    expected_seconds_since_license_received += kMinute;
    expected_seconds_since_initial_playback += kMinute;
    expected_seconds_since_last_playback = kMinute;
  }

  session_id_.clear();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  EXPECT_EQ(wvcdm::KEY_ADDED, decryptor_.RestoreKey(session_id_, key_set_id));

  // Query and validate usage information
  int64_t license_duration_remaining = 0;
  int64_t playback_duration_remaining = 0;
  QueryKeyStatus(false, &license_duration_remaining,
                 &playback_duration_remaining);

  int64_t delta =
      initial_license_duration_remaining - license_duration_remaining;
  EXPECT_LT(expected_seconds_since_license_received - kClockTolerance, delta);
  EXPECT_LT(delta, expected_seconds_since_license_received + kClockTolerance);
  delta = initial_playback_duration_remaining - playback_duration_remaining;
  EXPECT_LT(expected_seconds_since_initial_playback - kClockTolerance, delta);
  EXPECT_LT(delta, expected_seconds_since_initial_playback + kClockTolerance);

  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  GenerateKeyRelease(key_set_id);
  ValidateReleaseRequest(key_msg_, expected_seconds_since_license_received,
                         expected_seconds_since_initial_playback,
                         expected_seconds_since_last_playback);
  key_set_id_ = key_set_id;
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
}

INSTANTIATE_TEST_CASE_P(Cdm, WvCdmOfflineUsageReportTest,
                        ::testing::Values(0, 1, 2));
}  // namespace wvcdm

void show_menu(char* prog_name) {
  std::cout << std::endl;
  std::cout << "usage: " << prog_name << " [options]" << std::endl << std::endl;
  std::cout << "  enclose multiple arguments in '' when using adb shell"
            << std::endl;
  std::cout << "  e.g. adb shell '" << prog_name << " --server=\"url\"'"
            << std::endl;
  std::cout << "   or  adb shell '" << prog_name << " -u\"url\"'" << std::endl
            << std::endl;

  std::cout << std::setw(35) << std::left << "  -f/--use_full_path";
  std::cout << "specify server url is not a proxy server" << std::endl;
  std::cout << std::endl;

  std::cout << std::setw(35) << std::left << "  -i/--license_server_id=<gp/cp>";
  std::cout << "specifies which default server settings to use: " << std::endl;
  std::cout << std::setw(35) << std::left << " ";
  std::cout << "gp (case sensitive) for GooglePlay server" << std::endl;
  std::cout << std::setw(35) << std::left << " ";
  std::cout << "cp (case sensitive) for Content Protection server" << std::endl
            << std::endl;

  std::cout << std::setw(35) << std::left << "  -k/--keyid=<key_id>";
  std::cout << "configure the key id or pssh, in hex format" << std::endl
            << std::endl;

  std::cout << std::setw(35) << std::left << "  -p/--port=<port>";
  std::cout << "specifies the connection port" << std::endl << std::endl;

  std::cout << std::setw(35) << std::left << "  -s/--secure_transfer";
  std::cout << "use https transfer protocol" << std::endl << std::endl;

  std::cout << std::setw(35) << std::left << "  -u/--server=<server_url>";
  std::cout
      << "configure the license server url, please include http[s] in the url"
      << std::endl << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  bool show_usage = false;
  static const struct option long_options[] = {
      {"keyid", required_argument, NULL, 'k'},
      {"license_server_id", required_argument, NULL, 'i'},
      {"license_server_url", required_argument, NULL, 'u'},
      {NULL, 0, NULL, '\0'}};

  int option_index = 0;
  int opt = 0;
  while ((opt = getopt_long(argc, argv, "i:k:u:", long_options,
                            &option_index)) != -1) {
    switch (opt) {
      case 'i': {
        std::string license_id(optarg);
        if (!license_id.compare("gp")) {
          g_license_server_id = wvcdm::kGooglePlayServer;
        } else if (!license_id.compare("cp")) {
          g_license_server_id = wvcdm::kContentProtectionServer;
        } else {
          std::cout << "Invalid license server id" << optarg << std::endl;
          show_usage = true;
        }
        break;
      }
      case 'k': {
        g_key_id.clear();
        g_key_id.assign(optarg);
        break;
      }
      case 'u': {
        g_license_server.clear();
        g_license_server.assign(optarg);
        break;
      }
      case '?': {
        show_usage = true;
        break;
      }
    }
  }

  if (show_usage) {
    show_menu(argv[0]);
    return 0;
  }

  g_config = new wvcdm::ConfigTestEnv(g_license_server_id);
  g_client_auth.assign(g_config->client_auth());
  g_key_system.assign(g_config->key_system());
  g_wrong_key_id.assign(g_config->wrong_key_id());

  // The following variables are configurable through command line
  // options. If the command line arguments are absent, use the settings
  // in kLicenseServers[] pointed to by g_config.
  if (g_key_id.empty()) {
    g_key_id.assign(g_config->key_id());
  }
  if (g_license_server.empty()) {
    g_license_server.assign(g_config->license_server());
  }

  // Displays server url, port and key Id being used
  std::cout << std::endl;
  std::cout << "Server: " << g_license_server << std::endl;
  std::cout << "KeyID: " << g_key_id << std::endl << std::endl;

  g_key_id = wvcdm::a2bs_hex(g_key_id);
  g_config->set_license_server(g_license_server);

  int status = RUN_ALL_TESTS();
  delete g_config;
  return status;
}
