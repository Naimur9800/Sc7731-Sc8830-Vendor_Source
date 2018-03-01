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
#ifndef _SP_AF_CTRL_H_
#define _SP_AF_CTRL_H_
#include <sys/types.h>
#include "sp_af_common.h"
#include "AF_Common.h"

#define SFT_AF_LOGE af_ops->cb_sft_af_log

struct sft_af_context {
	uint32_t magic_start;
	void *isp_handle;
	uint32_t bypass;
	uint32_t active_win;
	uint32_t is_inited;
	uint32_t ae_is_stab;
	uint32_t ae_cur_lum;
	uint32_t ae_is_locked;
	uint32_t awb_is_stab;
	uint32_t awb_is_locked;
	uint32_t cur_env_mode;
	uint32_t af_mode;
	uint32_t cur_pos;
	uint32_t is_runing;
	AFDATA 		afCtl;
	AFDATA_RO 	afv;
	AFDATA_FV	fv;
	GLOBAL_OTP	gOtp;
	struct sp_afm_cfg_info	sprd_filter;
	struct sft_af_ctrl_ops af_ctrl_ops;
	uint32_t touch_win_cnt;
	struct win_coord win_pos[25];
	uint32_t cur_awb_r_gain;
	uint32_t cur_awb_g_gain;
	uint32_t cur_awb_b_gain;
	uint32_t cur_fps;
	uint32_t caf_active;
	uint32_t flash_on;
	struct sft_af_face_area fd_info;
	uint32_t cur_ae_again;
	uint32_t cur_ae_bv;
	uint32_t magic_end;
};

////////////////////////// Funcs //////////////////////////


///////////////////// for System ///////////////
sft_af_handle_t sft_af_init(void* isp_handle);
int32_t sft_af_deinit(sft_af_handle_t handle);
int32_t sft_af_calc(sft_af_handle_t handle);
int32_t sft_af_ioctrl(sft_af_handle_t handle, enum sft_af_cmd cmd, void *param0, void *param1);
#endif
