LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    TestPlayer.cpp

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES += \
    bionic \
    vendor/widevine/proprietary/include \
    external/stlport/stlport \
    vendor/widevine/proprietary/streamcontrol/include \
    vendor/widevine/proprietary/drmwvmplugin/include \
    frameworks/av/drm/libdrmframework/include \
    frameworks/av/drm/libdrmframework/plugins/common/include

LOCAL_C_INCLUDES_x86 += $(TOP)/system/core/include/arch/linux-x86

LOCAL_SHARED_LIBRARIES := \
    libstlport \
    libdrmframework \
    liblog \
    libutils \
    libz \
    libcutils \
    libdl \
    libWVStreamControlAPI_L$(BOARD_WIDEVINE_OEMCRYPTO_LEVEL) \
    libwvdrm_L$(BOARD_WIDEVINE_OEMCRYPTO_LEVEL)

LOCAL_MODULE:=test-wvplayer_L$(BOARD_WIDEVINE_OEMCRYPTO_LEVEL)

LOCAL_MODULE_TARGET_ARCH := $(WIDEVINE_SUPPORTED_ARCH)
LOCAL_MULTILIB := 32

include $(BUILD_EXECUTABLE)
