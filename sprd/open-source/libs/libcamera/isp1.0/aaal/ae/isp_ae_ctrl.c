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
#include <utils/Timers.h>
#include "isp_alg.h"
#include "isp_ae_alg_v00.h"
#include "isp_com.h"
#include "isp_ae_ctrl.h"

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

#define ISP_ID_INVALID 0xff
#define FRAME_RATE_20 20
// RGB to YUV
//Y = 0.299 * r + 0.587 * g + 0.114 * b
#define RGB_TO_Y(_r, _g, _b)	(int32_t)((77 * (_r) + 150 * (_g) + 29 * (_b)) >> 8)
//U = 128 - 0.1687 * r - 0.3313 * g + 0.5 * b
#define RGB_TO_U(_r, _g, _b)	(int32_t)(128 + ((128 * (_b) - 43 * (_r) - 85 * (_g)) >> 8))
//V = 128 + 0.5 * r - 0.4187 * g - 0.0813 * b
#define RGB_TO_V(_r, _g, _b)	(int32_t)(128  + ((128 * (_r) - 107 * (_g) - 21 * (_b)) >> 8))

/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/
enum isp_ae_status{
	ISP_AE_CLOSE=0x00,
	ISP_AE_IDLE,
	ISP_AE_RUN,
	ISP_AE_STATUS_MAX
};
/**---------------------------------------------------------------------------*
**				extend Variables and function			*
**---------------------------------------------------------------------------*/
static int32_t _isp_ae_fast_callback(uint32_t handler_id, int32_t eb);
static int32_t _isp_ae_set_gain(uint32_t handler_id, uint32_t gain);
static int32_t _isp_ae_set_exposure(uint32_t handler_id, uint32_t exposure, uint32_t dummy);

/**---------------------------------------------------------------------------*
**				Local Variables					*
**---------------------------------------------------------------------------*/
static pthread_mutex_t s_exp_ctrl_mutex={0x00};

/**---------------------------------------------------------------------------*
**				Constant Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/

/* _ae_ExpCtrlLock --
*@
*@
*@ return:
*/
static int32_t _ae_ExpCtrlLock(void)
{
	int32_t rtn = ISP_SUCCESS;

	rtn = pthread_mutex_lock(&s_exp_ctrl_mutex);

	return rtn;
}

/* _ae_ExpCtrlUnlock --
*@
*@
*@ return:
*/
static int32_t _ae_ExpCtrlUnlock(void)
{
	int32_t rtn=ISP_SUCCESS;

	rtn = pthread_mutex_unlock(&s_exp_ctrl_mutex);

	return rtn;
}


/* _isp_ae_monitor --
*@
*@
*@ return:
*/
static int32_t _isp_ae_monitor(uint32_t handler_id, uint32_t skip_num, void* fun)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	ae_param_ptr->awbm_skip(handler_id, skip_num);
	ae_param_ptr->awbm_bypass(handler_id, 0);

	ISP_LOG("%s, skip:%d", fun, skip_num);

	return rtn;
}

/* _isp_ae_get_v00_init_param_proc --
*@
*@
*@ return:
*/
uint32_t _isp_ae_get_v00_init_param_proc(uint32_t handler_id, void* in_param, void* out_param)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=in_param;
	struct isp_ae_v00_context* ae_v00_param_ptr=&ae_param_ptr->alg_v00_context;
	struct isp_ae_v00_init_param* ae_v00_init_ptr=out_param;

	//memset((void*)ae_v00_param_ptr, ISP_ZERO, sizeof(struct isp_ae_v00_context));

	ae_v00_param_ptr->target_lum = ae_param_ptr->target_lum;
	ae_v00_param_ptr->target_zone = ae_param_ptr->target_zone;
	ae_v00_param_ptr->quick_mode = ae_param_ptr->quick_mode;
	ae_v00_param_ptr->anti_exposure = ae_param_ptr->anti_exposure;

	ae_v00_param_ptr->ev = ae_param_ptr->ev;
	ae_v00_param_ptr->fix_fps = ae_param_ptr->fix_fps;
	ae_v00_param_ptr->frame_line = ae_param_ptr->frame_line;
	ae_v00_param_ptr->line_time = ae_param_ptr->line_time;
	ae_v00_param_ptr->e_ptr = ae_param_ptr->cur_e_ptr;
	ae_v00_param_ptr->g_ptr = ae_param_ptr->cur_g_ptr;
	ae_v00_param_ptr->weight_ptr = ae_param_ptr->cur_weight_ptr;
	ae_v00_param_ptr->max_index = ae_param_ptr->max_index;
	ae_v00_param_ptr->min_index = ae_param_ptr->min_index;
	ae_v00_param_ptr->cur_index = ae_param_ptr->cur_index;
	ae_v00_param_ptr->min_frame_line = ae_param_ptr->min_frame_line;
	ae_v00_param_ptr->max_frame_line = ae_param_ptr->max_frame_line;

	ae_v00_param_ptr->set_exposure = _isp_ae_set_exposure;
	ae_v00_param_ptr->set_gain = _isp_ae_set_gain;
	ae_v00_param_ptr->ae_fast_callback = _isp_ae_fast_callback;
	ae_v00_param_ptr->flash_calc = isp_flash_calculation;
	ae_v00_param_ptr->real_gain = isp_ae_get_real_gain;

	ae_v00_init_ptr->context_ptr=ae_v00_param_ptr;

	return rtn;
}


/* isp_ae_get_v00_frame_info --
*@
*@
*@ return:
*/
static int32_t _isp_ae_get_v00_frame_info(uint32_t handler_id, struct isp_ae_v00_frame_info* param_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);

	param_ptr->weight_ptr = ae_param_ptr->cur_weight_ptr;
	param_ptr->fix_fps = ae_param_ptr->fix_fps;
	param_ptr->frame_line = ae_param_ptr->frame_line;
	param_ptr->line_time = ae_param_ptr->line_time;
	param_ptr->e_ptr = ae_param_ptr->cur_e_ptr;
	param_ptr->g_ptr = ae_param_ptr->cur_g_ptr;
	param_ptr->max_index = ae_param_ptr->max_index;
	param_ptr->min_index = ae_param_ptr->min_index;
	param_ptr->min_frame_line = ae_param_ptr->min_frame_line;
	param_ptr->max_frame_line = ae_param_ptr->max_frame_line;

	return rtn;
}

