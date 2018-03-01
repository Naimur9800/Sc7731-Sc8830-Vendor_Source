// Copyright 2013 Google Inc. All Rights Reserved.

#include <errno.h>
#include <getopt.h>
#include <sstream>

#include "config_test_env.h"
#include "gtest/gtest.h"
#include "device_files.h"
#include "file_store.h"
#include "license_request.h"
#include "log.h"
#include "OEMCryptoCENC.h"
#include "oemcrypto_adapter.h"
#include "properties.h"
#include "string_conversions.h"
#include "url_request.h"
#include "test_printers.h"
#include "wv_cdm_constants.h"
#include "wv_cdm_event_listener.h"
#include "wv_content_decryption_module.h"

namespace {
const char kPathDelimiter = '/';

// HTTP response codes.
const int kHttpOk = 200;
const int kHttpBadRequest = 400;
const int kHttpInternalServerError = 500;

// Default license server, can be configured using --server command line option
// Default key id (pssh), can be configured using --keyid command line option
std::string g_client_auth;
wvcdm::ConfigTestEnv* g_config = NULL;
wvcdm::KeyId g_key_id;
wvcdm::CdmKeySystem g_key_system;
std::string g_license_server;
wvcdm::KeyId g_wrong_key_id;
wvcdm::LicenseServerId g_license_server_id =
    wvcdm::kContentProtectionServer;

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

SubSampleInfo clear_sub_sample = {
    true, 1, true, false, false,
    wvcdm::a2bs_hex("371EA35E1A985D75D198A7F41020DC23"),
    wvcdm::a2b_hex(
        "217ce9bde99bd91e9733a1a00b9b557ac3a433dc92633546156817fae26b6e1c"
        "942ac20a89ff79f4c2f25fba99d6a44618a8c0420b27d54e3da17b77c9d43cca"
        "595d259a1e4a8b6d7744cd98c5d3f921adc252eb7d8af6b916044b676a574747"
        "8df21fdc42f166880d97a2225cd5c9ea5e7b752f4cf81bbdbe98e542ee10e1c6"
        "ad868a6ac55c10d564fc23b8acff407daaf4ed2743520e02cda9680d9ea88e91"
        "029359c4cf5906b6ab5bf60fbb3f1a1c7c59acfc7e4fb4ad8e623c04d503a3dd"
        "4884604c8da8a53ce33db9ff8f1c5bb6bb97f37b39906bf41596555c1bcce9ed"
        "08a899cd760ff0899a1170c2f224b9c52997a0785b7fe170805fd3e8b1127659"),
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

SubSampleInfo clear_sub_sample_no_key = {
    false, 1, false, false, false,
    wvcdm::a2bs_hex("77777777777777777777777777777777"),
    wvcdm::a2b_hex(
        "217ce9bde99bd91e9733a1a00b9b557ac3a433dc92633546156817fae26b6e1c"
        "942ac20a89ff79f4c2f25fba99d6a44618a8c0420b27d54e3da17b77c9d43cca"
        "595d259a1e4a8b6d7744cd98c5d3f921adc252eb7d8af6b916044b676a574747"
        "8df21fdc42f166880d97a2225cd5c9ea5e7b752f4cf81bbdbe98e542ee10e1c6"
        "ad868a6ac55c10d564fc23b8acff407daaf4ed2743520e02cda9680d9ea88e91"
        "029359c4cf5906b6ab5bf60fbb3f1a1c7c59acfc7e4fb4ad8e623c04d503a3dd"
        "4884604c8da8a53ce33db9ff8f1c5bb6bb97f37b39906bf41596555c1bcce9ed"
        "08a899cd760ff0899a1170c2f224b9c52997a0785b7fe170805fd3e8b1127659"),
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

SubSampleInfo single_encrypted_sub_sample = {
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

SubSampleInfo switch_key_encrypted_sub_samples[2] = {
    // block 0, key SD, encrypted, 256b
    {true, 2, true, true, false,
     wvcdm::a2bs_hex("371EA35E1A985D75D198A7F41020DC23"),
     wvcdm::a2b_hex(
         "9efe1b7a3973324525e9b8c516855e554b52a73ce35cd181731b005a6525624f"
         "a03875c89aee02f1da7f556b7e7d9a3eba89fe3061194bc2d1446233ca022892"
         "ab95083f127d6ccb01b1368e6b6fa77e3570d84477a5517f2965ff72f8e0740c"
         "d8282c22e7724ce44d88526dcd2d601002b717d8ca3b97087d28f9e3810efb8e"
         "d4b08ee2da6bdb05a790b6363f8ee65cae8328e86848e4caf9be92db3e5492ad"
         "6363a26051c23cf23b9aee79a8002470c4a5834c6aae956b509a42f4110262e0"
         "565a043befd8ef3a335c9dfedca8d218f364215859d7daf7d040b1f0cb2eda87"
         "c1be18f323fb0235dd9a6e7b3b2fea1cb9c6e5bc2b349962f0b8f0b92e749db2"),
     wvcdm::a2b_hex(
         "38a715e73c9209544c47e5eb089146de8136df5c6ed01e3e8d9cea8ae18a81c9"
         "8c9c8ec67bf379dd80a21f57b0b00575827a240cd11332c5212defe9f1ef8b8e"
         "2399271767bfe81e5a11abf7bca1307578217c4d5f8b942ab04351b4725d6e24"
         "cd171fa3083570f7d7ae2b297224f701fd04d699c12c53e9ce9d3dab64ee6332"
         "5fba183b7a1f3f20acaeabc0c446c9ca0df39fafb1e2891c72500741ad5b7941"
         "4651729e30e9ddbb22f47a5026e09c795ff15a858123a7979e7be716cb8cd075"
         "e8bfb91bc0cc83f7cacd5c4772f7479a1193d9307bc5f837185faed5499e66a7"
         "e27db50b5d018d022279032016862883befd113b6c784889be8f9e6eb0f335f7"),
     wvcdm::a2b_hex("fd38d7f754a97128b78440433e1ef4a8"), 0, 3},
    // block 1, key HD, encrypted, 256b
    {true, 2, true, true, false,
     wvcdm::a2bs_hex("6D665122C01767FC087F340E6C335EA4"),
     wvcdm::a2b_hex(
         "d9392411d15f47de0d7dd854eae5eb5ffbd2d3f86c530d2ef619fc81725df866"
         "2e6267041b947863e5779812da3220824a3d154e7b094c1af70238c65877454e"
         "3c3bdcce836962ba29b69442d5e5b4a4ff32a4b026521f3fa4d442a400081cdd"
         "ba6ed313c43cc34443c4dc2b9cdcc9dbd478bf6afc4d515c07b42d8b151c15cc"
         "165270f6083ecd5c75313c496143068f45966bb53e35906c7568424e93e35989"
         "7da702fb89eb7c970af838d56a64a7a68f7cffe529807765d62540bb06bbc633"
         "6eeec62d20f5b639731e57a0851e23e146cb9502dbde93dc4aca20e471a3fa0b"
         "df01a74ecb48d5f57ac2be98fb21d19de7587d8d1e6e1788726e1544d05137f6"),
     wvcdm::a2b_hex(
         "c48a94d07c34c4315e01010dbcc63a038d50a023b1ff2a07deae6e498cb03f84"
         "57911d8c9d72fa5184c738d81a49999504b7cd4532b465436b7044606a6d40a2"
         "74a653c4b93ebaf8db585d180211a02e5501a8027f2235fe56682390325c88ee"
         "2ada85483eddb955c56f79634a2ceeb36d04b5d6faf7611817577d9b0fda088e"
         "921fbdd7fa594ee4f557f7393f51f3049cd36973f645badf7cc4672ef8d973da"
         "7dae8e59f32bf950c6569845a5261b5ed9cc500706eccf8d41f015b32026e16e"
         "ab274465d880ff99a5eaea603eea66c7b0e6679bfd87145de0ec1a73ebfff092"
         "866346a1d66db2923bca30664f417a6b66c07e91fb491be7872ebe5c9c2d03c2"),
     wvcdm::a2b_hex("f56ab022666de858920e532f19bb32f6"), 0, 3}};

SubSampleInfo partial_encrypted_sub_samples[3] = {
    // block 1, key SD, encrypted, 1-125b, offset 0
    {true, 3, true, true, false,
     wvcdm::a2bs_hex("371EA35E1A985D75D198A7F41020DC23"),
     wvcdm::a2b_hex(
         "53cc758763904ea5870458e6b23d36db1e6d7f7aaa2f3eeebb5393a7264991e7"
         "ce4f57b198326e1a208a821799b2a29c90567ab57321b06e51fc20dc9bc5fc55"
         "10720a8bb1f5e002c3e50ff70d2d806a9432cad237050d09581f5b0d59b00090"
         "b3ad69b4087f5a155b17e13c44d33fa007475d207fc4ac2ef3b571ecb9"),
     wvcdm::a2b_hex(
         "52e65334501acadf78e2b26460def3ac973771ed7c64001a2e82917342a7eab3"
         "047f5e85449692fae8f677be425a47bdea850df5a3ffff17043afb1f2b437ab2"
         "b1d5e0784c4ed8f97fc24b8f565e85ed63fb7d1365980d9aea7b8b58f488f83c"
         "1ce80b6096c60f3b113c988ff185b26e798da8fc6f327e4ff00e4b3fbf"),
     wvcdm::a2b_hex("6ba18dd40f49da7f64c368e4db43fc88"), 0, 1},
    // block 2, key SD, encrypted, 126-187b, offset 5
    {true, 3, true, true, false,
     wvcdm::a2bs_hex("371EA35E1A985D75D198A7F41020DC23"),
     wvcdm::a2b_hex(
         "f3c852"
         "ce00dc4806f0c6856ae1732e20308096478e1d822d75c2bb768119565d3bd6e6"
         "901e36164f4802355ee758fc46ef6cf5f852dd5256c7b1e5f96d29"),
     wvcdm::a2b_hex(
         "b1ed0a"
         "a054bce40ccb0ebc70b181d1a12055f46ac55e29c7c2473a29d2a366d240ec48"
         "7cede274f012813a877f99159e7062b6a37cfc9327a7bc2195814e"),
     wvcdm::a2b_hex("6ba18dd40f49da7f64c368e4db43fc8f"), 13, 0},
    // block 3, key SD, encrypted, 188-256b, offset 5
    {true, 3, true, true, false,
     wvcdm::a2bs_hex("371EA35E1A985D75D198A7F41020DC23"),
     wvcdm::a2b_hex(
         "3b20525d5e"
         "78b8e5aa344d5c4e425e67ddf889ea7c4bb1d49af67eba67718b765e0a940402"
         "8d306f4ce693ad6dc0a931d507fa14fff4d293d4170280b3e0fca2d628f722e8"),
     wvcdm::a2b_hex(
         "653b818d1d"
         "4ab9a9128361d8ca6a9d2766df5c096ee29f4f5204febdf217a94a5b560cd692"
         "cc36d3e071df789fdeac2fb7ec6dcd7af94bb1f85c22025b25e702e38212b927"),
     wvcdm::a2b_hex("6ba18dd40f49da7f64c368e4db43fc93"), 11, 2}};

SubSampleInfo single_encrypted_sub_sample_short_expiry = {
    // key 1, encrypted, 256b
    true, 1, true, true, false,
    wvcdm::a2bs_hex("9714593E1EEE57859D34ECFA821702BB"),
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
        "5a36c0b633b58faf22156d78fdfb608e54a8095788b2b0463ef78d030b4abf82"
        "eff34b8d9b7b6352e7d72de991b599662aa475da355033620152e2356ebfadee"
        "06172be9e1058fa177e223b9fdd191380cff53c3ea810c6fd852a1df4967b799"
        "415179a2276ec388ef763bab89605b9c6952c28dc8d6bf86b03fabbb46b392a3"
        "1dad15be602eeeeabb45070b3e25d6bb0217073b1fc44c9fe848594121fd6a91"
        "304d605e21f69615e1b57db18312b6b948725724b74e91d8aea7371e99532469"
        "1b358bdee873f1936b63efe83d190a53c2d21754d302d63ff285174023473755"
        "58b938c2e3ca4c2ce48942da97f9e45797f2c074ac6004734e93784a48af6160"),
    wvcdm::a2b_hex("4cca615fc013102892f91efee936639b"), 0, 3};

SubSampleInfo usage_info_sub_samples_icp[] = {
    {true, 1, true, true, false,
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
     wvcdm::a2b_hex("7362b5140c4ce0cd5f863858668d3f1a"), 0, 3},
    {true, 1, true, true, false,
     wvcdm::a2bs_hex("04B2B456405CAD7170BE848CA75AA2DF"),
     wvcdm::a2b_hex(
         "ccb68f97b03e7af1a9e208d91655ba645cc5a5d454f3cb7c3d621e98a7592d90"
         "4ff7023555f0e99bcf3918948f4fca7a430faf17d7d67268d81d8096d7c48809"
         "c14220e634680fbe0c760571dd86a1905835035f4a238c2d7f17bd1363b113c1"
         "91782aebb77a1064142a68b59ecdcc6990ed4244082464d91dbfe09e08744b2f"
         "d1e850a008acbbe129fbd050c8dc1b28cb8cc2c1e2d920ea458f74809297b513"
         "85307b481cbb81d6759385ee782d6c0e101c20ca1937cfd0d6e024da1a0f718a"
         "fb7c4ff3df1ca87e67602d28168233cc2448d44b79f405d4c6e67eb88d705050"
         "2a806cb986423e3b0e7a97738e1d1d143b4f5f926a4e2f37c7fbe65f56d5b690"),
     wvcdm::a2b_hex(
         "fa35aa1f5e5d7b958880d5eed9cc1bb81d36ebd04c0250a8c752ea5f413bbdcf"
         "3785790c8dba7a0b21c71346bb7f946a9b71c0d2fe87d2e2fab14e35ee8400e7"
         "097a7d2d9a25b468e848e8dee2388f890967516c7dab96db4713c7855f717aed"
         "2ae9c2895baaa636e4a610ab26b35d771d62397ba40d78694dab70dcbdfa91c3"
         "6af79ad6b6ebb479b4a5fbc242a8574ebe6717f0813fbd6f726ce2af4d522e66"
         "b36c940fce519c913db56a6372c3636b10c0149b4cd97e74c576765b533abdc2"
         "729f1470dd7f9a60d3572dcc9839582a4606ee17eaced39797daef8f885d3f8f"
         "e14877ae530451c4242bbc3934f85a5bb71b363351894f881896471cfeaf68b2"),
     wvcdm::a2b_hex("4a59e3e5f3e4f7e2f494ad09c12a9e4c"), 0, 3},
    {true, 1, true, true, false,
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
     wvcdm::a2b_hex("b358ab21ac90455bbf60490daad457e3"), 0, 3},
    {true, 1, true, true, false,
     wvcdm::a2bs_hex("5292104C011418973C31235953FC8205"),
     wvcdm::a2b_hex(
         "d433adfd3d892d5c3e7d54ab0218ee0712400a920d4b71d553912451169f9b79"
         "3f103260cf04c34f6a5944bb96da79946a62bdbcd804ca28b17656338edfa700"
         "5c090f2750663a026fd15a0b0e448adbbfd53f613ea3993d9fd504421b575f28"
         "12020bb8cca0ce333eabee0403df9f410c6473d7673d6991caab6ea2ece8f743"
         "5a3ca049fa00c96c9b7c47e3073d25d08f23b47ffc509c48a81a2f98c9ec8a1d"
         "e41764c14a5010df8b4692e8612a45bf0645601d4910119e6268ca4f6d8016a8"
         "3d933d53f44243674b522bae43043c068c8cae43f0ac224198de71315b3a6f82"
         "c1b523bbdcdb3e9f162c308684dd17e364b448ed0e90b0e496b8cf633a982708"),
     wvcdm::a2b_hex(
         "5efb5e5b913785e9935e67e763b8ff29a6687ac6c18d5a7e16951beb704f9c95"
         "f081ca28f54c3e237fb5a7b0444e9a3e17da91e5cf2c0a8f009a873fb079c339"
         "81b0ebc565b2c56d983ee33686fa5057c9891e246b67bb6950400acb06d5ae50"
         "0e61a7e9289ea67ec2e88e8d0cc3c494fd996e93270e9b264a21818987e969c5"
         "1e2955c5a53202e5aec1e2c906e1c006325112eb5c33ee37d0c07ea97d80c17f"
         "d56e0efcf40c8c98981a86c18a159f05d851891236c124641d4584c49ccd7478"
         "4f328a9cacae0f945238d98741b2969fe258903e85f963daba7168f05c18b09f"
         "660dae18de41b1c49769cd38e24b135c37a65b69533f5c7d085898faedfbed5d"),
     wvcdm::a2b_hex("cef7e8aaa6ec1154cb68a988f7c9e803"), 0, 3},
    {true, 1, true, true, false,
     wvcdm::a2bs_hex("D7C01C2F868AE314BCB893E4E9C6AC75"),
     wvcdm::a2b_hex(
         "fa5d28677721de488ffc209321529728c77bc338accd45ccc98ab2063fc8c373"
         "48c7698534175d72bf185690d19474d08c4fd4ed4eb46d858633f05337d70e92"
         "03f7ee6bec0f7003bdf6fa665ba172855a51a82da406348ba651a2f62888c30a"
         "7b4e1355bb94a9ff5c458f397c9a09e5d7785b286ef83142ddad324cc74e1929"
         "60ad1c34c425cdefbedcb62ca9b21ac4f3df7f5922e263cb7798de54b622ab3f"
         "64a0dd6ee1e40be6ecc857e657994ecac02ccfafc9036f382d7dbdf35c903356"
         "40b7c9db088143060b24f24b21c4a7c2faeb3d308e57c5a75955fd704cfe4dee"
         "71a4a7d823102b90eddded795ca6eb36282d777db8cfd783e50e5c2a816ee9ed"),
     wvcdm::a2b_hex(
         "d5db2f50c0f5a39414ddfa5129c2c641836a8c6312b26a210c996988e0c768d5"
         "9a3adff117293b52b0653c0d6e22589edda804fb8caa7442362fe4caf9053b6a"
         "2a34896399259a188f0c805de54b091a7eabff098b28d54584c01dd83301e4ca"
         "a01b226c4541af1592d4440e103eb55bbd08c471efb0856ec9ced43211fc3325"
         "3d402dff0d15f40833dd71259a8d40d527659ef3e5f9fd0826c9471dddb17e1e"
         "fab916abc957fb07d7eac4a368ac92a8fb16d995613af47303034ee57b59b1d7"
         "101aa031f5586b2f6b4c74372c4d7306db02509b5924d52c46a270f427743a85"
         "614f080d83f3b15cbc6600ddda43adff5d2941da13ebe49d80fd0cea5025412b"),
     wvcdm::a2b_hex("964c2dfda920357c668308d52d33c652"), 0, 3}};

// License duration + fudge factor
const uint32_t kSingleEncryptedSubSampleIcpLicenseDurationExpiration = 5 + 2;

struct SessionSharingSubSampleInfo {
  SubSampleInfo* sub_sample;
  bool session_sharing_enabled;
};

SessionSharingSubSampleInfo session_sharing_sub_samples[] = {
    {&clear_sub_sample, false},
    {&clear_sub_sample, true},
    {&clear_sub_sample_no_key, false},
    {&clear_sub_sample_no_key, true},
    {&single_encrypted_sub_sample, false},
    {&single_encrypted_sub_sample, true}};

struct UsageInfoSubSampleInfo {
  SubSampleInfo* sub_sample;
  uint32_t usage_info;
  wvcdm::SecurityLevel security_level;
  std::string app_id;
};

UsageInfoSubSampleInfo usage_info_sub_sample_info[] = {
    {&usage_info_sub_samples_icp[0], 1, wvcdm::kLevelDefault, ""},
    {&usage_info_sub_samples_icp[0], 3, wvcdm::kLevelDefault, ""},
    {&usage_info_sub_samples_icp[0], 5, wvcdm::kLevelDefault, ""},
    {&usage_info_sub_samples_icp[0], 3, wvcdm::kLevelDefault, "other app id"},
    {&usage_info_sub_samples_icp[0], 1, wvcdm::kLevel3, ""},
    {&usage_info_sub_samples_icp[0], 3, wvcdm::kLevel3, ""},
    {&usage_info_sub_samples_icp[0], 5, wvcdm::kLevel3, ""},
    {&usage_info_sub_samples_icp[0], 3, wvcdm::kLevel3, "other app id"}};

}  // namespace

namespace wvcdm {
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

class WvCdmRequestLicenseTest : public testing::Test {
 public:
  WvCdmRequestLicenseTest() {}
  ~WvCdmRequestLicenseTest() {}

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
    GenerateKeyRequest(init_data, license_type, NULL);
  }

  void GenerateKeyRequest(const std::string& init_data,
                          CdmLicenseType license_type,
                          CdmClientPropertySet* property_set) {
    wvcdm::CdmAppParameterMap app_parameters;
    std::string server_url;
    std::string key_set_id;
    EXPECT_EQ(wvcdm::KEY_MESSAGE,
              decryptor_.GenerateKeyRequest(
                  session_id_, key_set_id, "video/mp4", init_data, license_type,
                  app_parameters, property_set, &key_msg_, &server_url));
    EXPECT_EQ(0u, server_url.size());
  }

  void GenerateRenewalRequest(CdmLicenseType license_type,
                              std::string* server_url) {
      GenerateRenewalRequest(license_type, server_url, NULL);
  }

  void GenerateRenewalRequest(CdmLicenseType license_type,
                              std::string* server_url,
                              CdmClientPropertySet* property_set) {
    // TODO application makes a license request, CDM will renew the license
    // when appropriate.
    std::string init_data;
    wvcdm::CdmAppParameterMap app_parameters;
    EXPECT_EQ(wvcdm::KEY_MESSAGE,
              decryptor_.GenerateKeyRequest(
                  session_id_, key_set_id_, "video/mp4", init_data,
                  license_type, app_parameters, property_set, &key_msg_,
                  server_url));
    // TODO(edwinwong, rfrias): Add tests cases for when license server url
    // is empty on renewal. Need appropriate key id at the server.
    EXPECT_NE(0u, server_url->size());
  }

  void GenerateKeyRelease(CdmKeySetId key_set_id) {
      GenerateKeyRelease(key_set_id, NULL);
  }

  void GenerateKeyRelease(CdmKeySetId key_set_id,
                          CdmClientPropertySet* property_set) {
    CdmSessionId session_id;
    CdmInitData init_data;
    wvcdm::CdmAppParameterMap app_parameters;
    std::string server_url;
    EXPECT_EQ(wvcdm::KEY_MESSAGE,
              decryptor_.GenerateKeyRequest(
                  session_id, key_set_id, "video/mp4", init_data,
                  kLicenseTypeRelease, app_parameters, property_set, &key_msg_,
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

  void Provision(SecurityLevel level) {
    TestWvCdmClientPropertySet property_set_L3;
    TestWvCdmClientPropertySet* property_set = NULL;

    if (kLevel3 == level) {
      property_set_L3.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L3);
      property_set = &property_set_L3;
    }

    CdmResponseType status =
        decryptor_.OpenSession(g_key_system, property_set, &session_id_);
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

  std::string GetSecurityLevel(TestWvCdmClientPropertySet* property_set) {
    EXPECT_EQ(NO_ERROR,
        decryptor_.OpenSession(g_key_system, property_set, &session_id_));
    CdmQueryMap query_info;
    EXPECT_EQ(wvcdm::NO_ERROR,
              decryptor_.QuerySessionStatus(session_id_, &query_info));
    CdmQueryMap::iterator itr =
        query_info.find(wvcdm::QUERY_KEY_SECURITY_LEVEL);
    EXPECT_TRUE(itr != query_info.end());
    decryptor_.CloseSession(session_id_);
    if (itr != query_info.end()) {
      return itr->second;
    } else {
      return "ERROR";
    }
  }


  CdmSecurityLevel GetDefaultSecurityLevel() {
    std::string level = GetSecurityLevel(NULL).c_str();
    CdmSecurityLevel security_level = kSecurityLevelUninitialized;
    if (level.compare(wvcdm::QUERY_VALUE_SECURITY_LEVEL_L1) == 0) {
      security_level = kSecurityLevelL1;
    } else if (level.compare(wvcdm::QUERY_VALUE_SECURITY_LEVEL_L3) == 0) {
      security_level = kSecurityLevelL3;
    } else {
      EXPECT_TRUE(false) << "Default Security level is undefined: " << level;
    }
    return security_level;
  }

  wvcdm::WvContentDecryptionModule decryptor_;
  CdmKeyMessage key_msg_;
  CdmSessionId session_id_;
  CdmKeySetId key_set_id_;
};

TEST_F(WvCdmRequestLicenseTest, ProvisioningTest) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  std::string provisioning_server_url;
  CdmCertificateType cert_type = kCertificateWidevine;
  std::string cert_authority, cert, wrapped_key;

  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.GetProvisioningRequest(
                                 cert_type, cert_authority, &key_msg_,
                                 &provisioning_server_url));
  EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());

  std::string response =
      GetCertRequestResponse(g_config->provisioning_server_url());
  EXPECT_NE(0, static_cast<int>(response.size()));
  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.HandleProvisioningResponse(
                                 response, &cert, &wrapped_key));
  EXPECT_EQ(0, static_cast<int>(cert.size()));
  EXPECT_EQ(0, static_cast<int>(wrapped_key.size()));
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, UnprovisionTest) {
  CdmSecurityLevel security_level = GetDefaultSecurityLevel();
  DeviceFiles handle;
  EXPECT_TRUE(handle.Init(security_level));
  File file;
  handle.SetTestFile(&file);
  std::string certificate;
  std::string wrapped_private_key;
  EXPECT_TRUE(handle.RetrieveCertificate(&certificate, &wrapped_private_key));

  EXPECT_EQ(NO_ERROR, decryptor_.Unprovision(security_level));
  EXPECT_FALSE(handle.RetrieveCertificate(&certificate, &wrapped_private_key));
}

TEST_F(WvCdmRequestLicenseTest, ProvisioningRetryTest) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  std::string provisioning_server_url;
  CdmCertificateType cert_type = kCertificateWidevine;
  std::string cert_authority, cert, wrapped_key;

  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.GetProvisioningRequest(
                                 cert_type, cert_authority, &key_msg_,
                                 &provisioning_server_url));
  EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());

  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.GetProvisioningRequest(
                                 cert_type, cert_authority, &key_msg_,
                                 &provisioning_server_url));
  EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());

  std::string response =
      GetCertRequestResponse(g_config->provisioning_server_url());
  EXPECT_NE(0, static_cast<int>(response.size()));
  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.HandleProvisioningResponse(
                                 response, &cert, &wrapped_key));
  EXPECT_EQ(0, static_cast<int>(cert.size()));
  EXPECT_EQ(0, static_cast<int>(wrapped_key.size()));

  response =
      GetCertRequestResponse(g_config->provisioning_server_url());
  EXPECT_NE(0, static_cast<int>(response.size()));
  EXPECT_EQ(wvcdm::UNKNOWN_ERROR, decryptor_.HandleProvisioningResponse(
                                      response, &cert, &wrapped_key));
  EXPECT_EQ(0, static_cast<int>(cert.size()));
  EXPECT_EQ(0, static_cast<int>(wrapped_key.size()));
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, DISABLED_X509ProvisioningTest) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  std::string provisioning_server_url;
  CdmCertificateType cert_type = kCertificateX509;
  // TODO(rfrias): insert appropriate CA here
  std::string cert_authority = "cast.google.com";
  std::string cert, wrapped_key;

  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.GetProvisioningRequest(
                                 cert_type, cert_authority, &key_msg_,
                                 &provisioning_server_url));
  EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());

  std::string response =
      GetCertRequestResponse(g_config->provisioning_server_url());
  EXPECT_NE(0, static_cast<int>(response.size()));
  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.HandleProvisioningResponse(
                                 response, &cert, &wrapped_key));
  EXPECT_NE(0, static_cast<int>(cert.size()));
  EXPECT_NE(0, static_cast<int>(wrapped_key.size()));
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, PropertySetTest) {
  TestWvCdmClientPropertySet property_set_L1;
  TestWvCdmClientPropertySet property_set_L3;
  TestWvCdmClientPropertySet property_set_Ln;
  CdmSessionId session_id_L1;
  CdmSessionId session_id_L3;
  CdmSessionId session_id_Ln;

  property_set_L1.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L1);
  property_set_L1.set_use_privacy_mode(true);
  decryptor_.OpenSession(g_key_system, &property_set_L1, &session_id_L1);
  property_set_L3.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L3);
  property_set_L3.set_use_privacy_mode(false);

  CdmResponseType sts =
      decryptor_.OpenSession(g_key_system, &property_set_L3, &session_id_L3);

  if (NEED_PROVISIONING == sts) {
    std::string provisioning_server_url;
    CdmCertificateType cert_type = kCertificateWidevine;
    std::string cert_authority, cert, wrapped_key;
    EXPECT_EQ(NO_ERROR, decryptor_.GetProvisioningRequest(
                            cert_type, cert_authority, &key_msg_,
                            &provisioning_server_url));
    EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());
    std::string response =
        GetCertRequestResponse(g_config->provisioning_server_url());
    EXPECT_NE(0, static_cast<int>(response.size()));
    EXPECT_EQ(NO_ERROR, decryptor_.HandleProvisioningResponse(response, &cert,
                                                              &wrapped_key));
    EXPECT_EQ(NO_ERROR, decryptor_.OpenSession(g_key_system, &property_set_L3,
                                               &session_id_L3));
  } else {
    EXPECT_EQ(NO_ERROR, sts);
  }

  property_set_Ln.set_security_level("");
  decryptor_.OpenSession(g_key_system, &property_set_Ln, &session_id_Ln);

  std::string security_level;
  EXPECT_TRUE(Properties::GetSecurityLevel(session_id_L1, &security_level));
  EXPECT_TRUE(!security_level.compare(QUERY_VALUE_SECURITY_LEVEL_L1) ||
              !security_level.compare(QUERY_VALUE_SECURITY_LEVEL_L3));
  EXPECT_TRUE(Properties::UsePrivacyMode(session_id_L1));

  EXPECT_TRUE(Properties::GetSecurityLevel(session_id_L3, &security_level));
  EXPECT_EQ(security_level, QUERY_VALUE_SECURITY_LEVEL_L3);
  EXPECT_FALSE(Properties::UsePrivacyMode(session_id_L3));

  EXPECT_TRUE(Properties::GetSecurityLevel(session_id_Ln, &security_level));
  EXPECT_TRUE(security_level.empty() ||
              !security_level.compare(QUERY_VALUE_SECURITY_LEVEL_L3));

  std::string app_id = "not empty";
  EXPECT_TRUE(Properties::GetApplicationId(session_id_Ln, &app_id));
  EXPECT_STREQ("", app_id.c_str());

  property_set_Ln.set_app_id("com.unittest.mock.app.id");
  EXPECT_TRUE(Properties::GetApplicationId(session_id_Ln, &app_id));
  EXPECT_STREQ("com.unittest.mock.app.id", app_id.c_str());

  decryptor_.CloseSession(session_id_L1);
  decryptor_.CloseSession(session_id_L3);
  decryptor_.CloseSession(session_id_Ln);
}

