/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * V1.0
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "sensor_ov8858_raw_param.c"

#define FEATURE_OTP
#ifdef FEATURE_OTP
#include "sensor_ov8858_r2a_otp.c"
#endif

#define SENSOR_NAME			"ov8858"
#define I2C_SLAVE_ADDR			0x6c

#define ov8858_PID_ADDR			0x300B
#define ov8858_PID_VALUE			0x88
#define ov8858_VER_ADDR			0x300C
#define ov8858_VER_VALUE			0x58

/* sensor parameters begin */
#define SNAPSHOT_WIDTH			3264
#define SNAPSHOT_HEIGHT			2448
#define PREVIEW_WIDTH			1632
#define PREVIEW_HEIGHT			1224

#define LANE_NUM			2
#define RAW_BITS			10

#define SNAPSHOT_MIPI_PER_LANE_BPS	720
#define PREVIEW_MIPI_PER_LANE_BPS	720

#define SNAPSHOT_LINE_TIME		269
#define PREVIEW_LINE_TIME		269
#define SNAPSHOT_FRAME_LENGTH		2474
#define PREVIEW_FRAME_LENGTH		1244

/* please ref your spec */
#define FRAME_OFFSET			4
#define SENSOR_MAX_GAIN			0x03ff
#define SENSOR_BASE_GAIN		0x80
#define SENSOR_MIN_SHUTTER		4

#define FULL_SIZE_PREVIEW

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR			1

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
#define SUPPORT_AUTO_FRAME_LENGTH	0
/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN			0x10
/* please don't change it */
#define EX_MCLK				24

struct hdr_info_t {
	uint32_t capture_max_shutter;
	uint32_t capture_shutter;
	uint32_t capture_gain;
};

struct sensor_ev_info_t {
	uint16_t preview_shutter;
	uint16_t preview_gain;
};

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;

#ifdef FEATURE_OTP
/*#define ov8858_RG_Ratio_Typical 0x013f
#define ov8858_BG_Ratio_Typical 0x011e
#define ov8858_MODULE_ID 0x04 //guangzhen*/

struct ov8858_otp_info {
        int32_t module_integrator_id;
        int32_t lens_id;
        int32_t production_year;
        int32_t production_month;
        int32_t production_day;
        int32_t rg_ratio;
        int32_t bg_ratio;
        int32_t light_rg;
        int32_t light_bg;
        int32_t user_data[5];
        int32_t lenc[240];
        int32_t checksum;
        int32_t VCM_start;
        int32_t VCM_end;
        int32_t VCM_dir;
};
static uint32_t g_module_id = 0;

#define  BG_RATIO_TYPICAL_OFILM (0x0126)
#define  RG_RATIO_TYPICAL_OFILM (0x0142)
#define ov8858_RAW_PARAM_COM  0x0000
#define OV8858_RAW_PARAM_O_FILM (0x04)//guangzhen

LOCAL uint32_t _ov8858_GetRawInof(void);
LOCAL uint32_t _ov8858_com_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov8858_o_film_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov8858_o_film_update_otp(void* param_ptr);
LOCAL uint32_t _ov8858_cfg_otp(uint32_t  param);
LOCAL int32_t _ov8858_check_o_film_otp(int32_t cmd, int32_t index);
LOCAL int32_t _ov8858_read_o_film_otp(int32_t cmd, int32_t index, struct ov8858_otp_info *otp_ptr);
LOCAL int32_t _ov8858_update_o_film_otp_wb(void);
LOCAL int32_t _ov8858_update_o_film_otp_lenc(void);

LOCAL struct raw_param_info_tab s_ov8858_raw_param_tab[] = {
        {OV8858_RAW_PARAM_O_FILM, &s_ov8858_mipi_raw_info, _ov8858_o_film_Identify_otp, _ov8858_o_film_update_otp},
        {ov8858_RAW_PARAM_COM, &s_ov8858_mipi_raw_info, _ov8858_com_Identify_otp, PNULL},
        {RAW_INFO_END_ID, PNULL, PNULL, PNULL},
};
#endif

static SENSOR_IOCTL_FUNC_TAB_T s_ov8858_ioctl_func_tab;
struct sensor_raw_info *s_ov8858_mipi_raw_info_ptr = &s_ov8858_mipi_raw_info;

