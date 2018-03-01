LOCAL_PATH:= $(call my-dir)

ifeq ($(PLATFORM_VERSION),4.4.4)
include $(LOCAL_PATH)/4.4/4_4.mk
else
include $(LOCAL_PATH)/5.1/5_1.mk
endif