/* _isp_ae_clr_write_status --
*@
*@
*@ return:
*/
static int32_t _isp_ae_clr_write_status(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);

	ae_param_ptr->exposure_skip_conter = ISP_ZERO;
	ae_param_ptr->again_skip_conter = ISP_AE_SKIP_LOCK;
	ae_param_ptr->dgain_skip_conter = ISP_AE_SKIP_LOCK;

	return rtn;
}
/* _isp_ae_set_exposure_gain --
*@
*@
*@ return:
*/
uint32_t _isp_ae_set_exposure_gain_prehigh(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	if (ISP_EB == ae_param_ptr->ae_set_eb) {

		switch (ae_param_ptr->alg_id) {
			case 0:
			{
				rtn = isp_ae_v00_set_exposure_prehigh(handler_id);
				rtn = isp_ae_v00_set_gain(handler_id);
				break;
			}
			default :
				break;
		}

		ae_param_ptr->ae_set_eb = ISP_UEB;
		_isp_ae_clr_write_status(handler_id);
	}

	return rtn;
}


/* _isp_ae_set_status --
*@
*@
*@ return:
*/
static int32_t _isp_ae_set_status(uint32_t handler_id, uint32_t status)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);

	ae_param_ptr->status = status;

	return rtn;
}

/* _isp_ae_get_status --
*@
*@
*@ return:
*/
static int32_t _isp_ae_get_status(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);

	return ae_param_ptr->status;
}

/* _isp_ae_fast_callback --
*@
*@
*@ return:
*/
static int32_t _isp_ae_fast_callback(uint32_t handler_id, int32_t eb)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	uint32_t ae_stab=0x00;

	if ((ISP_SUCCESS==rtn)
		&&(ISP_EB==ae_param_ptr->fast_ae_get_stab)) {
		ISP_LOG("callback ISP_FAST_AE_STAB_CALLBACK");
		ISP_LOG("hait: stab: cur_idx: %d, cur_lum; %d\n", ae_param_ptr->cur_index, ae_param_ptr->cur_lum);
		ae_param_ptr->callback(handler_id, ISP_CALLBACK_EVT|ISP_FAST_AE_STAB_CALLBACK, (void*)&ae_stab, sizeof(uint32_t));
		ae_param_ptr->fast_ae_get_stab=ISP_UEB;
	}

	return rtn;
}

static int32_t _isp_ae_flash_callback(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	uint32_t flash_stab=0x00;

	ISP_LOG("callback ISP_FLASH_AE_CALLBACK");
	ae_param_ptr->callback(handler_id, ISP_CALLBACK_EVT|ISP_FLASH_AE_CALLBACK, (void*)&flash_stab, sizeof(uint32_t));

	return rtn;
}

/* _isp_ae_set_exposure --
*@
*@
*@ return:
*/
static int32_t _isp_ae_set_exposure(uint32_t handler_id, uint32_t exposure, uint32_t dummy)
{
	uint32_t rtn = ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	uint32_t ae_exposure = exposure;

	//ISP_LOG("index:%d, line:%d", ae_param_ptr->cur_index, ae_param_ptr->cur_exposure);
	if(PNULL!=ae_param_ptr->write_exposure) {
		ae_param_ptr->cur_exposure = exposure;
		ae_param_ptr->cur_dummy = dummy;
		ae_exposure = exposure&0x0000ffff;
		ae_exposure |= (dummy<<0x10)&0x0fff0000;
		ae_exposure |= (ae_param_ptr->param_index<<0x1c)&0xf0000000;
		ae_param_ptr->write_exposure(ae_exposure);
	} else {
		ISP_LOG("write_ae_value null error");
	}

	return rtn;
}

/* _isp_ae_set_gain --
*@
*@
*@ return:
*/
static int32_t _isp_ae_set_gain(uint32_t handler_id, uint32_t gain)
{
	uint32_t rtn = ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	uint32_t ae_gain = gain;

	//ISP_LOG("index:%d, gain:0x%x", ae_param_ptr->cur_index, ae_param_ptr->cur_gain);

	if(PNULL!=ae_param_ptr->write_gain) {
		ae_param_ptr->cur_gain = gain;
		ae_param_ptr->write_gain(ae_gain);
	} else {
		ISP_LOG("write_gain_value null error");
	}

	return rtn;
}

/* _isp_ae_change_expos_gain --
*@
*@
*@ return:
*/
static uint32_t _isp_ae_change_expos_gain(uint32_t handler_id, uint32_t shutter, uint32_t dummy, uint32_t again, uint32_t dgain)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			rtn = isp_ae_v00_change_expos_gain(handler_id, shutter, dummy, again, dgain);
			break;
		}
		default :
			break;
	}

	ae_param_ptr->ae_set_eb=ISP_EB;

	return rtn;
}

/* _isp_ae_set_real_iso --
*@
*@
*@ return:
*/
static int32_t _isp_ae_set_real_iso(uint32_t handler_id, uint32_t iso, uint32_t cur_gain)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);

	if (ISP_ISO_AUTO == iso) {
		ae_param_ptr->real_iso = (cur_gain*50/32)/10*10;
	} else {
		ae_param_ptr->real_iso = (1<<(iso-1)) * 100;
	}

	return rtn;
}

/* _isp_set_exp_gain_proc_param --
*@
*@
*@ return:
*/
static uint32_t _isp_set_exp_gain_proc_param(uint32_t handler_id,void* in_param)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	struct isp_ae_v00_calc_out_param* out_info = (struct isp_ae_v00_calc_out_param*)in_param;
	struct isp_ae_v00_exp_info* ae_exp_info = (struct isp_ae_v00_exp_info*)&out_info->exp;
	uint8_t i = 0;

	if (!ae_exp_info) {
		ISP_LOG("fail.");
		memset((void*)&ae_param_ptr->write_ctrl, 0x00, sizeof(struct isp_ae_exp_ctrl));
		return ISP_PARAM_NULL;
	}

	memcpy((void*)&ae_param_ptr->write_ctrl.exp, (void*)ae_exp_info, sizeof(struct isp_ae_exp_info));
	ae_param_ptr->write_ctrl.conter = 0;

	if (0 != ae_param_ptr->write_ctrl.exp.num) {
		ae_param_ptr->write_ctrl.eb = 1;
	} else {
		if (ae_param_ptr->cur_index != out_info->cur_index) {
			ae_param_ptr->cur_index = out_info->cur_index;
		}
	}

	EXIT :

	return rtn;
}

