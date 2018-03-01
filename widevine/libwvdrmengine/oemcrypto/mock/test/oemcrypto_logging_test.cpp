// Copyright 2014 Google Inc. All Rights Reserved.

#include "OEMCryptoCENC.h"

#include <string>
#include <gtest/gtest.h>
#include "log.h"
#include "oemcrypto_logging.h"
#include "oemcrypto_mock.cpp"

class OEMCryptoLoggingTest : public ::testing::Test {
 protected:
  OEMCryptoLoggingTest() {}

  void SetUp() {
    ::testing::Test::SetUp();
    ASSERT_EQ(OEMCrypto_SUCCESS, OEMCrypto_Initialize());
  }

  void TearDown() {
    OEMCrypto_Terminate();
    ::testing::Test::TearDown();
  }
};

TEST_F(OEMCryptoLoggingTest, TestDumpHexFunctions) {
  uint8_t vector[] = { 0xFA, 0x11 , 0x28 , 0x33 };
  std::string buffer = "";
  wvoec_mock::dump_hex_helper(buffer, "name", vector, (size_t)4);
  ASSERT_EQ(buffer, "name = \n     wvcdm::a2b_hex(\"FA112833\");\n");

  buffer = "";
  uint8_t vector2[] = { 0xFA, 0x11 , 0x28 , 0x33 ,
      0xFA, 0x11 , 0x28 , 0x33 , 0xFA, 0x11 ,
      0x28 , 0x33 , 0xFA, 0x11 , 0x28 ,  0x33 , 0xFA, 0x11 , 0x28 , 0x33 ,
      0xFA, 0x11 , 0x28 , 0x33 , 0x01, 0x14 , 0x28 , 0xAB,  0xFA, 0xCD ,
      0xEF , 0x67, 0x01, 0x14 , 0x28 , 0xAB,  0xFA, 0xCD , 0xEF , 0x67  };
  wvoec_mock::dump_hex_helper(buffer, "name", vector2, (size_t)40);

  ASSERT_EQ(buffer, "name = \n     wvcdm::a2b_hex(\"FA112833FA112833FA112833F"
            "A112833FA112833FA112833011428ABFACDEF67\"\n                    \""
            "011428ABFACDEF67\");\n");

  buffer = "";
  wvoec_mock::dump_array_part_helper(buffer, "array",
           (size_t) 5, "name", vector2, (size_t) 40);
  char* exp = "std::string s5_name = \n     wvcdm::a2b_hex(\"FA112833FA112833F"
              "A112833FA112833FA112833FA112833011428ABFACDEF67\"\n            "
              "        \"011428ABFACDEF67\");\narray[5].name = message_ptr + me"
              "ssage.find(s5_name.data());\n";
  ASSERT_EQ(buffer, exp);

  buffer = "";
  wvoec_mock::dump_array_part_helper(buffer, "array", (size_t) 5,
                                     "name", NULL, (size_t) 40);
  ASSERT_EQ(buffer, "array[5].name = NULL;\n");
}

TEST_F(OEMCryptoLoggingTest, TestChangeLoggingLevel) {
  wvcdm::LogPriority default_logging_level = wvcdm::LOG_WARN;
  wvoec_mock::SetLoggingLevel(1);
  ASSERT_EQ(wvcdm::g_cutoff, default_logging_level);

  wvoec_mock::SetLoggingLevel(2);
  ASSERT_EQ(wvcdm::g_cutoff, wvcdm::LOG_INFO);

  wvoec_mock::SetLoggingSettings(
              wvcdm::LOG_WARN,
              wvoec_mock::kLoggingDumpTraceAll);
  ASSERT_EQ(wvcdm::g_cutoff, wvcdm::LOG_WARN);
  ASSERT_EQ(wvoec_mock::LogCategoryEnabled(
            wvoec_mock::kLoggingDumpTraceAll), true);
  wvoec_mock::TurnOffLoggingForAllCategories();

  wvoec_mock::SetLoggingLevel(wvcdm::LOG_VERBOSE);
  ASSERT_EQ(wvcdm::g_cutoff, wvcdm::LOG_VERBOSE);

  wvoec_mock::SetLoggingLevel(1);
}

  namespace wvoec_mock {
TEST_F(OEMCryptoLoggingTest, TestChangeLoggingCategories) {
  TurnOffLoggingForAllCategories();
  ASSERT_EQ(LogCategoryEnabled(kLoggingTraceDecryption |
            kLoggingTraceOEMCryptoCalls), false);

  AddLoggingForCategories(kLoggingDumpKeyControlBlocks |
                          kLoggingDumpDerivedKeys);
  ASSERT_EQ(LogCategoryEnabled(kLoggingDumpKeyControlBlocks), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingTraceUsageTable), false);
  ASSERT_EQ(LogCategoryEnabled(kLoggingDumpTraceAll), true);

  RemoveLoggingForCategories(kLoggingDumpKeyControlBlocks |
                                   kLoggingTraceUsageTable);
  ASSERT_EQ(LogCategoryEnabled(kLoggingDumpKeyControlBlocks), false);

  ASSERT_EQ(LogCategoryEnabled(kLoggingDumpDerivedKeys), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingTraceUsageTable), false);

  TurnOffLoggingForAllCategories();
  bool flag = false;
  if (LogCategoryEnabled(kLoggingTraceUsageTable)) {
    flag = true;
  }
  ASSERT_EQ(flag, false);

  AddLoggingForCategories(kLoggingDumpTraceAll);
  if (LogCategoryEnabled(kLoggingDumpKeyControlBlocks)) {
    flag = true;
  }
  ASSERT_EQ(flag, true);

  ASSERT_EQ(LogCategoryEnabled(kLoggingTraceOEMCryptoCalls), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingDumpContentKeys), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingDumpKeyControlBlocks), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingDumpDerivedKeys), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingTraceNonce), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingTraceDecryption), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingTraceUsageTable), true);
  ASSERT_EQ(LogCategoryEnabled(kLoggingDumpTraceAll), true);

  flag= false;
  RemoveLoggingForCategories(kLoggingDumpKeyControlBlocks);
  if ( LogCategoryEnabled(kLoggingDumpKeyControlBlocks) ) {
    flag = true;
  }
  ASSERT_EQ(flag, false);
}

}  // namespace wvoec_mock

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  wvcdm::InitLogging(argc, argv);

  return RUN_ALL_TESTS();
}

