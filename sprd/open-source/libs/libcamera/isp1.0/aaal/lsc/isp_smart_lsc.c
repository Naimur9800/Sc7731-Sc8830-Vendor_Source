/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "isp_smart_lsc.h"
#include "isp_com.h"
#include "isp_alg.h"
#include "isp_ae_ctrl.h"
#include "lsc_adv.h"
#include "isp_log.h"
#include "sensor_drv_u.h"

//#define ISP_ADV_LSC_ENABLE


#ifdef WIN32
#define ISP_LOGE
#define ISP_LOGW
#define ISP_LOGI
#define ISP_LOGD
#define ISP_LOGV
#else
#define SMART_LSC_DEBUG_STR     "ISP_SMART_LSC: %d, %s: "
#define SMART_LSC_DEBUG_ARGS    __LINE__,__FUNCTION__

#define ISP_LOGE(format,...) ALOGE(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGW(format,...) ALOGW(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGI(format,...) ALOGI(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGD(format,...) ALOGD(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGV(format,...) ALOGV(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#endif

void isp_smart_lsc_set_param(struct lsc_adv_init_param *lsc_param, struct isp_adv_lsc_param *isp_adv_lsc)
{
	uint32_t i = 0;
	struct sensor_adv_lsc_param *adv_lsc = &isp_adv_lsc->adv_lsc;
	if (adv_lsc->enable) {
		if (0 == adv_lsc->alg_id) {
			lsc_param->tune_param.pre2_param.enable = 0;
		} else if (1 == adv_lsc->alg_id) {
			lsc_param->tune_param.pre2_param.enable = 1;
		}
		lsc_param->param_level = adv_lsc->param_level;
		lsc_param->tune_param.debug_level = adv_lsc->debug_level;
		if (lsc_param->param_level >= LSC_MAX_DEFAULT_NUM) {
			lsc_param->tune_param.gain_param.ct_percent = adv_lsc->ct_percent;
			lsc_param->tune_param.gain_param.min_gain = adv_lsc->gain_range.min;
			lsc_param->tune_param.gain_param.max_gain = adv_lsc->gain_range.max;
			lsc_param->tune_param.pre_param.correction_intensity = adv_lsc->lsc1.intensity;
			lsc_param->tune_param.pre_param.polyfit_level = adv_lsc->lsc1.polyfit_level;
			lsc_param->tune_param.pre_param.seg_info.center_r = adv_lsc->lsc1.seg1_param.center_r;
			lsc_param->tune_param.pre_param.seg_info.luma_thr = adv_lsc->lsc1.seg1_param.luma_thr;
			lsc_param->tune_param.pre_param.seg_info.luma_gain = adv_lsc->lsc1.seg1_param.luma_gain;
			lsc_param->tune_param.pre_param.seg_info.chroma_thr = adv_lsc->lsc1.seg1_param.chroma_thr;
			lsc_param->tune_param.pre_param.seg_info.chroma_gain = adv_lsc->lsc1.seg1_param.chroma_gain;
			lsc_param->tune_param.pre_param.flat_param.flat_ratio = adv_lsc->lsc1.seg_ratio;
			lsc_param->tune_param.pre_param.luma_r_gain[0] = adv_lsc->lsc1.seg2_param.luma_th[0].min;
			lsc_param->tune_param.pre_param.luma_r_gain[1] = adv_lsc->lsc1.seg2_param.luma_th[0].max;
			lsc_param->tune_param.pre_param.luma_g_gain[0] = adv_lsc->lsc1.seg2_param.luma_th[1].min;
			lsc_param->tune_param.pre_param.luma_g_gain[1] = adv_lsc->lsc1.seg2_param.luma_th[1].max;
			lsc_param->tune_param.pre_param.luma_b_gain[0] = adv_lsc->lsc1.seg2_param.luma_th[2].min;
			lsc_param->tune_param.pre_param.luma_b_gain[1] = adv_lsc->lsc1.seg2_param.luma_th[2].max;
			lsc_param->tune_param.pre_param.percent = adv_lsc->lsc1.gain_percent;
			lsc_param->tune_param.pre_param.gain_value[0] = adv_lsc->lsc1.new_gain.min;
			lsc_param->tune_param.pre_param.gain_value[1] = adv_lsc->lsc1.new_gain.max;
			lsc_param->tune_param.pre_param.last_gain_value[0] = adv_lsc->lsc1.last_gain.min;
			lsc_param->tune_param.pre_param.last_gain_value[1] = adv_lsc->lsc1.last_gain.max;
			lsc_param->tune_param.pre2_param.correction_intensity = adv_lsc->lsc2.intensity;
			lsc_param->tune_param.pre2_param.seg_info.center_r = adv_lsc->lsc2.seg1_param.center_r;
			lsc_param->tune_param.pre2_param.seg_info.luma_thr = adv_lsc->lsc2.seg1_param.luma_thr;
			lsc_param->tune_param.pre2_param.seg_info.luma_gain = adv_lsc->lsc2.seg1_param.luma_gain;
			lsc_param->tune_param.pre2_param.seg_info.chroma_thr = adv_lsc->lsc2.seg1_param.chroma_thr;
			lsc_param->tune_param.pre2_param.seg_info.chroma_gain = adv_lsc->lsc2.seg1_param.chroma_gain;
			lsc_param->tune_param.pre2_param.luma_r_gain[0] = adv_lsc->lsc2.seg2_param.luma_th[0].min;
			lsc_param->tune_param.pre2_param.luma_r_gain[1] = adv_lsc->lsc2.seg2_param.luma_th[0].max;
			lsc_param->tune_param.pre2_param.luma_g_gain[0] = adv_lsc->lsc2.seg2_param.luma_th[1].min;
			lsc_param->tune_param.pre2_param.luma_g_gain[1] = adv_lsc->lsc2.seg2_param.luma_th[1].max;
			lsc_param->tune_param.pre2_param.luma_b_gain[0] = adv_lsc->lsc2.seg2_param.luma_th[2].min;
			lsc_param->tune_param.pre2_param.luma_b_gain[1] = adv_lsc->lsc2.seg2_param.luma_th[2].max;
			lsc_param->tune_param.pre2_param.seg_ratio = adv_lsc->lsc2.seg_ratio;
			lsc_param->tune_param.pre2_param.gain_percent = adv_lsc->lsc2.gain_percent;
			lsc_param->tune_param.pre2_param.gain_value[0] = adv_lsc->lsc2.new_gain.min;
			lsc_param->tune_param.pre2_param.gain_value[1] = adv_lsc->lsc2.new_gain.max;
			lsc_param->tune_param.pre2_param.last_gain_value[0] = adv_lsc->lsc2.last_gain.min;
			lsc_param->tune_param.pre2_param.last_gain_value[1] = adv_lsc->lsc2.last_gain.max;
			lsc_param->tune_param.pre2_param.calc_cnt.num = adv_lsc->lsc2.calc_cnt.num;
			if (lsc_param->tune_param.pre2_param.calc_cnt.num <= SENSOR_PIECEWISE_SAMPLE_NUM) {
				for (i=0; i<lsc_param->tune_param.pre2_param.calc_cnt.num; i++) {
					lsc_param->tune_param.pre2_param.calc_cnt.point_value[i].x = (uint16_t)adv_lsc->lsc2.calc_cnt.samples[i].x;
					lsc_param->tune_param.pre2_param.calc_cnt.point_value[i].y = (uint16_t)adv_lsc->lsc2.calc_cnt.samples[i].y;
				}
			}
		}
	}
}
int32_t isp_smart_lsc_init(uint32_t handler_id)
{
	uint32_t rtn = ISP_SUCCESS;

#ifdef ISP_ADV_LSC_ENABLE

	lsc_adv_handle_t lsc_adv_handle = NULL;
	struct lsc_adv_init_param lsc_param = {0};
	struct isp_context* isp_cxt_ptr=ispGetAlgContext(handler_id);

	if (NULL == isp_cxt_ptr) {
		rtn = ISP_ERROR;
		ISP_LOGE("handle is NULL\n");
		return rtn;
	}

	switch (isp_cxt_ptr->cfg.data.format_pattern) {
	case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GRBG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_R:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_RGGB;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_B:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_BGGR;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GBRG;
		break;

	default:
		return ISP_ERROR;
	}

	isp_smart_lsc_set_param(&lsc_param, &isp_cxt_ptr->adv_lsc);

	if (NULL == isp_cxt_ptr->handle_lsc_adv) {
		lsc_adv_handle = lsc_adv_init(&lsc_param, NULL);
		if (NULL == lsc_adv_handle) {
		ISP_LOGE("lsc adv init failed");
		return ISP_ERROR;
		}
		isp_cxt_ptr->handle_lsc_adv = lsc_adv_handle;
		ISP_LOGI("LSC_ADV: handle=%p, gain pattern=%d", lsc_adv_handle, lsc_param.gain_pattern);
	}
#endif

	return rtn;
}

