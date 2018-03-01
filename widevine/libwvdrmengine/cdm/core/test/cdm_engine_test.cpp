// Copyright 2013 Google Inc. All Rights Reserved.
// These tests are for the cdm engine, and code below it in the stack.  In
// particular, we assume that the OEMCrypo layer works, and has a valid keybox.
// This is because we need a valid RSA certificate, and will attempt to connect
// to the provisioning server to request one if we don't.

#include <errno.h>
#include <getopt.h>

#include <string>

#if defined(CHROMIUM_BUILD)
#include "base/at_exit.h"
#include "base/message_loop/message_loop.h"
#endif
#include "cdm_engine.h"
#include "config_test_env.h"
#include "gtest/gtest.h"
#include "initialization_data.h"
#include "license_request.h"
#include "log.h"
#include "properties.h"
#include "scoped_ptr.h"
#include "string_conversions.h"
#include "test_printers.h"
#include "url_request.h"
#include "wv_cdm_constants.h"
#include "wv_cdm_types.h"

namespace {
// Http OK response code.
const int kHttpOk = 200;

// Default license server, can be configured using --server command line option
// Default key id (pssh), can be configured using --keyid command line option
std::string g_client_auth;
wvcdm::KeyId g_key_id_pssh;
wvcdm::KeyId g_key_id_unwrapped;
wvcdm::CdmKeySystem g_key_system;
std::string g_license_server;
wvcdm::KeyId g_wrong_key_id;

const std::string kCencMimeType = "video/mp4";
const std::string kWebmMimeType = "video/webm";

}  // namespace

namespace wvcdm {

class WvCdmEngineTest : public testing::Test {
 public:
  virtual void SetUp() {
    CdmResponseType status = cdm_engine_.OpenSession(g_key_system, NULL, &session_id_);
    if (status == NEED_PROVISIONING) {
      Provision();
      status = cdm_engine_.OpenSession(g_key_system, NULL, &session_id_);
    }
    ASSERT_EQ(NO_ERROR, status);
    ASSERT_NE("", session_id_) << "Could not open CDM session.";
  }

  virtual void TearDown() { cdm_engine_.CloseSession(session_id_); }

 protected:
  void Provision() {
    CdmProvisioningRequest prov_request;
    std::string provisioning_server_url;
    CdmCertificateType cert_type = kCertificateWidevine;
    std::string cert_authority;
    std::string cert, wrapped_key;
    ASSERT_EQ(NO_ERROR,
              cdm_engine_.GetProvisioningRequest(cert_type,
                                                cert_authority,
                                                &prov_request,
                                                &provisioning_server_url));
    UrlRequest url_request(provisioning_server_url);
    url_request.PostCertRequestInQueryString(prov_request);
    std::string message;
    bool ok = url_request.GetResponse(&message);
    EXPECT_TRUE(ok);
    ASSERT_EQ(NO_ERROR,
              cdm_engine_.HandleProvisioningResponse(message,
                                                    &cert, &wrapped_key));
  }

  void GenerateKeyRequest(const std::string& key_id,
                          const std::string& init_data_type_string) {
    CdmAppParameterMap app_parameters;
    std::string server_url;
    CdmKeySetId key_set_id;

    InitializationData init_data(init_data_type_string, key_id);

    EXPECT_EQ(KEY_MESSAGE,
              cdm_engine_.GenerateKeyRequest(session_id_,
                                             key_set_id,
                                             init_data,
                                             kLicenseTypeStreaming,
                                             app_parameters,
                                             &key_msg_,
                                             &server_url));
  }

  void GenerateRenewalRequest() {
    EXPECT_EQ(KEY_MESSAGE,
              cdm_engine_.GenerateRenewalRequest(session_id_,
                                                 &key_msg_,
                                                 &server_url_));
  }

  std::string GetKeyRequestResponse(const std::string& server_url,
                                    const std::string& client_auth) {
    return GetKeyRequestResponse(server_url, client_auth, true);
  }
  std::string FailToGetKeyRequestResponse(const std::string& server_url,
                                          const std::string& client_auth) {
    return GetKeyRequestResponse(server_url, client_auth, false);
  }

  // posts a request and extracts the drm message from the response
  std::string GetKeyRequestResponse(const std::string& server_url,
                                    const std::string& client_auth,
                                    bool expect_success) {
    // Use secure connection and chunk transfer coding.
    UrlRequest url_request(server_url + client_auth);
    if (!url_request.is_connected()) {
      return "";
    }

    url_request.PostRequest(key_msg_);
    std::string response;
    bool ok = url_request.GetResponse(&response);
    LOGD("response: %s\n", response.c_str());
    EXPECT_TRUE(ok);

    int status_code = url_request.GetStatusCode(response);
    if (expect_success) EXPECT_EQ(kHttpOk, status_code);

    if (status_code != kHttpOk) {
      return "";
    } else {
      std::string drm_msg;
      LicenseRequest lic_request;
      lic_request.GetDrmMessage(response, drm_msg);
      LOGV("drm msg: %u bytes\r\n%s", drm_msg.size(),
      HexEncode(reinterpret_cast<const uint8_t*>(drm_msg.data()),
                drm_msg.size()).c_str());
      return drm_msg;
    }
  }

  void VerifyNewKeyResponse(const std::string& server_url,
                            const std::string& client_auth){
    std::string resp = GetKeyRequestResponse(server_url,
                                             client_auth);
    CdmKeySetId key_set_id;
    EXPECT_EQ(wvcdm::KEY_ADDED, cdm_engine_.AddKey(session_id_, resp, &key_set_id));
  }

