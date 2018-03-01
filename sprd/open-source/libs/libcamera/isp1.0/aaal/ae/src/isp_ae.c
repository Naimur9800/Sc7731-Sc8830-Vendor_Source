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

#include "isp_alg.h"
#include "isp_ae_alg_v00.h"
#include "isp_com.h"
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
#define SMART_DENOISE 1
#define ISP_ID_INVALID 0xff
#define AE_SOFT_ID 0x20150623

// RGB to YUV
//Y = 0.299 * r + 0.587 * g + 0.114 * b
#define RGB_TO_Y(_r, _g, _b)	(int32_t)((77 * (_r) + 150 * (_g) + 29 * (_b)) >> 8)
//U = 128 - 0.1687 * r - 0.3313 * g + 0.5 * b
#define RGB_TO_U(_r, _g, _b)	(int32_t)(128 + ((128 * (_b) - 43 * (_r) - 85 * (_g)) >> 8))
//V = 128 + 0.5 * r - 0.4187 * g - 0.0813 * b
#define RGB_TO_V(_r, _g, _b)	(int32_t)(128  + ((128 * (_r) - 107 * (_g) - 21 * (_b)) >> 8))

#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )

/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/
enum isp_ae_status{
	ISP_AE_CLOSE=0x00,
	ISP_AE_IDLE,
	ISP_AE_RUN,
	ISP_AE_STATUS_MAX
};

struct isp_change_index {
	uint32_t* before_e_ptr;
	uint16_t* before_g_ptr;

	uint32_t* cur_e_ptr;
	uint16_t* cur_g_ptr;
	uint32_t cur_max_index;

	uint32_t index;
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

/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/

/* ispGetAeContext --
*@
*@
*@ return:
*/
static struct isp_context *s_cxt = NULL;

int32_t ispSetAeContext(uint32_t handler_id, void *cxt)
{
	uint32_t rtn = ISP_SUCCESS;

	if (cxt) {
		s_cxt = (struct isp_context *)cxt;
	} else {
		s_cxt = NULL;
		rtn = ISP_PARAM_NULL;
		ISP_LOG("error:isp context is NULL.");
	}
	return rtn;
}

struct isp_ae_param* ispGetAeContext(uint32_t handler_id)
{
	struct isp_context* isp_context_ptr = NULL;

	if (s_cxt) {
		isp_context_ptr = &s_cxt[handler_id];
	} else {
		return NULL;
	}

	return &isp_context_ptr->ae;
}

struct isp_context* ispGetAlgContext(uint32_t handler_id)
{
	struct isp_context* isp_context_ptr = NULL;

	if (s_cxt) {
		isp_context_ptr = &s_cxt[handler_id];
	} else {
		return NULL;
	}

	return isp_context_ptr;
}

/* isp_get_cur_lum --
*@
*@
*@ return:
*/
uint32_t isp_get_cur_lum(uint32_t handler_id, uint32_t* cur_lum, uint32_t rgb)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_context *isp_cxt_ptr = (struct isp_context*)ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	uint8_t* weight_tab_ptr=ae_param_ptr->cur_weight_ptr;
	struct isp_awb_param *awb_param_ptr = &isp_cxt_ptr->awb;
	
	//uint32_t valid_stat=0x00;
	uint32_t i=0x00;
	uint32_t valid_stat=0x00;
	__u64 r_stat=0x00;
	__u64 g_stat=0x00;
	__u64 b_stat=0x00;
	__u64 y_stat=0x00;
	uint32_t y=0x00;
	uint32_t y_test=0x00;

	if (0x01 == rgb)
	{
		while(i<1024) {
			g_stat+=ae_param_ptr->stat_g_ptr[i]*weight_tab_ptr[i];
			valid_stat+=weight_tab_ptr[i];
			i++;
		}

		g_stat <<= ISP_ONE;

		if(0x00!=valid_stat) {
			valid_stat*=ae_param_ptr->monitor.w*ae_param_ptr->monitor.h;
			g_stat/=valid_stat;
		} else {
			//error!
			rtn=ISP_ERROR;
		}

		*cur_lum=((uint32_t)g_stat);
	}

