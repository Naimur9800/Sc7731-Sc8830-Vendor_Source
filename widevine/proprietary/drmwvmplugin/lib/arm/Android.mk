LOCAL_PATH:= $(call my-dir)

ifneq ($(BOARD_USES_GENERIC_WIDEVINE),false)

#########################################################################
# libwvdrm_L?.so

include $(CLEAR_VARS)

LOCAL_MODULE := libwvdrm_L$(BOARD_WIDEVINE_OEMCRYPTO_LEVEL)
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_STRIP_MODULE := true

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := widevine
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_MULTILIB := 32
include $(BUILD_PREBUILT)

#########################################################################
# libwvocs_L?.a

include $(CLEAR_VARS)

LOCAL_MODULE := libwvocs_L$(BOARD_WIDEVINE_OEMCRYPTO_LEVEL)
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH := arm
include $(BUILD_PREBUILT)

endif