/* isp_exp_gain_proc --
*@
*@
*@ return:
*/
uint32_t isp_exp_gain_proc(uint32_t handler_id, uint32_t proc_mode, uint64_t system)
{

	//set frame info
	_isp_ae_set_frame_info(handler_id);

	_ae_ExpCtrlLock();
	{
		uint32_t rtn=ISP_SUCCESS;
		struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
		uint32_t exp_num = 0;
		uint32_t exp = 0;
		uint32_t dummy = 0;
		uint32_t again = 0;
		uint32_t skip_num = ae_param_ptr->skip_frame;
		uint32_t valid_index = 0;
		static nsecs_t system_time;

		if (!ae_param_ptr) {
			ISP_LOG("fail.");
			goto EXIT;
		}

		if (1 == ae_param_ptr->write_ctrl.eb && ae_param_ptr->write_ctrl.exp.num) {

			if ((ae_param_ptr->write_ctrl.system_time > system)
				&&(proc_mode == ISP_AE_SOF)) {
				ISP_LOG("AE_TEST:-----^_^----prv_write:%lld, sof:%lld\n", ae_param_ptr->write_ctrl.system_time, system);
				ae_param_ptr->write_ctrl.system_time = 0;
				goto EXIT;
			}

			ISP_LOG("conter:%d,num:%d", ae_param_ptr->write_ctrl.conter, ae_param_ptr->write_ctrl.exp.num);
			if (ae_param_ptr->write_ctrl.conter != ae_param_ptr->write_ctrl.exp.num) {
				exp_num = ae_param_ptr->write_ctrl.conter;
				exp = ae_param_ptr->write_ctrl.exp.tab[exp_num].line;
				dummy = ae_param_ptr->write_ctrl.exp.tab[exp_num].dummy;
				again = ae_param_ptr->write_ctrl.exp.tab[exp_num].again;
				ae_param_ptr->write_ctrl.conter++;

				_isp_ae_set_exposure(handler_id, exp, dummy);
				_isp_ae_set_gain(handler_id, again);

				system_time = systemTime(CLOCK_MONOTONIC);
				ae_param_ptr->write_ctrl.system_time = (uint64_t)(system_time/1000);
			}

			if (ae_param_ptr->write_ctrl.conter == ae_param_ptr->write_ctrl.exp.num) {
				ae_param_ptr->write_ctrl.eb = 0;
				if (1 < ae_param_ptr->write_ctrl.exp.num) {
					valid_index = ae_param_ptr->write_ctrl.exp.num-2;
					skip_num = 0;
				}

				ae_param_ptr->write_ctrl.valid_index = ae_param_ptr->write_ctrl.exp.tab[valid_index].index;
				rtn = _isp_ae_monitor(handler_id, skip_num, __FUNCTION__);
			}

		}

		EXIT :

		_ae_ExpCtrlUnlock();

		return rtn;
	}

}

/* isp_ae_get_gain--
*@
*@
*@ return:
*/
int32_t isp_ae_get_gain(uint32_t handler_id, uint32_t cur_index)
{// x=real_gain/32
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	struct isp_ae_v00_context* ae_v00_param_ptr=&ae_param_ptr->alg_v00_context;
	uint32_t real_gain = 0;

	real_gain = ae_v00_param_ptr->g_ptr[cur_index];

	real_gain =  isp_ae_get_real_gain(real_gain);

	return real_gain;
}

/* _isp_ae_proc --
*@
*@
*@ return:
*/
static uint32_t _isp_ae_proc(uint32_t handler_id, int32_t cur_index, int32_t cur_lum, int32_t cur_ev, int32_t fast_end)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	uint32_t iso;
	int32_t max_index;
	uint32_t ae_stab=0x00;

	//ISP_LOG("ae--_isp_ae_proc-0-bypass:%d \n", ae_param_ptr->bypass);
	if (!ae_param_ptr) {
		ISP_LOG("fail.");
		return ISP_PARAM_NULL;
	}
	iso=ae_param_ptr->iso;
	max_index=ae_param_ptr->max_index;
	_isp_ae_set_real_iso(handler_id, iso, isp_ae_get_gain(handler_id, cur_index));

	isp_ae_fast_smart_adjust(handler_id, cur_ev, fast_end);

	//isp_ae_flash_save_index(handler_id, ae_param_ptr->cur_index, ae_param_ptr->cur_lum);
	
	if(ae_param_ptr->cur_index!=cur_index) {
		ae_param_ptr->cur_index=cur_index;
		//isp_exp_gain_proc(handler_id, ISP_AE_DIR, 0);

		ae_param_ptr->stab_conter=ISP_ZERO;
		ae_param_ptr->stab=ISP_UEB;

		if(ISP_EB==ae_param_ptr->ae_get_change) {
			ISP_LOG("callback ISP_AE_CHG_CALLBACK");
			ae_param_ptr->callback(handler_id, ISP_CALLBACK_EVT|ISP_AE_CHG_CALLBACK, (void*)&ae_stab, sizeof(uint32_t));
			ae_param_ptr->ae_get_change=ISP_UEB;
		}
	} else if ((ae_param_ptr->cur_index ==cur_index) && (abs(ae_param_ptr->cur_lum - cur_lum) < 8)){
		ae_param_ptr->cur_skip_num=ae_param_ptr->skip_frame;
		ae_param_ptr->ae_set_eb=ISP_UEB;

		rtn = _isp_ae_monitor(handler_id, 0, __FUNCTION__);

		if(AE_STAB_NUM>ae_param_ptr->stab_conter) {
			ae_param_ptr->stab_conter++;
		}

		if(AE_STAB_NUM==ae_param_ptr->stab_conter) {

			if (( PNULL != ae_param_ptr->self_callback)
				&& (ISP_EB==ae_param_ptr->ae_get_stab)) {
				ISP_LOG("callback ISP_AE_STAB_CALLBACK");
				ae_param_ptr->self_callback(handler_id, ISP_AE_STAB_CALLBACK, NULL, ISP_ZERO);
				ae_param_ptr->ae_get_stab=ISP_UEB;
			}

			ae_param_ptr->stab_conter = ISP_ZERO;
			ae_param_ptr->stab=ISP_EB;
			ae_param_ptr->auto_eb=ISP_EB;
		}

		if((ISP_ZERO!=ae_param_ptr->smart)
			&&(ISP_EB==ae_param_ptr->auto_eb)) {
			isp_ae_stab_smart_adjust(handler_id, cur_ev);
			ae_param_ptr->auto_eb=ISP_UEB;
		}
	}

	EXIT :

	return rtn;
}