	return rtn;
}

/* _ispGetLineTime --
*@
*@
*@ return:
*/
uint32_t _ispGetLineTime(struct isp_resolution_info* resolution_info_ptr, uint32_t param_index)
{
	struct isp_resolution_info* resolution__ptr=resolution_info_ptr;
	uint32_t line_time=0x01;
	uint32_t handler_id = ISP_ID_INVALID;

	if(PNULL!=resolution__ptr) {

		if(0x00!=resolution__ptr[param_index].line_time) {
			line_time=resolution__ptr[param_index].line_time;
		} else {
			ISP_LOG("line time zero error");
		}
	} else {
		ISP_LOG("resolution ptr null error");
	}

	return line_time;
}

/* _ispGetFrameLine --
*@
*@
*@ return:
*/
uint32_t _ispGetFrameLine(uint32_t handler_id, uint32_t line_time, uint32_t fps)
{
	uint32_t frame_line=0x400;

	if((ISP_ZERO!=fps)
		&&(ISP_ZERO!=line_time)) {
		frame_line=10000000/(fps*line_time);
	} else {
		frame_line=ISP_AE_MAX_LINE;
		ISP_LOG("fps auto fps:%d, line_time:%d ", fps, line_time);
	}

	return frame_line;
}

/* _ispSetAeTabMaxIndex --
*@
*@
*@ return:
*/
uint32_t _ispSetAeTabMaxIndex(uint32_t handler_id, uint32_t mode, uint32_t iso,uint32_t fps)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr=&isp_context_ptr->ae;
	struct sensor_raw_info* raw_info_ptr=(struct sensor_raw_info*)isp_context_ptr->cfg.sensor_info_ptr;
	struct sensor_raw_fix_info* raw_fix_ptr=(struct sensor_raw_fix_info*)raw_info_ptr->fix_ptr;
	uint32_t fix_shutter=ISP_ZERO;
	uint32_t start_index=ISP_ZERO;
	uint32_t max_index=ISP_ZERO;
	uint32_t end_index=ISP_ZERO;
	uint32_t i=0x100*iso;

	isp_context_ptr->ae.tab[mode].start_index[iso]=raw_fix_ptr->ae.tab[mode].index[iso].start;
	isp_context_ptr->ae.tab[mode].max_index[iso]=raw_fix_ptr->ae.tab[mode].index[iso].max;
	max_index=isp_context_ptr->ae.tab[mode].max_index[iso];
	start_index=isp_context_ptr->ae.tab[mode].start_index[iso];
	end_index=i+max_index;

	if(0x00!=fps)
	{
		fix_shutter=fps*10;

		do
		{
			if((raw_fix_ptr->ae.tab[mode].e_ptr[i]<fix_shutter)
				||(end_index<i))
			{
				i--;
				break ;
			}
			i++;
		}while(1);

		max_index=i-0x100*iso;
	}
	else
	{
		i=end_index;
	}

	if(max_index<isp_context_ptr->ae.tab[mode].max_index[iso])
	{
		isp_context_ptr->ae.tab[mode].max_index[iso]=max_index;
	}

	if(max_index<isp_context_ptr->ae.tab[mode].start_index[iso])
	{
		isp_context_ptr->ae.tab[mode].start_index[iso]=max_index>>0x1;
	}

	return rtn;
}

/* _isp_get_real_exp --
*@
*@
*@ return:
*/
uint32_t _isp_get_real_exp(uint32_t cur_exp)
{
	uint32_t calc_exp = 0;

	calc_exp = 100000000/cur_exp;

	return calc_exp;
}

