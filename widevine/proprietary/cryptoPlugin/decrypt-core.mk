#
#  To be included by platform-specific vendor Android.mk to build
#  Widevine wvm static library.  Sets up includes and defines the core libraries
#  required.
#
include $(TOP)/vendor/widevine/proprietary/wvm/common.mk

ifndef BOARD_WIDEVINE_OEMCRYPTO_LEVEL
$(error BOARD_WIDEVINE_OEMCRYPTO_LEVEL not defined!)
endif

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libwvdecryptcommon

LOCAL_STATIC_LIBRARIES := \
        liboemcrypto