/* isp_ae_set_frame_info --
*@
*@
*@ return:
*/
int32_t _isp_ae_set_frame_info(uint32_t handler_id)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	uint32_t frame_info_eb = ISP_UEB;
	uint32_t update_ae_eb = ISP_UEB;

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	if (ISP_AE_IDLE == _isp_ae_get_status(handler_id)) {

		if(ISP_EB == ae_param_ptr->frame_info_eb) {
			frame_info_eb = ISP_EB;
			update_ae_eb = ISP_EB;
			isp_ae_set_index(handler_id, ae_param_ptr->calc_cur_index);
			ae_param_ptr->max_index = ae_param_ptr->calc_max_index;
			ae_param_ptr->min_index = ae_param_ptr->calc_min_index;
			ae_param_ptr->frame_info_eb = ISP_UEB;
		}

		if(ISP_EB == ae_param_ptr->weight_eb) {
			frame_info_eb = ISP_EB;
		}

		if(ISP_EB == frame_info_eb) {
			switch (ae_param_ptr->alg_id) {
				case 0:
				{
					struct isp_ae_v00_frame_info frame_info;
					_isp_ae_get_v00_frame_info(handler_id, &frame_info);
					rtn = isp_ae_v00_set_frame_info(handler_id, &frame_info);
					break;
				}
				default :
					break;
			}
		}

		if (ISP_EB == update_ae_eb) {
			isp_ae_update_expos_gain(handler_id);
			_isp_ae_set_exposure_gain(handler_id);
		}

	}

	return rtn;
}


/* _isp_ae_set_exposure_gain --
*@
*@
*@ return:
*/
uint32_t _isp_ae_set_exposure_gain(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	if (ISP_EB == ae_param_ptr->ae_set_eb) {

		switch (ae_param_ptr->alg_id) {
			case 0:
			{
				rtn = isp_ae_v00_set_exposure(handler_id);
				rtn = isp_ae_v00_set_gain(handler_id);
				break;
			}
			default :
				break;
		}

		/*
		if(PNULL != ae_param_ptr->write_dgain) {
			ae_param_ptr->write_dgain(ae_param_ptr->cur_dgain);
		}*/

		ae_param_ptr->ae_set_eb = ISP_UEB;
		_isp_ae_clr_write_status(handler_id);
	}

	return rtn;
}



/* isp_ae_init_context --
*@
*@
*@ return:
*/
int32_t isp_ae_init_context(uint32_t handler_id, void *cxt)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = NULL;

	if (ispSetAeContext(handler_id,(void *)cxt)){
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	isp_ae_v00_init_context(handler_id,&ae_param_ptr->alg_v00_context);


	return rtn;
}


/* isp_ae_init --
*@
*@
*@ return:
*/
uint32_t isp_ae_init(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr = NULL;
	struct isp_flash_param* flash_param_ptr = NULL;

	ae_param_ptr = ispGetAeContext(handler_id);
	flash_param_ptr = &ae_param_ptr->flash;
	ae_param_ptr->alg_id=0x00;

	_isp_ae_set_status(handler_id, ISP_AE_CLOSE);

	isp_GetAEVersion();

	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			struct isp_ae_v00_init_param ae_v00_init_param;
			_isp_ae_get_v00_init_param_proc(handler_id, (void*)ae_param_ptr, (void*)&ae_v00_init_param);
			isp_ae_v00_init(handler_id, (void*)&ae_v00_init_param);
			break;
		}
		default :
			break;
	}

	ae_param_ptr->ae_get_stab = ISP_EB;
	ae_param_ptr->ae_get_change = ISP_UEB;
	ae_param_ptr->stab_conter = ISP_ZERO;
	ae_param_ptr->monitor_conter = ae_param_ptr->skip_frame;
	ae_param_ptr->smart_sta_precent = ISP_ZERO;
	flash_param_ptr->effect = ISP_ZERO;
	ae_param_ptr->write_ctrl.valid_index = ae_param_ptr->cur_index;

	ae_param_ptr->auto_eb = ISP_UEB;
	ae_param_ptr->ae_set_eb = ISP_EB;
	isp_ae_update_expos_gain(handler_id);
	_isp_ae_set_exposure_gain(handler_id);

	ae_param_ptr->cur_skip_num=ISP_AE_SKIP_FOREVER;
	ae_param_ptr->init=ISP_EB;
	isp_ae_lock_init(handler_id, __FUNCTION__);
	_isp_ae_set_status(handler_id, ISP_AE_IDLE);

	if (ISP_UEB == ae_param_ptr->back_bypass) {
		ae_param_ptr->monitor_bypass=ISP_UEB;
		rtn = _isp_ae_monitor(handler_id, 0, __FUNCTION__);
	}
	ae_param_ptr->is_save_index = 0;

	return rtn;
}

/* isp_ae_deinit --
*@
*@
*@ return:
*/
uint32_t isp_ae_deinit(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			isp_ae_v00_deinit(handler_id, NULL);
			break;
		}
		default :
			break;
	}

	_isp_ae_set_status(handler_id, ISP_AE_CLOSE);

	ae_param_ptr->init=ISP_UEB;

	return rtn;
}


