# Copyright (C) 2008 The Android Open Source Project
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


LOCAL_PATH:= $(call my-dir)
# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.board.platform>.so
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc/
LOCAL_SRC_FILES := ../src/AF_Control.c ../src/AF_Interface.c ../src/AF_Main.c ../src/AF_Debug.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(LOCAL_PATH)/../lib

LOCAL_SHARED_LIBRARIES := libAF

LOCAL_MODULE := libsft_af_ctrl

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := ../lib/libAF.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

