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

#include "isp_com.h"
#include "isp_ae_alg_v00.h"
#include "aaa_log.h"

/**---------------------------------------------------------------------------*
 ** 				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/

#define AE_LUM_LOW_THREAD 20
#define AE_LUM_HIGH_THREAD 230
#define AE_LUM_ZONE 30

#define AE_ADJUST_MAX_STEP 50

/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/
enum ae_run_mode {
	AE_RUN_UP = 0,
	AE_RUN_DOWN,
	AE_RUN_MAX,
};

struct ae_index_calc
{
	int32_t cur_lum;
	uint32_t target_lum;
	uint32_t cur_index;

	uint32_t* e_ptr;
	uint16_t* g_ptr;
	int32_t max_index;
	int32_t min_index;
	int32_t(*real_gain)(uint32_t gain);
};

/**---------------------------------------------------------------------------*
**				extend Variables and function			*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Local Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Constant Variables					*
**---------------------------------------------------------------------------*/
static struct isp_ae_v00_context* s_ae_v00_context_tab[2]={0x00};

/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/
/* ispGetAeV00Context --
*@
*@
*@ return:
*/
int32_t ispSetAeV00Context(uint32_t handler_id, struct isp_ae_v00_context* context_ptr)
{
	int32_t rtn=ISP_SUCCESS;

	s_ae_v00_context_tab[handler_id] = context_ptr;

	return rtn;
}

/* ispGetAeV00Context --
*@
*@
*@ return:
*/
struct isp_ae_v00_context* ispGetAeV00Context(uint32_t handler_id)
{
	return s_ae_v00_context_tab[handler_id];
}

uint32_t isp_get_exposure(uint32_t handler_id, uint32_t* exposure)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	if((ISP_ZERO!=(*exposure))
		&&(ISP_ZERO!=ae_context_ptr->line_time)) {
		*exposure=(100000000/(*exposure))/ae_context_ptr->line_time;
	} else {
		ISP_LOG("exposure:%d, line_tme:%d cur_index:%d, max_index:%d error",(*exposure), ae_context_ptr->line_time, ae_context_ptr->cur_index, ae_context_ptr->max_index);
	}

//	ISP_LOG("----------line_tme:%d",ae_param_ptr->line_time);

	return rtn;
}

uint32_t isp_get_dummy(uint32_t handler_id, uint32_t exposure)
{
	uint32_t cur_dummy = 0;
	struct isp_ae_v00_context* ae_context_ptr = ispGetAeV00Context(handler_id);

	if (ISP_AE_MAX_LINE != ae_context_ptr->frame_line) {
		if (ae_context_ptr->frame_line < exposure) {
			exposure = ae_context_ptr->frame_line;
		}
		cur_dummy = ae_context_ptr->frame_line-exposure;
	} else {
		cur_dummy = 0;
	}

	ISP_LOG("min_index=%d,max_index=%d", ae_context_ptr->min_index, ae_context_ptr->max_index);

	if (ISP_AE_MAX_LINE != ae_context_ptr->min_frame_line) {
		ISP_LOG("exp_line = %d, min frame line = %d", exposure, ae_context_ptr->min_frame_line);
		if (exposure < ae_context_ptr->min_frame_line) {
			cur_dummy = ae_context_ptr->min_frame_line - exposure;
		}
		ISP_LOG("cur_dummy=%d", ae_context_ptr->cur_dummy);
	}

	return cur_dummy;
}