/* _isp_change_index --
*@
*@
*@ return:
*/
uint32_t _isp_change_index(uint32_t handler_id, struct isp_change_index* in_param, uint32_t* calc_index)
{
	int32_t rtn=ISP_SUCCESS;
	uint32_t* before_e_ptr = in_param->before_e_ptr;
	uint16_t* before_g_ptr = in_param->before_g_ptr;
	uint32_t* cur_e_ptr = in_param->cur_e_ptr;
	uint16_t* cur_g_ptr = in_param->cur_g_ptr;
	uint32_t cur_max_index = in_param->cur_max_index;
	uint32_t index = in_param->index;

	uint32_t before_exp = 0;
	uint16_t before_gain = 0;
	uint32_t cur_exp = 0;
	uint16_t cur_gain = 0;
	uint64_t before_accumulate = 0;
	uint64_t cur_accumulate = 0;

	ISP_LOG("INDEX: index:%d, %p, %p, %p, %p", index, before_e_ptr, before_g_ptr, cur_e_ptr, cur_g_ptr);
	//ISP_LOG("INDEX: index:%d, %d, %d, %d, %d", index, before_e_ptr[index], before_g_ptr[index], cur_e_ptr, cur_g_ptr[index]);

	if ((0 == before_e_ptr)
		|| (0 == before_g_ptr)
		|| (0 == cur_e_ptr)
		|| (0 == cur_g_ptr)) {
		goto EXIT;
	}

	before_exp = _isp_get_real_exp(before_e_ptr[index]);
	before_gain = isp_ae_get_real_gain(before_g_ptr[index]);

	if (cur_max_index <= index) {
		index = cur_max_index;
	}

	cur_exp = _isp_get_real_exp(cur_e_ptr[index]);
	cur_gain = isp_ae_get_real_gain(cur_g_ptr[index]);

	before_accumulate = before_exp*before_gain;
	cur_accumulate = cur_exp*cur_gain;

	if (before_accumulate < cur_accumulate) {
		do {
			index--;

			if (0 == index) {
				break;
			}

			cur_exp = _isp_get_real_exp(cur_e_ptr[index]);
			cur_gain = isp_ae_get_real_gain(cur_g_ptr[index]);

			before_accumulate = before_exp*before_gain;
			cur_accumulate = cur_exp*cur_gain;

			if (before_accumulate > cur_accumulate) {
				index++;
				break ;
			}

		}while(1);
	} else if (before_accumulate > cur_accumulate) {
		do {
			index++;

			if (cur_max_index < index) {
				index--;
				break;
			}

			cur_exp = _isp_get_real_exp(cur_e_ptr[index]);
			cur_gain = isp_ae_get_real_gain(cur_g_ptr[index]);

			before_accumulate = before_exp*before_gain;
			cur_accumulate = cur_exp*cur_gain;

			if (before_accumulate < cur_accumulate) {
				index--;
				break ;
			}
		}while(1);
	}


	EXIT:

	*calc_index = index;

	return rtn;
}

/* _ispSetFixFrameMaxIndex --
*@
*@
*@ return:
*/
uint32_t _ispSetFixFrameMaxIndex(uint32_t handler_id, uint32_t mode, uint32_t iso,uint32_t fps)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr=&isp_context_ptr->ae;
	struct isp_change_index index_param;
	uint32_t fix_shutter=0x00;
	uint32_t min_shutter=0x00;
	uint32_t start_index=0x00;
	uint32_t max_index=0x00;
	uint32_t end_index=0x00;
	uint32_t i=0x100*iso;
	uint32_t changed_index = 0;

	index_param.before_e_ptr = ae_param_ptr->cur_e_ptr;
	index_param.before_g_ptr = ae_param_ptr->cur_g_ptr;

	ae_param_ptr->cur_e_ptr = &ae_param_ptr->tab[mode].e_ptr[0x100*iso];
	ae_param_ptr->cur_g_ptr = &ae_param_ptr->tab[mode].g_ptr[0x100*iso];
	max_index=ae_param_ptr->tab[mode].max_index[iso];
	start_index=ae_param_ptr->tab[mode].start_index[iso];
	end_index=i+max_index;

	index_param.cur_e_ptr = ae_param_ptr->cur_e_ptr;
	index_param.cur_g_ptr = ae_param_ptr->cur_g_ptr;
	index_param.cur_max_index = max_index;
	index_param.index  = ae_param_ptr->cur_index;

	_isp_change_index(handler_id, &index_param, &changed_index);
	ae_param_ptr->calc_cur_index = changed_index;

	// min index
	min_shutter=100000000/(ae_param_ptr->min_exposure*ae_param_ptr->line_time);
	i=0x100*iso;
	do {
		if((ae_param_ptr->tab[mode].e_ptr[i]<=min_shutter)
			||(end_index<=i)) {
			break ;
		}
		i++;
	}while(1);

	ae_param_ptr->calc_min_index=i-0x100*iso;

	// max index
	if(0x00!=fps) {
		i=0x100*iso;
		fix_shutter=fps*10;

		do {
			if((ae_param_ptr->tab[mode].e_ptr[i]<fix_shutter)
				||(end_index<i)) {
				i--;
				break ;
			}
			i++;
		}while(1);

		max_index=i-0x100*iso;
	} else {
		i=end_index;
	}

	if(max_index!=(uint32_t)ae_param_ptr->calc_max_index) {
		ae_param_ptr->calc_max_index=max_index;
	}

	if(ae_param_ptr->calc_max_index<ae_param_ptr->calc_min_index) {
		ae_param_ptr->calc_min_index=ae_param_ptr->calc_max_index;
	}

	if(max_index<(uint32_t)ae_param_ptr->calc_cur_index) {
		ae_param_ptr->calc_cur_index=max_index;
	}

	ae_param_ptr->frame_info_eb = ISP_EB;

	//ISP_LOG("iso:%d, cur_index:%d, max_index:%d\n",iso, ae_param_ptr->cur_index, ae_param_ptr->max_index);

	return rtn;
}

