#include "sensor_raw.h"

struct smart_tuning_param s_smart_tuning_param[8] = {0};
struct isp_smart_param smart_param_common = {0};


static void set_cmc_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_CMC10;
	cfg->smart_id = ISP_SMART_CMC;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_CT;
	component->y_type = ISP_SMART_Y_TYPE_WEIGHT_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	////////////////////////////////////////

	/*for one section: low light*/	
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = -256;
	bv_range->max = 0;	

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 6;
	func->num = sample_idx;	
	

	/*for one section: indoor*/
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 16;
	bv_range->max = 62;

	sample_idx = 0;
	func->samples[sample_idx].x = 3300;
	func->samples[sample_idx++].y = 1;
	func->samples[sample_idx].x = 3700;
	func->samples[sample_idx++].y = 2;	
	func->samples[sample_idx].x = 4300;
	func->samples[sample_idx++].y = 2;	
	func->samples[sample_idx].x = 4800;
	func->samples[sample_idx++].y = 3;		
	func->num = sample_idx;	

	/*for one section: outdoor*/	
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 90;
	bv_range->max = 256;	

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 8;
	func->num = sample_idx;		

	////////////////////////////////////////
	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_lnc_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;	
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;
	
	cfg->block_id = ISP_BLK_2D_LSC;
	cfg->smart_id = ISP_SMART_LNC;
	cfg->enable = 1;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_CT;
	component->y_type = ISP_SMART_Y_TYPE_WEIGHT_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 1;

	section_idx = 0;

	////////////////////////////////////////

	/*for one section: low light*/	
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = -255;
	bv_range->max = -1;

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 7;
	func->num = sample_idx;
	

	/*for one section: indoor*/
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 15;
	bv_range->max = 61;

	sample_idx = 0;
	func->samples[sample_idx].x = 3350;
	func->samples[sample_idx++].y = 1;
	func->samples[sample_idx].x = 3750;
	func->samples[sample_idx++].y = 2;	
	func->samples[sample_idx].x = 4350;
	func->samples[sample_idx++].y = 2;	
	func->samples[sample_idx].x = 4850;
	func->samples[sample_idx++].y = 3;		
	func->num = sample_idx;	

	/*for one section: outdoor*/	
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 89;
	bv_range->max = 255;	

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 8;
	func->num = sample_idx;		

	////////////////////////////////////////
	component->section_num = section_idx;
	
	cfg->component_num = comp_idx;
}

static void set_gamma_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;
	
	cfg->block_id = ISP_BLK_RGB_GAMC;
	cfg->smart_id = ISP_SMART_GAMMA;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV;
	component->y_type = ISP_SMART_Y_TYPE_WEIGHT_VALUE;
	component->flash_val = 4;
	component->use_flash_val = 0;

	section_idx = 0;

	////////////////////////////////////////

	/*for one section: low light*/	
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = -255;
	bv_range->max = -1;	

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->samples[sample_idx].x = 57;
	func->samples[sample_idx++].y = 3;
	func->samples[sample_idx].x = 76;
	func->samples[sample_idx++].y = 5;	
	func->num = sample_idx;	
	////////////////////////////////////////
	component->section_num = section_idx;
	
	cfg->component_num = comp_idx;
}


static void set_saturation_depress_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;	
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;	

	cfg->block_id = ISP_BLK_CMC10;
	cfg->smart_id = ISP_SMART_SATURATION_DEPRESS;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	////////////////////////////////////////

	/*for one section: low light*/	
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = -255;
	bv_range->max = 256;	

	sample_idx = 0;
	func->samples[sample_idx].x = -5;
	func->samples[sample_idx++].y = 128;
	func->samples[sample_idx].x = 60;
	func->samples[sample_idx++].y = 192;	
	func->samples[sample_idx].x = 92;
	func->samples[sample_idx++].y = 255;		
	func->num = sample_idx;	

	////////////////////////////////////////
	component->section_num = section_idx;
	
	cfg->component_num = comp_idx;
}

static void set_color_cast_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_CCE_V1;
	cfg->smart_id = ISP_SMART_COLOR_CAST;
	cfg->enable = 0;

	/*component 0: hue*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_CT;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 2000;
	func->samples[sample_idx++].y = 45;
	func->samples[sample_idx].x = 3500;
	func->samples[sample_idx++].y = 0;
	func->num = sample_idx;
	////////////////////////////////////////
	
	component->section_num = section_idx;

	/****************************************/

	/*component 1: saturation*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_CT;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 2050;
	func->samples[sample_idx++].y = 30;
	func->samples[sample_idx].x = 3250;
	func->samples[sample_idx++].y = 0;
	func->num = sample_idx;
	////////////////////////////////////////
	
	component->section_num = section_idx;

	/****************************************/

	cfg->component_num = comp_idx;
}

