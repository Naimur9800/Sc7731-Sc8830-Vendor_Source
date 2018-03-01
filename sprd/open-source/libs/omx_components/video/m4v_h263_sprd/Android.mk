LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8825)
include $(LOCAL_PATH)/sc8825/Android.mk
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc7710)
include $(LOCAL_PATH)/sc7710/Android.mk
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
include $(LOCAL_PATH)/sc8830/Android.mk
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
include $(LOCAL_PATH)/sc8830/Android.mk
endif

include $(TOP)/vendor/sprd/open-source/libs/omx_components/video/m4v_h263_sprd/thumbnail/Android.mk

