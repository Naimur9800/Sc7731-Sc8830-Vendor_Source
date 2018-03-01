LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main.c modem_boot.c packet.c crc16.c

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libhardware_legacy

ifeq ($(strip $(TARGET_USERIMAGES_USE_EXT4)),true)
LOCAL_CFLAGS := -DCONFIG_EMMC
endif

LOCAL_MODULE := modem_control

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
