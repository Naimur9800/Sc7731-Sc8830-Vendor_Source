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
#define LOG_TAG "isp_alg"

#include "isp_com.h"
#include "isp_alg.h"
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
#define ISP_SOFT_ID 0x20150304

#define ISP_ID_INVALID 0xff

#define SMART_DENOISE 1
#define SMART_FOLLOW_INDEX 1

#define raw_overlap_up 11
#define raw_overlap_down 10
#define raw_overlap_left 14
#define raw_overlap_right 12

#define yuv_overlap_up 3
#define yuv_overlap_down 2
#define yuv_overlap_left 6
#define yuv_overlap_right 4

// RGB to YUV
//Y = 0.299 * r + 0.587 * g + 0.114 * b
#define RGB_TO_Y(_r, _g, _b)	(int32_t)((77 * (_r) + 150 * (_g) + 29 * (_b)) >> 8)
//U = 128 - 0.1687 * r - 0.3313 * g + 0.5 * b
#define RGB_TO_U(_r, _g, _b)	(int32_t)(128 + ((128 * (_b) - 43 * (_r) - 85 * (_g)) >> 8))
//V = 128 + 0.5 * r - 0.4187 * g - 0.0813 * b
#define RGB_TO_V(_r, _g, _b)	(int32_t)(128  + ((128 * (_r) - 107 * (_g) - 21 * (_b)) >> 8))

#define RGB_TO_YUV_0 0x012B
#define RGB_TO_YUV_1 0x024B
#define RGB_TO_YUV_2 0x0072
#define RGB_TO_YUV_3 (0-0x00A8)
#define RGB_TO_YUV_4 (0-0x014B)
#define RGB_TO_YUV_5 0x01F4
#define RGB_TO_YUV_6 0x01F4
#define RGB_TO_YUV_7 (0-0x01A3)
#define RGB_TO_YUV_8 (0-0x0051)

#define YUV_TO_RGB_0 0x03E8
#define YUV_TO_RGB_1 0x0000
#define YUV_TO_RGB_2 0x057A
#define YUV_TO_RGB_3 0x03E8
#define YUV_TO_RGB_4 (0-0x0158)
#define YUV_TO_RGB_5 (0-0x02CA)
#define YUV_TO_RGB_6 0x03E8
#define YUV_TO_RGB_7 0x0499
#define YUV_TO_RGB_8 0x0000

/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				extend Variables and function			*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Local Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Constant Variables					*
**---------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
**					Constant Variables				*
**---------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/
/* _isp_ae_get_flash_effect--
*@
*@
*@ return:
*/
/*
uint32_t _isp_ae_get_flash_effect(uint32_t handler_id, uint32_t ratio, uint32_t targe_lum,
										uint32_t bf_exp, uint32_t bf_gain, uint32_t bf_lum,
										uint32_t pref_exp, uint32_t pref_gain, uint32_t pref_lum,
										uint32_t* effect)
{
	uint32_t bef_flash_ex = bf_exp;
	uint32_t bef_flash_gain = bf_gain;
	uint32_t bef_flash_lum = bf_lum;
	uint32_t pre_flash_ex = pref_exp;
	uint32_t pre_flash_gain = pref_gain;
	uint32_t pre_flash_lum = pref_lum;
	uint64_t mflash = 0;
	uint64_t evflash = 0;
	uint32_t k = 0;

	if (NULL == effect) {
		ISP_LOG("effect %p param is error!", effect);
		return ISP_ERROR;
	}
	*effect  = 0;

	if (!bef_flash_lum) {
		*effect = 1024;
		goto exit;
	}

	if (((bef_flash_lum <= (targe_lum+3)) && (bef_flash_lum >= (targe_lum-3)))
		&& ((pre_flash_lum <= (targe_lum+3)) && (pre_flash_lum >= (targe_lum-3))))  {
		ISP_LOG("target lum same as cur_lum");
		mflash = (uint64_t)bef_flash_gain * (uint64_t)bef_flash_ex;
		evflash = (uint64_t)pre_flash_gain * (uint64_t)pre_flash_ex;
		//ISP_LOG("hait: mflash: %d, ")
	}else{
		mflash = (uint64_t)pre_flash_lum * (uint64_t)bef_flash_ex * (uint64_t)bef_flash_gain;
		evflash = (uint64_t)bef_flash_lum * (uint64_t)pre_flash_ex * (uint64_t)pre_flash_gain;
	}

	k = (uint32_t)((mflash<<8)/(evflash));
	*effect = ((6*k-(6<<8)) <<10)/(6*k-(5<<8));
	if (*effect > 1024) {
		*effect = 1024;
	}

exit:
	ISP_LOG("hait: effect=%d", *effect);
	return ISP_SUCCESS;
}
*/
#if 0

static uint32_t _isp_ae_get_flash_effect(uint32_t handler_id, struct isp_ae_v00_flash_alg_param* v00_flash_ptr, uint32_t *effect)
{
	uint32_t rtn = ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_flash_param* flash_param_ptr=&ae_param_ptr->flash;
	uint32_t target_lum=v00_flash_ptr->target_lum;
	uint32_t prv_index=v00_flash_ptr->prv_index;
	uint32_t cur_index=v00_flash_ptr->cur_index;
	uint32_t ratio = flash_param_ptr->ratio * flash_param_ptr->adjus_coef /256;
	uint32_t bef_flash_ex = ae_param_ptr->cur_e_ptr[prv_index];
	uint32_t bef_flash_gain = ae_param_ptr->cur_g_ptr[prv_index];
	bef_flash_gain = isp_ae_get_real_gain(bef_flash_gain);
	uint32_t bef_flash_lum = v00_flash_ptr->prv_lum;

	uint32_t pre_flash_ex = ae_param_ptr->cur_e_ptr[cur_index];
	uint32_t pre_flash_gain = ae_param_ptr->cur_g_ptr[cur_index];
	pre_flash_gain = isp_ae_get_real_gain(pre_flash_gain);
	uint32_t pre_flash_lum = v00_flash_ptr->cur_lum;
	uint64_t mflash = 0;
	uint64_t evflash = 0;
	uint32_t k = 0;

	if (NULL == effect) {
		ISP_LOG("effect %p param is error!", effect);
		return ISP_ERROR;
	}
	*effect  = 0;

	if (!bef_flash_lum) {
		*effect = 1024;
		goto exit;
	}

	if (((bef_flash_lum <= (target_lum+3)) && (bef_flash_lum >= (target_lum-3)))
		&& ((pre_flash_lum <= (target_lum+3)) && (pre_flash_lum >= (target_lum-3))))  {
		ISP_LOG("target lum same as cur_lum");
		mflash = (uint64_t)bef_flash_gain * (uint64_t)bef_flash_ex;
		evflash = (uint64_t)pre_flash_gain * (uint64_t)pre_flash_ex;
	}else{
		mflash = (uint64_t)pre_flash_lum * (uint64_t)bef_flash_ex * (uint64_t)bef_flash_gain;
		evflash = (uint64_t)bef_flash_lum * (uint64_t)pre_flash_ex * (uint64_t)pre_flash_gain;
	}

	k = (uint32_t)((mflash<<8)/(evflash));
	
	if (k>256) {
		*effect = (ratio * (k-(1<<8)))/((ratio * (k-(1<<8))+(1<<16))>>10);

	} else {
		*effect = k;
	}

	if (*effect > 1024) {
		*effect = 1024;
	}

exit:
	ISP_LOG("x *effect = %ld mflash = %lld, evflash = %lld", *effect, mflash, evflash);
	return rtn;
}