TEST_F(WvCdmRequestLicenseTest, ForceL3Test) {
  TestWvCdmClientPropertySet property_set;
  property_set.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L3);

  File file;
  DeviceFiles handle;
  EXPECT_TRUE(handle.Init(kSecurityLevelL3));
  handle.SetTestFile(&file);
  EXPECT_TRUE(handle.DeleteAllFiles());

  EXPECT_EQ(NEED_PROVISIONING,
            decryptor_.OpenSession(g_key_system, &property_set, &session_id_));
  std::string provisioning_server_url;
  CdmCertificateType cert_type = kCertificateWidevine;
  std::string cert_authority, cert, wrapped_key;
  EXPECT_EQ(NO_ERROR, decryptor_.GetProvisioningRequest(
                          cert_type, cert_authority, &key_msg_,
                          &provisioning_server_url));
  EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());
  std::string response =
      GetCertRequestResponse(g_config->provisioning_server_url());
  EXPECT_NE(0, static_cast<int>(response.size()));
  EXPECT_EQ(NO_ERROR, decryptor_.HandleProvisioningResponse(response, &cert,
                                                            &wrapped_key));

  EXPECT_EQ(NO_ERROR,
            decryptor_.OpenSession(g_key_system, &property_set, &session_id_));
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, PrivacyModeTest) {
  TestWvCdmClientPropertySet property_set;

  property_set.set_use_privacy_mode(true);
  decryptor_.OpenSession(g_key_system, &property_set, &session_id_);

  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  std::string resp = GetKeyRequestResponse(g_license_server, g_client_auth);
  EXPECT_EQ(decryptor_.AddKey(session_id_, resp, &key_set_id_),
            wvcdm::NEED_KEY);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, PrivacyModeWithServiceCertificateTest) {
  TestWvCdmClientPropertySet property_set;

  property_set.set_use_privacy_mode(true);
  property_set.set_service_certificate(a2bs_hex(kServiceCertificate));
  decryptor_.OpenSession(g_key_system, &property_set, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, BaseMessageTest) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  GetKeyRequestResponse(g_license_server, g_client_auth);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, WrongMessageTest) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);

  std::string wrong_message = wvcdm::a2bs_hex(g_wrong_key_id);
  GenerateKeyRequest(wrong_message, kLicenseTypeStreaming);
  // We should receive a response with no license, i.e. the extracted license
  // response message should be empty or an HTTP error
  UrlRequest url_request(g_license_server + g_client_auth);
  if (!url_request.is_connected()) {
    return;
  }
  url_request.PostRequest(key_msg_);
  std::string message;
  int resp_bytes = url_request.GetResponse(&message);

  int status_code = url_request.GetStatusCode(message);
  std::string drm_msg;
  if (kHttpOk == status_code) {
    LicenseRequest lic_request;
    lic_request.GetDrmMessage(message, drm_msg);
    LOGV("HTTP response body: (%u bytes)", drm_msg.size());
  }
  EXPECT_TRUE(drm_msg.empty());
  EXPECT_TRUE(kHttpOk == status_code || kHttpBadRequest == status_code ||
              kHttpInternalServerError == status_code);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, AddStreamingKeyTest) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, AddKeyOfflineTest) {
  Unprovision();
  Provision(kLevelDefault);

  // override default settings unless configured through the command line
  std::string key_id;
  std::string client_auth;
  GetOfflineConfiguration(&key_id, &client_auth);

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, RestoreOfflineKeyTest) {
  Unprovision();
  Provision(kLevelDefault);

  // override default settings unless configured through the command line
  std::string key_id;
  std::string client_auth;
  GetOfflineConfiguration(&key_id, &client_auth);

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);

  CdmKeySetId key_set_id = key_set_id_;
  EXPECT_FALSE(key_set_id_.empty());
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  EXPECT_EQ(wvcdm::KEY_ADDED, decryptor_.RestoreKey(session_id_, key_set_id));
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, ReleaseOfflineKeyTest) {
  Unprovision();
  Provision(kLevelDefault);

  // override default settings unless configured through the command line
  std::string key_id;
  std::string client_auth;
  GetOfflineConfiguration(&key_id, &client_auth);

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);

  CdmKeySetId key_set_id = key_set_id_;
  EXPECT_FALSE(key_set_id_.empty());
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  EXPECT_EQ(wvcdm::KEY_ADDED, decryptor_.RestoreKey(session_id_, key_set_id));
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  GenerateKeyRelease(key_set_id);
  key_set_id_ = key_set_id;
  VerifyKeyRequestResponse(g_license_server, client_auth, false);
}