/* _isp_ae_calc_exposure_gain --
*@
*@
*@ return:
*/
uint32_t isp_ae_calc_exposure_gain(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);
	int32_t cur_index=ae_context_ptr->cur_index;
	uint32_t exposure=0x00;

	ae_context_ptr->cur_dummy=ISP_ZERO;
	ae_context_ptr->cur_gain=ae_context_ptr->g_ptr[cur_index];
	exposure=ae_context_ptr->e_ptr[cur_index];

	if(0x00!=exposure) {

		isp_get_exposure(handler_id, &exposure);

		if(ISP_AE_MAX_LINE!=ae_context_ptr->frame_line) {
			if(ae_context_ptr->frame_line<exposure) {
				exposure=ae_context_ptr->frame_line;
			}
			ae_context_ptr->cur_dummy=ae_context_ptr->frame_line-exposure;
		} else {
			ae_context_ptr->cur_dummy=ISP_ZERO;
		}

		ISP_LOG("cur_index=%d,min_index=%d,max_index=%d", cur_index, ae_context_ptr->min_index,
				ae_context_ptr->max_index);

		if (ISP_AE_MAX_LINE != ae_context_ptr->min_frame_line) {
			ISP_LOG("exp_line=%d, min frame line=%d", exposure, ae_context_ptr->min_frame_line);
			if (exposure < ae_context_ptr->min_frame_line) {
				ae_context_ptr->cur_dummy = ae_context_ptr->min_frame_line - exposure;
			}
			ISP_LOG("cur_dummy=%d", ae_context_ptr->cur_dummy);
		}
		if (ISP_AE_MAX_LINE != ae_context_ptr->max_frame_line) {
			if (cur_index > ae_context_ptr->max_index) {
				cur_index = ae_context_ptr->max_index;
				exposure = ae_context_ptr->e_ptr[cur_index];
				isp_get_exposure(handler_id, &exposure);
			}
			ISP_LOG("exp_line=%d, max_frame_line=%d", exposure, ae_context_ptr->max_frame_line);
		}
		ae_context_ptr->cur_exposure = exposure;
		ISP_LOG("index:%d, exp:%d, gain:%d, index:%d\n", cur_index, exposure, ae_context_ptr->cur_gain, ae_context_ptr->cur_index);
	} else {
		ISP_LOG("index:%d, exp:%d, gain:%d, index:%d, error\n", cur_index, exposure, ae_context_ptr->cur_gain, ae_context_ptr->cur_index);
	}

	return rtn;
}

