WIDEVINE_SUPPORTED_ARCH := arm x86
include $(call all-subdir-makefiles)

# sprd widewine integration

# libdrmwvmplugin.so
include $(CLEAR_VARS)
-include $(TOP)/vendor/widevine/proprietary/drmwvmplugin/plugin-core.mk
LOCAL_MODULE := libdrmwvmplugin
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/drm
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES += liboemcrypto
LOCAL_PRELINK_MODULE := false
# Android L release supports both 32 bit and 64 bit architecture. Since mediaserver and
# drmserver both run as 32 bit, set this build variable to build 32 bit Widevine libraries.
LOCAL_MULTILIB := 32
include $(BUILD_SHARED_LIBRARY)

# libwvm.so
include $(CLEAR_VARS)
-include $(TOP)/vendor/widevine/proprietary/wvm/wvm-core.mk
LOCAL_MODULE := libwvm
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES += liboemcrypto
LOCAL_PRELINK_MODULE := false
LOCAL_MULTILIB := 32
include $(BUILD_SHARED_LIBRARY)

# libdrmdecrypt.so
include $(CLEAR_VARS)
-include $(TOP)/vendor/widevine/proprietary/cryptoPlugin/decrypt-core.mk
LOCAL_C_INCLUDES := \
$(TOP)/frameworks/native/include/media/hardware \
$(TOP)/vendor/widevine/proprietary/cryptoPlugin
LOCAL_SHARED_LIBRARIES := \
libstagefright_foundation \
liblog \
libutils \
libcutils \
libssl \
libcrypto
LOCAL_MODULE := libdrmdecrypt
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
include $(BUILD_SHARED_LIBRARY)