/* _isp_ae_cal --
*@
*@
*@ return:
*/
uint32_t isp_ae_calculation(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	uint32_t stab = ISP_EB;
//	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	struct isp_context *isp_cxt_ptr = (struct isp_context*)ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr = ispGetAeContext(handler_id);
	struct isp_awb_param *awb_param_ptr = &isp_cxt_ptr->awb;

	struct isp_get_lum_param get_lum_param;
	uint32_t wYlayer=0x00;
	int32_t ae_cur_index = 0x00;
	int32_t ae_cur_lum = 0x00;
	int32_t ae_cur_ev = 0x00;
	int32_t ae_fast_end = 0x00;

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	rtn = isp_get_cur_lum(handler_id,&wYlayer, 1);

	ae_param_ptr->cur_lum = wYlayer;
	ae_cur_lum = wYlayer;

	if (ae_param_ptr->ae_skip_calc_num) {
		ISP_LOGI("ae_param_ptr->reserved1 %d", ae_param_ptr->reserved1);
		ae_param_ptr->monitor_bypass=ISP_UEB;
		ae_param_ptr->ae_skip_calc_num --;
		rtn = _isp_ae_monitor(handler_id, 0, __FUNCTION__);
		goto EXIT;
	}
	if(ISP_EB == ae_param_ptr->bypass) {
		rtn = _isp_ae_monitor(handler_id, 0, __FUNCTION__);
		goto EXIT;
	}

	_isp_ae_set_status(handler_id, ISP_AE_RUN);

	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			struct isp_ae_v00_calc_param ae_v00_calc = {0};
			struct isp_ae_v00_calc_out_param ae_v00_calc_out = {0};
			ae_v00_calc.cur_lum=wYlayer;
			ae_v00_calc.valid_index=ae_param_ptr->write_ctrl.valid_index;
			rtn = isp_ae_v00_calculation(handler_id, (void*)&ae_v00_calc, (void*)&ae_v00_calc_out);
			ae_cur_index = ae_v00_calc_out.cur_index;
			ae_cur_ev = ae_v00_calc_out.cur_ev;
			ae_fast_end = ae_v00_calc_out.fast_end;
			_isp_set_exp_gain_proc_param(handler_id, (void*)&ae_v00_calc_out);
			ISP_LOG("exp.num:%d, index:(%d,%d,%d), wYlayer:%d", ae_param_ptr->write_ctrl.exp.num
					,ae_param_ptr->write_ctrl.valid_index, ae_param_ptr->cur_index, ae_cur_index, wYlayer);
			break;
		}
		default :
			break;
	}

	if (ISP_SUCCESS == rtn) {
		_isp_ae_proc(handler_id, ae_cur_index, ae_cur_lum, ae_cur_ev, ae_fast_end);
	}
	if (ae_param_ptr->ae_set_cb_ext) {
		ae_param_ptr->callback(handler_id, ISP_CALLBACK_EVT|ISP_FLASH_CAP_PROC_CALLBACK, (void*)&stab, sizeof(uint32_t));
		isp_ae_set_stab_ext(handler_id,ISP_UEB);
	}

	EXIT :

	_isp_ae_set_status(handler_id, ISP_AE_IDLE);

	return rtn;
}

/* isp_ae_update_expos_gain --
*@
*@
*@ return:
*/
uint32_t isp_ae_update_expos_gain(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			rtn = isp_ae_v00_update_expos_gain(handler_id);
			break;
		}
		default :
			break;
	}

	ae_param_ptr->ae_set_eb=ISP_EB;

	return rtn;
}

/* isp_ae_set_exposure_gain --
*@
*@
*@ return:
*/
uint32_t isp_ae_set_exposure_gain(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	//set frame info
	_isp_ae_set_frame_info(handler_id);

	if (ISP_EB == ae_param_ptr->ae_set_eb) {

		switch (ae_param_ptr->alg_id) {
			case 0:
			{
				rtn = isp_ae_v00_set_exposure(handler_id);
				rtn = isp_ae_v00_set_gain(handler_id);
				break;
			}
			default :
				break;
		}
		ae_param_ptr->monitor_bypass=ISP_UEB;
	}

	ae_param_ptr->ae_set_eb = ISP_UEB;

	return rtn;
}

/* isp_ae_flash_eb --
*@
*@
*@ return:
*/
int32_t isp_ae_flash_eb(uint32_t handler_id, uint32_t eb)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			rtn = isp_ae_v00_flash(handler_id, eb);
			break;
		}
		default :
			break;
	}

	return rtn;
}

int32_t isp_ae_flash_reback_index(uint32_t handler_id)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			rtn = isp_ae_v00_flash_reback_index(handler_id,&ae_param_ptr->cur_index, &ae_param_ptr->cur_lum);
			ae_param_ptr->write_ctrl.valid_index = ae_param_ptr->cur_index;
			isp_ae_set_index(handler_id, ae_param_ptr->cur_index);
			isp_ae_update_expos_gain(handler_id);
			_isp_ae_set_exposure_gain(handler_id);
			break;
		}
		default :
			break;
	}

	return rtn;
}

int32_t isp_ae_flash_save_index(uint32_t handler_id, uint32_t cur_index, uint32_t cur_lum)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	if (ae_param_ptr->is_save_index) {
		ae_param_ptr->is_save_index = 0;
		switch (ae_param_ptr->alg_id) {
			case 0:
			{
				rtn = isp_ae_v00_flash_save_index(handler_id, cur_index, cur_lum);
				break;
			}
			default :
				break;
		}
		ae_param_ptr->callback(handler_id, ISP_CALLBACK_EVT|ISP_AE_STAB_CALLBACK, NULL, 0);
	}

	return rtn;
}

/* isp_ae_set_monitor_size --
*@
*@
*@ return:
*/
int32_t isp_ae_set_monitor_size(uint32_t handler_id, void* param_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_size* size_ptr = param_ptr;

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	return rtn;
}

/* isp_ae_set_alg --
*@
*@
*@ return:
*/
int32_t isp_ae_set_alg(uint32_t handler_id, uint32_t mode)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			rtn = isp_ae_v00_set_alg(handler_id, mode);
			break;
		}
		default :
			break;
	}

	return rtn;
}