/* _isp_ae_fast_calculation--
*@
*@
*@ return:
*/
uint32_t isp_ae_fast_calculation(uint32_t handler_id, uint32_t cur_lum, uint32_t target_lum, uint32_t wDeadZone, int32_t* index)
{
	uint32_t rtn = ISP_ERROR;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);
	uint32_t max_index=ae_context_ptr->max_index;
	uint32_t min_index=ae_context_ptr->min_index;
	uint32_t cur_index=*index;
	uint32_t target_lum_low_thr=target_lum-wDeadZone;
	uint32_t target_lum_high_thr=target_lum+wDeadZone;
	uint32_t wYlayer=cur_lum;
	uint32_t calc_lum=0x00;
	uint32_t cur_gain=0x00;
	uint32_t cur_exposure=0x00;
	uint32_t target_gain=0x00;
	uint32_t target_exposure=0x00;
	uint32_t cur_accumulate=0x00;
	uint32_t target_accumulate=0x00;
	uint32_t i=0x00;

	if(0x0a>wYlayer) {
		wYlayer=0x0a;
	}

	cur_index=cur_index;
	max_index=max_index;
	min_index=min_index;
	cur_exposure=ae_context_ptr->e_ptr[cur_index];
	cur_gain=ae_context_ptr->g_ptr[cur_index];
	cur_gain=ae_context_ptr->real_gain(cur_gain);

	if(wYlayer>AE_LUM_HIGH_THREAD) {
	/*>AE_LUM_HIGH_THREAD*/
		if(min_index<cur_index) {
			target_lum_high_thr=AE_LUM_LOW_THREAD+AE_LUM_ZONE+wDeadZone;
			do {
				cur_index--;
				target_exposure=ae_context_ptr->e_ptr[cur_index];
				target_gain=ae_context_ptr->g_ptr[cur_index];
				target_gain=ae_context_ptr->real_gain(target_gain);

				calc_lum=(wYlayer*((target_gain*cur_exposure)/target_exposure))/cur_gain;

				if((target_lum_high_thr>calc_lum)
					||(min_index==cur_index)) {
					if(min_index==cur_index) {
						ae_context_ptr->alg_mode=ISP_ALG_NORMAL;
					}
					break;
				}
			}while(1);
		} else {
			ae_context_ptr->alg_mode=ISP_ALG_NORMAL;
		}

	} else if(wYlayer>AE_LUM_LOW_THREAD) {
	/*AE_LUM_LOW_THREAD< >AE_LUM_HIGH_THREAD*/
		target_lum_low_thr=target_lum-wDeadZone;
		target_lum_high_thr=target_lum+wDeadZone;
		ae_context_ptr->alg_mode=ISP_ALG_NORMAL;

		if(target_lum_high_thr<wYlayer) {
			if(min_index<cur_index) {
				do {
					cur_index--;
					target_exposure=ae_context_ptr->e_ptr[cur_index];
					target_gain=ae_context_ptr->g_ptr[cur_index];
					target_gain=ae_context_ptr->real_gain(target_gain);

					calc_lum=(wYlayer*((target_gain*cur_exposure)/target_exposure))/cur_gain;

					if((target_lum_high_thr>calc_lum)
						||(min_index==cur_index)) {
						break;
					}
				}while(1);
			}
		} else if(target_lum_low_thr>wYlayer) {
			if(max_index>cur_index) {
				do {
					cur_index++;
					target_exposure=ae_context_ptr->e_ptr[cur_index];
					target_gain=ae_context_ptr->g_ptr[cur_index];
					target_gain=ae_context_ptr->real_gain(target_gain);

					calc_lum=(wYlayer*((target_gain*cur_exposure)/target_exposure))/cur_gain;

					if((target_lum_low_thr<calc_lum)
						||(max_index==cur_index)) {
						break;
					}
				}while(1);
			}
		}

	} else {
	/*>AE_LUM_LOW_THREAD*/

		if(max_index>cur_index) {
			target_lum_low_thr=AE_LUM_HIGH_THREAD-AE_LUM_ZONE-wDeadZone;

			target_lum_low_thr*=10;
			wYlayer*=10;

			do {
				cur_index++;
				target_exposure=ae_context_ptr->e_ptr[cur_index];
				target_gain=ae_context_ptr->g_ptr[cur_index];
				target_gain=ae_context_ptr->real_gain(target_gain);

				calc_lum=(wYlayer*((target_gain*cur_exposure)/target_exposure))/cur_gain;

				if((target_lum_low_thr<calc_lum)
					||(max_index==cur_index)) {
					if(max_index==cur_index) {
						ae_context_ptr->alg_mode=ISP_ALG_NORMAL;
					}
					break;
				}
			}while(1);

		} else {
			ae_context_ptr->alg_mode=ISP_ALG_NORMAL;
		}
	}

	cur_index=cur_index;
	ISP_LOG("hait: cur_indx: %d, lum: %d ae_context_ptr->alg_mode %d \n", cur_index, wYlayer, ae_context_ptr->alg_mode);

	if(ISP_ALG_NORMAL==ae_context_ptr->alg_mode) {
		ae_context_ptr->flash.cur_index=cur_index;
		rtn=ISP_SUCCESS;
	}

	*index=cur_index;

	return rtn;
}

