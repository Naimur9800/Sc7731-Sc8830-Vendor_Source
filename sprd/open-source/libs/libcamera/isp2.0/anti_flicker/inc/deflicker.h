/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _DEFLICKER_H_
#define _DEFLICKER_H_

#ifdef __cplusplus
extern "C"
{
#endif

int antiflcker_sw_init();
int antiflcker_sw_deinit();
int antiflcker_sw_process(int input_img_width, int input_img_height, int *debug_sat_img_H_scaling, int exposure_time, int reg_flicker_thrd_frame, int reg_flicker_thrd_frame_still, int reg_flicker_thrd_video_still);

#ifdef __cplusplus
}
#endif

#endif
