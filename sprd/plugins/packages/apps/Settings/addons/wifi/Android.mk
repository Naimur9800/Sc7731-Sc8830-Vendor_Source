LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_SRC_FILES += src/plugin/sprd/supportcmcc/IWifiSettings.aidl

LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_PACKAGE_NAME := WifiSettingsAddon

LOCAL_APK_LIBRARIES += Settings

LOCAL_DEX_PREOPT := false

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