static void set_gain_offset_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_CCE_V1;
	cfg->smart_id = ISP_SMART_GAIN_OFFSET;
	cfg->enable = 0;

	/*component 0*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_CT;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 10;
	func->samples[sample_idx++].y = 60;
	func->samples[sample_idx].x = 50;
	func->samples[sample_idx++].y = 30;
	func->samples[sample_idx].x = 80;
	func->samples[sample_idx++].y = 100;
	func->samples[sample_idx].x = 140;
	func->samples[sample_idx++].y = 255;
	func->num = sample_idx;
	////////////////////////////////////////
	
	component->section_num = section_idx;

	/****************************************/

	/*component 1*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 11;
	func->samples[sample_idx++].y = 61;
	func->samples[sample_idx].x = 25;
	func->samples[sample_idx++].y = 31;
	func->samples[sample_idx].x = 100;
	func->samples[sample_idx++].y = 100;
	func->samples[sample_idx].x = 160;
	func->samples[sample_idx++].y = 255;
	func->num = sample_idx;
	////////////////////////////////////////
	
	component->section_num = section_idx;

	/****************************************/

	/*component 2*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 11;
	func->samples[sample_idx++].y = 61;
	func->samples[sample_idx].x = 25;
	func->samples[sample_idx++].y = 31;
	func->samples[sample_idx].x = 100;
	func->samples[sample_idx++].y = 100;
	func->samples[sample_idx].x = 160;
	func->samples[sample_idx++].y = 255;
	func->num = sample_idx;
	////////////////////////////////////////
	
	component->section_num = section_idx;

	/****************************************/

	cfg->component_num = comp_idx;

}

static void set_uvcdn_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;	
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_UV_CDN;
	cfg->smart_id = ISP_SMART_UVCDN;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 10;
	func->samples[sample_idx++].y = 60;
	func->samples[sample_idx].x = 50;
	func->samples[sample_idx++].y = 30;
	func->samples[sample_idx].x = 80;
	func->samples[sample_idx++].y = 100;
	func->samples[sample_idx].x = 140;
	func->samples[sample_idx++].y = 255;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_edge_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;	
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_EDGE;
	cfg->smart_id = ISP_SMART_EDGE;
	cfg->enable = 0;

	/*component 0*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = -8;
	func->samples[sample_idx++].y = 33;
	func->samples[sample_idx].x = 89;
	func->samples[sample_idx++].y = 128;
	func->samples[sample_idx].x = 93;
	func->samples[sample_idx++].y = 192;
	func->samples[sample_idx].x = 140;
	func->samples[sample_idx++].y = 255;
	func->num = sample_idx;
	////////////////////////////////////////
	
	component->section_num = section_idx;

	/****************************************/

	/*component 1*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 33;
	func->samples[sample_idx++].y = 20;
	func->samples[sample_idx].x = 66;
	func->samples[sample_idx++].y = 60;
	func->samples[sample_idx].x = 99;
	func->samples[sample_idx++].y = 120;
	func->num = sample_idx;
	////////////////////////////////////////
	
	component->section_num = section_idx;

	/****************************************/

	/*component 2*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 56;
	func->samples[sample_idx++].y = 61;
	func->samples[sample_idx].x = 89;
	func->samples[sample_idx++].y = 23;
	func->samples[sample_idx].x = 123;
	func->samples[sample_idx++].y = 100;
	func->num = sample_idx;
	////////////////////////////////////////

	component->section_num = section_idx;

	/****************************************/	

	cfg->component_num = comp_idx;
}

static void set_edge_param_v1(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;	
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_EDGE_V1;
	cfg->smart_id = ISP_SMART_EDGE;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];
	
	bv_range->min = -20;
	bv_range->max = 255;

	sample_idx = 0;
	func->samples[sample_idx].x = -8;
	func->samples[sample_idx++].y = 33;
	func->samples[sample_idx].x = 89;
	func->samples[sample_idx++].y = 128;
	func->samples[sample_idx].x = 93;
	func->samples[sample_idx++].y = 192;
	func->samples[sample_idx].x = 140;
	func->samples[sample_idx++].y = 255;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_pref_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;	
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_PREF_V1;
	cfg->smart_id = ISP_SMART_PREF;
	cfg->enable = 0;

	/*component 0*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 0;
	bv_range->max = 0;

	sample_idx = 0;
	func->samples[sample_idx].x = 22;
	func->samples[sample_idx++].y = 38;
	func->samples[sample_idx].x = 55;
	func->samples[sample_idx++].y = 79;
	func->samples[sample_idx].x = 90;
	func->samples[sample_idx++].y = 56;
	func->num = sample_idx;
	////////////////////////////////////////

	component->section_num = section_idx;

	/****************************************/	

	cfg->component_num = comp_idx;
}