/* isp_ae_set_ev --
*@
*@
*@ return:
*/
int32_t isp_ae_set_ev(uint32_t handler_id, int32_t ev)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			rtn = isp_ae_v00_set_ev(handler_id, ev);
			break;
		}
		default :
			break;
	}

	return rtn;
}


/* isp_ae_set_index --
*@
*@
*@ return:
*/
int32_t isp_ae_set_index(uint32_t handler_id, uint32_t index)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			rtn = isp_ae_v00_set_index(handler_id, index);
			break;
		}
		default :
			break;
	}

	ae_param_ptr->cur_index = index;

	return rtn;
}

/* isp_ae_set_flash_exposure_gain --
*@
*@
*@ return:
*/
uint32_t isp_ae_set_flash_exposure_gain(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_flash_param* flash_param_ptr;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	flash_param_ptr=&ae_param_ptr->flash;

	ISP_LOG("flash_param_ptr->next_index= %d", flash_param_ptr->next_index);
	isp_ae_set_index(handler_id, flash_param_ptr->next_index);
	isp_ae_update_expos_gain(handler_id);
	_isp_ae_set_exposure_gain(handler_id);
	isp_ae_set_index(handler_id, flash_param_ptr->prv_index);

	return rtn;
}

/* isp_ae_set_flash_exposure_gain_prehigh --
*@
*@
*@ return:
*/
uint32_t isp_ae_set_flash_exposure_gain_prehigh(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_flash_param* flash_param_ptr;

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	flash_param_ptr=&ae_param_ptr->flash;

	ISP_LOG("flash_param_ptr->next_index = %d", flash_param_ptr->next_index);
	isp_ae_set_index(handler_id, flash_param_ptr->next_index);
	isp_ae_update_expos_gain(handler_id);

	ae_param_ptr->alg_v00_context.flash.prv_index = flash_param_ptr->prv_index;
	_isp_ae_set_exposure_gain_prehigh(handler_id);

	return rtn;
}

/* isp_ae_save_iso --
*@
*@
*@ return:
*/
int32_t isp_ae_save_iso(uint32_t handler_id, uint32_t iso)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ae_param_ptr->back_iso=iso;

	return rtn;
}

/* isp_ae_get_save_iso --
*@
*@
*@ return:
*/
uint32_t isp_ae_get_save_iso(uint32_t handler_id)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	return ae_param_ptr->back_iso;
}

/* isp_ae_set_iso --
*@
*@
*@ return:
*/
int32_t isp_ae_set_iso(uint32_t handler_id, uint32_t iso)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ae_param_ptr->cur_iso=iso;

	if (ISP_ISO_AUTO != iso) {
		ae_param_ptr->real_iso=iso;
	}

	return rtn;
}

/* isp_ae_get_iso --
*@
*@
*@ return:
*/
int32_t isp_ae_get_iso(uint32_t handler_id, uint32_t* iso)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	*iso=ae_param_ptr->real_iso;

	return rtn;
}

/* isp_ae_set_fast_stab --
*@
*@
*@ return:
*/
int32_t isp_ae_set_fast_stab(uint32_t handler_id, uint32_t eb)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ae_param_ptr->fast_ae_get_stab=eb;

	return rtn;
}

/* isp_ae_set_stab --
*@
*@
*@ return:
*/
int32_t isp_ae_set_stab(uint32_t handler_id, uint32_t eb)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ae_param_ptr->ae_get_stab=eb;

	return rtn;
}

/* isp_ae_get_stab --
*@
*@
*@ return:
*/
int32_t isp_ae_get_stab(uint32_t handler_id, uint32_t* stab)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	*stab = ae_param_ptr->stab;

	return rtn;
}

/* isp_ae_set_stab_ext --
*@
*@
*@ return:
*/
int32_t isp_ae_set_stab_ext(uint32_t handler_id, uint32_t eb)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ISP_LOG("isp_ae_set_stab_ext eb=%d", eb);
	ae_param_ptr->ae_set_cb_ext = eb;

	return rtn;
}
/* isp_ae_set_change --
*@
*@
*@ return:
*/
int32_t isp_ae_set_change(uint32_t handler_id, uint32_t eb)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ae_param_ptr->ae_get_change=eb;

	return rtn;
}

/* isp_ae_set_param_index --
*@
*@
*@ return:
*/
int32_t isp_ae_set_param_index(uint32_t handler_id, uint32_t index)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ae_param_ptr->param_index=index;

	return rtn;
}

/* isp_ae_ctrl_set --
*@
*@
*@ return:
*/
int32_t isp_ae_ctrl_set(uint32_t handler_id, void* param_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_ae_ctrl* ae_ctrl_ptr = (struct isp_ae_ctrl*)param_ptr;

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	if (ISP_AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode) {
		isp_ae_set_index(handler_id, ae_ctrl_ptr->index);
		isp_ae_update_expos_gain(handler_id);
	} else {
		_isp_ae_change_expos_gain(handler_id, ae_ctrl_ptr->shutter, ae_ctrl_ptr->dummy, ae_ctrl_ptr->again, ae_ctrl_ptr->dgain);
		ae_param_ptr->again_skip = ae_ctrl_ptr->skipa;
		ae_param_ptr->dgain_skip = ae_ctrl_ptr->skipd;
		ae_param_ptr->ae_set_eb=ISP_EB;
	}
	isp_ae_set_exposure_gain(handler_id);

	return rtn;
}

/* isp_ae_ctrl_get --
*@
*@
*@ return:
*/
int32_t isp_ae_ctrl_get(uint32_t handler_id, void* param_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_ae_ctrl* ae_ctrl_ptr = (struct isp_ae_ctrl*)param_ptr;

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ae_ctrl_ptr->index = ae_param_ptr->cur_index;
	ae_ctrl_ptr->lum = ae_param_ptr->cur_lum;
	ae_ctrl_ptr->shutter = ae_param_ptr->cur_exposure;
	ae_ctrl_ptr->dummy= ae_param_ptr->cur_dummy;
	ae_ctrl_ptr->again = ae_param_ptr->cur_gain;
	ae_ctrl_ptr->dgain = ae_param_ptr->cur_dgain;
	ae_ctrl_ptr->skipa = ae_param_ptr->again_skip;
	ae_ctrl_ptr->skipd = ae_param_ptr->dgain_skip;

	return rtn;
}