/* _isp_ae_calc_inidex--
*@
*@
*@ return:
*/
uint32_t isp_ae_calc_inidex( struct ae_index_calc* calc_param_ptr)
{
	uint32_t handler_id=0x00;
	struct ae_index_calc calc_param;
	struct ae_index_calc* param_ptr = &calc_param;
	uint32_t cur_lum = ISP_ZERO;
	uint32_t calc_index = ISP_ZERO;
	uint32_t calc_lum =ISP_ZERO;
	uint32_t cur_gain = ISP_ZERO;
	uint64_t cur_exposure = ISP_ZERO;
	uint64_t target_gain = ISP_ZERO;
	uint64_t target_exposure = ISP_ZERO;
	uint32_t target_lum = ISP_ZERO;
	uint32_t max_index = ISP_ZERO;
	uint32_t min_index = ISP_ZERO;
	uint32_t max_adjst_step = 100;
	uint32_t adjst_step = ISP_ZERO;

	if (NULL != calc_param_ptr) {
		memcpy((void*)param_ptr, (void*)calc_param_ptr, sizeof(struct ae_index_calc));

		calc_index = param_ptr->cur_index;
		max_index = param_ptr->max_index;
		min_index = param_ptr->min_index;

		if ((max_index < calc_index)
			||(min_index > calc_index)) {
			goto EXIT;
		}

		cur_lum = param_ptr->cur_lum;
		target_lum = param_ptr->target_lum;
		cur_exposure = param_ptr->e_ptr[calc_index];
		cur_gain = param_ptr->g_ptr[calc_index];
		cur_gain = param_ptr->real_gain(cur_gain);

		if ((target_lum > cur_lum) 
			&& (max_index > calc_index)) {

			do {
				adjst_step++;
				calc_index++;
				target_exposure=param_ptr->e_ptr[calc_index];
				target_gain=param_ptr->g_ptr[calc_index];
				target_gain=param_ptr->real_gain(target_gain);
				calc_lum=(cur_lum*((target_gain*cur_exposure)/target_exposure))/cur_gain;

				if((target_lum <= calc_lum)
					||(max_index==calc_index)
					||(max_adjst_step < adjst_step)) {
					break;
				}
			}while(1);

		} else if ((target_lum < cur_lum)
			&& (min_index < calc_index)) {

			do {
				adjst_step++;
				calc_index--;
				target_exposure=param_ptr->e_ptr[calc_index];
				target_gain=param_ptr->g_ptr[calc_index];
				target_gain=param_ptr->real_gain(target_gain);
				calc_lum=(cur_lum*((target_gain*cur_exposure)/target_exposure))/cur_gain;

				if((target_lum >= calc_lum)
					||(min_index==calc_index)
					||(max_adjst_step < adjst_step)) {
					break;
				}
			}while(1);
		}
	}

	EXIT:

	return calc_index;
}

/* _isp_ae_get_rough_thr--
*@ mode: 0-up 1-down
*@ ae step 1/16
*@ return:
*/
int32_t isp_ae_get_rough_thr(enum ae_run_mode mode, int32_t lum_thr, uint32_t strong)
{
	int32_t calc_thr = lum_thr * 10;
	uint32_t i = ISP_ZERO;

	if (ISP_ZERO != strong) {
		if (AE_RUN_UP == mode) {
			for (i = ISP_ZERO; i < strong; i++) {
				calc_thr += (calc_thr>>0x04);
			}
		} else if (AE_RUN_DOWN == mode) {
			for (i = ISP_ZERO; i < strong; i++) {
				calc_thr = (calc_thr<<0x04)/17;
			}
		}
	}

	return calc_thr;
}

/* isp_ae_get_max_adjust--
*@
*@
*@ return:
*/
int32_t isp_ae_get_max_adjust(int32_t back_index, int32_t cur_index)
{
	int32_t calc_index = cur_index;

	if ( AE_ADJUST_MAX_STEP < (abs(back_index - cur_index))) {

		if (back_index < cur_index) {
			calc_index = back_index + AE_ADJUST_MAX_STEP;
		} else {
			calc_index = back_index - AE_ADJUST_MAX_STEP;
		}
	}

	return calc_index;
}

