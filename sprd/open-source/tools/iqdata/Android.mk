LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := iqdata_daemon.c
LOCAL_MODULE := iqdata_daemon
LOCAL_STATIC_LIBRARIES := libcutils liblog
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