/* isp_ae_ctrl_get --
*@
*@
*@ return:
*/
int32_t isp_ae_get_ev_lum(uint32_t handler_id)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	uint16_t cur_index = ae_param_ptr->cur_index;
	uint16_t max_index = ae_param_ptr->max_index;
	uint16_t bias_index = ae_param_ptr->lum_cali_index;
	int32_t cur_lum = ae_param_ptr->lum_cali_lux;
	uint32_t target_lum = ae_param_ptr->target_lum - ae_param_ptr->target_zone + ae_param_ptr->ev;
	uint32_t stat_lum = ae_param_ptr->cur_lum;
	uint32_t i=0x00;
	int32_t bv = 0;
	if (max_index == cur_index) {

		if (stat_lum >= ae_param_ptr->target_lum)
			bv = 0;
		else {
			bv = ((int32_t)stat_lum - (int32_t)ae_param_ptr->target_lum) / 8;
		}

	} else {
		if (max_index >= cur_index)
			bv = (max_index - cur_index);
		else
			bv = 0;
	}

	return bv;

}

/* isp_ae_stop_callback_handler --
*@
*@
*@ return:
*/
int32_t isp_ae_stop_callback_handler(uint32_t handler_id)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	uint32_t ae_stab=ISP_NO_READY;

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	if (ISP_EB==ae_param_ptr->fast_ae_get_stab) {
		ISP_LOG("Stop ISP_FAST_AE_STAB_CALLBACK");
		ae_param_ptr->callback(handler_id, ISP_CALLBACK_EVT|ISP_FAST_AE_STAB_CALLBACK, (void*)&ae_stab, sizeof(uint32_t));
		ae_param_ptr->fast_ae_get_stab=ISP_UEB;
	}

	if(ISP_EB==ae_param_ptr->ae_get_stab) {
		ISP_LOG("Stop ISP_AE_STAB_CALLBACK");
		ae_param_ptr->self_callback(handler_id, ISP_CALLBACK_EVT|ISP_AE_STAB_CALLBACK, (void*)&ae_stab, sizeof(uint32_t));
		ae_param_ptr->ae_get_stab=ISP_UEB;
	}

	return rtn;
}

/* isp_ae_set_denosie_level --
*@
*@
*@ return:
*/
int32_t isp_ae_set_denosie_level(uint32_t handler_id, uint32_t level)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	ae_param_ptr->cur_denoise_level=level;

	return rtn;
}

/* isp_ae_get_denosie_level --
*@
*@
*@ return:
*/
int32_t isp_ae_get_denosie_level(uint32_t handler_id, uint32_t* level)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	*level=ae_param_ptr->cur_denoise_level;

	if(AE_SMART_DENOISE==(AE_SMART_DENOISE&ae_param_ptr->smart)) {
		*level |= 0x80000000;
	}

	return rtn;
}

int32_t isp_ae_set_denosie_diswei_level(uint32_t handler_id, uint32_t level)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	ae_param_ptr->cur_denoise_diswei_level=level;

	return rtn;
}


int32_t isp_ae_get_denosie_diswei_level(uint32_t handler_id, uint32_t* level)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	*level=ae_param_ptr->cur_denoise_diswei_level;

	if(AE_SMART_DENOISE==(AE_SMART_DENOISE&ae_param_ptr->smart)) {
		*level |= 0x80000000;
	}

	return rtn;
}

int32_t isp_ae_set_denosie_ranwei_level(uint32_t handler_id, uint32_t level)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	ae_param_ptr->cur_denoise_ranwei_level=level;

	return rtn;
}

int32_t isp_ae_get_denosie_ranwei_level(uint32_t handler_id, uint32_t* level)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	*level=ae_param_ptr->cur_denoise_ranwei_level;

	if(AE_SMART_DENOISE==(AE_SMART_DENOISE&ae_param_ptr->smart)) {
		*level |= 0x80000000;
	}

	return rtn;
}

int32_t isp_ae_set_denoise_diswei(uint32_t handler_id,struct isp_context* isp_context_ptr, uint32_t level)
{
	int32_t rtn=ISP_SUCCESS;
	//struct isp_context* isp_context_ptr = ispGetAlgContext(handler_id);
	uint32_t de_level=level;
	uint32_t prv_de_level=level;

	if (ISP_DENOISE_MAX_LEVEL < de_level) {
		de_level = ISP_DENOISE_MAX_LEVEL;
	}

	isp_ae_get_denosie_diswei_level(handler_id, &prv_de_level);

	if (de_level != (0xff&prv_de_level)) {
		uint8_t *diswei_tab_ptr = PNULL;
		isp_get_denoise_tab(de_level, &diswei_tab_ptr, PNULL);
		memcpy((void*)&isp_context_ptr->denoise.diswei, (void*)diswei_tab_ptr, 19);
		isp_context_ptr->tune.denoise=ISP_EB;
	}

	return rtn;
}

int32_t isp_ae_set_denoise_ranwei(uint32_t handler_id, struct isp_context* isp_context_ptr,uint32_t level)
{
	uint32_t rtn=ISP_SUCCESS;
	//struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	uint32_t de_level=level;
	uint32_t prv_de_level=level;

	if (ISP_DENOISE_MAX_LEVEL < de_level) {
		de_level = ISP_DENOISE_MAX_LEVEL;
	}

	isp_ae_get_denosie_ranwei_level(handler_id, &prv_de_level);

	if (de_level != (0xff&prv_de_level)) {
		uint8_t* ranwei_tab_ptr = PNULL;
		isp_get_denoise_tab(de_level, PNULL, &ranwei_tab_ptr);
		memcpy((void*)&isp_context_ptr->denoise.ranwei, (void*)ranwei_tab_ptr, 31);
		isp_context_ptr->tune.denoise=ISP_EB;
	}

	return rtn;
}