int32_t isp_smart_lsc_calc(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;

#ifdef ISP_ADV_LSC_ENABLE
	struct isp_context* isp_cxt_ptr=ispGetAlgContext(handler_id);
	lsc_adv_handle_t lsc_adv_handle = NULL;
	struct lsc_adv_pre_calc_param pre_calc_param = {{NULL}, {0},{0}};
	struct lsc_adv_pre_calc_result pre_calc_result = {0};
	struct isp_awb_statistic_info *stat_info = {NULL};
	struct isp_awb_param *awb_param_ptr = NULL;
	uint32_t i = 0;

	if (NULL == isp_cxt_ptr) {
		rtn = ISP_ERROR;
		ISP_LOGE("handle is NULL\n");
		return rtn;
	}
	if (0 == isp_cxt_ptr->adv_lsc.adv_lsc.enable)
		return rtn;

	lsc_adv_handle = (lsc_adv_handle_t)isp_cxt_ptr->handle_lsc_adv;

	if (NULL == lsc_adv_handle) {
		ISP_LOGE("lsc handle is NULL\n");
		return ISP_ERROR;
	}

	stat_info = &isp_cxt_ptr->awb_stat;
	awb_param_ptr = &isp_cxt_ptr->awb;

	pre_calc_param.stat_img.r = stat_info->r_info;
	pre_calc_param.stat_img.g = stat_info->g_info;
	pre_calc_param.stat_img.b = stat_info->b_info;
	pre_calc_param.stat_size.w = awb_param_ptr->stat_img_size.w;
	pre_calc_param.stat_size.h = awb_param_ptr->stat_img_size.h;
	pre_calc_param.block_size.w = awb_param_ptr->win_size.w;
	pre_calc_param.block_size.h = awb_param_ptr->win_size.h;
	rtn = lsc_adv_pre_calc(lsc_adv_handle, &pre_calc_param, &pre_calc_result);

	if ((ISP_SUCCESS == rtn\
		&& 1 == pre_calc_result.update)) {
		struct isp_lnc_param *lsc_ptr = NULL;
		struct lsc_adv_calc_param calc_param;
		struct lsc_adv_calc_result calc_result = {0};
		struct isp_lnc_map *lsc_tab = NULL;
		uint32_t size_index = 0;
		uint32_t width = isp_cxt_ptr->src.w;
		uint32_t height = isp_cxt_ptr->src.h;
		uint32_t lnc_grid = 0;
		uint32_t gain_w = 0;
		uint32_t gain_h = 0;
		struct isp_lnc_map *lnc_tab = NULL;
		struct isp_awb_adjust *cur_lnc = NULL;

		size_index = isp_cxt_ptr->param_index - ISP_ONE;
		lnc_tab = isp_cxt_ptr->lnc_map_tab[size_index];
		cur_lnc = &isp_cxt_ptr->lnc.cur_lnc;

		lnc_grid = lnc_tab[cur_lnc->index0].grid_pitch;
		gain_w = _ispGetLensGridPitch(width, lnc_grid);
		gain_h = _ispGetLensGridPitch(height, lnc_grid);

		ISP_LOGI("gain w=%d, gain h=%d, grid=%d, index=%d", gain_w, gain_h, lnc_grid, cur_lnc->index0);
	
		memset(&calc_param, 0, sizeof(calc_param));

		lsc_ptr = &isp_cxt_ptr->lnc;
		lsc_tab = &isp_cxt_ptr->lnc_map_tab[0][0];
		lnc_grid = lsc_tab[lsc_ptr->cur_lnc.index0].grid_pitch;
		calc_param.stat_size.w = awb_param_ptr->stat_img_size.w;
		calc_param.stat_size.h = awb_param_ptr->stat_img_size.h;
		calc_param.lnc_size.w =gain_w;
		calc_param.lnc_size.h = gain_h;
		calc_param.fix_gain = (uint16_t *)lsc_ptr->lnc_ptr;
		calc_param.ct = awb_param_ptr->cur_ct;
		calc_param.cur_index.value[0] = lsc_ptr->cur_lnc.index0;
		calc_param.cur_index.value[1] = lsc_ptr->cur_lnc.index1;
		calc_param.cur_index.weight[0] = (1024 - lsc_ptr->cur_lnc.alpha)/4;
		calc_param.cur_index.weight[1] = 256 - calc_param.cur_index.weight[0];
		calc_result.dst_gain = (uint16_t *)lsc_ptr->lnc_ptr;

		rtn = lsc_adv_calculation(lsc_adv_handle, FUNC_UPDATE_MODE, &calc_param, &calc_result);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("lsc adv gain map calc error");
			return rtn;
		}

		//memcpy((void*)&isp_cxt_ptr->smart_lsc_log_info, (void)&calc_result.log_info, sizeof(isp_cxt_ptr->smart_lsc_log_info));
		isp_cxt_ptr->smart_lsc_log_info = calc_result.log_info;

		ISP_LOG("lnc_ptr = 0x%x, lnc_len=%d", (uint32_t)lsc_ptr->lnc_ptr, lsc_ptr->lnc_len);
		rtn = ispSetLncParam(handler_id, (uint32_t)lsc_ptr->lnc_ptr, lsc_ptr->lnc_len);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ispSetLncParam failed = %d", rtn);
			return ISP_ERROR;
		}

		rtn = isp_change_param(handler_id, ISP_CHANGE_LNC_RELOAD, NULL);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("isp_change_param failed = %d", rtn);
			return ISP_ERROR;
		}
	}

	ISP_LOGI("ISP_LSC_ADV: handle=%p, rtn=%d", lsc_adv_handle, rtn);