/* isp_flash_calculation--
*@
*@
*@ return:
*/
int32_t isp_flash_calculation(uint32_t handler_id, struct isp_ae_v00_flash_alg_param* v00_flash_ptr)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t alg_success=0;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_flash_param* flash_param_ptr=&ae_param_ptr->flash;
	int32_t target_lum_high_thr=v00_flash_ptr->target_lum+v00_flash_ptr->target_zone;
	int32_t prv_gain=0x00;
	int32_t prv_exposure=0x00;
	int32_t cur_gain=0x00;
	int32_t cur_exposure=0x00;
	int32_t calc_gain=0x00;
	int32_t calc_exposure=0x00;
	int32_t target_lum=v00_flash_ptr->target_lum;
	int32_t prv_lum=v00_flash_ptr->prv_lum;
	__s64 wYlayer=v00_flash_ptr->cur_lum;
	int32_t prv_index=v00_flash_ptr->prv_index;
	int32_t cur_index=v00_flash_ptr->cur_index;
	int32_t min_index=ae_param_ptr->min_index;
	int32_t max_index=ae_param_ptr->max_index;
	__s64 delta_lum=0x00;
	__s64 flash_lum=0x00;
	__s64 calc_lum=0x00;
	int32_t effect=ISP_ZERO;
	int32_t flash_stab=0x00;

	flash_param_ptr->effect=ISP_ZERO;
	flash_param_ptr->prv_index=v00_flash_ptr->prv_index;
	ISP_LOG("piano isp_flash_calculation cur %d max %d prv %d", cur_index, max_index, prv_index);
	if((max_index != cur_index)
		&& ((prv_index + 5) <= cur_index)) {
		flash_param_ptr->set_ae=ISP_UEB;
		flash_param_ptr->set_awb=ISP_UEB;
	} else {
		if (1)//prv_lum<=wYlayer) 
		{
			prv_exposure=ae_param_ptr->cur_e_ptr[prv_index];
			prv_gain=ae_param_ptr->cur_g_ptr[prv_index];
			prv_gain=isp_ae_get_real_gain(prv_gain);

			cur_exposure=ae_param_ptr->cur_e_ptr[cur_index];
			cur_gain=ae_param_ptr->cur_g_ptr[cur_index];
			cur_gain=isp_ae_get_real_gain(cur_gain);

			delta_lum = ((__s64)wYlayer*(((__s64)prv_gain*(__s64)cur_exposure)/(__s64)prv_exposure)/(__s64)cur_gain);

			if(delta_lum<=prv_lum){
				delta_lum = 0;
			}else{
				delta_lum = delta_lum - prv_lum;
			}
			flash_lum = prv_lum + (((__s64)delta_lum*(__s64)flash_param_ptr->ratio)>>0x08);
			if (1 > ((flash_param_ptr->ratio)>>0x08))
				wYlayer = ((flash_lum*cur_gain*prv_exposure)/cur_exposure)/prv_gain;

			if(target_lum<wYlayer) {
				do {
					cur_index--;
					calc_exposure=ae_param_ptr->cur_e_ptr[cur_index];
					calc_gain=ae_param_ptr->cur_g_ptr[cur_index];
					calc_gain=isp_ae_get_real_gain(calc_gain);

					calc_lum=(((__s64)wYlayer*(__s64)calc_gain*(__s64)cur_exposure)/(__s64)calc_exposure)/(__s64)cur_gain;

					if((target_lum_high_thr>calc_lum)
						||(min_index==cur_index)) {
						break;
					}
				}while(1);
			}

			//_isp_ae_get_flash_effect(handler_id, flash_param_ptr->ratio, target_lum, (uint32_t)prv_exposure, prv_gain, prv_lum, cur_exposure, cur_gain, v00_flash_ptr->cur_lum, (uint32_t*)&effect);
			_isp_ae_get_flash_effect(handler_id, v00_flash_ptr, &effect);
		}

		flash_param_ptr->next_index=cur_index;
		flash_param_ptr->set_ae=ISP_EB;
		flash_param_ptr->set_awb=ISP_EB;
		flash_param_ptr->effect=effect;
	}
	
	flash_param_ptr->next_index=cur_index;
	
	ae_param_ptr->callback(handler_id, ISP_CALLBACK_EVT|ISP_FLASH_AE_CALLBACK, (void*)&flash_stab, sizeof(uint32_t));

	return rtn;
}
#endif

#define CAL_CISION 256
#define QUANTIFICAT 1024

static uint32_t _isp_ae_get_flash_effect(uint32_t handler_id, struct isp_ae_v00_flash_alg_param* v00_flash_ptr, float k, int32_t *effect)
{
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_flash_param* flash_param_ptr=&ae_param_ptr->flash;
	int32_t before_flash_luma = v00_flash_ptr->prv_lum;

	float effect_ratio = 0.0;

	if (NULL == effect) {
		ISP_LOG("effect %p param is error!", effect);
		return ISP_ERROR;
	}

	*effect = QUANTIFICAT;

	if (!before_flash_luma)
		goto exit_end;

 	if (1.0 < k)
		k = 1.0;
	else if(k > -0.000001 && k < 0.000001)
		goto exit_end;

	effect_ratio = flash_param_ptr->ratio * (1 - k) / (k * CAL_CISION);
	effect_ratio = effect_ratio / (effect_ratio + 1);
	*effect = effect_ratio * QUANTIFICAT + 0.5;
	ISP_LOG("effect_ratio = %f *effect = %d", effect_ratio, *effect);

	*effect = (*effect > QUANTIFICAT) ? QUANTIFICAT : *effect;

exit_end:
	ISP_LOG("x *effect = %d", *effect);
	return 0;
}