TEST_F(WvCdmRequestLicenseTest, ReleaseRetryOfflineKeyTest) {
  Unprovision();
  Provision(kLevelDefault);

  // override default settings unless configured through the command line
  std::string key_id;
  std::string client_auth;
  GetOfflineConfiguration(&key_id, &client_auth);

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);

  CdmKeySetId key_set_id = key_set_id_;
  EXPECT_FALSE(key_set_id_.empty());
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  EXPECT_EQ(wvcdm::KEY_ADDED, decryptor_.RestoreKey(session_id_, key_set_id));
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  GenerateKeyRelease(key_set_id);

  session_id_.clear();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  EXPECT_EQ(wvcdm::UNKNOWN_ERROR, decryptor_.RestoreKey(session_id_, key_set_id));
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  GenerateKeyRelease(key_set_id);
  key_set_id_ = key_set_id;
  VerifyKeyRequestResponse(g_license_server, client_auth, false);
}

TEST_F(WvCdmRequestLicenseTest, ReleaseRetryL3OfflineKeyTest) {
  Unprovision();

  TestWvCdmClientPropertySet property_set;
  property_set.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L3);

  // override default settings unless configured through the command line
  std::string key_id;
  std::string client_auth;
  GetOfflineConfiguration(&key_id, &client_auth);

  CdmResponseType sts =
      decryptor_.OpenSession(g_key_system, &property_set, &session_id_);

  if (NEED_PROVISIONING == sts) {
    std::string provisioning_server_url;
    CdmCertificateType cert_type = kCertificateWidevine;
    std::string cert_authority, cert, wrapped_key;
    EXPECT_EQ(NO_ERROR, decryptor_.GetProvisioningRequest(
                            cert_type, cert_authority, &key_msg_,
                            &provisioning_server_url));
    EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());
    std::string response =
        GetCertRequestResponse(g_config->provisioning_server_url());
    EXPECT_NE(0, static_cast<int>(response.size()));
    EXPECT_EQ(NO_ERROR, decryptor_.HandleProvisioningResponse(response, &cert,
                                                              &wrapped_key));
    EXPECT_EQ(NO_ERROR, decryptor_.OpenSession(g_key_system, &property_set,
                                               &session_id_));
  } else {
    EXPECT_EQ(NO_ERROR, sts);
  }

  decryptor_.OpenSession(g_key_system, &property_set, &session_id_);
  GenerateKeyRequest(key_id, kLicenseTypeOffline, &property_set);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);

  CdmKeySetId key_set_id = key_set_id_;
  EXPECT_FALSE(key_set_id_.empty());
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  decryptor_.OpenSession(g_key_system, &property_set, &session_id_);
  EXPECT_EQ(wvcdm::KEY_ADDED, decryptor_.RestoreKey(session_id_, key_set_id));
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  GenerateKeyRelease(key_set_id, &property_set);

  session_id_.clear();
  decryptor_.OpenSession(g_key_system, &property_set, &session_id_);
  EXPECT_EQ(
      wvcdm::UNKNOWN_ERROR, decryptor_.RestoreKey(session_id_, key_set_id));
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  GenerateKeyRelease(key_set_id, &property_set);
  key_set_id_ = key_set_id;
  VerifyKeyRequestResponse(g_license_server, client_auth, false);
}