/* isp_ae_set_denoise --
*@
*@
*@ return:
*/
uint32_t isp_ae_set_denoise(uint32_t handler_id, uint32_t level)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	uint32_t de_level=level;
	uint32_t ranwei_lvl = 0;
	uint32_t diswei_lvl = 0;
	uint32_t denoise_lvl = 0;

	if (ISP_DENOISE_MAX_LEVEL > de_level) {
		uint8_t *diswei_tab_ptr = PNULL;
		uint8_t *ranwei_tab_ptr = PNULL;

		isp_get_denoise_tab(de_level, &diswei_tab_ptr, &ranwei_tab_ptr);
		memcpy((void*)&isp_context_ptr->denoise.diswei, (void*)diswei_tab_ptr, 19);
		memcpy((void*)&isp_context_ptr->denoise.ranwei, (void*)ranwei_tab_ptr, 31);
		isp_context_ptr->denoise.bypass = isp_context_ptr->denoise_bak.bypass;
	}else{
		if(ISP_ZERO!=(AE_SMART_DENOISE&ae_param_ptr->smart)) {
			isp_ae_get_denosie_diswei_level(handler_id, &diswei_lvl);
			isp_ae_get_denosie_ranwei_level(handler_id, &ranwei_lvl);

			isp_ae_set_denoise_diswei(handler_id, isp_context_ptr,diswei_lvl&0xff);
			isp_ae_set_denosie_diswei_level(handler_id, diswei_lvl&0xff);
			isp_ae_set_denoise_ranwei(handler_id,isp_context_ptr, ranwei_lvl&0xff);
			isp_ae_set_denosie_ranwei_level(handler_id, ranwei_lvl&0xff);

			if((0 ==(diswei_lvl&0xff)) && (0 == (ranwei_lvl&0xff))){
					ISP_LOG("tune off denoise !!!");
					isp_context_ptr->denoise.bypass = ISP_EB;
			}else{
					ISP_LOG("recover denoise !!!");
					isp_context_ptr->denoise.bypass = isp_context_ptr->denoise_bak.bypass;
			}
		}else{
			uint8_t *diswei_tab_ptr = PNULL;
			uint8_t *ranwei_tab_ptr = PNULL;
			isp_ae_get_denosie_level(handler_id, &denoise_lvl);
			denoise_lvl &= 0xff;

			isp_get_denoise_tab(denoise_lvl, &diswei_tab_ptr, &ranwei_tab_ptr);
			memcpy((void*)&isp_context_ptr->denoise.diswei[0], (void*)diswei_tab_ptr, 19);
			memcpy((void*)&isp_context_ptr->denoise.ranwei[0], (void*)ranwei_tab_ptr, 31);
			isp_context_ptr->denoise.bypass = isp_context_ptr->denoise_bak.bypass;

		}
	}
	isp_context_ptr->tune.denoise=ISP_EB;


	return rtn;
}

/* isp_ae_get_denosie_info --
*@
*@
*@ return:
*/
int32_t isp_ae_get_denosie_info(uint32_t handler_id, uint32_t* param_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	*param_ptr++=ae_param_ptr->prv_noise_info.y_level;
	*param_ptr++=ae_param_ptr->prv_noise_info.uv_level;

	return rtn;
}

int32_t flash_reback_maxidx(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_ae_param *ae_param_ptr=ispGetAeContext(handler_id);

	if (ae_param_ptr->back_max_index_flag) {
		ae_param_ptr->max_index = ae_param_ptr->back_max_index;
		ISP_LOG("reback max index %d", ae_param_ptr->back_max_index);
	}

	return rtn;
}
int32_t ispNotifyAe(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t is_black = 0;
	struct isp_ae_param *ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);
	uint32_t new_index = 0;
	uint32_t cur_index;
	uint32_t cur_exp;

	cur_index = ae_param_ptr->cur_index;
	cur_exp = ae_context_ptr->e_ptr[cur_index];
	ISP_LOG("cur_exp %d", cur_exp);

	if (cur_exp < FRAME_RATE_20*10) {
		ae_param_ptr->back_max_index = ae_param_ptr->max_index;

		_ispGetNewFpsIndex(handler_id, 2, &new_index);
		ae_param_ptr->calc_cur_index = new_index;
		ae_param_ptr->calc_max_index = new_index;
		ae_param_ptr->frame_info_eb = ISP_EB;

		isp_ae_set_index(handler_id, new_index);
		isp_ae_update_expos_gain(handler_id);
		_isp_ae_set_exposure_gain(handler_id);
		ae_param_ptr->back_max_index_flag = 1;
	} else {
		ae_param_ptr->back_max_index_flag = 0;
	}

	return rtn;
}

/* isp_ae_lock_init --
*@
*@
*@ return:
*/
int32_t isp_ae_lock_init(uint32_t handler_id, void* fun)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	ae_param_ptr->lock_conter = 0;
	ae_param_ptr->lock_eb = 0;

	return rtn;
}

/* isp_ae_luck --
*@
*@
*@ return:
*/
int32_t isp_ae_lock(uint32_t handler_id, void* fun)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	ae_param_ptr->lock_conter++;

	if (0 != ae_param_ptr->lock_conter) {
		ae_param_ptr->lock_eb = 1;
	}

	return rtn;
}

/* isp_ae_unluck --
*@
*@
*@ return:
*/
int32_t isp_ae_unlock(uint32_t handler_id, void* fun)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (0 < ae_param_ptr->lock_conter) {
		ae_param_ptr->lock_conter--;
		if (0 == ae_param_ptr->lock_conter) {
			ae_param_ptr->lock_eb = 0;
		}
	} else {
		ae_param_ptr->lock_conter = 0;
		ae_param_ptr->lock_eb = 0;
	}

	return rtn;
}

/* isp_ae_set_skipnum_ext --
*@
*@
*@ return:
*/
int32_t isp_ae_set_skipnum_ext(uint32_t handler_id, uint32_t skipnum)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	ISP_LOG("isp_ae_set_skip_ext skipnum=%d", skipnum);
	ae_param_ptr->ae_skip_calc_num = skipnum;

	return rtn;
}
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/


