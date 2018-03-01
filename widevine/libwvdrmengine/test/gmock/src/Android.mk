# Copyright 2014 Google Inc. All Rights Reserved.

# gMock builds 2 libraries: libgmock and libgmock_main. libgmock contains most
# of the code and libgmock_main just provides a common main to run the test.
# (i.e. If you link against libgmock_main you shouldn't provide a main() entry
# point.  If you link against libgmock_main, do not also link against
# libgtest_main, as libgmock_main is a superset of libgtest_main, and they will
# conflict in subtle ways.)
#
# We build these 2 libraries for the target device only.

LOCAL_PATH := $(call my-dir)

libgmock_target_includes := \
  $(LOCAL_PATH)/.. \
  $(LOCAL_PATH)/../include \
  $(TOP)/external/gtest/include \

libgmock_host_includes := \
  $(LOCAL_PATH)/.. \
  $(LOCAL_PATH)/../include \
  $(TOP)/external/gtest/include \

libgmock_cflags := \
  -DGTEST_HAS_TR1_TUPLE \
  -DGTEST_USE_OWN_TR1_TUPLE \
  -Wno-missing-field-initializers \

#######################################################################
# gmock lib target

include $(CLEAR_VARS)

LOCAL_SDK_VERSION := 9

LOCAL_NDK_STL_VARIANT := stlport_static

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := gmock-all.cc

LOCAL_C_INCLUDES := $(libgmock_target_includes)

LOCAL_CFLAGS += $(libgmock_cflags)

LOCAL_MODULE := libgmock

LOCAL_MODULE_TARGET_ARCH := arm mips x86

include $(BUILD_STATIC_LIBRARY)

#######################################################################
# gmock_main lib target

include $(CLEAR_VARS)

LOCAL_SDK_VERSION := 9

LOCAL_NDK_STL_VARIANT := stlport_static

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := gmock_main.cc

LOCAL_C_INCLUDES := $(libgmock_target_includes)

LOCAL_CFLAGS += $(libgmock_cflags)

LOCAL_MODULE := libgmock_main

LOCAL_MODULE_TARGET_ARCH := arm mips x86

include $(BUILD_STATIC_LIBRARY)