TEST_F(WvCdmRequestLicenseTest, ExpiryOnReleaseOfflineKeyTest) {
  Unprovision();
  Provision(kLevelDefault);

  // override default settings unless configured through the command line
  std::string key_id;
  std::string client_auth;
  GetOfflineConfiguration(&key_id, &client_auth);

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);

  CdmKeySetId key_set_id = key_set_id_;
  EXPECT_FALSE(key_set_id_.empty());
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  key_set_id_.clear();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  CdmSessionId restore_session_id = session_id_;
  TestWvCdmEventListener listener;
  EXPECT_TRUE(decryptor_.AttachEventListener(restore_session_id, &listener));
  EXPECT_EQ(wvcdm::KEY_ADDED,
            decryptor_.RestoreKey(restore_session_id, key_set_id));

  session_id_.clear();
  key_set_id_.clear();
  EXPECT_TRUE(listener.session_id().size() == 0);
  GenerateKeyRelease(key_set_id);
  key_set_id_ = key_set_id;
  EXPECT_TRUE(listener.session_id().size() != 0);
  EXPECT_TRUE(listener.session_id().compare(restore_session_id) == 0);
  EXPECT_TRUE(listener.event_type() == LICENSE_EXPIRED_EVENT);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);
  decryptor_.CloseSession(restore_session_id);
}