uint32_t _ispCalcRangeFps(uint32_t handler_id, uint32_t mode, uint32_t iso)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_context* isp_context_ptr = ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr = &isp_context_ptr->ae;
	uint32_t max_shutter = 0;
	uint32_t min_shutter = 0;
	uint32_t max_index = 0;
	uint32_t min_fps = 0;
	uint32_t max_fps = 0;
	uint32_t *e_ptr = NULL;
	uint32_t i = 0;
	uint32_t fps_max_index = 0;
	uint32_t fps_min_index = 0;


	ae_param_ptr->min_frame_line = ISP_AE_MAX_LINE;
	ae_param_ptr->max_frame_line = ISP_AE_MAX_LINE;
	min_fps = ae_param_ptr->range_fps.min_fps;
	max_fps = ae_param_ptr->range_fps.max_fps;

	e_ptr = &ae_param_ptr->tab[mode].e_ptr[0x100*iso];
	max_index = ae_param_ptr->tab[mode].max_index[iso];

	ISP_LOG("min_fps=%d, max_fps=%d, max_index=%d, mode=%d, iso=%d",
			min_fps, max_fps, max_index, mode, iso);

	if (max_index >= 256) {
		max_index = 255;
	}

	fps_min_index = ae_param_ptr->calc_min_index;
	fps_max_index = ae_param_ptr->calc_max_index;

	//min_fps ==> max index
	if (min_fps) {
		min_shutter = min_fps * 10;
		i = max_index;
		while (i) {
			//ISP_LOG("e_ptr[i]=0x%x", e_ptr[i]);
			if (e_ptr[i] >= min_shutter) {
				break ;
			}
			--i;
		}
		fps_max_index = i;
	}


	ISP_LOG("fps min_index=%d,max_index=%d",
		fps_min_index, fps_max_index);

	if (fps_max_index < fps_min_index) {
		fps_min_index = fps_max_index;
	}

	if (max_fps) {
		ae_param_ptr->min_frame_line = _ispGetFrameLine(handler_id, isp_context_ptr->ae.line_time, max_fps);
	}
	if (min_fps) {
		fps_max_index = MIN(fps_max_index, (uint32_t)ae_param_ptr->calc_max_index);
		ae_param_ptr->calc_max_index = fps_max_index;
		if (fps_max_index < (uint32_t)ae_param_ptr->calc_cur_index) {
			ae_param_ptr->calc_cur_index = fps_max_index;
		}
		ae_param_ptr->max_frame_line = _ispGetFrameLine(handler_id, isp_context_ptr->ae.line_time, min_fps);
	}
	ae_param_ptr->frame_info_eb = ISP_EB;

	ISP_LOG("calc cur_index=%d,min_index=%d,min_frame=%d,max_index=%d, max_frame=%d",
		ae_param_ptr->calc_cur_index,
		ae_param_ptr->calc_min_index,
		ae_param_ptr->min_frame_line,
		ae_param_ptr->calc_max_index,
		ae_param_ptr->max_frame_line);

	return rtn;
}