int32_t isp_flash_calculation(uint32_t handler_id, struct isp_ae_v00_flash_alg_param* v00_flash_ptr)
{
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_flash_param* flash_param_ptr=&ae_param_ptr->flash;
	int32_t target_lum_high_thr = v00_flash_ptr->target_lum+v00_flash_ptr->target_zone;
	int32_t before_flash_index = v00_flash_ptr->prv_index;
	int32_t pre_flash_index = v00_flash_ptr->cur_index;
	int32_t min_index = ae_param_ptr->min_index;
	int32_t max_index = ae_param_ptr->max_index;
	int32_t target_lum = v00_flash_ptr->target_lum;
	int32_t before_flash_luma = v00_flash_ptr->prv_lum;
	int32_t pre_flash_luma = v00_flash_ptr->cur_lum;
	int32_t cur_index = v00_flash_ptr->cur_index;

	int32_t before_flash_exp = 0;
	int32_t before_flash_gain = 0;
	int32_t pre_flash_exp = 0;
	int32_t pre_flash_gain = 0;
	float pre_flash_ev = 0;
	float before_flash_ev = 0;
	float k = 0;

	/* for awb */
	int32_t effect = 0;
	int32_t flash_stab = 0x00;

	/* find lum temp */
	int32_t calc_gain = 0x00;
	int32_t calc_exposure = 0x00;
	int32_t calc_lum = 0x00;

	/* final capture calculate luma */
	int32_t cap_luma = 0;

	flash_param_ptr->effect=ISP_ZERO;
	flash_param_ptr->prv_index=v00_flash_ptr->prv_index;

	if((max_index != cur_index)
		&& ((before_flash_index + 5) <= cur_index)) {
		flash_param_ptr->set_ae = ISP_UEB;
		flash_param_ptr->set_awb = ISP_UEB;
		ISP_LOG("come in max_index != cur_index");
	} else {
		before_flash_exp = ae_param_ptr->cur_e_ptr[before_flash_index];
		before_flash_gain = ae_param_ptr->cur_g_ptr[before_flash_index];
		before_flash_gain = isp_ae_get_real_gain(before_flash_gain);
		ISP_LOG("before_flash_exp = %d before_flash_gain = %d", before_flash_exp, before_flash_gain);

		pre_flash_exp = ae_param_ptr->cur_e_ptr[pre_flash_index];
		pre_flash_gain = ae_param_ptr->cur_g_ptr[pre_flash_index];
		pre_flash_gain = isp_ae_get_real_gain(pre_flash_gain);
		ISP_LOG("pre_flash_exp = %d pre_flash_gain = %d", pre_flash_exp, pre_flash_gain);

		pre_flash_ev = pre_flash_gain * 1.0 / pre_flash_exp;
		before_flash_ev = before_flash_gain * 1.0 / before_flash_exp;
		k = (before_flash_luma * pre_flash_ev) * 1.0 / (pre_flash_luma * before_flash_ev);
		ISP_LOG("before_flash_luma = %d, pre_flash_ev = %f before_flash_ev = %f pre_flash_luma = %d k = %f",
				before_flash_luma, pre_flash_ev, before_flash_ev, pre_flash_luma, k);

		cap_luma = pre_flash_luma * (k * CAL_CISION + (1 - k) * flash_param_ptr->ratio) / CAL_CISION + 0.5;
 		ISP_LOG("cap_luma = %d, flash_param_ptr->ratio = %d", cap_luma, flash_param_ptr->ratio);
		if (target_lum < cap_luma) {
			do {
				cur_index--;
				calc_exposure = ae_param_ptr->cur_e_ptr[cur_index];
				calc_gain = ae_param_ptr->cur_g_ptr[cur_index];
				calc_gain = isp_ae_get_real_gain(calc_gain);

				calc_lum=((cap_luma * calc_gain * pre_flash_exp) / calc_exposure) / pre_flash_gain;

				if((target_lum_high_thr>calc_lum)
					||(min_index==cur_index)) {
					break;
				}
			} while(1);
		}
		_isp_ae_get_flash_effect(handler_id, v00_flash_ptr, k, &effect);
	}
	ISP_LOG("cur_index = %d", cur_index);
	flash_param_ptr->next_index = cur_index;
	flash_param_ptr->set_ae = ISP_EB;
	flash_param_ptr->set_awb = ISP_EB;
	flash_param_ptr->effect = effect;

	flash_param_ptr->next_index = cur_index;
	
	ae_param_ptr->callback(handler_id,
				ISP_CALLBACK_EVT | ISP_FLASH_AE_CALLBACK,
				(void*)&flash_stab,
				sizeof(uint32_t));

	return 0;
}

/* isp_ae_get_real_gain--
*@
*@
*@ return:
*/
int32_t isp_ae_get_real_gain(uint32_t gain)
{// x=real_gain/32
	uint32_t real_gain;
	uint32_t cur_gain=0x00;
	uint32_t i=0x00;

	cur_gain=(gain>>0x04)&0xfff;
	real_gain=((gain&0x0f)<<0x01)+0x20;
	
	for(i=0x00; i<11; i++) {
		if(0x01==(cur_gain&0x01)) {
			real_gain*=0x02;
		}
		cur_gain>>=0x01;
	}

	return real_gain;
}

/* _ispGetFetchPitch --
*@
*@
*@ return:
*/
int32_t _ispGetFetchPitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format)
{
	int32_t rtn=ISP_SUCCESS;

	pitch_ptr->chn0=ISP_ZERO;
	pitch_ptr->chn1=ISP_ZERO;
	pitch_ptr->chn2=ISP_ZERO;

	switch(format)
	{
		case ISP_DATA_YUV422_3FRAME:
		{
			pitch_ptr->chn0=width;
			pitch_ptr->chn1=width>>ISP_ONE;
			pitch_ptr->chn2=width>>ISP_ONE;
			break;
		}
		case ISP_DATA_YUV422_2FRAME:
		case ISP_DATA_YVU422_2FRAME:
		{
			pitch_ptr->chn0=width;
			pitch_ptr->chn1=width;
			break;
		}
		case ISP_DATA_YUYV:
		case ISP_DATA_UYVY:
		case ISP_DATA_YVYU:
		case ISP_DATA_VYUY:
		{
			pitch_ptr->chn0=width<<ISP_ONE;
			break;
		}
		case ISP_DATA_NORMAL_RAW10:
		{
			pitch_ptr->chn0=width<<ISP_ONE;
			break;
		}
		case ISP_DATA_CSI2_RAW10:
		{
			pitch_ptr->chn0=(width*5)>>ISP_TWO;
			break;
		}
		default :
		{
			break;
		}
	}

	return rtn;
}

/* _ispGetStorePitch --
*@
*@
*@ return:
*/
int32_t _ispGetStorePitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format)
{
	int32_t rtn=ISP_SUCCESS;

	pitch_ptr->chn0=ISP_ZERO;
	pitch_ptr->chn1=ISP_ZERO;
	pitch_ptr->chn2=ISP_ZERO;

	switch(format)
	{
		case ISP_DATA_YUV422_3FRAME:
		case ISP_DATA_YUV420_3_FRAME:
		{
			pitch_ptr->chn0=width;
			pitch_ptr->chn1=width>>ISP_ONE;
			pitch_ptr->chn2=width>>ISP_ONE;
			break;
		}
		case ISP_DATA_YUV422_2FRAME:
		case ISP_DATA_YVU422_2FRAME:
		case ISP_DATA_YUV420_2FRAME:
		case ISP_DATA_YVU420_2FRAME:
		{
			pitch_ptr->chn0=width;
			pitch_ptr->chn1=width;
			break;
		}
		case ISP_DATA_UYVY:
		{
			pitch_ptr->chn0=width<<ISP_ONE;
			break;
		}

		default :
		{
			break;
		}
	}
	return rtn;
}