TEST_F(WvCdmRequestLicenseTest, StreamingLicenseRenewal) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  std::string license_server;
  GenerateRenewalRequest(kLicenseTypeStreaming, &license_server);
  if (license_server.empty()) license_server = g_license_server;
  VerifyKeyRequestResponse(license_server, g_client_auth, true);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, OfflineLicenseRenewal) {
  // override default settings unless configured through the command line
  std::string key_id;
  std::string client_auth;
  GetOfflineConfiguration(&key_id, &client_auth);

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);

  std::string license_server;
  GenerateRenewalRequest(kLicenseTypeOffline, &license_server);
  if (license_server.empty()) license_server = g_license_server;
  VerifyKeyRequestResponse(license_server, client_auth, true);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, RemoveKeys) {
  ASSERT_EQ(NO_ERROR, decryptor_.OpenSession(g_key_system, NULL, &session_id_));
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
  ASSERT_EQ(NO_ERROR, decryptor_.CancelKeyRequest(session_id_));
  ASSERT_EQ(NO_ERROR, decryptor_.CloseSession(session_id_));
}

TEST_F(WvCdmRequestLicenseTest, UsageInfoRetryTest) {
  Unprovision();
  Provision(kLevelDefault);

  CdmSecurityLevel security_level = GetDefaultSecurityLevel();
  std::string app_id = "";
  DeviceFiles handle;
  EXPECT_TRUE(handle.Init(security_level));
  File file;
  handle.SetTestFile(&file);
  EXPECT_TRUE(handle.DeleteAllUsageInfoForApp(app_id));

  SubSampleInfo* data = &usage_info_sub_samples_icp[0];
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  std::string key_id = a2bs_hex(
      "000000427073736800000000"                  // blob size and pssh
      "EDEF8BA979D64ACEA3C827DCD51D21ED00000022"  // Widevine system id
      "08011a0d7769646576696e655f74657374220f73"  // pssh data
      "747265616d696e675f636c697033");

  GenerateKeyRequest(key_id, kLicenseTypeStreaming, NULL);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  std::vector<uint8_t> decrypt_buffer(data->encrypt_data.size());
  CdmDecryptionParameters decryption_parameters(
      &data->key_id, &data->encrypt_data.front(), data->encrypt_data.size(),
      &data->iv, data->block_offset, &decrypt_buffer[0]);
  decryption_parameters.is_encrypted = data->is_encrypted;
  decryption_parameters.is_secure = data->is_secure;
  EXPECT_EQ(NO_ERROR, decryptor_.Decrypt(session_id_, data->validate_key_id,
                                         decryption_parameters));

  EXPECT_TRUE(std::equal(data->decrypt_data.begin(), data->decrypt_data.end(),
                         decrypt_buffer.begin()));
  decryptor_.CloseSession(session_id_);

  uint32_t num_usage_info = 0;
  CdmUsageInfo usage_info;
  CdmUsageInfoReleaseMessage release_msg;
  CdmResponseType status = decryptor_.GetUsageInfo(app_id, &usage_info);
  EXPECT_EQ(usage_info.empty() ? NO_ERROR : KEY_MESSAGE, status);

  // Discard and retry to verify usage reports can be generated multiple times
  // before release.
  status = decryptor_.GetUsageInfo(app_id, &usage_info);
  EXPECT_EQ(usage_info.empty() ? NO_ERROR : KEY_MESSAGE, status);
  while (usage_info.size() > 0) {
    for (size_t i = 0; i < usage_info.size(); ++i) {
      release_msg =
          GetUsageInfoResponse(g_license_server, g_client_auth, usage_info[i]);
      EXPECT_EQ(NO_ERROR, decryptor_.ReleaseUsageInfo(release_msg));
    }
    status = decryptor_.GetUsageInfo(app_id, &usage_info);
    switch (status) {
      case KEY_MESSAGE: EXPECT_FALSE(usage_info.empty()); break;
      case NO_ERROR: EXPECT_TRUE(usage_info.empty()); break;
      default: FAIL() << "GetUsageInfo failed with error "
                    << static_cast<int>(status) ; break;
    }
  }
}

class WvCdmUsageInfoTest
    : public WvCdmRequestLicenseTest,
      public ::testing::WithParamInterface<UsageInfoSubSampleInfo*> {};