#endif

	return rtn;
}

int32_t isp_smart_lsc_reload(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;

#ifdef ISP_ADV_LSC_ENABLE
	struct isp_context* isp_cxt_ptr = ispGetAlgContext(handler_id);
	struct isp_awb_param *awb_param = &isp_cxt_ptr->awb;
	uint32_t size_index = ISP_ZERO;
	struct isp_lnc_map *lnc_tab = NULL;
	struct isp_lnc_param *lnc_param = NULL;
	struct isp_awb_adjust *cur_lnc = NULL;
	uint32_t lnc_len = 0;
	uint32_t alpha = 0;
	uint32_t dec_ratio = 0;
	uint16_t lnc_grid = 0;

	uint32_t gain_w = 0;
	uint32_t gain_h = 0;
	struct lsc_adv_calc_param lsc_adv_param;
	struct lsc_adv_calc_result lsc_adv_result = {0};
	lsc_adv_handle_t handle = isp_cxt_ptr->handle_lsc_adv;
	uint32_t cur_ct = awb_param->cur_ct;
	uint32_t width = isp_cxt_ptr->src.w;
	uint32_t height = isp_cxt_ptr->src.h;

	if (0 == isp_cxt_ptr->adv_lsc.adv_lsc.enable)
		return rtn;
	if (NULL == handle) {
		ISP_LOGE("lsc handle is NULL\n");
		return ISP_ERROR;
	}

	memset(&lsc_adv_param, 0, sizeof(lsc_adv_param));
	lnc_param = &isp_cxt_ptr->lnc;
	size_index = isp_cxt_ptr->param_index - ISP_ONE;
	lnc_tab = isp_cxt_ptr->lnc_map_tab[size_index];
	cur_lnc = &lnc_param->cur_lnc;
	lnc_grid = lnc_tab[cur_lnc->index0].grid_pitch;

	gain_w = _ispGetLensGridPitch(width, lnc_grid);
	gain_h = _ispGetLensGridPitch(height, lnc_grid);

	ISP_LOGI("img_size=(%d,%d), grid=%d, gain_size=(%d,%d)", width, height, lnc_grid, gain_w, gain_h);

	lsc_adv_param.ct = cur_ct;
	lsc_adv_param.cur_index.value[0] = cur_lnc->index0;
	lsc_adv_param.cur_index.value[1] = cur_lnc->index1;
	lsc_adv_param.cur_index.weight[0] = (1024 - alpha) / 4;
	lsc_adv_param.cur_index.weight[1] = 256 - lsc_adv_param.cur_index.weight[0];
	lsc_adv_param.stat_size.w = awb_param->stat_img_size.w;
	lsc_adv_param.stat_size.h = awb_param->stat_img_size.h;
	lsc_adv_param.lnc_size.w = gain_w;
	lsc_adv_param.lnc_size.h = gain_h;

	lsc_adv_param.fix_gain = (uint16_t *)lnc_param->lnc_ptr;
	lsc_adv_result.dst_gain = (uint16_t *)lnc_param->lnc_ptr;

	ISP_LOGI("lsc adv calc: handle= 0x%x", (uint32_t)handle);
	rtn = lsc_adv_calculation(handle, ENVI_UPDATE_MODE, &lsc_adv_param, &lsc_adv_result);
	ISP_LOGI("lsc adv rtn = %d", rtn);
	//memcpy((void*)&isp_cxt_ptr->smart_lsc_log_info, (void)&lsc_adv_result.log_info, sizeof(isp_cxt_ptr->smart_lsc_log_info));
	isp_cxt_ptr->smart_lsc_log_info = lsc_adv_result.log_info;
#endif

	return rtn;
}

int32_t isp_smart_lsc_deinit(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;

#ifdef ISP_ADV_LSC_ENABLE
	struct isp_context* isp_cxt_ptr=ispGetAlgContext(handler_id);
	lsc_adv_handle_t lsc_adv_handle = NULL;

	if(NULL == isp_cxt_ptr) {
		rtn = ISP_ERROR;
		ISP_LOGE("handle is NULL\n");
		return rtn;
	}

	lsc_adv_handle = isp_cxt_ptr->handle_lsc_adv;
	if (NULL == lsc_adv_handle) {
		ISP_LOGE("smart lsc handle is NULL\n");
		rtn = ISP_ERROR;
		return rtn;
	}
	rtn = lsc_adv_deinit(lsc_adv_handle, NULL, NULL);
	isp_cxt_ptr->handle_lsc_adv = NULL;

	ISP_LOGI("ISP_LSC_ADV: handle=%p, rtn=%d", lsc_adv_handle, rtn);
#endif

	return rtn;
}