/* _ispAeInfoSet --
*@
*@
*@ return:
*/
uint32_t _ispAeInfoSet(uint32_t handler_id)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr=&isp_context_ptr->ae;
	uint32_t line_time=isp_context_ptr->ae.line_time;
	uint32_t mode=0x00;
	uint32_t iso=0x00;
	uint32_t fps=0x00;

	/*
	ISP_NORMAL_50HZ=0x00,
	ISP_NORMAL_60HZ,
	ISP_NIGHT_50HZ,
	ISP_NIGHT_60HZ,
	*/
	if(ISP_FLICKER_50HZ==ae_param_ptr->flicker){
		if(ISP_NIGHT==ae_param_ptr->mode)
		{
			ae_param_ptr->tab_type=ISP_NIGHT_50HZ;
		}
		else
		{
			ae_param_ptr->tab_type=ISP_NORMAL_50HZ;
		}
	}
	else if(ISP_FLICKER_60HZ==ae_param_ptr->flicker)
	{
		if(ISP_NIGHT==ae_param_ptr->mode)
		{
			ae_param_ptr->tab_type=ISP_NIGHT_60HZ;
		}
		else
		{
			ae_param_ptr->tab_type=ISP_NORMAL_60HZ;
		}
	}

	/*
	ISP_ISO_AUTO=0x00,
	ISP_ISO_100,
	ISP_ISO_200,
	ISP_ISO_400,
	ISP_ISO_800,
	ISP_ISO_1600,
	*/
	ae_param_ptr->iso=ae_param_ptr->cur_iso;

	/*fps*/
	if(ISP_NIGHT==ae_param_ptr->mode)
	{
		ae_param_ptr->fix_fps=ae_param_ptr->night_fps;
	}
	else
	{
		ae_param_ptr->fix_fps=ae_param_ptr->normal_fps;
	}

	if(ISP_AE_FIX==ae_param_ptr->frame_mode)
	{
		ae_param_ptr->fix_fps=ae_param_ptr->sport_fps;
	}

	if(ISP_ZERO!=ae_param_ptr->video_fps)
	{
		ae_param_ptr->fix_fps=ae_param_ptr->video_fps;
	}

	mode=ae_param_ptr->tab_type;
	iso=ae_param_ptr->iso;
	fps=ae_param_ptr->fix_fps;

	_ispSetFixFrameMaxIndex(handler_id, mode, iso, fps);
	ae_param_ptr->frame_line=_ispGetFrameLine(handler_id, line_time, fps);

	_ispCalcRangeFps(handler_id, mode, iso);
	return rtn;
}

/* _ispGetAeIndexIsMax --
*@
*@
*@ return:
*/
int32_t _ispGetAeIndexIsMax(uint32_t handler_id, uint32_t* eb)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_context* isp_context_ptr = ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr = &isp_context_ptr->ae;
	uint32_t ae_mode = ae_param_ptr->tab_type;
	uint32_t iso = ae_param_ptr->iso;
	uint32_t cur_index = ae_param_ptr->cur_index;
	uint32_t max_index = ae_param_ptr->tab[ae_mode].max_index[iso];

	*eb=ISP_ZERO;

	if(max_index==cur_index)
	{
		*eb=ISP_ONE;
	}

	ISP_LOG("Ae is max index: 0x%x",*eb);

	return rtn;
}

int32_t isp_GetAEVersion(void)
{
	int32_t rtn=ISP_SUCCESS;
	uint32_t handler_id=0;

	ISP_LOG("--AE INIT-- soft id:0x%08x", AE_SOFT_ID);

	return rtn;
}

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