TEST_P(WvCdmUsageInfoTest, UsageInfo) {
  Unprovision();

  UsageInfoSubSampleInfo* usage_info_data = GetParam();
  TestWvCdmClientPropertySet client_property_set;
  TestWvCdmClientPropertySet* property_set = NULL;
  if (kLevel3 == usage_info_data->security_level) {
    client_property_set.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L3);
    property_set = &client_property_set;
    Provision(kLevel3);
    Provision(kLevelDefault);
  } else {
    Provision(kLevelDefault);
  }

  CdmSecurityLevel security_level = GetDefaultSecurityLevel();
  DeviceFiles handle;
  EXPECT_TRUE(handle.Init(security_level));
  File file;
  handle.SetTestFile(&file);
  EXPECT_TRUE(handle.DeleteAllUsageInfoForApp(usage_info_data->app_id));

  for (size_t i = 0; i < usage_info_data->usage_info; ++i) {
    SubSampleInfo* data = usage_info_data->sub_sample + i;
    decryptor_.OpenSession(g_key_system, property_set, &session_id_);
    std::string key_id = a2bs_hex(
        "000000427073736800000000"                  // blob size and pssh
        "EDEF8BA979D64ACEA3C827DCD51D21ED00000022"  // Widevine system id
        "08011a0d7769646576696e655f74657374220f73"  // pssh data
        "747265616d696e675f636c6970");

    char ch = 0x33 + i;
    key_id.append(1, ch);

    GenerateKeyRequest(key_id, kLicenseTypeStreaming, property_set);
    VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

    std::vector<uint8_t> decrypt_buffer(data->encrypt_data.size());
    CdmDecryptionParameters decryption_parameters(
        &data->key_id, &data->encrypt_data.front(), data->encrypt_data.size(),
        &data->iv, data->block_offset, &decrypt_buffer[0]);
    decryption_parameters.is_encrypted = data->is_encrypted;
    decryption_parameters.is_secure = data->is_secure;
    EXPECT_EQ(NO_ERROR, decryptor_.Decrypt(session_id_, data->validate_key_id,
                                           decryption_parameters));

    EXPECT_TRUE(std::equal(data->decrypt_data.begin(), data->decrypt_data.end(),
                           decrypt_buffer.begin()));
    decryptor_.CloseSession(session_id_);
  }

  uint32_t num_usage_info = 0;
  CdmUsageInfo usage_info;
  CdmUsageInfoReleaseMessage release_msg;
  CdmResponseType status =
      decryptor_.GetUsageInfo(usage_info_data->app_id, &usage_info);
  EXPECT_EQ(usage_info.empty() ? NO_ERROR : KEY_MESSAGE, status);
  while (usage_info.size() > 0) {
    for (size_t i = 0; i < usage_info.size(); ++i) {
      release_msg =
          GetUsageInfoResponse(g_license_server, g_client_auth, usage_info[i]);
      EXPECT_EQ(NO_ERROR, decryptor_.ReleaseUsageInfo(release_msg));
    }
    status = decryptor_.GetUsageInfo(usage_info_data->app_id, &usage_info);
    switch (status) {
      case KEY_MESSAGE: EXPECT_FALSE(usage_info.empty()); break;
      case NO_ERROR: EXPECT_TRUE(usage_info.empty()); break;
      default: FAIL() << "GetUsageInfo failed with error "
                    << static_cast<int>(status) ; break;
    }
  }
}

INSTANTIATE_TEST_CASE_P(Cdm, WvCdmUsageInfoTest,
                        ::testing::Values(&usage_info_sub_sample_info[0],
                                          &usage_info_sub_sample_info[1],
                                          &usage_info_sub_sample_info[2],
                                          &usage_info_sub_sample_info[3],
                                          &usage_info_sub_sample_info[4],
                                          &usage_info_sub_sample_info[5],
                                          &usage_info_sub_sample_info[6],
                                          &usage_info_sub_sample_info[7]));

TEST_F(WvCdmRequestLicenseTest, QueryUnmodifiedSessionStatus) {
  // Test that the global value is returned when no properties are modifying it.
  CdmQueryMap system_query_info;
  CdmQueryMap::iterator system_itr;
  ASSERT_EQ(wvcdm::NO_ERROR, decryptor_.QueryStatus(&system_query_info));
  system_itr = system_query_info.find(wvcdm::QUERY_KEY_SECURITY_LEVEL);
  ASSERT_TRUE(system_itr != system_query_info.end());

  EXPECT_EQ(system_itr->second, GetSecurityLevel(NULL));
}

TEST_F(WvCdmRequestLicenseTest, QueryModifiedSessionStatus) {
  // Test that L3 is returned when properties downgrade security.
  Provision(kLevel3);
  TestWvCdmClientPropertySet property_set_L3;
  property_set_L3.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L3);

  EXPECT_EQ(QUERY_VALUE_SECURITY_LEVEL_L3, GetSecurityLevel(&property_set_L3));
}

TEST_F(WvCdmRequestLicenseTest, QueryKeyStatus) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  CdmQueryMap query_info;
  CdmQueryMap::iterator itr;
  EXPECT_EQ(wvcdm::NO_ERROR,
            decryptor_.QueryKeyStatus(session_id_, &query_info));

  itr = query_info.find(wvcdm::QUERY_KEY_LICENSE_TYPE);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_EQ(wvcdm::QUERY_VALUE_STREAMING, itr->second);
  itr = query_info.find(wvcdm::QUERY_KEY_PLAY_ALLOWED);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_EQ(wvcdm::QUERY_VALUE_TRUE, itr->second);
  itr = query_info.find(wvcdm::QUERY_KEY_PERSIST_ALLOWED);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_EQ(wvcdm::QUERY_VALUE_FALSE, itr->second);
  itr = query_info.find(wvcdm::QUERY_KEY_RENEW_ALLOWED);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_EQ(wvcdm::QUERY_VALUE_TRUE, itr->second);

  int64_t remaining_time;
  std::istringstream ss;
  itr = query_info.find(wvcdm::QUERY_KEY_LICENSE_DURATION_REMAINING);
  ASSERT_TRUE(itr != query_info.end());
  ss.str(itr->second);
  ASSERT_TRUE(ss >> remaining_time);
  EXPECT_LT(0, remaining_time);
  itr = query_info.find(wvcdm::QUERY_KEY_PLAYBACK_DURATION_REMAINING);
  ASSERT_TRUE(itr != query_info.end());
  ss.clear();
  ss.str(itr->second);
  ASSERT_TRUE(ss >> remaining_time);
  EXPECT_LT(0, remaining_time);

  itr = query_info.find(wvcdm::QUERY_KEY_RENEWAL_SERVER_URL);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_LT(0u, itr->second.size());

  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, QueryStatus) {
  CdmQueryMap query_info;
  CdmQueryMap::iterator itr;
  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.QueryStatus(&query_info));

  itr = query_info.find(wvcdm::QUERY_KEY_SECURITY_LEVEL);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_EQ(2u, itr->second.size());
  EXPECT_EQ(wvcdm::QUERY_VALUE_SECURITY_LEVEL_L3.at(0), itr->second.at(0));

  itr = query_info.find(wvcdm::QUERY_KEY_DEVICE_ID);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_GT(itr->second.size(), 0u);

  itr = query_info.find(wvcdm::QUERY_KEY_SYSTEM_ID);
  ASSERT_TRUE(itr != query_info.end());
  std::istringstream ss(itr->second);
  uint32_t system_id;
  EXPECT_TRUE(ss >> system_id);
  EXPECT_TRUE(ss.eof());

  itr = query_info.find(wvcdm::QUERY_KEY_PROVISIONING_ID);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_EQ(16u, itr->second.size());
}