/* _ispGetSliceHeightNum --
*@
*@
*@ return:
*/
int32_t _ispGetSliceHeightNum(struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr)
{
	int32_t rtn=ISP_SUCCESS;

	slice_ptr->all_slice_num.h=src_size_ptr->h/slice_ptr->max_size.h;

	if(ISP_ZERO!=(src_size_ptr->h%slice_ptr->max_size.h)) {
		slice_ptr->all_slice_num.h++;
	}

	return rtn;
}

/* _ispGetSliceWidthNum --
*@
*@
*@ return:
*/
int32_t _ispGetSliceWidthNum(struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr)
{
	int32_t rtn=ISP_SUCCESS;

	slice_ptr->all_slice_num.w=src_size_ptr->w/slice_ptr->max_size.w;

	if(ISP_ZERO!=(src_size_ptr->w%slice_ptr->max_size.w)) {
		slice_ptr->all_slice_num.w++;
	}

	return rtn;
}

/* _ispSetSlicePosInfo --
*@
*@
*@ return:
*/
int32_t _ispSetSlicePosInfo(struct isp_slice_param* slice_ptr)
{
	int32_t rtn=ISP_SUCCESS;

	if((ISP_ZERO==slice_ptr->complete_line)
		&&(slice_ptr->all_line==slice_ptr->max_size.h)) {
		slice_ptr->pos_info=ISP_SLICE_ALL;
	} else {
		if(ISP_ZERO==slice_ptr->complete_line) {
			slice_ptr->pos_info=ISP_SLICE_FIRST;
		} else if(slice_ptr->all_line==(slice_ptr->complete_line+slice_ptr->max_size.h)) {
			slice_ptr->pos_info=ISP_SLICE_LAST;
		} else {
			slice_ptr->pos_info=ISP_SLICE_MID;
		}
	}

	return rtn;
}

/* _ispAddSliceBorder --
*@
*@
*@ return:
*/
int32_t _ispAddSliceBorder(enum isp_slice_type type, enum isp_process_type proc_type, struct isp_slice_param* slice_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	uint16_t up=0x00;
	uint16_t down=0x00;
	uint16_t left=0x00;
	uint16_t right=0x00;

	switch(proc_type)
	{
		case ISP_PROC_BAYER:
		{
			switch(type)
			{
				case ISP_WAVE:
				{
					up=2;
					down=2;
					left=2;
					right=2;
					break;
				}
				case ISP_GLB_GAIN:
				{
					up=5;
					down=5;
					left=5;
					right=5;
					break;
				}
				case ISP_CFA:
				{
					up=8;
					down=8;
					left=8;
					right=8;
					break;
				}
				case ISP_PREF:
				case ISP_BRIGHT:
				{
					up=9;
					down=8;
					left=10;
					right=8;
					break;
				}
				case ISP_CSS:
				{
					up=11;
					down=10;
					left=14;
					right=12;
					break;
				}
				default:
					break;
			}
			break;
		}
		case ISP_PROC_YUV:
		{
			switch(type)
			{
				case ISP_BRIGHT:
				{
					up=1;
					down=0;
					left=2;
					right=0;
					break;
				}
				case ISP_CSS:
				{
					up=3;
					down=2;
					left=6;
					right=4;
					break;
				}
				default:
					break;
			}
			break;
		}
		default :
			break;
	}

	switch(slice_ptr->edge_info)
	{
		case 0: //center
		{
			slice_ptr->size[type].x+=left;
			slice_ptr->size[type].y+=up;
			slice_ptr->size[type].w-=left+right;
			slice_ptr->size[type].h-=up+down;
			break;
		}
		case 1: //right
		{
			slice_ptr->size[type].x+=left;
			slice_ptr->size[type].y+=up;
			slice_ptr->size[type].w-=left;
			slice_ptr->size[type].h-=up+down;
			break;
		}
		case 2: //left
		{
			slice_ptr->size[type].y+=up;
			slice_ptr->size[type].w-=right;
			slice_ptr->size[type].h-=up+down;
			break;
		}
		case 3: //left right
		{
			slice_ptr->size[type].y+=up;
			slice_ptr->size[type].h-=up+down;
			break;
		}
		case 4: //down
		{
			slice_ptr->size[type].x+=left;
			slice_ptr->size[type].y+=up;
			slice_ptr->size[type].w-=left+right;
			slice_ptr->size[type].h-=up;
			break;
		}
		case 5: //down right
		{
			slice_ptr->size[type].x+=left;
			slice_ptr->size[type].y+=up;
			slice_ptr->size[type].w-=left;
			slice_ptr->size[type].h-=up;
			break;
		}
		case 6: //down left
		{
			slice_ptr->size[type].y+=up;
			slice_ptr->size[type].w-=right;
			slice_ptr->size[type].h-=up;
			break;
		}
		case 7: //down left right
		{
			slice_ptr->size[type].y+=up;
			slice_ptr->size[type].h-=up;
			break;
		}
		case 8: //up
		{
			slice_ptr->size[type].x+=left;
			slice_ptr->size[type].w-=left+right;
			slice_ptr->size[type].h-=down;
			break;
		}
		case 9: //up right
		{
			slice_ptr->size[type].x+=left;
			slice_ptr->size[type].w-=left;
			slice_ptr->size[type].h-=down;
			break;
		}
		case 10: //up left
		{
			slice_ptr->size[type].w-=right;
			slice_ptr->size[type].h-=down;
			break;
		}
		case 11: //up left right
		{
			slice_ptr->size[type].h-=down;
			break;
		}
		case 12: //up down
		{
			slice_ptr->size[type].x+=left;
			slice_ptr->size[type].w-=left+right;
		break;
		}
		case 13: //up down right
		{
			slice_ptr->size[type].x+=left;
			slice_ptr->size[type].w-=left;
		break;
		}
		case 14: //up down left
		{
			slice_ptr->size[type].w-=right;
		break;
		}
		case 15: //up down left right
		{
			break;
		}
		default:
		{
			break;
		}
	}

	return rtn;
}

