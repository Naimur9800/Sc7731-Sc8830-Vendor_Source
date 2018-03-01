#ifndef _ISP_LSC_ADV_H_
#define _ISP_LSC_ADV_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#include <sys/types.h>

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
#define LSC_SECTION_MAX_NUM            0x10
#define LSC_MAX_DEFAULT_NUM            10



/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/
typedef void* lsc_adv_handle_t;

enum smart_lsc_calc_status{
	SMART_LSC_ALG_LOCK = 0,
	SMART_LSC_ALG_UNLOCK = 1,
};
enum lsc_gain_pattern {
	LSC_GAIN_PATTERN_GRBG = 0,
	LSC_GAIN_PATTERN_RGGB = 1,
	LSC_GAIN_PATTERN_BGGR = 2,
	LSC_GAIN_PATTERN_GBRG = 3,
};

enum lsc_gain_calc_mode{
	FUNC_UPDATE_MODE,
	ENVI_UPDATE_MODE,
};

struct lsc_size {
	uint32_t w;
	uint32_t h;
};

struct lsc_point_t{
	uint32_t x;
	uint32_t y;

};

struct lsc_weight_value {
	int32_t value[2];
	uint32_t weight[2];
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

struct lsc_section_ctrl_t{
	uint32_t num;
	struct lsc_point_t point_value[LSC_SECTION_MAX_NUM];
};

struct lsc_adv_pre_init_param{
	uint32_t correction_intensity;
	struct seg_curve_param seg_info;
	uint32_t luma_r_gain[2];
	uint32_t luma_g_gain[2];
	uint32_t luma_b_gain[2];
	struct flat_change_param flat_param;
	uint32_t percent;
	uint32_t gain_value[2];
	uint32_t last_gain_value[2];
	uint32_t polyfit_level;

};

struct lsc_adv_pre_init2_param{
	uint32_t enable;
	uint32_t correction_intensity;
	uint32_t center_diff;
	struct seg_curve_param seg_info;
	uint32_t luma_r_gain[2];
	uint32_t luma_g_gain[2];
	uint32_t luma_b_gain[2];
	uint32_t seg_ratio;
	uint32_t gain_percent;
	uint32_t gain_value[2];
	uint32_t last_gain_value[2];
	struct lsc_section_ctrl_t calc_cnt;
};

struct lsc_adv_pre_calc_param{
	struct statistic_image_t stat_img;
	struct lsc_size stat_size;
	struct lsc_size block_size;
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
	struct lsc_adv_pre_init2_param pre2_param;
	uint32_t debug_level;
};

struct lsc_adv_init_param{
	uint32_t gain_pattern;
	struct lsc_adv_tune_param tune_param;
	uint32_t param_level;
};

struct lsc_adv_calc_param{
	struct lsc_size stat_size;
	struct lsc_size lnc_size;
	uint16_t *fix_gain;
	struct lsc_weight_value cur_index;
	uint32_t ct;
};

struct lsc_adv_info {
	uint32_t flat_num;
	uint32_t flat_status1;
	uint32_t flat_status2;
	uint32_t stat_r_var;
	uint32_t stat_b_var;
	uint32_t cali_status;

	uint32_t alg_calc_cnt;
	struct lsc_weight_value cur_index;
	struct lsc_weight_value calc_index;
	uint32_t cur_ct;

	uint32_t alg2_enable;
	uint32_t alg2_seg1_num;
	uint32_t alg2_seg2_num;
	uint32_t alg2_seg_num;
	uint32_t alg2_seg_valid;
	uint32_t alg2_cnt;
	uint32_t center_same_num[4];
};

struct lsc_adv_calc_result{
	uint16_t *dst_gain;
	struct lsc_adv_info log_info;
};


/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/

int32_t adv_lsc_alg_ioctrl(lsc_adv_handle_t handle, enum smart_lsc_calc_status status);
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