/* _isp_ae_succesive_calculation--
*@
*@
*@ return:
*/
uint32_t isp_ae_succesive_calculation(uint32_t handler_id, uint32_t cur_lum, uint32_t target_lum, uint32_t wDeadZone, int32_t* index)
{
	uint32_t rtn = ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);
	struct ae_index_calc index_calc_param;
	int32_t cur_index=*index;
	int32_t wYlayer=cur_lum;
	uint32_t strong = 2;
	int32_t calc_index=0x00;
	int32_t smooth_high_thr=target_lum+wDeadZone;
	int32_t smooth_low_thr=target_lum-wDeadZone;
	int32_t rough_high_thr=ISP_ZERO;
	int32_t rough_low_thr=ISP_ZERO;


	index_calc_param.cur_lum = cur_lum;
	index_calc_param.cur_index = cur_index;
	index_calc_param.e_ptr = ae_context_ptr->e_ptr;
	index_calc_param.g_ptr = ae_context_ptr->g_ptr;
	index_calc_param.max_index = ae_context_ptr->max_index;
	index_calc_param.min_index = ae_context_ptr->min_index;
	index_calc_param.real_gain = ae_context_ptr->real_gain;

	rough_high_thr = isp_ae_get_rough_thr(AE_RUN_UP, smooth_high_thr, strong);
	rough_low_thr = isp_ae_get_rough_thr(AE_RUN_DOWN, smooth_low_thr, strong);
	smooth_high_thr *= 10;
	smooth_low_thr *= 10;
	wYlayer *= 10;

	index_calc_param.cur_lum *= 10; 

	if(wYlayer > rough_high_thr) {
		index_calc_param.target_lum = smooth_high_thr;
		cur_index = isp_ae_calc_inidex(&index_calc_param);
	} else if (wYlayer > smooth_high_thr) {
		cur_index -= 0x01;
	} else if(wYlayer > smooth_low_thr) {
		cur_index += 0x00;
	} else if (wYlayer >= rough_low_thr) {
		cur_index += 0x01;
	} else {
		index_calc_param.target_lum = rough_low_thr;
		cur_index = isp_ae_calc_inidex(&index_calc_param);
	}

	if(cur_index > ae_context_ptr->max_index) {
		cur_index = ae_context_ptr->max_index;
	}

	if(cur_index < ae_context_ptr->min_index) {
		cur_index=ae_context_ptr->min_index;
	}

	*index=cur_index;

	return rtn;
}

/* isp_ae_succesive_fast--
*@
*@
*@ return:
*/
uint32_t isp_ae_succesive_fast(uint32_t handler_id, uint32_t cur_lum, uint32_t target_lum, uint32_t wDeadZone, int32_t* index)
{
	uint32_t rtn = ISP_ERROR;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);
	struct ae_index_calc index_calc_param;
	int32_t back_index=*index;
	int32_t cur_index=*index;
	int32_t wYlayer=cur_lum;
	int32_t calc_index=0x00;
	int32_t smooth_high_thr=target_lum+wDeadZone;
	int32_t smooth_low_thr=target_lum-wDeadZone;
	int32_t rough_high_thr=ISP_ZERO;
	int32_t rough_low_thr=ISP_ZERO;

	index_calc_param.cur_lum = cur_lum;
	index_calc_param.cur_index = cur_index;
	index_calc_param.e_ptr = ae_context_ptr->e_ptr;
	index_calc_param.g_ptr = ae_context_ptr->g_ptr;
	index_calc_param.max_index = ae_context_ptr->max_index;
	index_calc_param.min_index = ae_context_ptr->min_index;
	index_calc_param.real_gain = ae_context_ptr->real_gain;

	smooth_high_thr *= 10;
	smooth_low_thr *= 10;
	wYlayer *= 10;
	index_calc_param.cur_lum *= 10; 

	if (wYlayer > smooth_high_thr) {
		index_calc_param.target_lum = smooth_low_thr;
		cur_index = isp_ae_calc_inidex(&index_calc_param);
	} else if(wYlayer > smooth_low_thr) {
		cur_index += 0x00;
		ae_context_ptr->flash.cur_index=cur_index;
		ae_context_ptr->alg_mode = ISP_ALG_NORMAL;
		rtn = ISP_SUCCESS;
	} else {
		index_calc_param.target_lum = smooth_low_thr;
		cur_index = isp_ae_calc_inidex(&index_calc_param);
	}

	cur_index = isp_ae_get_max_adjust(back_index, cur_index);

	if(cur_index > ae_context_ptr->max_index) {
		cur_index = ae_context_ptr->max_index;
	}

	if(cur_index < ae_context_ptr->min_index) {
		cur_index=ae_context_ptr->min_index;
	}

	if((cur_index == ae_context_ptr->min_index)
		|| (cur_index == ae_context_ptr->max_index)) {
		ae_context_ptr->flash.cur_index=cur_index;
		ae_context_ptr->alg_mode = ISP_ALG_NORMAL;
		rtn = ISP_SUCCESS;
	}

	*index=cur_index;

	return rtn;
}


/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/


