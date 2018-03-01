// Copyright 2013 Google Inc. All Rights Reserved.

// For platform specific test vectors

#include <string>

namespace wvcdm {
namespace test_vectors {

// for FileStore unit tests
static const std::string kExistentFile = "/system/bin/sh";
static const std::string kExistentDir = "/system/bin";
static const std::string kNonExistentFile = "/system/bin/enoext";
static const std::string kNonExistentDir = "/system/bin_enoext";
static const std::string kTestDir = "/data/mediadrm/IDM0/";

}  // namespace test_vectors
}  // namespace wvcdm
