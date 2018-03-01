LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        Testlibwvm.cpp

LOCAL_C_INCLUDES+=                      \
    bionic                              \
    vendor/widevine/proprietary/include \
    external/stlport/stlport            \
    frameworks/av/media/libstagefright

LOCAL_C_INCLUDES_x86 += $(TOP)/system/core/include/arch/linux-x86

LOCAL_SHARED_LIBRARIES := \
    libstlport            \
    libdrmframework       \
    libstagefright        \
    liblog                \
    libutils              \
    libz                  \
    libdl

LOCAL_MODULE:=test-libwvm

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE_TARGET_ARCH := $(WIDEVINE_SUPPORTED_ARCH)

include $(BUILD_EXECUTABLE)
