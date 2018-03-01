#ifndef _ISP_LSC_ADV_H_
#define _ISP_LSC_ADV_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#include "isp_type.h"

/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/




/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/
typedef void* lsc_adv_handle_t;

enum lsc_gain_calc_mode{
	FUNC_UPDATE_MODE,
	ENVI_UPDATE_MODE,
};

struct seg_curve_param{
	uint32_t center_r;
	uint32_t luma_thr;
	uint32_t chroma_thr;
	uint32_t luma_gain;
	uint32_t chroma_gain;
};

struct flat_change_param{
	uint32_t flat_ratio;
	uint32_t section_num;
	uint32_t last_ratio;
};

struct statistic_image_t{
	uint32_t *r;
	uint32_t *g;
	uint32_t *b;

};

struct lsc_adv_pre_init_param{
	uint32_t correction_intensity;
	uint32_t sphere_gain_num;
	struct seg_curve_param seg_info;
	struct flat_change_param flat_param;

};

struct lsc_adv_pre_calc_param{
	struct statistic_image_t stat_img;
	struct isp_size stat_size;
	struct isp_size block_size;	
};

struct lsc_adv_pre_calc_result {
	uint32_t update;
};

struct lsc_adv_gain_init_param{
	uint32_t percent;
	uint32_t max_gain;
	uint32_t min_gain;   
	uint32_t ct_percent;
	
};

struct lsc_adv_tune_param{
	struct lsc_adv_pre_init_param pre_param;
	struct lsc_adv_gain_init_param gain_param;
	uint32_t debug_level;
};

struct lsc_adv_init_param{
	uint32_t gain_pattern;
	struct lsc_adv_tune_param tune_param;
	uint32_t enable_user_param;
};

struct lsc_adv_calc_param{
	struct isp_size lnc_size;
	uint16_t *fix_gain;
	struct isp_weight_value cur_index;
	uint32_t ct;
};

struct lsc_adv_calc_result{
	uint16_t *dst_gain;

};


/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/

lsc_adv_handle_t lsc_adv_init(struct lsc_adv_init_param *param, void *result);
int32_t lsc_adv_pre_calc(lsc_adv_handle_t handle, struct lsc_adv_pre_calc_param *param, 
							struct lsc_adv_pre_calc_result *result);
int32_t lsc_adv_calculation(lsc_adv_handle_t handle, enum lsc_gain_calc_mode mode, struct lsc_adv_calc_param *param,
							struct lsc_adv_calc_result *result);
int32_t lsc_adv_deinit(lsc_adv_handle_t handle, void *param, void *result);


/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