  void VerifyRenewalKeyResponse(const std::string& server_url,
                                const std::string& client_auth) {
    std::string resp = GetKeyRequestResponse(server_url, client_auth);
    EXPECT_EQ(wvcdm::KEY_ADDED, cdm_engine_.RenewKey(session_id_, resp));
  }

  CdmEngine cdm_engine_;
  std::string key_msg_;
  std::string session_id_;
  std::string server_url_;
};

// Test that provisioning works, even if device is already provisioned.
TEST_F(WvCdmEngineTest, ProvisioningTest) {
  Provision();
}

TEST_F(WvCdmEngineTest, BaseIsoBmffMessageTest) {
  GenerateKeyRequest(g_key_id_pssh, kCencMimeType);
  GetKeyRequestResponse(g_license_server, g_client_auth);
}

// TODO(juce): Set up with correct test data.
TEST_F(WvCdmEngineTest, DISABLED_BaseWebmMessageTest) {
  GenerateKeyRequest(g_key_id_unwrapped, kWebmMimeType);
  GetKeyRequestResponse(g_license_server, g_client_auth);
}

TEST_F(WvCdmEngineTest, WrongMessageTest) {
  std::string wrong_message = a2bs_hex(g_wrong_key_id);
  GenerateKeyRequest(wrong_message, kCencMimeType);

  // We should receive a response with no license, i.e. the extracted license
  // response message should be empty.
  ASSERT_EQ("", FailToGetKeyRequestResponse(g_license_server, g_client_auth));
}

TEST_F(WvCdmEngineTest, NormalDecryptionIsoBmff) {
  GenerateKeyRequest(g_key_id_pssh, kCencMimeType);
  VerifyNewKeyResponse(g_license_server, g_client_auth);
}

// TODO(juce): Set up with correct test data.
TEST_F(WvCdmEngineTest, DISABLED_NormalDecryptionWebm) {
  GenerateKeyRequest(g_key_id_unwrapped, kWebmMimeType);
  VerifyNewKeyResponse(g_license_server, g_client_auth);
}

TEST_F(WvCdmEngineTest, LicenseRenewal) {
  GenerateKeyRequest(g_key_id_pssh, kCencMimeType);
  VerifyNewKeyResponse(g_license_server, g_client_auth);

  GenerateRenewalRequest();
  VerifyRenewalKeyResponse(server_url_.empty() ? g_license_server : server_url_,
                           g_client_auth);
}

}  // namespace wvcdm

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  wvcdm::InitLogging(argc, argv);

  wvcdm::ConfigTestEnv config(wvcdm::kContentProtectionServer);
  g_client_auth.assign(config.client_auth());
  g_key_system.assign(config.key_system());
  g_wrong_key_id.assign(config.wrong_key_id());

  // The following variables are configurable through command line options.
  g_license_server.assign(config.license_server());
  g_key_id_pssh.assign(config.key_id());
  std::string license_server(g_license_server);

  int show_usage = 0;
  static const struct option long_options[] = {
    { "keyid", required_argument, NULL, 'k' },
    { "server", required_argument, NULL, 's' },
    { NULL, 0, NULL, '\0' }
  };

  int option_index = 0;
  int opt = 0;
  while ((opt = getopt_long(argc, argv, "k:s:v", long_options, &option_index)) != -1) {
    switch (opt) {
      case 'k': {
        g_key_id_pssh.clear();
        g_key_id_pssh.assign(optarg);
        break;
      }
      case 's': {
        g_license_server.clear();
        g_license_server.assign(optarg);
        break;
      }
      case 'v': {
        // This option _may_ have already been consumed by wvcdm::InitLogging()
        // above, depending on the platform-specific logging implementation.
        // We only tell getopt about it so that it is not an error.  We ignore
        // the option here when seen.
        // TODO: Stop passing argv to InitLogging, and instead set the log
        // level here through the logging API.  We should keep all command-line
        // parsing at the application level, rather than split between various
        // apps and various platform-specific logging implementations.
        break;
      }
      case '?': {
        show_usage = 1;
        break;
      }
    }
  }

  if (show_usage) {
    std::cout << std::endl;
    std::cout << "usage: " << argv[0] << " [options]" << std::endl << std::endl;
    std::cout << "  enclose multiple arguments in '' when using adb shell" << std::endl;
    std::cout << "  e.g. adb shell '" << argv[0] << " --server=\"url\"'" << std::endl << std::endl;

    std::cout << std::setw(30) << std::left << "    --server=<server_url>";
    std::cout << "configure the license server url, please include http[s] in the url" << std::endl;
    std::cout << std::setw(30) << std::left << " ";
    std::cout << "default: " << license_server << std::endl;

    std::cout << std::setw(30) << std::left << "    --keyid=<key_id>";
    std::cout << "configure the key id or pssh, in hex format" << std::endl;
    std::cout << std::setw(30) << std::left << "      default keyid:";
    std::cout << g_key_id_pssh << std::endl;
    return 0;
  }

  std::cout << std::endl;
  std::cout << "Server: " << g_license_server << std::endl;
  std::cout << "KeyID: " << g_key_id_pssh << std::endl << std::endl;

  g_key_id_pssh = wvcdm::a2bs_hex(g_key_id_pssh);
  config.set_license_server(g_license_server);
  config.set_key_id(g_key_id_pssh);

  // Extract the key ID from the PSSH box.
  wvcdm::InitializationData extractor(wvcdm::CENC_INIT_DATA_FORMAT,
                                      g_key_id_pssh);
  g_key_id_unwrapped = extractor.data();

#if defined(CHROMIUM_BUILD)
  base::AtExitManager exit;
  base::MessageLoop ttr(base::MessageLoop::TYPE_IO);
#endif
  return RUN_ALL_TESTS();
}
