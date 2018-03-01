#jinglong.chen add
LOCAL_PATH := $(call my-dir)

ifeq ($(PLATFORM_VERSION),4.4.4)
#source files
WLNPI_OBJS	  += wlnpi.c cmd.c
#cflags
IWNPI_CFLAGS ?= -O2 -g
IWNPI_CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -DCONFIG_LIBNL20
#include dirs
INCLUDES += external/libnl-headers

#Build wlnpi tool
include $(CLEAR_VARS)
LOCAL_MODULE := iwnpi
LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils

LOCAL_STATIC_LIBRARIES += libcutils   \
                          libutils
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = $(IWNPI_CFLAGS)
LOCAL_SRC_FILES = $(WLNPI_OBJS)
LOCAL_C_INCLUDES = $(INCLUDES)
LOCAL_STATIC_LIBRARIES += libnl_2
include $(BUILD_EXECUTABLE)
endif
