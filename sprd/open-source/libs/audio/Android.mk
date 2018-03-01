# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifeq ($(strip $(BOARD_USES_TINYALSA_AUDIO)),true)

LOCAL_PATH := $(call my-dir)

#TinyAlsa audio

include $(CLEAR_VARS)

LOCAL_MODULE := audio.primary.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar -g

ifeq ($(strip $(BOARD_USES_LINE_CALL)), true)
LOCAL_CFLAGS += -D_VOICE_CALL_VIA_LINEIN
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
LOCAL_CFLAGS += -DAUDIO_SPIPE_TD
LOCAL_CFLAGS += -D_LPA_IRAM
endif

ifeq ($(TARGET_BUILD_VARIANT),userdebug)
LOCAL_CFLAGS += -DAUDIO_DEBUG
endif

ifneq ($(filter scx35_sc9620referphone scx35_sc9620openphone scx35_sc9620openphone_zt, $(TARGET_BOARD)),)
LOCAL_CFLAGS += -DVB_CONTROL_PARAMETER_V2
endif

ifeq ($(strip $(AUDIO_CONTROL_PARAMETER_V2)), true)
LOCAL_CFLAGS += -DVB_CONTROL_PARAMETER_V2
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
LOCAL_CFLAGS += -DAUDIO_SPIPE_TD
LOCAL_CFLAGS += -D_LPA_IRAM
endif

ifeq ($(strip $(BOARD_USES_SS_VOIP)), true)
# Default case, Nothing to do.
else
LOCAL_CFLAGS += -DVOIP_DSP_PROCESS
endif

ifeq ($(strip $(BOARD_AUDIO_OLD_MODEM)), true)
	LOCAL_CFLAGS += -DAUDIO_OLD_MODEM
endif


LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	external/expat/lib \
	system/media/audio_utils/include \
	system/media/audio_effects/include \
	vendor/sprd/open-source/apps/engmode \
	vendor/sprd/open-source/libs/audio/vb_pga \
	vendor/sprd/open-source/libs/audio/record_process \
	vendor/sprd/open-source/libs/audio/nv_exchange \
	vendor/sprd/open-source/libs/libatchannel \
	vendor/sprd/open-source/libs/audio/DumpData
	
	BOARD_EQ_DIR := v1

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
	BOARD_EQ_DIR := v2
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
	BOARD_EQ_DIR := v2
endif

LOCAL_C_INCLUDES += vendor/sprd/open-source/libs/audio/vb_effect/$(BOARD_EQ_DIR)

LOCAL_SRC_FILES := audio_hw.c tinyalsa_util.c audio_pga.c \
			record_process/aud_proc_config.c.arm \
			record_process/aud_filter_calc.c.arm

ifeq ($(strip $(AUDIO_MUX_PIPE)), true)
LOCAL_SRC_FILES  += audio_mux_pcm.c
LOCAL_CFLAGS += -DAUDIO_MUX_PCM
endif

LOCAL_SHARED_LIBRARIES := \
	liblog libcutils libtinyalsa libaudioutils \
	libexpat libdl \
	libvbeffect libvbpga libnvexchange libdumpdata \
	libatchannel  \
	libvbeffect libvbpga libnvexchange libdumpdata\
	libatchannel libhardware_legacy


LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