static const SENSOR_REG_T ov8858_init_setting[] = {
	//@@ SIZE_3264X2448_15FPS_MIPI_2LANE
	//100 99 3264 2448 ; Resolution
	//102 80 1
	//102 3601 5dc ;15fps
	//102 40 0 ; HDR Mode off
	//;FPGA set-up
	//c8 01 f2 ;MIPI FPGA;
	//CL 100 100;delay

	{0x103,0x01},
	//{0x303f,0x01},
	//{0x3012,0x6c},
	{SENSOR_WRITE_DELAY, 0x0a},  //;delay 5ms
	{0x100,0x00},
	{0x302,0x1e},//mipi colock
	{0x303,0x00},
	{0x304,0x03},
	{0x30e,0x02},
	{0x30d,0x1e},//mipi
	{0x30f,0x04},
	{0x312,0x03},
	{0x31e,0x0c},
	{0x3600,0x00},
	{0x3601,0x00},
	{0x3602,0x00},
	{0x3603,0x00},
	{0x3604,0x22},
	{0x3605,0x20},
	{0x3606,0x00},
	{0x3607,0x20},
	{0x3608,0x11},
	{0x3609,0x28},
	{0x360a,0x00},
	{0x360b,0x05},
	{0x360c,0xd4},
	{0x360d,0x40},
	{0x360e,0x0c},
	{0x360f,0x20},
	{0x3610,0x07},
	{0x3611,0x20},
	{0x3612,0x88},
	{0x3613,0x80},
	{0x3614,0x58},
	{0x3615,0x00},
	{0x3616,0x4a},
	{0x3617,0x40},
	{0x3618,0x5a},
	{0x3619,0x70},
	{0x361a,0x99},
	{0x361b,0x0a},
	{0x361c,0x07},
	{0x361d,0x00},
	{0x361e,0x00},
	{0x361f,0x00},
	{0x3638,0xff},
	{0x3633,0x0f},
	{0x3634,0x0f},
	{0x3635,0x0f},
	{0x3636,0x12},
	{0x3645,0x13},
	{0x3646,0x83},
	{0x364a,0x07},
	{0x3015,0x00},
	{0x3018,0x32},
	{0x3020,0x93},
	{0x3022,0x01},
	{0x3031,0x0a},
	{0x3034,0x00},
	{0x3106,0x01},
	{0x3305,0xf1},
	{0x3308,0x00},
	{0x3309,0x28},
	{0x330a,0x00},
	{0x330b,0x20},
	{0x330c,0x00},
	{0x330d,0x00},
	{0x330e,0x00},
	{0x330f,0x40},
	{0x3307,0x04},
	{0x3500,0x00},
	{0x3501,0x9a},
	{0x3502,0x20},
	{0x3503,0x80},
	{0x3505,0x80},
	{0x3508,0x02},
	{0x3509,0x00},
	{0x350c,0x00},
	{0x350d,0x80},
	{0x3510,0x00},
	{0x3511,0x02},
	{0x3512,0x00},
	{0x3700,0x18},
	{0x3701,0x0c},
	{0x3702,0x28},
	{0x3703,0x19},
	{0x3704,0x14},
	{0x3705,0x00},
	{0x3706,0x82},
	{0x3707,0x04},
	{0x3708,0x24},
	{0x3709,0x33},
	{0x370a,0x01},
	{0x370b,0x82},
	{0x370c,0x04},
	{0x3718,0x12},
	{0x3719,0x31},
	{0x3712,0x42},
	{0x3714,0x24},
	{0x371e,0x19},
	{0x371f,0x40},
	{0x3720,0x05},
	{0x3721,0x05},
	{0x3724,0x06},
	{0x3725,0x01},
	{0x3726,0x06},
	{0x3728,0x05},
	{0x3729,0x02},
	{0x372a,0x03},
	{0x372b,0x53},
	{0x372c,0xa3},
	{0x372d,0x53},
	{0x372e,0x06},
	{0x372f,0x10},
	{0x3730,0x01},
	{0x3731,0x06},
	{0x3732,0x14},
	{0x3733,0x10},
	{0x3734,0x40},
	{0x3736,0x20},
	{0x373a,0x05},
	{0x373b,0x06},
	{0x373c,0x0a},
	{0x373e,0x03},
	{0x3750,0x0a},
	{0x3751,0x0e},
	{0x3755,0x10},
	{0x3758,0x00},
	{0x3759,0x4c},
	{0x375a,0x06},
	{0x375b,0x13},
	{0x375c,0x20},
	{0x375d,0x02},
	{0x375e,0x00},
	{0x375f,0x14},
	{0x3768,0x22},
	{0x3769,0x44},
	{0x376a,0x44},
	{0x3761,0x00},
	{0x3762,0x00},
	{0x3763,0x00},
	{0x3766,0xff},
	{0x376b,0x00},
	{0x3772,0x23},
	{0x3773,0x02},
	{0x3774,0x16},
	{0x3775,0x12},
	{0x3776,0x04},
	{0x3777,0x00},
	{0x3778,0x1a},
	{0x37a0,0x44},
	{0x37a1,0x3d},
	{0x37a2,0x3d},
	{0x37a3,0x00},
	{0x37a4,0x00},
	{0x37a5,0x00},
	{0x37a6,0x00},
	{0x37a7,0x44},
	{0x37a8,0x4c},
	{0x37a9,0x4c},
	{0x3760,0x00},
	{0x376f,0x01},
	{0x37aa,0x44},
	{0x37ab,0x2e},
	{0x37ac,0x2e},
	{0x37ad,0x33},
	{0x37ae,0x0d},
	{0x37af,0x0d},
	{0x37b0,0x00},
	{0x37b1,0x00},
	{0x37b2,0x00},
	{0x37b3,0x42},
	{0x37b4,0x42},
	{0x37b5,0x31},
	{0x37b6,0x00},
	{0x37b7,0x00},
	{0x37b8,0x00},
	{0x37b9,0xff},
	{0x3800,0x00},
	{0x3801,0x0c},
	{0x3802,0x00},
	{0x3803,0x0c},
	{0x3804,0x0c},
	{0x3805,0xd3},
	{0x3806,0x09},
	{0x3807,0xa3},
	{0x3808,0x0c},
	{0x3809,0xc0},
	{0x380a,0x09},
	{0x380b,0x90},
	{0x380c,0x07},
	{0x380d,0x94},
	{0x380e,0x09},
	{0x380f,0xaa},
	{0x3810,0x00},
	{0x3811,0x04},
	{0x3813,0x02},
	{0x3814,0x01},
	{0x3815,0x01},
	{0x3820,0x00},//0x00->0x06
	{0x3821,0x40},//0x46->0x40
	{0x382a,0x01},
	{0x382b,0x01},
	{0x3830,0x06},
	{0x3836,0x01},
	{0x3837,0x18},
	{0x3841,0xff},
	{0x3846,0x48},
	{0x3d85,0x16},
	{0x3d8c,0x73},
	{0x3d8d,0xde},
	{0x3f08,0x08},
	{0x4000,0xf1},
	{0x4001,0x00},
	{0x4005,0x10},
	{0x4002,0x27},
	{0x4009,0x81},
	{0x400b,0x0c},
	{0x4011,0x20},
	{0x401b,0x00},
	{0x401d,0x00},
	{0x4020,0x00},
	{0x4021,0x04},
	{0x4022,0x0c},
	{0x4023,0x60},
	{0x4024,0x0f},
	{0x4025,0x36},
	{0x4026,0x0f},
	{0x4027,0x37},
	{0x4028,0x00},
	{0x4029,0x02},
	{0x402a,0x04},
	{0x402b,0x08},
	{0x402c,0x00},
	{0x402d,0x02},
	{0x402e,0x04},
	{0x402f,0x08},
	{0x401f,0x00},
	{0x4034,0x3f},
	{0x403d,0x04},
	{0x4300,0xff},
	{0x4301,0x00},
	{0x4302,0x0f},
	{0x4316,0x00},
	{0x4503,0x18},
	{0x4600,0x01},
	{0x4601,0x97},
	{0x481f,0x32},
	{0x4837,0x16},//16
	{0x4850,0x10},
	{0x4851,0x32},
	{0x4b00,0x2a},
	{0x4b0d,0x00},
	{0x4d00,0x04},
	{0x4d01,0x18},
	{0x4d02,0xc3},
	{0x4d03,0xff},
	{0x4d04,0xff},
	{0x4d05,0xff},
	{0x5000,0x7e},
	{0x5001,0x01},
	{0x5002,0x08},
	{0x5003,0x20},
	{0x5046,0x12},
	{0x5780,0x3e},
	{0x5781,0x0f},
	{0x5782,0x44},
	{0x5783,0x02},
	{0x5784,0x01},
	{0x5785,0x00},
	{0x5786,0x00},
	{0x5787,0x04},
	{0x5788,0x02},
	{0x5789,0x0f},
	{0x578a,0xfd},
	{0x578b,0xf5},
	{0x578c,0xf5},
	{0x578d,0x03},
	{0x578e,0x08},
	{0x578f,0x0c},
	{0x5790,0x08},
	{0x5791,0x04},
	{0x5792,0x00},
	{0x5793,0x52},
	{0x5794,0xa3},
	{0x5871,0x0d},
	{0x5870,0x18},
	{0x586e,0x10},
	{0x586f,0x08},
	{0x58f8,0x3d},
	{0x5901,0x00},
	{0x5b00,0x02},
	{0x5b01,0x10},
	{0x5b02,0x03},
	{0x5b03,0xcf},
	{0x5b05,0x6c},
	{0x5e00,0x00},
	{0x5e01,0x41},
	{0x4825,0x3a},
	{0x4826,0x40},
	{0x4808,0x25},
	{0x3763,0x18},
	{0x3768,0xcc},
	{0x470b,0x28},
	{0x4202,0x00},
	{0x400d,0x10},
	{0x4040,0x07},
	{0x403e,0x08},
	{0x4041,0xc6},
	{0x3007,0x80},
	{0x400a,0x01},
	{0x3015,0x01},
	{0x0100, 0x00}
};