TEST_F(WvCdmRequestLicenseTest, QueryKeyControlInfo) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  CdmQueryMap query_info;
  CdmQueryMap::iterator itr;
  EXPECT_EQ(wvcdm::NO_ERROR,
            decryptor_.QueryKeyControlInfo(session_id_, &query_info));

  uint32_t oem_crypto_session_id;
  itr = query_info.find(wvcdm::QUERY_KEY_OEMCRYPTO_SESSION_ID);
  ASSERT_TRUE(itr != query_info.end());
  std::istringstream ss;
  ss.str(itr->second);
  EXPECT_TRUE(ss >> oem_crypto_session_id);

  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, SecurityLevelPathBackwardCompatibility) {
  Unprovision();
  Provision(kLevelDefault);

  // override default settings unless configured through the command line
  std::string key_id;
  std::string client_auth;

  GetOfflineConfiguration(&key_id, &client_auth);

  CdmQueryMap query_info;
  CdmQueryMap::iterator itr;
  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.QueryStatus(&query_info));
  itr = query_info.find(wvcdm::QUERY_KEY_SECURITY_LEVEL);
  ASSERT_TRUE(itr != query_info.end());
  EXPECT_EQ(2u, itr->second.size());
  EXPECT_TRUE(itr->second.compare(wvcdm::QUERY_VALUE_SECURITY_LEVEL_L3) == 0 ||
              itr->second.compare(wvcdm::QUERY_VALUE_SECURITY_LEVEL_L1) == 0);

  CdmSecurityLevel security_level =
      (itr->second.compare(wvcdm::QUERY_VALUE_SECURITY_LEVEL_L1) == 0)
          ? kSecurityLevelL1
          : kSecurityLevelL3;

  std::string base_path;
  EXPECT_TRUE(Properties::GetDeviceFilesBasePath(security_level, &base_path));

  std::vector<std::string> security_dirs;
  EXPECT_TRUE(Properties::GetSecurityLevelDirectories(&security_dirs));
  size_t pos = std::string::npos;
  for (size_t i = 0; i < security_dirs.size(); i++) {
    pos = base_path.rfind(security_dirs[i]);
    if (std::string::npos != pos) break;
  }

  EXPECT_NE(std::string::npos, pos);
  std::string old_base_path(base_path, 0, pos);
  File file;
  for (size_t i = 0; i < security_dirs.size(); i++) {
    std::string path = old_base_path + kPathDelimiter + security_dirs[i];
    file.Remove(path);
  }

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  std::string provisioning_server_url;
  CdmCertificateType cert_type = kCertificateWidevine;
  std::string cert_authority, cert, wrapped_key;
  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.GetProvisioningRequest(
                                 cert_type, cert_authority, &key_msg_,
                                 &provisioning_server_url));
  EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());
  std::string response =
      GetCertRequestResponse(g_config->provisioning_server_url());
  EXPECT_NE(0, static_cast<int>(response.size()));
  EXPECT_EQ(wvcdm::NO_ERROR, decryptor_.HandleProvisioningResponse(
                                 response, &cert, &wrapped_key));
  decryptor_.CloseSession(session_id_);

  std::vector<std::string> files;
  EXPECT_TRUE(file.List(base_path, &files));
  size_t number_of_files = files.size();

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);
  CdmKeySetId key_set_id = key_set_id_;
  EXPECT_FALSE(key_set_id_.empty());
  decryptor_.CloseSession(session_id_);

  EXPECT_TRUE(file.List(base_path, &files));
  int number_of_new_files = files.size() - number_of_files;
  EXPECT_LE(1, number_of_new_files);
  EXPECT_GE(2, number_of_new_files);

  for (size_t i = 0; i < files.size(); ++i) {
    std::string from = base_path + files[i];
    if (file.IsRegularFile(from)) {
      std::string to = old_base_path + files[i];
      EXPECT_TRUE(file.Copy(from, to));
    }
  }
  EXPECT_TRUE(file.Remove(base_path));

  // Setup complete to earlier version (non-security level based) path.
  // Restore persistent license, retrieve L1, L3 streaming licenses to verify
  session_id_.clear();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  EXPECT_EQ(wvcdm::KEY_ADDED, decryptor_.RestoreKey(session_id_, key_set_id));
  decryptor_.CloseSession(session_id_);

  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);
  decryptor_.CloseSession(session_id_);

  if (security_level != kSecurityLevelL1) return;

  TestWvCdmClientPropertySet property_set;
  property_set.set_security_level(QUERY_VALUE_SECURITY_LEVEL_L3);

  EXPECT_EQ(wvcdm::NEED_PROVISIONING,
            decryptor_.OpenSession(g_key_system, &property_set, &session_id_));

  EXPECT_EQ(NO_ERROR, decryptor_.GetProvisioningRequest(
                          cert_type, cert_authority, &key_msg_,
                          &provisioning_server_url));
  EXPECT_EQ(provisioning_server_url, g_config->provisioning_server_url());
  response =
      GetCertRequestResponse(g_config->provisioning_server_url());
  EXPECT_NE(0, static_cast<int>(response.size()));
  EXPECT_EQ(NO_ERROR, decryptor_.HandleProvisioningResponse(response, &cert,
                                                            &wrapped_key));

  EXPECT_EQ(NO_ERROR,
            decryptor_.OpenSession(g_key_system, &property_set, &session_id_));
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, client_auth, false);
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, DISABLED_OfflineLicenseDecryptionTest) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  /*
    // key 1, encrypted, 256b
    DecryptionData data;
    data.is_encrypted = true;
    data.is_secure = false;
    data.key_id = wvcdm::a2bs_hex("30313233343536373839414243444546");
    data.encrypt_data = wvcdm::a2b_hex(
        "b6d7d2430aa82b1cb8bd32f02e1f3b2a8d84f9eddf935ced5a6a98022cbb4561"
        "8346a749fdb336858a64d7169fd0aa898a32891d14c24bed17fdc17fd62b8771"
        "a8e22e9f093fa0f2aacd293d471b8e886d5ed8d0998ab2fde2d908580ff88c93"
        "c0f0bbc14867267b3a3955bb6e7d05fca734a3aec3463d786d555cad83536ebe"
        "4496d934d40df2aba5aea98c1145a2890879568ae31bb8a85d74714a4ad75785"
        "7488523e697f5fd370eac746d56990a81cc76a178e3d6d65743520cdbc669412"
        "9e73b86214256c67430cf78662346cab3e2bdd6f095dddf75b7fb3868c5ff5ff"
        "3e1bbf08d456532ffa9df6e21a8bb2664c2d2a6d47ee78f9a6d53b2f2c8c087c");
    data.iv = wvcdm::a2b_hex("86856b9409743ca107b043e82068c7b6");
    data.block_offset = 0;
    data.decrypt_data = wvcdm::a2b_hex(
        "cc4a7fed8c5ac6e316e45317805c43e6d62a383ad738219c65e7a259dc12b46a"
        "d50a3f8ce2facec8eeadff9cfa6b649212b88602b41f6d4c510c05af07fd523a"
        "e7032634d9f8db5dd652d35f776376c5fc56e7031ed7cb28b72427fd4b367b6d"
        "8c4eb6e46ed1249de5d24a61aeb08ebd60984c10581042ca8b0ef6bc44ec34a0"
        "d4a77d68125c9bb1ace6f650e8716540f5b20d6482f7cfdf1b57a9ee9802160c"
        "a632ce42934347410abc61bb78fba11b093498572de38bca96101ecece455e3b"
        "5fef6805c44a2609cf97ce0dac7f15695c8058c590eda517f845108b90dfb29c"
        "e73f3656000399f2fd196bc6fc225f3a7b8f578237751fd485ff070b5289e5cf");

    std::vector<uint8_t> decrypt_buffer;
    size_t encrypt_length = data.encrypt_data.size();
    decrypt_buffer.resize(encrypt_length);

    EXPECT_EQ(NO_ERROR, decryptor_.Decrypt(session_id_,
                                           data.is_encrypted,
                                           data.is_secure,
                                           data.key_id,
                                           &data.encrypt_data.front(),
                                           encrypt_length,
                                           data.iv,
                                           data.block_offset,
                                           &decrypt_buffer.front(),
                                           0));

    EXPECT_TRUE(std::equal(data.decrypt_data.begin(), data.decrypt_data.end(),
        decrypt_buffer.begin()));
  */
  decryptor_.CloseSession(session_id_);
}

TEST_F(WvCdmRequestLicenseTest, DISABLED_RestoreOfflineLicenseDecryptionTest) {
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeOffline);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
  CdmKeySetId key_set_id = key_set_id_;
  EXPECT_FALSE(key_set_id_.empty());
  decryptor_.CloseSession(session_id_);

  session_id_.clear();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  EXPECT_EQ(wvcdm::KEY_ADDED, decryptor_.RestoreKey(session_id_, key_set_id));
  /*
    // key 1, encrypted, 256b
    DecryptionData data;
    data.is_encrypted = true;
    data.is_secure = false;
    data.key_id = wvcdm::a2bs_hex("30313233343536373839414243444546");
    data.encrypt_data = wvcdm::a2b_hex(
        "b6d7d2430aa82b1cb8bd32f02e1f3b2a8d84f9eddf935ced5a6a98022cbb4561"
        "8346a749fdb336858a64d7169fd0aa898a32891d14c24bed17fdc17fd62b8771"
        "a8e22e9f093fa0f2aacd293d471b8e886d5ed8d0998ab2fde2d908580ff88c93"
        "c0f0bbc14867267b3a3955bb6e7d05fca734a3aec3463d786d555cad83536ebe"
        "4496d934d40df2aba5aea98c1145a2890879568ae31bb8a85d74714a4ad75785"
        "7488523e697f5fd370eac746d56990a81cc76a178e3d6d65743520cdbc669412"
        "9e73b86214256c67430cf78662346cab3e2bdd6f095dddf75b7fb3868c5ff5ff"
        "3e1bbf08d456532ffa9df6e21a8bb2664c2d2a6d47ee78f9a6d53b2f2c8c087c");
    data.iv = wvcdm::a2b_hex("86856b9409743ca107b043e82068c7b6");
    data.block_offset = 0;
    data.decrypt_data = wvcdm::a2b_hex(
        "cc4a7fed8c5ac6e316e45317805c43e6d62a383ad738219c65e7a259dc12b46a"
        "d50a3f8ce2facec8eeadff9cfa6b649212b88602b41f6d4c510c05af07fd523a"
        "e7032634d9f8db5dd652d35f776376c5fc56e7031ed7cb28b72427fd4b367b6d"
        "8c4eb6e46ed1249de5d24a61aeb08ebd60984c10581042ca8b0ef6bc44ec34a0"
        "d4a77d68125c9bb1ace6f650e8716540f5b20d6482f7cfdf1b57a9ee9802160c"
        "a632ce42934347410abc61bb78fba11b093498572de38bca96101ecece455e3b"
        "5fef6805c44a2609cf97ce0dac7f15695c8058c590eda517f845108b90dfb29c"
        "e73f3656000399f2fd196bc6fc225f3a7b8f578237751fd485ff070b5289e5cf");

    std::vector<uint8_t> decrypt_buffer;
    size_t encrypt_length = data.encrypt_data.size();
    decrypt_buffer.resize(encrypt_length);

    EXPECT_EQ(NO_ERROR, decryptor_.Decrypt(session_id_,
                                           data.is_encrypted,
                                           data.is_secure,
                                           data.key_id,
                                           &data.encrypt_data.front(),
                                           encrypt_length,
                                           data.iv,
                                           data.block_offset,
                                           &decrypt_buffer.front(),
                                           0));

    EXPECT_TRUE(std::equal(data.decrypt_data.begin(), data.decrypt_data.end(),
        decrypt_buffer.begin()));
  */
  decryptor_.CloseSession(session_id_);
}