static void set_ctm_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_CTM;
	cfg->smart_id = ISP_SMART_COLOR_TRANSFORM;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_CT;
	component->y_type = ISP_SMART_Y_TYPE_WEIGHT_VALUE;
	component->flash_val = 4;
	component->use_flash_val = 0;

	section_idx = 0;

	/*for one section: low light*/
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -255;
	bv_range->max = -1;

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->num = sample_idx;

	/*for one section: indoor*/
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 15;
	bv_range->max = 230;

	sample_idx = 0;
	func->samples[sample_idx].x = 3350;
	func->samples[sample_idx++].y = 1;
	func->samples[sample_idx].x = 3750;
	func->samples[sample_idx++].y = 2;
	func->samples[sample_idx].x = 4350;
	func->samples[sample_idx++].y = 2;
	func->samples[sample_idx].x = 8000;
	func->samples[sample_idx++].y = 3;
	func->num = sample_idx;

	/*for one section: outdoor*/
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 240;
	bv_range->max = 255;

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 8;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_hsv_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_HSV;
	cfg->smart_id = ISP_SMART_HSV;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_CT;
	component->y_type = ISP_SMART_Y_TYPE_WEIGHT_VALUE;
	component->flash_val = 4;
	component->use_flash_val = 0;

	section_idx = 0;

	/*for one section: low light*/
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -255;
	bv_range->max = -1;

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->num = sample_idx;

	/*for one section: indoor*/
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 15;
	bv_range->max = 61;

	sample_idx = 0;
	func->samples[sample_idx].x = 3350;
	func->samples[sample_idx++].y = 1;
	func->samples[sample_idx].x = 3750;
	func->samples[sample_idx++].y = 2;
	func->samples[sample_idx].x = 4350;
	func->samples[sample_idx++].y = 2;
	func->samples[sample_idx].x = 4850;
	func->samples[sample_idx++].y = 3;
	func->num = sample_idx;

	/*for one section: outdoor*/
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = 89;
	bv_range->max = 255;

	sample_idx = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 7;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_pwd_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_PRE_WAVELET_V1;
	cfg->smart_id = ISP_SMART_PRE_WAVELET;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -255;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -255;
	func->samples[sample_idx++].y = 4;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 6;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 8;
	func->samples[sample_idx].x = 60;
	func->samples[sample_idx++].y = 10;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_bpc_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_BPC_V1;
	cfg->smart_id = ISP_SMART_BPC;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -255;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -255;
	func->samples[sample_idx++].y = 4;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 6;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 8;
	func->samples[sample_idx].x = 40;
	func->samples[sample_idx++].y = 10;
	func->samples[sample_idx].x = 60;
	func->samples[sample_idx++].y = 12;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_nlm_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_NLM;
	cfg->smart_id = ISP_SMART_NLM;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -255;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -255;
	func->samples[sample_idx++].y = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 8;
	func->samples[sample_idx].x = 40;
	func->samples[sample_idx++].y = 10;
	func->samples[sample_idx].x = 60;
	func->samples[sample_idx++].y = 12;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_rgb_precdn_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_RGB_PRECDN;
	cfg->smart_id = ISP_SMART_RGB_PRECDN;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -255;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -255;
	func->samples[sample_idx++].y = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 8;
	func->samples[sample_idx].x = 40;
	func->samples[sample_idx++].y = 10;
	func->samples[sample_idx].x = 60;
	func->samples[sample_idx++].y = 12;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}


static void set_yuv_precdn_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_YUV_PRECDN;
	cfg->smart_id = ISP_SMART_YUV_PRECDN;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -255;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -255;
	func->samples[sample_idx++].y = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 8;
	func->samples[sample_idx].x = 40;
	func->samples[sample_idx++].y = 10;
	func->samples[sample_idx].x = 75;
	func->samples[sample_idx++].y = 12;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_uv_postcdn_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_UV_POSTCDN;
	cfg->smart_id = ISP_SMART_UV_POSTCDN;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -20;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -20;
	func->samples[sample_idx++].y = 0;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 30;
	func->samples[sample_idx].x = 40;
	func->samples[sample_idx++].y = 60;
	func->samples[sample_idx].x = 75;
	func->samples[sample_idx++].y = 100;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}