/* _ispGetSliceSize --
*@
*@
*@ return:
*/
int32_t _ispGetSliceSize(enum isp_process_type proc_type, struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	uint32_t i=ISP_ZERO;
	uint16_t overlap_up=ISP_ZERO;
	uint16_t overlap_down=ISP_ZERO;
	uint16_t overlap_left=ISP_ZERO;
	uint16_t overlap_right=ISP_ZERO;
	uint16_t slice_x=ISP_ZERO;
	uint16_t slice_y=ISP_ZERO;
	uint16_t slice_width=ISP_ZERO;
	uint16_t slice_height=ISP_ZERO;

	if(ISP_PROC_BAYER==proc_type) {// raw
		overlap_up=raw_overlap_up;
		overlap_down=raw_overlap_down;
		overlap_left=raw_overlap_left;
		overlap_right=raw_overlap_right;
	} else {// yuv
		overlap_up=yuv_overlap_up;
		overlap_down=yuv_overlap_down;
		overlap_left=yuv_overlap_left;
		overlap_right=yuv_overlap_right;
	}

	if(ISP_ZERO==slice_ptr->cur_slice_num.w) {
		overlap_left=ISP_ZERO;
	}

	if(slice_ptr->all_slice_num.w==(slice_ptr->cur_slice_num.w+ISP_ONE)) {
		overlap_right=ISP_ZERO;
	}

	if(ISP_SLICE_ALL==slice_ptr->pos_info) {
		overlap_up=ISP_ZERO;
		overlap_down=ISP_ZERO;
	}

	if(ISP_SLICE_FIRST==slice_ptr->pos_info) {
		overlap_up=ISP_ZERO;
	}

	if(ISP_SLICE_LAST==slice_ptr->pos_info) {
		overlap_down=ISP_ZERO;
	}

	slice_x=slice_ptr->max_size.w*slice_ptr->cur_slice_num.w;
	slice_y=slice_ptr->complete_line;

	if(slice_ptr->max_size.w*(slice_ptr->cur_slice_num.w+ISP_ONE)<=src_size_ptr->w) {
		slice_width=slice_ptr->max_size.w;
	} else {
		slice_width=src_size_ptr->w-(slice_ptr->max_size.w*slice_ptr->cur_slice_num.w);
	}

	if(slice_ptr->max_size.h*(slice_ptr->cur_slice_num.h+ISP_ONE)<=src_size_ptr->h) {
		slice_height=slice_ptr->max_size.h;
	} else {
		slice_height=src_size_ptr->h-(slice_ptr->max_size.h*slice_ptr->cur_slice_num.h);
	}

	while(i<ISP_SLICE_TYPE_MAX) {
		slice_ptr->size[i].x=slice_x;
		slice_ptr->size[i].y=slice_y;
		slice_ptr->size[i].w=slice_width;
		slice_ptr->size[i].h=slice_height;

		slice_ptr->size[i].x-=overlap_left;
		slice_ptr->size[i].y-=overlap_up;

		slice_ptr->size[i].w+=(overlap_left+overlap_right);
		slice_ptr->size[i].h+=(overlap_up+overlap_down);

		i++;
	}

	slice_ptr->size[ISP_STORE].w=slice_width;
	slice_ptr->size[ISP_STORE].h=slice_height;

	return rtn;
}

/* _ispGetSliceEdgeInfo --
*@
*@
*@ return:
*/
int32_t _ispGetSliceEdgeInfo(struct isp_slice_param* slice_ptr)
{
	int32_t rtn=ISP_SUCCESS;

	slice_ptr->edge_info=0x00;

	if(ISP_ZERO==slice_ptr->cur_slice_num.w) {
		slice_ptr->edge_info|=ISP_SLICE_LEFT;
	}

	if(slice_ptr->all_slice_num.w==(slice_ptr->cur_slice_num.w+ISP_ONE)) {
		slice_ptr->edge_info|=ISP_SLICE_RIGHT;
	}

	if(ISP_SLICE_ALL==slice_ptr->pos_info) {
		slice_ptr->edge_info|=ISP_SLICE_UP;
		slice_ptr->edge_info|=ISP_SLICE_DOWN;
	}

	if(ISP_SLICE_FIRST==slice_ptr->pos_info) {
		slice_ptr->edge_info|=ISP_SLICE_UP;
	}

	if(ISP_SLICE_LAST==slice_ptr->pos_info) {
		slice_ptr->edge_info|=ISP_SLICE_DOWN;
	}

	return rtn;
}

/* _ispGetLensGridPitch --
*@
*@
*@ return:
*/
uint16_t _ispGetLensGridPitch(uint16_t src_width, uint8_t len_grid, uint32_t isp_id)
{
	uint16_t pitch=ISP_SUCCESS;

	if(0 == len_grid) {
		return pitch;
	}

	//pitch = ((src_width/2-1) % len_grid)?((src_width/2-1) / len_grid + 2) : ((src_width/2-1) / len_grid + 1);
	if(ISP_ZERO!=((src_width/ISP_TWO-ISP_ONE) % len_grid)) {
		pitch=((src_width/ISP_TWO-ISP_ONE)/len_grid+ISP_TWO);
	} else {
		pitch=((src_width/ISP_TWO-ISP_ONE)/len_grid+ISP_ONE);
	}

	if(isp_id == SC9630_ISP_ID)
	{
		pitch +=2;
	}
	return pitch;
}

/* _ispGetLncAddr --
*@
*@
*@ return:
*/
int32_t _ispGetLncAddr(struct isp_lnc_param* param_ptr,struct isp_slice_param* isp_ptr, uint16_t src_width ,uint32_t isp_id)
{
	int32_t rtn=ISP_SUCCESS;
	uint32_t handler_id = ISP_ID_INVALID;
	struct isp_slice_param* slice_ptr=isp_ptr;
	uint32_t StartGridRow=0x00;
	uint32_t StartGridCol=0x00;
	uint32_t EndGridRow=0x00;
	uint32_t EndGridCol=0x00;
	uint32_t start_col=slice_ptr->size[ISP_LENS].x;
	uint32_t start_row=slice_ptr->size[ISP_LENS].y;
	uint32_t end_col=slice_ptr->size[ISP_LENS].x+slice_ptr->size[ISP_LENS].w-ISP_ONE;
	uint32_t end_row=slice_ptr->size[ISP_LENS].y+slice_ptr->size[ISP_LENS].h-ISP_ONE;

	if(ISP_ZERO!=param_ptr->map.grid_pitch)
	{
		StartGridCol = (start_col / 2) / param_ptr->map.grid_pitch;
		StartGridRow = (start_row / 2) / param_ptr->map.grid_pitch;
		EndGridCol = (end_col / 2) / param_ptr->map.grid_pitch+ISP_ONE;
		EndGridRow = (end_row / 2) / param_ptr->map.grid_pitch+ISP_ONE;

		slice_ptr->size[ISP_LENS].x=start_col&0xff;
		slice_ptr->size[ISP_LENS].y=start_row&0xff;
		slice_ptr->size[ISP_LENS].w=EndGridCol-StartGridCol+ISP_ONE;
		slice_ptr->size[ISP_LENS].h=EndGridRow-StartGridRow+ISP_ONE;

		if(isp_id == SC9630_ISP_ID)
		{
			slice_ptr->size[ISP_LENS].w+=2;
			slice_ptr->size[ISP_LENS].h+=2;			
		}

		param_ptr->map.grid_pitch=_ispGetLensGridPitch(src_width, param_ptr->map.grid_pitch,isp_id);
		param_ptr->map.param_addr+=(StartGridRow * param_ptr->map.grid_pitch+StartGridCol) * 4 * 2;
	} else {
		ISP_LOG("lnc grid_pitch zero error");
		rtn = ISP_ERROR;
	}

	return rtn;
}