static const SENSOR_REG_T ov8858_preview_setting[] = {
	//@@ Preview setting
	//100 99 1632 1224;32 ; Resolution
	//{0x0100,0x00},//
	{0x3769,0x44},// ;
	{0x376a,0x44},// ;
	{0x3808,0x06},//
	{0x3809,0x60},//;68
	{0x380a,0x04},//
	{0x380b,0xc8},//;d0
	{0x380c,0x07},//
	{0x380d,0x94},//;
	{0x380e,0x04},//;05;0a
	{0x380f,0xdc},//;30;0d
	{0x3814,0x03},//
//	{0x3820,0x00},//{0x3821,0x40},//
	{0x3821,0x67},//{0x3821,0x40},//
	{0x382a,0x03},//
	{0x302b,0x01},// ;
	{0x3830,0x08},//
	{0x3836,0x02},//
	{0x4000,0xf1},//
	{0x4001,0x10},//
	{0x4022,0x04},//;06
	{0x4023,0x90},//;00
	{0x4025,0x2a},//
	{0x4027,0x2b},//
	{0x402a,0x04},// ;
	{0x402b,0x04},//
	{0x402e,0x04},//
	{0x402f,0x08},//
	{0x4600,0x00},//
	{0x4601,0xcb},//
	{0x382d,0x7f},//
	{0x5901,0x00},// ;
	{0x0100,0x01}
};

static const SENSOR_REG_T ov8858_snapshot_setting[] = {
	//@@ Capture setting
	//100 99 3264 2448 ; Resolution

	//{0x0100,0x00},//;
	{0x3769,0x44},// ;
	{0x376a,0x44},// ;
	{0x3808,0x0c},//
	{0x3809,0xc0},//;d0
	{0x380a,0x09},//
	{0x380b,0x90},//;a0
	{0x380c,0x07},//
	{0x380d,0x94},//
	{0x380e,0x09},//
	{0x380f,0xaa},//;60;0d
	{0x3814,0x01},//
	{0x3821,0x46},//40},//
	{0x382a,0x01},//
	{0x302b,0x01},// ;
	{0x3830,0x06},//
	{0x3836,0x01},//
	{0x4000,0xf1},//
	{0x4001,0x00},//
	{0x4022,0x0c},//
	{0x4023,0x60},//
	{0x4025,0x36},//
	{0x4027,0x37},//
	{0x402a,0x04},// ;
	{0x402b,0x08},//
	{0x402e,0x04},//
	{0x402f,0x08},//
	{0x4600,0x01},//
	{0x4601,0x97},//
	{0x382d,0xff},//
	{0x5901,0x00},// ;
	//{0x0100,0x00}
};