static void set_iir_nr_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_IIRCNR_IIR;
	cfg->smart_id = ISP_SMART_IIRCNR_IIR;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -20;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -20;
	func->samples[sample_idx++].y = 1;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 30;
	func->samples[sample_idx].x = 40;
	func->samples[sample_idx++].y = 50;
	func->samples[sample_idx].x = 75;
	func->samples[sample_idx++].y = 100;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_bdn_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_BL_NR_V1;
	cfg->smart_id = ISP_SMART_BDN;
	cfg->enable = 0;

	/*component 0*/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -20;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -20;
	func->samples[sample_idx++].y = 1;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 6;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 30;
	func->samples[sample_idx].x = 40;
	func->samples[sample_idx++].y = 50;
	func->samples[sample_idx].x = 75;
	func->samples[sample_idx++].y = 100;
	func->num = sample_idx;

	component->section_num = section_idx;

	/*component 1*/
	/****************************************/
	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;

	section_idx = 0;

	////////////////////////////////////////
	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -20;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = 33;
	func->samples[sample_idx++].y = 20;
	func->samples[sample_idx].x = 66;
	func->samples[sample_idx++].y = 60;
	func->samples[sample_idx].x = 99;
	func->samples[sample_idx++].y = 120;
	func->num = sample_idx;
	////////////////////////////////////////
	
	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

static void set_uvdiv_param(struct isp_smart_block_cfg *cfg)
{
	uint32_t i = 0;
	uint32_t section_idx = 0;
	uint32_t sample_idx = 0;
	uint32_t comp_idx = 0;
	struct isp_smart_component_cfg *component = NULL;
	struct isp_range *bv_range = NULL;
	struct isp_piecewise_func *func = NULL;

	cfg->block_id = ISP_BLK_UVDIV_V1;
	cfg->smart_id = ISP_SMART_UVDIV;
	cfg->enable = 0;

	component = &cfg->component[comp_idx++];
	component->x_type = ISP_SMART_X_TYPE_BV_GAIN;
	component->y_type = ISP_SMART_Y_TYPE_VALUE;
	component->flash_val = 6;
	component->use_flash_val = 0;

	section_idx = 0;

	bv_range = &component->bv_range[section_idx];
	func = &component->func[section_idx++];

	bv_range->min = -20;
	bv_range->max = 100;

	sample_idx = 0;
	func->samples[sample_idx].x = -20;
	func->samples[sample_idx++].y = 1;
	func->samples[sample_idx].x = 0;
	func->samples[sample_idx++].y = 5;
	func->samples[sample_idx].x = 20;
	func->samples[sample_idx++].y = 30;
	func->samples[sample_idx].x = 40;
	func->samples[sample_idx++].y = 50;
	func->samples[sample_idx].x = 75;
	func->samples[sample_idx++].y = 100;
	func->num = sample_idx;

	component->section_num = section_idx;

	cfg->component_num = comp_idx;
}

struct smart_tuning_param *get_smart_param()
{
	uint32_t block_id = 0;

	s_smart_tuning_param[0].bypass = 0;
	s_smart_tuning_param[0].version = 0;
	s_smart_tuning_param[0].data.data_ptr = &smart_param_common;
	s_smart_tuning_param[0].data.size = sizeof(smart_param_common);

	set_lnc_param(&smart_param_common.block[block_id++]);
	set_color_cast_param(&smart_param_common.block[block_id++]);
	set_cmc_param(&smart_param_common.block[block_id++]);
	set_saturation_depress_param(&smart_param_common.block[block_id++]);
	set_hsv_param(&smart_param_common.block[block_id++]);
	set_ctm_param(&smart_param_common.block[block_id++]);
	set_edge_param_v1(&smart_param_common.block[block_id++]);
	set_pref_param(&smart_param_common.block[block_id++]);
	set_uvcdn_param(&smart_param_common.block[block_id++]);
	set_gamma_param(&smart_param_common.block[block_id++]);
	set_gain_offset_param(&smart_param_common.block[block_id++]);
	set_pwd_param(&smart_param_common.block[block_id++]);
	set_bpc_param(&smart_param_common.block[block_id++]);
	set_nlm_param(&smart_param_common.block[block_id++]);
	set_rgb_precdn_param(&smart_param_common.block[block_id++]);
	set_yuv_precdn_param(&smart_param_common.block[block_id++]);
	set_uv_postcdn_param(&smart_param_common.block[block_id++]);
	set_iir_nr_param(&smart_param_common.block[block_id++]);
	set_bdn_param(&smart_param_common.block[block_id++]);
	set_uvdiv_param(&smart_param_common.block[block_id++]);
	
	smart_param_common.block_num = block_id;

	return s_smart_tuning_param;
}
