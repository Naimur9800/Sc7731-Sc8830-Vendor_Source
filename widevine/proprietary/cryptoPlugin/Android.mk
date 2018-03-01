LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BOARD_WIDEVINE_OEMCRYPTO_LEVEL),1)
LOCAL_CFLAGS := -DREQUIRE_SECURE_BUFFERS
endif

LOCAL_SRC_FILES := \
        WVCryptoPlugin.cpp

LOCAL_C_INCLUDES := \
        $(TOP)/vendor/widevine/proprietary/wvm/include \
        $(TOP)/external/openssl/include

LOCAL_CFLAGS += -Wno-unused-parameter

LOCAL_MODULE:= libwvdecryptcommon
LOCAL_MODULE_TAGS := optional

# Not 64-bit compatible, WVCryptoPlugin::decrypt stores a pointer in a uint32
LOCAL_MULTILIB := 32

include $(BUILD_STATIC_LIBRARY)