// TODO(rfrias, edwinwong): pending L1 OEMCrypto due to key block handling
/*
TEST_F(WvCdmRequestLicenseTest, KeyControlBlockDecryptionTest) {
  decryptor_.OpenSession(g_key_system, &session_id_);
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  DecryptionData data;

  // block 4, key 2, encrypted
  data.is_encrypted = true;
  data.is_secure = false;
  data.key_id = wvcdm::a2bs_hex("0915007CAA9B5931B76A3A85F046523E");
  data.encrypt_data = wvcdm::a2b_hex(
      "6758ac1c6ccf5d08479e3bfc62bbc0fd154aff4415aa7ed53d89e3983248d117"
      "ab5137ae7cedd9f9d7321d4cf35a7013237afbcc2d893d1d928efa94e9f7e2ed"
      "1855463cf75ff07ecc0246b90d0734f42d98aeea6a0a6d2618a8339bd0aca368"
      "4fb4a4670c0385e5bd5de9e2d8b9226851b8f8955adfbab968793b46fd152f5e"
      "e608467bb2695836f8f76c32731f5e208176d05e4b07020d58f6282c477f3840"
      "b8079c02e8bd1d03191d190cc505ddfbb2e9bacc794534c91fe409d62f5389b9"
      "35ed66134bd30f09f8da9dbfe6b8cf53d13cae34dae6e89109216e3a02233d5c"
      "2f66aef74313aae4a99b654b485b5cc207b2dc8d44a8b99a4dc196a9820eccef");
  data.iv = wvcdm::a2b_hex("c8f2d133ec357fe727cd233b3bfa755f");
  data.block_offset = 0;
  data.decrypt_data = wvcdm::a2b_hex(
      "34bab89185f1be990dfc454410c7c9093d008bc783908838b02a65b26db28759"
      "dca9dc5f117b3c8c3898358722d1b4c490e5a5d168ba0f9f8a3d4371b8fd1057"
      "2d6dd65f3f9d1850de8d76dc71bd6dc6c23da4e1223fcc3e47162033a6f82890"
      "e2bd6e9d6ddbe453830afc89064ed18078c786f8f746fcbafd88e83e7160cce5"
      "62fa7a7d699ef8421bda020d242ae4f61a786213b707c3b17b83d77510f9a07e"
      "d9d7e47d8f8fa2aff86eb26d61ddf384a27513e3facf6b1f5fe6c0d063b8856c"
      "c486d930393ea79ba73ba293eda39059e2ce9ee7bd5d31ab11f35e55dc35dfe0"
      "ea5e2ec684014852add6e29ce7d88a1595641ae4c0dd10155526b5a87560ec9d");

  std::vector<uint8_t> decrypt_buffer;
  size_t encrypt_length = data[i].encrypt_data.size();
  decrypt_buffer.resize(encrypt_length);

  EXPECT_EQ(NO_ERROR, decryptor_.Decrypt(session_id_,
                                         data.is_encrypted,
                                         data.is_secure,
                                         data.key_id,
                                         &data.encrypt_data.front(),
                                         encrypt_length,
                                         data.iv,
                                         data.block_offset,
                                         &decrypt_buffer.front()));

    EXPECT_TRUE(std::equal(data.decrypt_data.begin(),
        data.decrypt_data.end(),
        decrypt_buffer.begin()));
  }
  decryptor_.CloseSession(session_id_);
}
*/

class WvCdmSessionSharingTest
    : public WvCdmRequestLicenseTest,
      public ::testing::WithParamInterface<SessionSharingSubSampleInfo*> {};

TEST_P(WvCdmSessionSharingTest, SessionSharingTest) {
  SessionSharingSubSampleInfo* session_sharing_info = GetParam();

  TestWvCdmClientPropertySet property_set;
  property_set.set_session_sharing_mode(
      session_sharing_info->session_sharing_enabled);

  decryptor_.OpenSession(g_key_system, &property_set, &session_id_);
  CdmSessionId gp_session_id_1 = session_id_;
  GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, g_client_auth, false);

  // TODO(rfrias): Move content information to ConfigTestEnv
  std::string gp_client_auth2 =
      "?source=YOUTUBE&video_id=z3S_NhwueaM&oauth=ya.gtsqawidevine";
  std::string gp_key_id2 = wvcdm::a2bs_hex(
      "000000347073736800000000"                    // blob size and pssh
      "edef8ba979d64acea3c827dcd51d21ed00000014"    // Widevine system id
      "08011210bdf1cb4fffc6506b8b7945b0bd2917fb");  // pssh data

  decryptor_.OpenSession(g_key_system, &property_set, &session_id_);
  CdmSessionId gp_session_id_2 = session_id_;
  GenerateKeyRequest(gp_key_id2, kLicenseTypeStreaming);
  VerifyKeyRequestResponse(g_license_server, gp_client_auth2, false);

  SubSampleInfo* data = session_sharing_info->sub_sample;
  std::vector<uint8_t> decrypt_buffer(data->encrypt_data.size());
  CdmDecryptionParameters decryption_parameters(
      &data->key_id, &data->encrypt_data.front(), data->encrypt_data.size(),
      &data->iv, data->block_offset, &decrypt_buffer[0]);
  decryption_parameters.is_encrypted = data->is_encrypted;
  decryption_parameters.is_secure = data->is_secure;

  if (session_sharing_info->session_sharing_enabled || !data->is_encrypted) {
    EXPECT_EQ(NO_ERROR,
              decryptor_.Decrypt(gp_session_id_2, data->validate_key_id,
                                 decryption_parameters));
    EXPECT_TRUE(std::equal(data->decrypt_data.begin(), data->decrypt_data.end(),
                           decrypt_buffer.begin()));
  } else {
    EXPECT_EQ(NEED_KEY,
              decryptor_.Decrypt(gp_session_id_2, data->validate_key_id,
                                 decryption_parameters));
  }

  decryptor_.CloseSession(gp_session_id_1);
  decryptor_.CloseSession(gp_session_id_2);
}

TEST_F(WvCdmRequestLicenseTest, DecryptionKeyExpiredTest) {
  const std::string kCpKeyId = a2bs_hex(
      "000000347073736800000000"                    // blob size and pssh
      "EDEF8BA979D64ACEA3C827DCD51D21ED00000014"    // Widevine system id
      "0801121030313233343536373839616263646566");  // pssh data
  SubSampleInfo* data = &single_encrypted_sub_sample_short_expiry;
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  if (data->retrieve_key) {
    GenerateKeyRequest(kCpKeyId, kLicenseTypeStreaming);
    VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
  }

  std::vector<uint8_t> decrypt_buffer(data->encrypt_data.size());
  CdmDecryptionParameters decryption_parameters(
      &data->key_id, &data->encrypt_data.front(), data->encrypt_data.size(),
      &data->iv, data->block_offset, &decrypt_buffer[0]);
  decryption_parameters.is_encrypted = data->is_encrypted;
  decryption_parameters.is_secure = data->is_secure;
  EXPECT_EQ(NO_ERROR, decryptor_.Decrypt(session_id_, data->validate_key_id,
                                         decryption_parameters));
  sleep(kSingleEncryptedSubSampleIcpLicenseDurationExpiration);
  EXPECT_EQ(NEED_KEY, decryptor_.Decrypt(session_id_, data->validate_key_id,
                                         decryption_parameters));
  decryptor_.CloseSession(session_id_);
}

INSTANTIATE_TEST_CASE_P(Cdm, WvCdmSessionSharingTest,
                        ::testing::Range(&session_sharing_sub_samples[0],
                                         &session_sharing_sub_samples[6]));

class WvCdmDecryptionTest
    : public WvCdmRequestLicenseTest,
      public ::testing::WithParamInterface<SubSampleInfo*> {};

TEST_P(WvCdmDecryptionTest, DecryptionTest) {
  SubSampleInfo* data = GetParam();
  decryptor_.OpenSession(g_key_system, NULL, &session_id_);
  if (data->retrieve_key) {
    GenerateKeyRequest(g_key_id, kLicenseTypeStreaming);
    VerifyKeyRequestResponse(g_license_server, g_client_auth, false);
  }

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
  decryptor_.CloseSession(session_id_);
}

INSTANTIATE_TEST_CASE_P(Cdm, WvCdmDecryptionTest,
                        ::testing::Values(&clear_sub_sample,
                                          &clear_sub_sample_no_key,
                                          &single_encrypted_sub_sample,
                                          &switch_key_encrypted_sub_samples[0],
                                          &partial_encrypted_sub_samples[0]));

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
  std::cout << "cp (case sensitive) for Content Protection server"
            << std::endl << std::endl;

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
