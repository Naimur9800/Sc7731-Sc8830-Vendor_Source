LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    TestPlugin.cpp \
    ../src/WVMLogging.cpp

LOCAL_C_INCLUDES+= \
    bionic \
    vendor/widevine/proprietary/include \
    vendor/widevine/proprietary/drmwvmplugin/include \
    vendor/widevine/proprietary/streamcontrol/include \
    external/stlport/stlport \
    frameworks/av/drm/libdrmframework/include \
    frameworks/av/drm/libdrmframework/plugins/common/include

LOCAL_C_INCLUDES_x86 += $(TOP)/system/core/include/arch/linux-x86

LOCAL_SHARED_LIBRARIES := \
    libstlport            \
    liblog                \
    libutils              \
    libz                  \
    libdl

LOCAL_STATIC_LIBRARIES := \
    libdrmframeworkcommon

LOCAL_MODULE:=test-wvdrmplugin

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE_TARGET_ARCH := $(WIDEVINE_SUPPORTED_ARCH)

include $(BUILD_EXECUTABLE)