/* _ispGetLncCurrectParam --
*@
*@
*@ return:
*/
int32_t _ispGetLncCurrectParam(void* lnc0_ptr,void* lnc1_ptr, uint32_t lnc_len, uint32_t alpha, void* dst_lnc_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	uint32_t i=0x00;
	uint32_t handler_id=0;

	uint16_t* dst_ptr = (uint16_t*)dst_lnc_ptr;
	uint16_t* src0_ptr = (uint16_t*)lnc0_ptr;
	uint16_t* src1_ptr = (uint16_t*)lnc1_ptr;

	if ((NULL == lnc0_ptr)
		||(NULL == dst_lnc_ptr)) {
		AAA_LOG("lnc buf null %p, %p, %p error", lnc0_ptr, lnc1_ptr, dst_lnc_ptr);
		return ISP_ERROR;
	}

	alpha = (alpha > ISP_ALPHA_ONE) ? ISP_ALPHA_ONE : alpha;

	if (ISP_ALPHA_ZERO == alpha) {
		memcpy(dst_lnc_ptr, lnc0_ptr, lnc_len);
	} else {
		if (NULL == lnc1_ptr) {
			AAA_LOG("lnc1 buf null %p, %p, %p, error", lnc0_ptr, lnc1_ptr, dst_lnc_ptr);
			return ISP_ERROR;
		}

		if (ISP_ALPHA_ONE == alpha) {
			memcpy(dst_lnc_ptr, lnc1_ptr, lnc_len);
		} else {
			uint32_t lnc_num = lnc_len / sizeof(uint16_t);
			for (i = 0x00; i<lnc_num; i++) {
				dst_ptr[i] = (src0_ptr[i] * (ISP_ALPHA_ONE - alpha) + src1_ptr[i] * alpha) / ISP_ALPHA_ONE;
			}
		}
	}

	return rtn;
}

/* _ispGetFetchAddr --
*@
*@
*@ return:
*/
int32_t _ispGetFetchAddr(uint32_t handler_id, struct isp_fetch_param* fetch_ptr)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_context* isp_context_ptr = ispGetAlgContext(handler_id);
	struct isp_size* cur_slice_ptr = &isp_context_ptr->slice.cur_slice_num;
	struct isp_size* all_slice_ptr = &isp_context_ptr->slice.all_slice_num;
	uint32_t ch0_offset=ISP_ZERO;
	uint32_t ch1_offset=ISP_ZERO;
	uint32_t ch2_offset=ISP_ZERO;
	uint32_t slice_w_offset=ISP_ZERO;
	uint32_t slice_h_offset=ISP_ZERO;
	uint16_t overlap_up=ISP_ZERO;
	uint16_t overlap_left=ISP_ZERO;
	uint16_t src_width=isp_context_ptr->src.w;
	uint16_t slice_width=isp_context_ptr->slice.max_size.w;
	uint16_t slice_height=isp_context_ptr->slice.max_size.h;
	uint16_t fetch_width=isp_context_ptr->slice.size[ISP_FETCH].w;
	uint32_t start_col=0x00;
	uint32_t end_col=0x00;
	uint16_t complete_line=isp_context_ptr->slice.complete_line;

	uint32_t mipi_word_num_start[16] = {0,
					1,1,1,1,
					2,2,2,
					3,3,3,
					4,4,4,
					5,5};
	uint32_t mipi_word_num_end[16] = {0,
					2,2,2,2,
					3,3,3,3,
					4,4,4,4,
					5,5,5};

	if((ISP_FETCH_NORMAL_RAW10==isp_context_ptr->com.fetch_color_format)
		||(ISP_FETCH_CSI2_RAW10==isp_context_ptr->com.fetch_color_format)) {// raw
		overlap_up=raw_overlap_up;
		overlap_left=raw_overlap_left;
	} else {// yuv
		overlap_up=yuv_overlap_up;
		overlap_left=yuv_overlap_left;
	}

	fetch_ptr->bypass=isp_context_ptr->featch.bypass;
	fetch_ptr->sub_stract=isp_context_ptr->featch.sub_stract;
	fetch_ptr->addr.chn0=isp_context_ptr->featch.addr.chn0;
	fetch_ptr->addr.chn1=isp_context_ptr->featch.addr.chn1;
	fetch_ptr->addr.chn2=isp_context_ptr->featch.addr.chn2;
	fetch_ptr->pitch.chn0=isp_context_ptr->featch.pitch.chn0;
	fetch_ptr->pitch.chn1=isp_context_ptr->featch.pitch.chn1;
	fetch_ptr->pitch.chn2=isp_context_ptr->featch.pitch.chn2;
	fetch_ptr->mipi_word_num=isp_context_ptr->featch.mipi_word_num;
	fetch_ptr->mipi_byte_rel_pos==isp_context_ptr->featch.mipi_byte_rel_pos;

	if(ISP_ZERO!=cur_slice_ptr->w) {
		slice_w_offset=overlap_left;
	}

	if((ISP_SLICE_MID==isp_context_ptr->slice.pos_info)
		||(ISP_SLICE_LAST==isp_context_ptr->slice.pos_info)) {
		slice_h_offset=overlap_up;
	}

	switch(isp_context_ptr->com.fetch_color_format)
	{
		case ISP_FETCH_YUV422_3FRAME:
		{
			ch0_offset=src_width*(complete_line-slice_h_offset)+slice_width*cur_slice_ptr->w-slice_w_offset;
			ch1_offset=(src_width*(complete_line-slice_h_offset)+slice_width*cur_slice_ptr->w-slice_w_offset)>>0x01;
			ch2_offset=(src_width*(complete_line-slice_h_offset)+slice_width*cur_slice_ptr->w-slice_w_offset)>>0x01;
			break;
		}
		case ISP_FETCH_YUV422_2FRAME:
		case ISP_FETCH_YVU422_2FRAME:
		{
			ch0_offset=src_width*(complete_line-slice_h_offset)+slice_width*cur_slice_ptr->w-slice_w_offset;
			ch1_offset=src_width*(complete_line-slice_h_offset)+slice_width*cur_slice_ptr->w-slice_w_offset;
			break;
		}
		case ISP_FETCH_YUYV:
		case ISP_FETCH_UYVY:
		case ISP_FETCH_YVYU:
		case ISP_FETCH_VYUY:
		case ISP_FETCH_NORMAL_RAW10:
		{
			ch0_offset=(src_width*(complete_line-slice_h_offset)+slice_width*cur_slice_ptr->w-slice_w_offset)<<0x01;
			break;
		}
		case ISP_FETCH_CSI2_RAW10:
		{
			ch0_offset=((src_width*(complete_line-slice_h_offset)+slice_width*cur_slice_ptr->w-slice_w_offset)*0x05)>>0x02;
			break;
		}
		default :
		{
			break;
		}
	}

	fetch_ptr->addr.chn0+=ch0_offset;
	fetch_ptr->addr.chn1+=ch1_offset;
	fetch_ptr->addr.chn2+=ch2_offset;

	start_col=slice_width*cur_slice_ptr->w-slice_w_offset;
	end_col=start_col+fetch_width-ISP_ONE;

	fetch_ptr->mipi_byte_rel_pos=start_col&0x0f;
	fetch_ptr->mipi_word_num=(((end_col+1)>>4)*5+mipi_word_num_end[(end_col+1)&0x0f])-//end word num
				(((start_col+1)>>4)*5+mipi_word_num_start[(start_col+1)&0x0f])+//start word num
				1;

	return rtn;
}

