#Build libengbt
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := bt_cmd_executer.c 

ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
  LOCAL_CFLAGS += -DSPRD_WCNBT_SR2351
endif

ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
  LOCAL_CFLAGS += -DSPRD_WCNBT_MARLIN
endif


LOCAL_MODULE := libengbt
LOCAL_MODULE_TAGS := debug

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
                          libhardware \
                          libhardware_legacy \
			  libcutils
include $(BUILD_SHARED_LIBRARY)