static SENSOR_REG_TAB_INFO_T s_ov8858_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(ov8858_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
#ifndef FULL_SIZE_PREVIEW
	{ADDR_AND_LEN_OF_ARRAY(ov8858_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
#endif
	{ADDR_AND_LEN_OF_ARRAY(ov8858_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_ov8858_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
		
#ifndef FULL_SIZE_PREVIEW
	{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},
#endif

	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},
};

static const SENSOR_REG_T s_ov8858_1632X1224_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static const SENSOR_REG_T s_ov8858_3264X2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static SENSOR_VIDEO_INFO_T s_ov8858_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_ov8858_1632X1224_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_ov8858_3264X2448_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t ov8858_set_video_mode(uint32_t param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0x00;
	uint32_t mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_ov8858_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_ov8858_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr)
	     || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov8858_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
	/* bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	 * bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	 * other bit: reseved
	 */
	SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,
	/* preview mode */
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
	/* image effect */
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	/* while balance mode */
	0,
	/* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	 * bit[8:31] reseved
	 */
	7,
	/* reset pulse level */
	SENSOR_LOW_PULSE_RESET,
	/* reset pulse width(ms) */
	50,
	/* 1: high level valid; 0: low level valid */
	SENSOR_LOW_LEVEL_PWDN,
	/* count of identify code */
	1,
	/* supply two code to identify sensor.
	 * for Example: index = 0-> Device id, index = 1 -> version id
	 * customer could ignore it.
	 */
	{{ov8858_PID_ADDR, ov8858_PID_VALUE}
	 ,
	 {ov8858_VER_ADDR, ov8858_VER_VALUE}
	 }
	,
	/* voltage of avdd */
	SENSOR_AVDD_2800MV,
	/* max width of source image */
	SNAPSHOT_WIDTH,
	/* max height of source image */
	SNAPSHOT_HEIGHT,
	/* name of sensor */
	SENSOR_NAME,
	/* define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	 * if set to SENSOR_IMAGE_FORMAT_MAX here,
	 * image format depent on SENSOR_REG_TAB_INFO_T
	 */
	SENSOR_IMAGE_FORMAT_RAW,
	/*  pattern of input image form sensor */
	SENSOR_IMAGE_PATTERN_RAWRGB_B,
	/* point to resolution table information structure */
	s_ov8858_resolution_tab_raw,
	/* point to ioctl function table */
	&s_ov8858_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_ov8858_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_ov8858_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1200MV,
	/* skip frame num before preview */
	1,
	/* skip frame num before capture */
	1,
	/* deci frame num during preview */
	0,
	/* deci frame num during video preview */
	0,
	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0}
	,
	0,
	/* skip frame num while change setting */
	1,
};

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t ov8858_get_default_frame_length(uint32_t mode)
{
	return s_ov8858_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov8858_read_gain(void)
{
	uint16_t gain_h = 0;
	uint16_t gain_l = 0;

//	gain_h = Sensor_ReadReg(0xYYYY) & 0xff;
//	gain_l = Sensor_ReadReg(0xYYYY) & 0xff;

	return ((gain_h << 8) | gain_l);
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov8858_write_gain(uint32_t gain)
{
	Sensor_WriteReg(0x3508, (gain >> 0x08) & 0x07);/*8*/
	Sensor_WriteReg(0x3509, gain & 0xff);/*0-7*/

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov8858_read_frame_length(void)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;

	frame_len_h = Sensor_ReadReg(0x380e) & 0xff;
	frame_len_l = Sensor_ReadReg(0x380f) & 0xff;
	
	return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov8858_write_frame_length(uint32_t frame_len)
{
	Sensor_WriteReg(0x380e, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x380f, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8858_read_shutter(void)
{
	uint16_t shutter_h = 0;
	uint16_t shutter_l = 0;

	//shutter_h = Sensor_ReadReg(0xYYYY) & 0xff;
	//shutter_l = Sensor_ReadReg(0xYYYY) & 0xff;

	return (shutter_h << 8) | shutter_l;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov8858_write_shutter(uint32_t shutter)
{
	uint16_t value=0x00;
	value=(shutter<<0x04)&0xff;
	Sensor_WriteReg(0x3502, value);
	value=(shutter>>0x04)&0xff;
	Sensor_WriteReg(0x3501, value);
	value=(shutter>>0x0c)&0x0f;
	Sensor_WriteReg(0x3500, value);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t ov8858_update_exposure(uint32_t shutter)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + FRAME_OFFSET) > fr_len) ? (shutter + FRAME_OFFSET) : fr_len;

	cur_fr_len = ov8858_read_frame_length();

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		ov8858_write_frame_length(dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	ov8858_write_shutter(shutter);
	return shutter;
}

#define DW9714_VCM_SLAVE_ADDR (0x18 >> 1)

/*==============================================================================
 * Description:
 * init vcm driver DW9714
 * you can change this function acording your spec if it's necessary
 * mode:
 * 1: Direct Mode
 * 2: Dual Level Control Mode
 * 3: Linear Slope Cntrol Mode
 *============================================================================*/
static uint32_t dw9714_init(uint32_t mode)
{
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	SENSOR_PRINT("mode = %d\n", mode);
	switch (mode) {
	case 1:
		/* When you use direct mode after power on, you don't need register set. Because, DLC disable is default.*/
		break;
	case 2:
		/*Protection off */
		cmd_val[0] = 0xec;
		cmd_val[1] = 0xa3;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*DLC and MCLK[1:0] setting */
		cmd_val[0] = 0xa1;
		/*for better performace, cmd_val[1][1:0] should adjust to matching with Tvib of your camera VCM*/
		cmd_val[1] = 0x0e;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*T_SRC[4:0] setting */
		cmd_val[0] = 0xf2;
		/*for better performace, cmd_val[1][7:3] should be adjusted to matching with Tvib of your camera VCM*/
		cmd_val[1] = 0x90;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*Protection on */
		cmd_val[0] = 0xdc;
		cmd_val[1] = 0x51;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
		break;
	case 3:
		/*Protection off */
		cmd_val[0] = 0xec;
		cmd_val[1] = 0xa3;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*DLC and MCLK[1:0] setting */
		cmd_val[0] = 0xa1;
		cmd_val[1] = 0x05;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*T_SRC[4:0] setting */
		cmd_val[0] = 0xf2;
		/*for better performace, cmd_val[1][7:3] should be adjusted to matching with the Tvib of your camera VCM*/
		cmd_val[1] = 0x00;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*Protection on */
		cmd_val[0] = 0xdc;
		cmd_val[1] = 0x51;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
		break;
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * write parameter to vcm driver IC DW9714
 * you can change this function acording your spec if it's necessary
 * uint16_t s_4bit: for better performace of linear slope control, s_4bit should be given a value corresponding to step period
 *============================================================================*/
static uint32_t dw9714_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint16_t step_4bit = 0x09;

	ALOGE("%d", param);
ALOGE("dw9714_write_af");
	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param & 0xfff0) >> 4;
	cmd_val[1] = ((param & 0x0f) << 4) | step_4bit;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	return ret_value;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
/*static uint32_t ov8858_power_on(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov8858_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov8858_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov8858_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov8858_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov8858_mipi_raw_info.reset_pulse_level;
	BOOLEAN vcm_power_down = SENSOR_LOW_LEVEL_PWDN;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		usleep(12 * 1000);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20 * 1000);
		//Sensor_VCM_PowerDown(!vcm_power_down);
		//usleep(2000);
		dw9714_init(2);		
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10 * 1000);
		Sensor_PowerDown(!power_down);
		usleep(10 * 1000);
		//Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		//Sensor_VCM_PowerDown(vcm_power_down);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}*/
static uint32_t ov8858_power_on(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov8858_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov8858_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov8858_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov8858_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov8858_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov8858_yuv_info.reset_pulse_width;
        ALOGE("ov8858_power_on");
	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
               //_Sensor_Device_Af_PowerDown(1);
		//_ov8858_dw9714_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(10*1000);
		// Reset sensor
		Sensor_Reset(reset_level);
		usleep(20*1000);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
                //_Sensor_Device_Af_PowerDown(0);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov8858: _ov8858_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8858_identify(uint32_t param)
{
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	ALOGE("ov8858 mipi raw identify");

	pid_value = Sensor_ReadReg(ov8858_PID_ADDR);

	if (ov8858_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov8858_VER_ADDR);
		ALOGE("ov8858 Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov8858_VER_VALUE == ver_value) {
			ret_value=_ov8858_GetRawInof();
                    if(SENSOR_SUCCESS != ret_value)
                    {
                            SENSOR_PRINT_ERR("SENSOR_ov8858: the module is unknow error !");
                    	}
		} else {
			ALOGE("ov8858 Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		ALOGE("ov8858 identify fail, pid_value = %d", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static uint32_t ov8858_get_resolution_trim_tab(uint32_t param)
{
	return (uint32_t) s_ov8858_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov8858_before_snapshot(uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_ov8858_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov8858_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = ov8858_get_default_frame_length(capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//ov8858_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//ov8858_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}

	cap_shutter = ov8858_update_exposure(cap_shutter);
	cap_gain = gain;
	ov8858_write_gain(cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //ov8858_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //ov8858_read_gain();
	/* limit HDR capture min fps to 10;
	 * MaxFrameTime = 1000000*0.1us;
	 */
	s_hdr_info.capture_max_shutter = 1000000 / cap_linetime;

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static uint32_t ov8858_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xffff;
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d", mode, exposure_line);
	s_current_default_frame_length = ov8858_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = ov8858_update_exposure(exposure_line);

	return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(uint32_t param)
{
	uint32_t real_gain = 0;
	real_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1);
	real_gain = real_gain * (((param >> 5) & 0x01) + 1) * (((param >> 6) & 0x01) + 1);
	real_gain = real_gain * (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);
	real_gain = real_gain * (((param >> 9) & 0x01) + 1) * (((param >> 10) & 0x01) + 1);
	real_gain = real_gain * (((param >> 11) & 0x01) + 1);

	return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov8858_write_gain_value(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	real_gain = isp_to_real_gain(param);

	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	ov8858_write_gain(real_gain);

	return ret_value;
}



/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t ov8858_write_af(uint32_t param)
{
	return dw9714_write_af(param);
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void ov8858_increase_hdr_exposure(uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		ov8858_update_exposure(s_hdr_info.capture_shutter * ev_multiplier);
		ov8858_write_gain(s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		if (SENSOR_MAX_GAIN < gain)
			gain = SENSOR_MAX_GAIN;

		ov8858_update_exposure(s_hdr_info.capture_shutter * shutter_multiply);
		ov8858_write_gain(gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void ov8858_decrease_hdr_exposure(uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		ov8858_write_gain(s_hdr_info.capture_gain / ev_divisor);
		ov8858_update_exposure(s_hdr_info.capture_shutter);
	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		ov8858_write_gain(s_hdr_info.capture_gain / gain_multiply);
		ov8858_update_exposure(shutter);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov8858_set_hdr_ev(uint32_t param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		ov8858_decrease_hdr_exposure(ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 1;
		ov8858_increase_hdr_exposure(ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 2;
		ov8858_increase_hdr_exposure(ev_multiplier);
		break;
	default:
		break;
	}
	return ret;
}

/*==============================================================================
 * Description:
 * extra functoin
 * you can add functions reference SENSOR_EXT_FUNC_CMD_E which from sensor_drv_u.h
 *============================================================================*/
static uint32_t ov8858_ext_func(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = ov8858_set_hdr_ev(param);
		break;
	default:
		break;
	}

	return rtn;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8858_stream_on(uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x01);
	//usleep(15*1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8858_stream_off(uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(15*1000);
	
	return 0;
}
#ifdef FEATURE_OTP

/*static uint32_t ov8858_cfg_otp(uint32_t param)
{
	uint32_t rtn = SENSOR_FAIL;
	uint32_t i = 0x00;
	struct otp_param_info_tab *tab_ptr = (struct otp_param_info_tab *)s_ov8858_raw_param_tab;
	uint16_t module_id= 0;

	module_id=ov8858_otp_identify_otp(NULL);

	SENSOR_PRINT("module_id = 0x%x", module_id);

	for (i = 0; i < NUMBER_OF_ARRAY(s_ov8858_raw_param_tab); i++) {
		if (tab_ptr[i].module_id == module_id) {
			rtn = SENSOR_SUCCESS;
			break;
		}
	}

	if (SENSOR_SUCCESS != rtn) {
		//disable lsc
		Sensor_WriteReg(0x3300,0x01);
	}
	else{

		SENSOR_PRINT("otp_table_id = %d", i);
		Sensor_WriteReg(0x3300,0x00);

		if (PNULL != s_ov8858_raw_param_tab[i].update_otp) {
			s_ov8858_raw_param_tab[i].update_otp(&s_ov8858_raw_param_tab[i]);
		}
	}

	return rtn;
}*/

LOCAL uint32_t _ov8858_cfg_otp(uint32_t  param)
{
        uint32_t rtn=SENSOR_SUCCESS;
        struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8858_raw_param_tab;
        uint32_t module_id=g_module_id;
        uint16_t stream_value = 0;

        SENSOR_PRINT_ERR("SENSOR_ov8858: _ov8858_cfg_otp");

        stream_value = Sensor_ReadReg(0x0100);
        SENSOR_PRINT("_ov8858_cfg_otp: stream_value = 0x%x\n", stream_value);
        if(1 != (stream_value & 0x01)) {
                Sensor_WriteReg(0x0100, 0x01);
                usleep(5 * 1000);
        }

        if (PNULL!=tab_ptr[module_id].cfg_otp) {
                tab_ptr[module_id].cfg_otp(0);
                }

        if(1 != (stream_value & 0x01)) {
                Sensor_WriteReg(0x0100, stream_value);
                usleep(5 * 1000);
        }

        return rtn;
}

LOCAL int32_t _ov8858_check_o_film_otp(int32_t cmd, int32_t index)
{
        int32_t rtn_index = 0;
        int32_t flag = 0x00, i;
        int32_t address_start = 0x00;
        int32_t address_end = 0x00;
	int32_t temp1=0x0;

        //set 0x5002[3] to int32_t temp1;
        temp1 = Sensor_ReadReg(0x5002);
        Sensor_WriteReg(0x5002, (0x00 & 0x08) | (temp1 & (~0x08)));

        switch (cmd)
        {
                case 0://for check otp info
                {
                        address_start = 0x7010;
                        address_end = 0x7010;
                        /* read otp into buffer*/
                        Sensor_WriteReg(0x3d84, 0xc0);

                        /* program disable, manual mode*/
                        /*partial mode OTP write start address*/
                        Sensor_WriteReg(0x3d88, (address_start>>8));
                        Sensor_WriteReg(0x3d89, (address_start & 0xff));
                        /*partial mode OTP write end address*/
                        Sensor_WriteReg(0x3d8A, (address_end>>8));
                        Sensor_WriteReg(0x3d8B, (address_end & 0xff));
                        Sensor_WriteReg(0x3d81, 0x01);

                        /*read otp*/
                        usleep(10*1000);
                        flag = Sensor_ReadReg(0x7010);
                }
                break;

                case 1://for check opt wb
                {
                        address_start = 0x7010;
                        address_end = 0x7010;
                        /*read otp into buffer*/
                        Sensor_WriteReg(0x3d84, 0xc0);

                        /* program disable, manual mode*/
                        /*partial mode OTP write start address*/
                        Sensor_WriteReg(0x3d88, (address_start>>8));
                        Sensor_WriteReg(0x3d89, (address_start & 0xff));

                        /*partial mode OTP write end address*/
                        Sensor_WriteReg(0x3d8A, (address_end>>8));
                        Sensor_WriteReg(0x3d8B, (address_end & 0xff));
                        Sensor_WriteReg(0x3d81, 0x01);
                        usleep(10*1000);
                        /*read OTP; select group*/
                        flag = Sensor_ReadReg(0x7010);
                }
                break;

                case 2://for check otp lenc
                {
                        address_start = 0x7028;
                        address_end = 0x7028;
                        /* read otp into buffer*/
                        Sensor_WriteReg(0x3d84, 0xc0);
                        /*
                        program disable, manual mode
                        partial mode OTP write start address
                        */
                        Sensor_WriteReg(0x3d88, (address_start>>8));
                        Sensor_WriteReg(0x3d89, (address_start & 0xff));
                        /*partial mode OTP write end address*/
                        Sensor_WriteReg(0x3d8A, (address_end>>8));
                        Sensor_WriteReg(0x3d8B, (address_end & 0xff));
                        Sensor_WriteReg(0x3d81, 0x01);
                        usleep(10*1000);
                        flag = Sensor_ReadReg(0x7028);
                }
                break;

                case 3://for check otp vcm
                {
                        address_start = 0x7021;
                        address_end = 0x7021;
                        // read otp into buffer
                        Sensor_WriteReg(0x3d84, 0xc0);
                        // program disable, manual mode
                        //partial mode OTP write start address
                        Sensor_WriteReg(0x3d88, (address_start>>8));
                        Sensor_WriteReg(0x3d89, (address_start & 0xff));
                        // partial mode OTP write end address
                        Sensor_WriteReg(0x3d8A, (address_end>>8));
                        Sensor_WriteReg(0x3d8B, (address_end & 0xff));
                        Sensor_WriteReg(0x3d81, 0x01);
                        usleep(10*1000);
                        //select group
                        flag = Sensor_ReadReg(0x7021);
                }
                break;

                default:
                break;
        }

        if(index==1) {
                flag = (flag>>6) & 0x03;
        }else if (index==2) {
                flag = (flag>>4) & 0x03;
        } else {
                flag = (flag>>2) & 0x03;
        }

        /* clear otp buffer*/
        for (i=address_start;i<=address_end;i++) {
                Sensor_WriteReg(i, 0x00);
        }

        //set 0x5002[3] to temp1 = Sensor_ReadReg(0x5002);
        Sensor_WriteReg(0x5002, (0x08 & 0x08) | (temp1 & (~0x08)));

        if (flag == 0x00) {//Empty
                rtn_index = 0;
        } else if (flag & 0x02) {//Inalid
                rtn_index = 1;
        } else {//Valid
                rtn_index = 2;
        }

        return rtn_index;
}

LOCAL int32_t _ov8858_read_o_film_otp(int32_t cmd, int32_t index, struct ov8858_otp_info *otp_ptr)
{
        int32_t i = 0x00;
        int32_t address_start = 0x00;
        int32_t address_end = 0x00;
        int32_t temp = 0;
	int32_t temp1=0x0;

        //set 0x5002[3] to int32_t temp1;
        temp1 = Sensor_ReadReg(0x5002);
        Sensor_WriteReg(0x5002, (0x00 & 0x08) | (temp1 & (~0x08)));

        /*read otp into buffer*/
        Sensor_WriteReg(0x3d84, 0xc0);

        switch (cmd)
        {
                case 0:/*for read otp info*/
                {
                        /*program disable, manual mode; select group*/
                        if (index==1) {
                                address_start = 0x7011;
                                address_end = 0x7018;
                        }
                        else if (index==2) {
                                address_start = 0x7019;
                                address_end = 0x7020;
                        }

                        /*partial mode OTP write start address*/
                        Sensor_WriteReg(0x3d88, (address_start>>8));
                        Sensor_WriteReg(0x3d89, (address_start & 0xff));

                        /*partial mode OTP write end address*/
                        Sensor_WriteReg(0x3d8A, (address_end>>8));
                        Sensor_WriteReg(0x3d8B, (address_end & 0xff));
                        Sensor_WriteReg(0x3d81, 0x01);
                        usleep(10*1000);
                        /*load otp into buffer*/
                        (*otp_ptr).module_integrator_id = Sensor_ReadReg(address_start);
                        (*otp_ptr).lens_id = Sensor_ReadReg(address_start + 1);
                        (*otp_ptr).production_year = Sensor_ReadReg(address_start + 2);
                        (*otp_ptr).production_month = Sensor_ReadReg(address_start + 3);
                        (*otp_ptr).production_day = Sensor_ReadReg(address_start + 4);
                }
                break;

                case 1:/*for read otp wb*/
                {
                        /* program disable, manual mode select group*/
                        if (index==1) {
                                address_start = 0x7011;
                                address_end = 0x7018;
                        }
                        else if (index==2) {
                                address_start = 0x7019;
                                address_end = 0x7020;
                        }

                        /*partial mode OTP write start address*/
                        Sensor_WriteReg(0x3d88, (address_start>>8));
                        Sensor_WriteReg(0x3d89, (address_start & 0xff));

                        /*partial mode OTP write end address*/
                        Sensor_WriteReg(0x3d8A, (address_end>>8));
                        Sensor_WriteReg(0x3d8B, (address_end & 0xff));
                        Sensor_WriteReg(0x3d81, 0x01);
                        usleep(10*1000);
                        /*load otp into buffer*/
                        temp = Sensor_ReadReg(address_start + 7);
                        (*otp_ptr).rg_ratio = (Sensor_ReadReg(address_start + 5)<<2) + ((temp>>6) & 0x03);
                        (*otp_ptr).bg_ratio = (Sensor_ReadReg(address_start + 6)<<2) + ((temp>>4) & 0x03);
                        (*otp_ptr).light_rg = 0;
                        (*otp_ptr).light_bg = 0;
                }
                break;

                case 2:/*for read otp lenc*/
                {
                        /* program disable, manual mode; select group*/
                        if (index==1) {
                                address_start = 0x7029;
                                address_end = 0x7119;
                        }
                        else if (index==2) {
                                address_start = 0x711a;
                                address_end = 0x720a;
                        }
                        /*partial mode OTP write start address*/
                        Sensor_WriteReg(0x3d88, (address_start>>8));
                        Sensor_WriteReg(0x3d89, (address_start & 0xff));
                        /*partial mode OTP write end address*/
                        Sensor_WriteReg(0x3d8A, (address_end>>8));
                        Sensor_WriteReg(0x3d8B, (address_end & 0xff));
                        Sensor_WriteReg(0x3d81, 0x01);
                        usleep(10*1000);
                        /* load otp into buffer */
                        for(i=0; i<240; i++) {
                                (* otp_ptr).lenc[i]=Sensor_ReadReg(address_start + i);
                        }
                        (* otp_ptr).checksum=Sensor_ReadReg(address_start + 240);
                }
                break;

                case 3:
                {
                        // program disable, manual mode
                        //check group
                        if(index==1){
                                address_start = 0x7022;
                                address_end = 0x7024;
                        } else if(index==2) {
                                address_start = 0x7025;
                                address_end = 0x7027;
                        }
                        //partial mode OTP write start address
                        Sensor_WriteReg(0x3d88, (address_start>>8));
                        Sensor_WriteReg(0x3d89, (address_start & 0xff));
                        // partial mode OTP write end address
                        Sensor_WriteReg(0x3d8A, (address_end>>8));
                        Sensor_WriteReg(0x3d8B, (address_end & 0xff));
                        Sensor_WriteReg(0x3d81, 0x01);
                        usleep(10*1000);
                        // load otp into buffer
                        //flag and lsb of VCM start code
                        temp = Sensor_ReadReg(address_start + 2);
                        (* otp_ptr).VCM_start = (Sensor_ReadReg(address_start)<<2) | ((temp>>6) & 0x03);
                        (* otp_ptr).VCM_end = (Sensor_ReadReg(address_start + 1) << 2) | ((temp>>4) & 0x03);
                        (* otp_ptr).VCM_dir = (temp>>2) & 0x03;
                }
                break;

                default:
                break;
        }

        /* clear otp buffer */
        for (i=address_start; i<=address_end; i++) {
                Sensor_WriteReg(i, 0x00);
        }

        //set 0x5002[3] to temp1 = Sensor_ReadReg(0x5002);
        Sensor_WriteReg(0x5002, (0x08 & 0x08) | (temp1 & (~0x08)));

        return 0;
}

/*
call this function after OV8858 initialization
return value: 0 update success; 1, no OTP
*/
LOCAL int32_t _ov8858_update_o_film_otp_wb(void)
{
        struct ov8858_otp_info current_otp;
        int32_t i;
        int32_t otp_index;/*bank 1,2,3*/
        int32_t temp, rg, bg;
        int32_t R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
        int32_t BG_Ratio_Typical = BG_RATIO_TYPICAL_OFILM;
        int32_t RG_Ratio_Typical = RG_RATIO_TYPICAL_OFILM;

        /*
        R/G and B/G of current camera module is read out from sensor OTP
        check first OTP with valid data
        */
        for(i=1;i<=2;i++) {
                temp = _ov8858_check_o_film_otp(1, i);
                if (temp == 2) {
                        otp_index = i;
                        break;
                }
        }
        if (i==3) {
                /* no valid wb OTP data*/
                return 1;
        }

        /*set right bank*/
        _ov8858_read_o_film_otp(1, otp_index, &current_otp);
        SENSOR_PRINT_ERR("huanggq, read ov8858 otp, otp_index=%d, rg_ratio=0x%x light_rg=0x%x bg_ratio=0x%x light_bg=0x%x", otp_index,
                                        current_otp.rg_ratio, current_otp.light_rg, current_otp.bg_ratio, current_otp.light_bg );
        if(current_otp.light_rg==0) {
                /*no light source information in OTP, light factor = 1*/
                rg = current_otp.rg_ratio;
        }
        else {
                rg = current_otp.rg_ratio * (current_otp.light_rg + 512) / 1024;
        }
        if(current_otp.light_bg==0) {
                /*not light source information in OTP, light factor = 1*/
                bg = current_otp.bg_ratio;
        }
        else {
                bg = current_otp.bg_ratio * (current_otp.light_bg + 512) / 1024;
        }

        /*calculate G gain
        0x400 = 1x gain*/
        if(bg < BG_Ratio_Typical) {
                if (rg< RG_Ratio_Typical) {
                        /*
                        current_otp.bg_ratio < BG_Ratio_typical &&
                        current_otp.rg_ratio < RG_Ratio_typical
                        */
                        G_gain = 0x400;
                        B_gain = 0x400 * BG_Ratio_Typical / bg;
                        R_gain = 0x400 * RG_Ratio_Typical / rg;
                }
                else {
                        /* current_otp.bg_ratio < BG_Ratio_typical &&
                         current_otp.rg_ratio >= RG_Ratio_typical
                         */
                        R_gain = 0x400;
                        G_gain = 0x400 * rg / RG_Ratio_Typical;
                        B_gain = G_gain * BG_Ratio_Typical /bg;
                }
        }
        else {
                if (rg < RG_Ratio_Typical) {
                        /* current_otp.bg_ratio >= BG_Ratio_typical &&
                        current_otp.rg_ratio < RG_Ratio_typical
                        */
                        B_gain = 0x400;
                        G_gain = 0x400 * bg / BG_Ratio_Typical;
                        R_gain = G_gain * RG_Ratio_Typical / rg;
                }
                else {
                        /*
                        current_otp.bg_ratio >= BG_Ratio_typical &&
                        current_otp.rg_ratio >= RG_Ratio_typical
                        */
                        G_gain_B = 0x400 * bg / BG_Ratio_Typical;
                        G_gain_R = 0x400 * rg / RG_Ratio_Typical;
                        if(G_gain_B > G_gain_R ) {
                                B_gain = 0x400;
                                G_gain = G_gain_B;
                                R_gain = G_gain * RG_Ratio_Typical /rg;
                }
                else {
                        R_gain = 0x400;
                        G_gain = G_gain_R;
                        B_gain = G_gain * BG_Ratio_Typical / bg;
                }
                }
        }

        /*update_awb_gain(R_gain, G_gain, B_gain)
        R_gain, sensor red gain of AWB, 0x400 =1
        G_gain, sensor green gain of AWB, 0x400 =1
        B_gain, sensor blue gain of AWB, 0x400 =1
        */
        if (R_gain>0x400) {//huanggq, ???, 0x5032=MWB mtk, 0x5018=AWBM
                Sensor_WriteReg(0x5032, R_gain>>8);
                Sensor_WriteReg(0x5033, R_gain & 0x00ff);
        }
        if (G_gain>0x400) {
                Sensor_WriteReg(0x5034, G_gain>>8);
                Sensor_WriteReg(0x5035, G_gain & 0x00ff);
        }
        if (B_gain>0x400) {
                Sensor_WriteReg(0x5036, B_gain>>8);
                Sensor_WriteReg(0x5037, B_gain & 0x00ff);
        }

        return 0;
}

/* call this function after OV8858 initialization
return value: 0 update success*/
LOCAL int32_t _ov8858_update_o_film_otp_lenc(void)
{
        struct ov8858_otp_info current_otp;
        int32_t i;
        int32_t otp_index;
        int32_t temp;

        /* check first lens correction OTP with valid data */
        for (i=1;i<=2;i++) {
                temp = _ov8858_check_o_film_otp(2, i);
                if (temp == 2) {
                        otp_index = i;
                        break;
                }
        }

        if (i>2) {
                /* no valid LSC OTP data */
		SENSOR_PRINT_ERR("no valid LSC OTP data");
                return 1;
        }

#if 0
        for( i=0; i < 240; i++)
        //      SENSOR_PRINT_ERR("huanggq, read ov8858 otp, otp_index=%d, 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x ", otp_index,
        //                              current_otp.lenc[0+i*8], current_otp.lenc[1+i*8], current_otp.lenc[2+i*8], current_otp.lenc[3+i*8], current_otp.lenc[4+i*8], current_otp.lenc[5+i*8], current_otp.lenc[6+i*8], current_otp.lenc[7+i*8] );
                current_otp.lenc[i] = 0;
#endif

        _ov8858_read_o_film_otp(2, otp_index, &current_otp);

#if 0
        SENSOR_PRINT_ERR("huanggq, read ov8858 otp, _ov8858_update_o_film_otp_lenc read:");
        for( i=0; i < 240/8; i++)
                SENSOR_PRINT_ERR("huanggq, read ov8858 otp, otp_index=%d, 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x ", otp_index,
                                        current_otp.lenc[0+i*8], current_otp.lenc[1+i*8], current_otp.lenc[2+i*8], current_otp.lenc[3+i*8], current_otp.lenc[4+i*8], current_otp.lenc[5+i*8], current_otp.lenc[6+i*8], current_otp.lenc[7+i*8] );
#endif

        /*update_lenc*/
        temp = Sensor_ReadReg(0x5000);
        temp = 0x80 | temp;
        Sensor_WriteReg(0x5000, temp);
        for(i=0;i<240;i++) {
                Sensor_WriteReg(0x5800 + i, current_otp.lenc[i]);
        }

        return 0;
}


LOCAL uint32_t _ov8858_o_film_Identify_otp(void* param_ptr)
{
        uint32_t rtn=SENSOR_FAIL;
        uint32_t param_id;
        struct ov8858_otp_info current_otp;
        int32_t i;
        int32_t otp_index;
        int32_t temp;
        int32_t ret;

        SENSOR_PRINT_ERR("SENSOR_ov8858: _ov8858_o_film_Identify_otp");

        /*read param id from sensor omap*/
        for (i=1;i<=2;i++) {
                temp = _ov8858_check_o_film_otp(0, i);
                SENSOR_PRINT_ERR("_ov8858_o_film_Identify_otp i=%d temp = %d \n",i,temp);
                if (temp == 2) {
                        otp_index = i;
                        break;
                }
        }

        if (i <= 2) {
                _ov8858_read_o_film_otp(0, otp_index, &current_otp);
                SENSOR_PRINT_ERR("read ov8858 otp  module_id = 0x%x, lens_id=0x%x, %d-%d-%d \n", current_otp.module_integrator_id, current_otp.lens_id,
                                                current_otp.production_year, current_otp.production_month, current_otp.production_day  );
                if (OV8858_RAW_PARAM_O_FILM == current_otp.module_integrator_id) {
                        SENSOR_PRINT_ERR("SENSOR_OV8858: This is o_film module!!\n");
                        rtn = SENSOR_SUCCESS;
                } else {
                        SENSOR_PRINT_ERR("SENSOR_OV8858: check module id faided!!\n");
                        rtn = SENSOR_FAIL;
                }
        } else {
                /* no valid wb OTP data */
                SENSOR_PRINT_ERR("ov8858_check_otp_module_id no valid wb OTP data\n");
                rtn = SENSOR_FAIL;
        }

        return rtn;
}

LOCAL uint32_t  _ov8858_o_film_update_otp(void* param_ptr)
{

        SENSOR_PRINT("SENSOR_OV8858: _ov8858_o_film_update_otp");

        _ov8858_update_o_film_otp_wb();
      _ov8858_update_o_film_otp_lenc();

        return 0;
}

LOCAL uint32_t _ov8858_com_Identify_otp(void* param_ptr)
{
        uint32_t rtn=SENSOR_FAIL;
        uint32_t param_id;

        SENSOR_PRINT_ERR("SENSOR_ov8858: _ov8858_com_Identify_otp");

        /*read param id from sensor omap*/
        param_id=ov8858_RAW_PARAM_COM;

        if(ov8858_RAW_PARAM_COM==param_id){
                rtn=SENSOR_SUCCESS;
        }

        return rtn;
}

LOCAL uint32_t _ov8858_GetRawInof(void)
{
        uint32_t rtn=SENSOR_SUCCESS;
        struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8858_raw_param_tab;
        uint32_t param_id;
        uint32_t i=0x00;
        uint16_t stream_value = 0;

        stream_value = Sensor_ReadReg(0x0100);
        SENSOR_PRINT("_ov8858_GetRawInof:stream_value = 0x%x\n", stream_value);
        if (1 != (stream_value & 0x01)) {
                Sensor_WriteReg(0x0100, 0x01);
                usleep(5 * 1000);
        }

        /*read param id from sensor omap*/
        param_id=ov8858_RAW_PARAM_COM;

        for(i=0x00; ; i++)
        {
                g_module_id = i;
                if(RAW_INFO_END_ID==tab_ptr[i].param_id){
                        if(NULL==s_ov8858_mipi_raw_info_ptr){
                                SENSOR_PRINT_ERR("SENSOR_ov8858: ov5647_GetRawInof no param error");
                                rtn=SENSOR_FAIL;
                        }
                        SENSOR_PRINT_ERR("SENSOR_ov8858: ov8858_GetRawInof end");
                        break;
                }
                else if(PNULL!=tab_ptr[i].identify_otp){
                        if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
                        {
                                s_ov8858_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
                                SENSOR_PRINT_ERR("SENSOR_ov8858: ov8858_GetRawInof success");
                                break;
                        }
                }
        }

        if(1 != (stream_value & 0x01)) {
                Sensor_WriteReg(0x0100, stream_value);
                usleep(5 * 1000);
        }

        return rtn;
}

#endif
/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov8858_group_hold_on(void)
{

}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov8858_group_hold_off(void)
{

}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = ov8858_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_ov8858_ioctl_func_tab = {
	.power = ov8858_power_on,
	.identify = ov8858_identify,
	.get_trim = ov8858_get_resolution_trim_tab,
	.before_snapshort = ov8858_before_snapshot,
	.write_ae_value = ov8858_write_exposure,
	.write_gain_value = ov8858_write_gain_value,
	.af_enable = ov8858_write_af,
	.set_focus = ov8858_ext_func,
	//.set_video_mode = ov8858_set_video_mode,
	.stream_on = ov8858_stream_on,
	.stream_off = ov8858_stream_off,
	#ifdef FEATURE_OTP
	.cfg_otp=_ov8858_cfg_otp,
	#endif

	//.group_hold_on = ov8858_group_hold_on,
	//.group_hold_of = ov8858_group_hold_off,
};