int32_t isp_GetChipVersion(void)
{
	int32_t rtn=ISP_SUCCESS;
	uint32_t handler_id=0;

	AAA_LOG("--AAA INIT-- soft id:0x%08x", ISP_SOFT_ID);

	return rtn;
}

int32_t isp_InterplateCMC(uint32_t handler_id, uint16_t *out, uint16_t *src[2], uint16_t alpha)
{	
	uint32_t i;

	if (NULL == out) {
		AAA_LOG("cmc interpolation: out buffer invalid");
		return ISP_ERROR;
	}

	alpha = (alpha > ISP_ALPHA_ONE) ? ISP_ALPHA_ONE : alpha;

	if (ISP_ALPHA_ZERO == alpha) {

		if (NULL == src[0]) {
			AAA_LOG("cmc interpolation: src[0] buffer invalid");
			return ISP_ERROR;
		}

		memcpy(out, src[0], 9 * sizeof(uint16_t));
	} else if (ISP_ALPHA_ONE == alpha) {

		if (NULL == src[1]) {
			AAA_LOG("cmc interpolation: src[1] buffer invalid");
			return ISP_ERROR;
		}

		memcpy(out, src[1], 9 * sizeof(uint16_t));
	} else{

		if (NULL == src[1] || NULL == src[0]) {
			AAA_LOG("cmc interpolation: src[0] | src[1] buffer invalid");
			return ISP_ERROR;
		}


		for (i=0; i<9; i++)
		{
			int32_t x0 = (int16_t)(src[0][i] << 2);
			int32_t x1 = (int16_t)(src[1][i] << 2);
			int32_t x = (x0 * (int32_t)(ISP_ALPHA_ONE - alpha) + x1 * (int32_t)alpha) / ISP_ALPHA_ONE;
			
			out[i] = (uint16_t)((x >> 2) & 0x3fff);

		}
	}

	return ISP_SUCCESS;
}

int32_t  isp_SetCMC_By_Reduce(uint32_t handler_id, uint16_t *cmc_out, uint16_t *cmc_in, int32_t percent, uint8_t *is_update)
{
	#define RGB_TO_YUV_0 0x012B
	#define RGB_TO_YUV_1 0x024B
	#define RGB_TO_YUV_2 0x0072
	#define RGB_TO_YUV_3 (0-0x00A8)
	#define RGB_TO_YUV_4 (0-0x014B)
	#define RGB_TO_YUV_5 0x01F4
	#define RGB_TO_YUV_6 0x01F4
	#define RGB_TO_YUV_7 (0-0x01A3)
	#define RGB_TO_YUV_8 (0-0x0051)

	#define YUV_TO_RGB_0 0x03E8
	#define YUV_TO_RGB_1 0x0000
	#define YUV_TO_RGB_2 0x057A
	#define YUV_TO_RGB_3 0x03E8
	#define YUV_TO_RGB_4 (0-0x0158)
	#define YUV_TO_RGB_5 (0-0x02CA)
	#define YUV_TO_RGB_6 0x03E8
	#define YUV_TO_RGB_7 0x0499
	#define YUV_TO_RGB_8 0x0000
	
	__s64 cmc_matrix[9];
	__s64 matrix_0[9];
	__s64 matrix_1[9];
	uint8_t i=0x00;
	int16_t calc_matrix[9];
	uint16_t *matrix_ptr = PNULL;
	/* for warning handler_id*/
	handler_id = handler_id;

	*is_update = ISP_UEB;

	matrix_ptr = (uint16_t*)cmc_out;

	percent = (0 == percent) ? 1 : percent;
	
	if(255 == percent)
	{
		if ((matrix_ptr[0]!=cmc_in[0])
			||(matrix_ptr[1]!=cmc_in[1])
			||(matrix_ptr[2]!=cmc_in[2])
			||(matrix_ptr[3]!=cmc_in[3])
			||(matrix_ptr[4]!=cmc_in[4])
			||(matrix_ptr[5]!=cmc_in[5])
			||(matrix_ptr[6]!=cmc_in[6])
			||(matrix_ptr[7]!=cmc_in[7])
			||(matrix_ptr[8]!=cmc_in[8])) {

			matrix_ptr[0]=cmc_in[0];
			matrix_ptr[1]=cmc_in[1];
			matrix_ptr[2]=cmc_in[2];
			matrix_ptr[3]=cmc_in[3];
			matrix_ptr[4]=cmc_in[4];
			matrix_ptr[5]=cmc_in[5];
			matrix_ptr[6]=cmc_in[6];
			matrix_ptr[7]=cmc_in[7];
			matrix_ptr[8]=cmc_in[8];
			*is_update=ISP_EB;
		}
	} else if (ISP_ZERO<percent) {
		matrix_0[0]=YUV_TO_RGB_0*255;
		matrix_0[1]=YUV_TO_RGB_1*percent;
		matrix_0[2]=YUV_TO_RGB_2*percent;
		matrix_0[3]=YUV_TO_RGB_3*255;
		matrix_0[4]=YUV_TO_RGB_4*percent;
		matrix_0[5]=YUV_TO_RGB_5*percent;
		matrix_0[6]=YUV_TO_RGB_6*255;
		matrix_0[7]=YUV_TO_RGB_7*percent;
		matrix_0[8]=YUV_TO_RGB_8*percent;

		matrix_1[0]=matrix_0[0]*RGB_TO_YUV_0+matrix_0[1]*RGB_TO_YUV_3+matrix_0[2]*RGB_TO_YUV_6;
		matrix_1[1]=matrix_0[0]*RGB_TO_YUV_1+matrix_0[1]*RGB_TO_YUV_4+matrix_0[2]*RGB_TO_YUV_7;
		matrix_1[2]=matrix_0[0]*RGB_TO_YUV_2+matrix_0[1]*RGB_TO_YUV_5+matrix_0[2]*RGB_TO_YUV_8;
		matrix_1[3]=matrix_0[3]*RGB_TO_YUV_0+matrix_0[4]*RGB_TO_YUV_3+matrix_0[5]*RGB_TO_YUV_6;
		matrix_1[4]=matrix_0[3]*RGB_TO_YUV_1+matrix_0[4]*RGB_TO_YUV_4+matrix_0[5]*RGB_TO_YUV_7;
		matrix_1[5]=matrix_0[3]*RGB_TO_YUV_2+matrix_0[4]*RGB_TO_YUV_5+matrix_0[5]*RGB_TO_YUV_8;
		matrix_1[6]=matrix_0[6]*RGB_TO_YUV_0+matrix_0[7]*RGB_TO_YUV_3+matrix_0[8]*RGB_TO_YUV_6;
		matrix_1[7]=matrix_0[6]*RGB_TO_YUV_1+matrix_0[7]*RGB_TO_YUV_4+matrix_0[8]*RGB_TO_YUV_7;
		matrix_1[8]=matrix_0[6]*RGB_TO_YUV_2+matrix_0[7]*RGB_TO_YUV_5+matrix_0[8]*RGB_TO_YUV_8;

		matrix_0[0]=cmc_in[0];
		matrix_0[1]=cmc_in[1];
		matrix_0[2]=cmc_in[2];
		matrix_0[3]=cmc_in[3];
		matrix_0[4]=cmc_in[4];
		matrix_0[5]=cmc_in[5];
		matrix_0[6]=cmc_in[6];
		matrix_0[7]=cmc_in[7];
		matrix_0[8]=cmc_in[8];

		for(i=0x00; i<9; i++)
		{
			if(ISP_ZERO!=(matrix_0[i]&0x2000))
			{
				matrix_0[i]|=0xffffffffffffc000;
			}
		}

		cmc_matrix[0]=matrix_1[0]*matrix_0[0]+matrix_1[1]*matrix_0[3]+matrix_1[2]*matrix_0[6];
		cmc_matrix[1]=matrix_1[0]*matrix_0[1]+matrix_1[1]*matrix_0[4]+matrix_1[2]*matrix_0[7];
		cmc_matrix[2]=matrix_1[0]*matrix_0[2]+matrix_1[1]*matrix_0[5]+matrix_1[2]*matrix_0[8];
		cmc_matrix[3]=matrix_1[3]*matrix_0[0]+matrix_1[4]*matrix_0[3]+matrix_1[5]*matrix_0[6];
		cmc_matrix[4]=matrix_1[3]*matrix_0[1]+matrix_1[4]*matrix_0[4]+matrix_1[5]*matrix_0[7];
		cmc_matrix[5]=matrix_1[3]*matrix_0[2]+matrix_1[4]*matrix_0[5]+matrix_1[5]*matrix_0[8];
		cmc_matrix[6]=matrix_1[6]*matrix_0[0]+matrix_1[7]*matrix_0[3]+matrix_1[8]*matrix_0[6];
		cmc_matrix[7]=matrix_1[6]*matrix_0[1]+matrix_1[7]*matrix_0[4]+matrix_1[8]*matrix_0[7];
		cmc_matrix[8]=matrix_1[6]*matrix_0[2]+matrix_1[7]*matrix_0[5]+matrix_1[8]*matrix_0[8];

		cmc_matrix[0]=cmc_matrix[0]/1000/1000/255;
		cmc_matrix[1]=cmc_matrix[1]/1000/1000/255;
		cmc_matrix[2]=cmc_matrix[2]/1000/1000/255;
		cmc_matrix[3]=cmc_matrix[3]/1000/1000/255;
		cmc_matrix[4]=cmc_matrix[4]/1000/1000/255;
		cmc_matrix[5]=cmc_matrix[5]/1000/1000/255;
		cmc_matrix[6]=cmc_matrix[6]/1000/1000/255;
		cmc_matrix[7]=cmc_matrix[7]/1000/1000/255;
		cmc_matrix[8]=cmc_matrix[8]/1000/1000/255;

		calc_matrix[0]=cmc_matrix[0]&0x3fff;
		calc_matrix[1]=cmc_matrix[1]&0x3fff;
		calc_matrix[2]=cmc_matrix[2]&0x3fff;
		calc_matrix[3]=cmc_matrix[3]&0x3fff;
		calc_matrix[4]=cmc_matrix[4]&0x3fff;
		calc_matrix[5]=cmc_matrix[5]&0x3fff;
		calc_matrix[6]=cmc_matrix[6]&0x3fff;
		calc_matrix[7]=cmc_matrix[7]&0x3fff;
		calc_matrix[8]=cmc_matrix[8]&0x3fff;

		if ((matrix_ptr[0]!=calc_matrix[0])
			||(matrix_ptr[1]!=calc_matrix[1])
			||(matrix_ptr[2]!=calc_matrix[2])
			||(matrix_ptr[3]!=calc_matrix[3])
			||(matrix_ptr[4]!=calc_matrix[4])
			||(matrix_ptr[5]!=calc_matrix[5])
			||(matrix_ptr[6]!=calc_matrix[6])
			||(matrix_ptr[7]!=calc_matrix[7])
			||(matrix_ptr[8]!=calc_matrix[8])) {

			matrix_ptr[0]=calc_matrix[0];
			matrix_ptr[1]=calc_matrix[1];
			matrix_ptr[2]=calc_matrix[2];
			matrix_ptr[3]=calc_matrix[3];
			matrix_ptr[4]=calc_matrix[4];
			matrix_ptr[5]=calc_matrix[5];
			matrix_ptr[6]=calc_matrix[6];
			matrix_ptr[7]=calc_matrix[7];
			matrix_ptr[8]=calc_matrix[8];
			*is_update = ISP_EB;
		}
	}

	return ISP_SUCCESS;

}

int32_t isp_InterplateCCE(uint32_t handler_id, uint16_t dst[9], uint16_t src[9], uint16_t coef[3], uint16_t base_gain)
{
	int32_t matrix[3][3] = {0x00}; 
	int32_t *matrix_ptr = NULL;
	uint16_t *src_ptr = NULL;
	uint16_t *dst_ptr = NULL;
	int32_t tmp = 0;
	uint32_t i = 0, j = 0;

	if ((NULL == src)
		|| (NULL == dst)
		|| (NULL == coef)) {		
		ISP_LOG("--isp_InterplateCCE:invalid addr %p, %p, %p\n", src, dst, coef);
		return ISP_PARAM_NULL;
	}
	src_ptr = (uint16_t*)src;
	matrix_ptr = (int32_t*)&matrix[0][0];
	for (i=0x00; i<9; i++) {
		if (ISP_ZERO!=(src_ptr[i]&0x8000)) {
			*(matrix_ptr + i) = (src_ptr[i]) | 0xffff0000;
		} else {
			*(matrix_ptr + i) = src_ptr[i];
		}
	}

	dst_ptr = (uint16_t*)dst;
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {

			if (coef[j] == base_gain)
				tmp = matrix[i][j];
			else
				tmp = (((int32_t)coef[j]) * matrix[i][j] + base_gain / 2) / base_gain;

			*dst_ptr = (uint16_t)(((uint32_t)tmp) & 0xffff);
			dst_ptr++;
		}
	}

	return ISP_SUCCESS;
}

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

