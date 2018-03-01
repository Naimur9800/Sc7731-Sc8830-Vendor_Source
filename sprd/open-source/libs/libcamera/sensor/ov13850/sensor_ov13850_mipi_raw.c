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

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "sensor_ov13850_raw_param_v3.c"
#else
#endif

#define ov13850_I2C_ADDR_W         (0x10)
#define ov13850_I2C_ADDR_R         (0x10)

#define OV13850_FLIP_MIRROR
#define DW9714_VCM_SLAVE_ADDR (0x18>>1)

//#define use_sensor_gain	

#define OV13850_RAW_PARAM_COM  0x0000
#define OV13850_RAW_PARAM_OFLIM  0x0007
#define OV13850_OTP_CASE 			1
#define OV13850_UPDATE_LNC		1
#define OV13850_UPDATE_WB			2
#define OV13850_UPDATE_LNC_WB	3
#define OV13850_UPDATE_VCM		4

#define OV13850_MIN_FRAME_LEN_PRV  0x5e8
static int s_ov13850_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;

LOCAL unsigned long _ov13850_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov13850_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov13850_Identify(unsigned long param);
LOCAL unsigned long _ov13850_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _ov13850_after_snapshot(unsigned long param);
LOCAL unsigned long _ov13850_StreamOn(unsigned long param);
LOCAL unsigned long _ov13850_StreamOff(unsigned long param);
LOCAL unsigned long _ov13850_write_exposure(unsigned long param);
LOCAL unsigned long _ov13850_write_gain(unsigned long param);
LOCAL unsigned long _ov13850_write_af(unsigned long param);
LOCAL unsigned long _ov13850_flash(unsigned long param);
LOCAL unsigned long _ov13850_ExtFunc(unsigned long ctl_param);
LOCAL unsigned long _dw9174_SRCInit(unsigned long mode);
LOCAL uint32_t _ov13850_get_VTS(void);
LOCAL uint32_t _ov13850_set_VTS(int VTS);
LOCAL unsigned long _ov13850_ReadGain(unsigned long param);
LOCAL unsigned long _ov13850_set_video_mode(unsigned long param);
LOCAL uint32_t _ov13850_get_shutter(void);
LOCAL uint32_t _ov13850_com_Identify_otp(void* param_ptr);
//LOCAL uint32_t _ov13850_Oflim_Identify_otp(void* param_ptr);
LOCAL unsigned long _ov13850_cfg_otp(unsigned long  param);
//LOCAL uint32_t _ov13850_update_otp(void* param_ptr);

LOCAL const struct raw_param_info_tab s_ov13850_raw_param_tab[]={
	//{OV13850_RAW_PARAM_COM, &s_ov13850_mipi_raw_info, _ov13850_Oflim_Identify_otp, _ov13850_update_otp},
	{OV13850_RAW_PARAM_COM, &s_ov13850_mipi_raw_info, _ov13850_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_ov13850_mipi_raw_info_ptr=NULL;

static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T ov13850_common_init[] = 
{
	//XVCLK=24Mhz, SCLK=4x120Mhz, MIPI 640Mbps, DACCLK=240Mhz
	{0x0103, 0x01}, // software reset
	{0x0300, 0x01}, // PLL
	{0x0301, 0x00}, // PLL
	{0x0302, 0x28}, // PLL
	{0x0303, 0x00}, // PLL
	{0x030a, 0x00}, // PLL
	{0x300f, 0x11}, // MIPI 10-bit mode
	{0x3010, 0x01}, // MIPI PHY
	{0x3011, 0x76}, // MIPI PHY
	{0x3012, 0x41}, // MIPI 4 lane
	{0x3013, 0x12}, // MIPI control
	{0x3014, 0x11}, // MIPI control
	{0x3106, 0x00},
	{0x3210, 0x47},
	{0x3500, 0x00}, // exposure HH
	{0x3501, 0x67}, // exposure H
	{0x3502, 0x80}, // exposure L
	{0x3506, 0x00}, // short exposure HH
	{0x3507, 0x02}, // short exposure H
	{0x3508, 0x00}, // shour exposure L
#ifdef use_sensor_gain
	{0x3509, 0x00}, // use sensor gain
#else
	{0x3509, 0x10}, // use real gain
#endif	
	{0x350a, 0x00}, // gain H
	{0x350b, 0x10}, // gain L
	{0x350e, 0x00}, // short gain H
	{0x350f, 0x10}, // short gain L
	{0x3601, 0x9c}, // analog control
	{0x3602, 0x02}, // analog control
	{0x3603, 0x48}, // analog control
	{0x3604, 0xa5}, // analog control
	{0x3605, 0x9f}, // analog control
	{0x3607, 0x00}, // analog control
	{0x360a, 0x40}, // analog control
	{0x360b, 0xc8}, // analog control
	{0x360c, 0x4a}, // analog control
	{0x3611, 0x10}, // PLL2
	{0x3612, 0x23},  // PLL2
	{0x3613, 0x33},  // PLL@
	{0x3641, 0x82},
	{0x3660, 0x82},
	{0x3668, 0x20},
	{0x3667, 0xa0},
	{0x3702, 0xa0},
	{0x3703, 0x24},
	{0x3708, 0x3c},
	{0x3709, 0x0d},
	{0x3720, 0x66},
	{0x3722, 0x84},
	{0x3728, 0x00},
	{0x372f, 0x08},
	{0x3710, 0x08},
	{0x3718, 0x10},
	{0x3719, 0x08},
	{0x371c, 0xfc},
	{0x3760, 0x13},
	{0x3761, 0x33},
	{0x3767, 0x24},
	{0x3768, 0x0c},
	{0x3769, 0x34},
	{0x3d84, 0x00},  // OTP program disable                                                                   
	{0x3d85, 0x17},  // OTP power up load data enable, power load setting enable, software load setting enable
	{0x3d8c, 0x73},  // OTP start address H                                                                   
	{0x3d8d, 0xbf},  // OTP start address L                                                                   
	{0x3800, 0x00},  // H crop start H                                                                        
	{0x3801, 0x08},  // H crop start L                                                                        
	{0x3802, 0x00},  // V crop start H                                                                        
	{0x3803, 0x04},  // V crop start L                                                                        
	{0x3804, 0x10},  // H crop end H                                                                          
	{0x3805, 0x97},  // H crop end L                                                                          
	{0x3806, 0x0c},  // V crop end H                                                                          
	{0x3807, 0x4b},  // V crop end L                                                                          
	{0x3808, 0x08},  // H output size H                                                                       
	{0x3809, 0x40},  // H output size L                                                                       
	{0x380a, 0x06},  // V output size H                                                                       
	{0x380b, 0x20},  // V output size L                                                                       
	{0x380c, 0x25},  // HTS H                                                                                 
	{0x380d, 0x80},  // HTS L                                                                                 
	{0x380e, 0x06},  // VTS H                                                                                 
	{0x380f, 0x80},  // VTS L                                                                                 
	{0x3810, 0x00},  // H win off H                                                                           
	{0x3811, 0x04},  // H win off L                                                                           
	{0x3812, 0x00},  // V win off H                                                                           
	{0x3813, 0x02},  // V win off L                                                                           
	{0x3814, 0x31},  // H inc
	{0x3815, 0x31},  // V inc
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x02},  // V flip on, V bin off
	{0x3821, 0x05},  // H mirror off, H bin on
#else
	{0x3820, 0x06},  // V flip off, V bin off 
	{0x3821, 0x01},  // H mirror on, H bin on
#endif
	{0x3834, 0x00},
	{0x3835, 0x1c},  // cut_en, vts_auto, blk_col_dis
	{0x3836, 0x08},
	{0x3837, 0x02},
	{0x4000, 0xf1},  // BLC offset trig en, format change trig en, gain trig en, exp trig en, median en
	{0x4001, 0x00},  // BLC
	{0x400b, 0x0c},  // BLC
	{0x4011, 0x00},  // BLC
	{0x401a, 0x00},  // BLC
	{0x401b, 0x00},  // BLC
	{0x401c, 0x00},  // BLC
	{0x401d, 0x00},  // BLC
	{0x4020, 0x02},  // BLC
	{0x4021, 0x40},  // BLC
	{0x4022, 0x03},  // BLC
	{0x4023, 0x3f},  // BLC
	{0x4024, 0x06},  // BLC
	{0x4025, 0xf8},  // BLC
	{0x4026, 0x07},  // BLC
	{0x4027, 0xf7},  // BLC
	{0x4028, 0x00},  // BLC
	{0x4029, 0x02},  // BLC
	{0x402a, 0x04},  // BLC
	{0x402b, 0x08},  // BLC
	{0x402c, 0x02},  // BLC
	{0x402d, 0x02},  // BLC
	{0x402e, 0x0c},  // BLC
	{0x402f, 0x08},  // BLC
	{0x4500, 0x82},  // BLC
	{0x4501, 0x38},  // BLC
	{0x4603, 0x00},  // VFIFO              
	{0x4837, 0x14},  // MIPI global timing 
	{0x4d00, 0x04},  // temperature monitor
	{0x4d01, 0x42},  // temperature monitor
	{0x4d02, 0xc1},  // temperature monitor
	{0x4d03, 0x93},  // temperature monitor
	{0x4d04, 0xf5},  // temperature monitor
	{0x4d05, 0xc1},  // temperature monitor
	{0x5000, 0x0f},  // windowing enable, BPC on, WPC on, Lenc on
	{0x5001, 0x03},  // BLC enable, MWB on                       
	{0x5002, 0x03},
	{0x5013, 0x40},
	{0x501c, 0x00},
	{0x501d, 0x10},
	{0x5400, 0x00},
	{0x5401, 0x81},
	{0x5402, 0x00},
	{0x5403, 0x00},
	{0x5404, 0x01},
	{0x5405, 0x00},
	{0x5b00, 0x00},
	{0x5b01, 0x00},
	{0x5b02, 0x01},
	{0x5b03, 0x9b},
	{0x5b04, 0xa2},
	{0x5b05, 0x6c},
	{0x5e00, 0x00},  // test pattern disable
	{0x5e10, 0x0c},  // ISP test disable
	{0x0100, 0x00},  // software standby
};

LOCAL const SENSOR_REG_T ov13850_2112x1568_setting[] = 
{
	// Raw 10bit 2112x1568 30fps 4lane 640M bps/lane
	//XVCLK=24Mhz, SCLK=4x120Mhz, MIPI 640Mbps, DACCLK=240Mhz
	{0x0100, 0x00},  // software standby
	{0x0300, 0x01},  // PLL
	{0x0302, 0x28},  // PLL
	{0x3501, 0x67},  // Exposure H
	{0x3801, 0x08},  // H crop start L
	{0x3803, 0x04},  // V crop start L
	{0x3805, 0x97},  // H crop end L
	{0x3807, 0x4b},  // V crop end L
	{0x3808, 0x08},  // H output size H
	{0x3809, 0x40},  // H output size L
	{0x380a, 0x06},  // V output size H
	{0x380b, 0x20},  // V output size L
	{0x380c, 0x25},  // HTS H
	{0x380d, 0x80},  // HTS L
	{0x380e, 0x06},  // VTS H
	{0x380f, 0x80},  // VTS L
	{0x3813, 0x02},  // V win off
	{0x3814, 0x31},  // H inc
	{0x3815, 0x31},  // V inc
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x02},  // V flip on, V bin off
	{0x3821, 0x05},  // H mirror off, H bin on
#else
	{0x3820, 0x06},  // V flip off, V bin off
	{0x3821, 0x01},  // H mirror on, H bin on
#endif
	{0x3836, 0x08}, 
	{0x3837, 0x02}, 
	{0x4020, 0x02}, 
	{0x4021, 0x40}, 
	{0x4022, 0x03}, 
	{0x4023, 0x3f}, 
	{0x4024, 0x06}, 
	{0x4025, 0xf8}, 
	{0x4026, 0x07}, 
	{0x4027, 0xf7}, 
	{0x4603, 0x00},  // VFIFO             
	{0x4837, 0x14},  // MIPI global timing
};

LOCAL const SENSOR_REG_T ov13850_4208x3120_setting[] = 
{
	// Raw 10bit 4208x3120 15fps 4lane 640M bps/lane
	//XVCLK=24Mhz, SCLK=4x120Mhz, MIPI 640Mbps, DACCLK=240Mhz
	{0x0100, 0x00},  // software standby
	{0x0300, 0x01},  // PLL
	{0x0302, 0x28},  // PLL
	{0x3501, 0xcf},  // Exposure H
	{0x3801, 0x14},  // H crop start L
	{0x3803, 0x0c},  // V crop start L
	{0x3805, 0x8b},  // H crop end L
	{0x3807, 0x43},  // V crop end L
	{0x3808, 0x10},  // H output size H
	{0x3809, 0x70},  // H output size L
	{0x380a, 0x0c},  // V output size H
	{0x380b, 0x30},  // V output size L
	{0x380c, 0x25},  // HTS H
	{0x380d, 0x80},  // HTS L
	{0x380e, 0x0d},  // VTS H
	{0x380f, 0x00},  // VTS L
	{0x3813, 0x04},  // V win off
	{0x3814, 0x11},  // H inc
	{0x3815, 0x11},  // V inc
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x00}, //V flip off, V bin off
	{0x3821, 0x04}, //H mirror on, H bin off
#else
	{0x3820, 0x04},  // V flip on, V bin off
	{0x3821, 0x00},  // H mirror off, H bin off
#endif
	{0x3836, 0x04}, 
	{0x3837, 0x01}, 
	{0x4020, 0x04}, 
	{0x4021, 0x90}, 
	{0x4022, 0x0b}, 
	{0x4023, 0xef}, 
	{0x4024, 0x0d}, 
	{0x4025, 0xc0}, 
	{0x4026, 0x0d}, 
	{0x4027, 0xc3}, 
	{0x4603, 0x01},  // VFIFO
	{0x4837, 0x14},  // MIPI global timing
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov13850_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov13850_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13850_2112x1568_setting), 2112, 1568, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13850_4208x3120_setting), 4208, 3120, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov13850_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0,  2112, 1568, 200, 640, 1664, {0, 0,  2112, 1568}},  //vts
	{0, 0, 4208, 3120, 200, 640, 3328, {0, 0, 4208, 3120}},

	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov13850_2112x1568_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};

LOCAL const SENSOR_REG_T  s_ov13850_4208x3120_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};

LOCAL SENSOR_VIDEO_INFO_T s_ov13850_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 200, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov13850_2112x1568_video_tab},
	{{{15, 15, 200, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov13850_4208x3120_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov13850_set_video_mode(unsigned long param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t         i = 0x00;
	uint32_t         mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_ov13850_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov13850_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%lx", param);
	return 0;
}

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov13850_ioctl_func_tab = {
	PNULL,
	_ov13850_PowerOn,
	PNULL,
	_ov13850_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov13850_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov13850_set_brightness,
	PNULL, // _ov13850_set_contrast,
	PNULL,
	PNULL,			//_ov13850_set_saturation,

	PNULL, //_ov13850_set_work_mode,
	PNULL, //_ov13850_set_image_effect,

	_ov13850_BeforeSnapshot,
	_ov13850_after_snapshot,
	_ov13850_flash,
	PNULL,
	_ov13850_write_exposure,
	PNULL,
	_ov13850_write_gain,
	PNULL,
	PNULL,
	_ov13850_write_af,
	PNULL,
	PNULL, //_ov13850_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov13850_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov13850_GetExifInfo,
	_ov13850_ExtFunc,
	PNULL, //_ov13850_set_anti_flicker,
	_ov13850_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov13850_StreamOn,
	_ov13850_StreamOff,
	_ov13850_cfg_otp,
};


SENSOR_INFO_T g_ov13850_mipi_raw_info = {
	ov13850_I2C_ADDR_W,	// salve i2c write address
	ov13850_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN |
	    SENSOR_IMAGE_EFFECT_BLUE |
	    SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	5,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x300A, 0xD8},		// supply two code to identify sensor.
	 {0x300B, 0x50}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	4208,			// max width of source image
	3120,			// max height of source image
	"ov13850",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov13850_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov13850_ioctl_func_tab,	// point to ioctl function table
	&s_ov13850_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov13850_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	1,			// skip frame num before preview
	1,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
	s_ov13850_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov13850_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_ov13850_InitRawTuneInfo(void)
{
		uint32_t rtn=0x00;
	
#if 0
		struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	
		struct isp_mode_param* mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
		struct isp_mode_param* mode_prv0_ptr = raw_sensor_ptr->mode_ptr[1].addr;
		struct isp_mode_param* mode_cap0_ptr = raw_sensor_ptr->mode_ptr[5].addr;
		struct isp_mode_param* mode_video0_ptr = raw_sensor_ptr->mode_ptr[9].addr;
	
		/* modify common mode */
		if (mode_common_ptr != NULL) {
			int i;
			for (i=0; i<mode_common_ptr->block_num; i++) {
				struct isp_block_header* header = &(mode_common_ptr->block_header[i]);
				uint8_t* data = (uint8_t*)mode_common_ptr + header->offset;
				switch (header->block_id)
				{
				case ISP_BLK_PRE_GBL_GAIN_V1: {
						/* modify block data */
						struct sensor_pre_global_gain_param* block = (struct sensor_pre_global_gain_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_BLC_V1: {
						/* modify block data */
						struct sensor_blc_param_v1* block = (struct sensor_blc_param_v1*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_RGB_GAIN_V1: {
						/* modify block data */
						struct sensor_rgb_gain_param* block = (struct sensor_rgb_gain_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_PRE_WAVELET_V1: {
						/* modify block data */
						struct sensor_pwd_param* block = (struct sensor_pwd_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_NLC_V1: {
						/* modify block data */
						struct sensor_nlc_v1_param* block = (struct sensor_nlc_v1_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_2D_LSC: {
						/* modify block data */
						struct sensor_2d_lsc_param* block = (struct sensor_2d_lsc_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_1D_LSC: {
						/* modify block data */
						struct sensor_1d_lsc_param* block = (struct sensor_1d_lsc_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_BINNING4AWB_V1: {
						/* modify block data */
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_AWB_V1: {
						/* modify block data */
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_AE_V1: {
						/* modify block data */
	
						/* modify bypass */
						header->bypass = 0;
					}
					break;
	
				case	ISP_BLK_BPC_V1: {
						/* modify block data */
						struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_BL_NR_V1: {
						/* modify block data */
						struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_GRGB_V1: {
						/* modify block data */
						struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_RGB_GAIN2: {
						/* modify block data */
						struct sensor_rgb_gain2_param* block = (struct sensor_rgb_gain2_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_NLM: {
						/* modify block data */
						struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_CFA_V1: {
						/* modify block data */
						struct sensor_cfa_param_v1* block = (struct sensor_cfa_param_v1*)data;
	
						/* modify bypass */
						header->bypass = 0;
					}
					break;
	
				case	ISP_BLK_CMC10: {
						/* modify block data */
						struct sensor_cmc_v1_param* block = (struct sensor_cmc_v1_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_RGB_GAMC: {
						/* modify block data */
						struct sensor_frgb_gammac_param* block = (struct sensor_frgb_gammac_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_CMC8: {
						/* modify block data */
						struct sensor_cmc_v1_param* block = (struct sensor_cmc_v1_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_CTM: {
						/* modify block data */
						struct sensor_ctm_param* block = (struct sensor_ctm_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_CCE_V1: {
						/* modify block data */
						struct sensor_cce_param_v1* block = (struct sensor_cce_param_v1*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_HSV: {
						/* modify block data */
						struct sensor_hsv_param* block = (struct sensor_hsv_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_RADIAL_CSC: {
						/* modify block data */
						struct sensor_radial_csc_param* block = (struct sensor_radial_csc_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_RGB_PRECDN: {
						/* modify block data */
						struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_POSTERIZE: {
						/* modify block data */
						struct sensor_posterize_param* block = (struct sensor_posterize_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_AF_V1: {
						/* modify block data */
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_YIQ_AEM: {
						/* modify block data */
						struct sensor_yiq_ae_param* block = (struct sensor_yiq_ae_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_YIQ_AFL: {
						/* modify block data */
						struct sensor_y_afl_param* block = (struct sensor_y_afl_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_YIQ_AFM: {
						/* modify block data */
						struct sensor_y_afm_param* block = (struct sensor_y_afm_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_YUV_PRECDN: {
						/* modify block data */
						struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_PREF_V1: {
						/* modify block data */
						struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_BRIGHT: {
						/* modify block data */
						struct sensor_bright_param* block = (struct sensor_bright_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_CONTRAST: {
						/* modify block data */
						struct sensor_contrast_param* block = (struct sensor_contrast_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_HIST_V1: {
						/* modify block data */
						struct sensor_yuv_hists_param* block = (struct sensor_yuv_hists_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_HIST2: {
						/* modify block data */
						struct sensor_yuv_hists2_param* block = (struct sensor_yuv_hists2_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_AUTO_CONTRAST_V1: {
						/* modify block data */
						struct sensor_auto_contrast_param_v1* block = (struct sensor_auto_contrast_param_v1*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_UV_CDN: {
						/* modify block data */
						struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_EDGE_V1: {
						/* modify block data */
						struct sensor_ee_param* block = (struct sensor_ee_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_EMBOSS_V1: {
						/* modify block data */
						struct sensor_emboss_param_v1* block = (struct sensor_emboss_param_v1*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_CSS_V1: {
						/* modify block data */
						struct sensor_css_v1_param* block = (struct sensor_css_v1_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_SATURATION_V1: {
						/* modify block data */
						struct sensor_saturation_v1_param* block = (struct sensor_saturation_v1_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_HUE_V1: {
						/* modify block data */
						struct sensor_hue_param_v1* block = (struct sensor_hue_param_v1*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_UV_POSTCDN: {
						/* modify block data */
						struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_Y_GAMMC: {
						/* modify block data */
						struct sensor_y_gamma_param* block = (struct sensor_y_gamma_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_YDELAY: {
						/* modify block data */
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_IIRCNR_IIR: {
						/* modify block data */
						struct sensor_iir_nr_param* block = (struct sensor_iir_nr_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_UVDIV_V1: {
						/* modify block data */
						struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				case	ISP_BLK_IIRCNR_YRANDOM: {
						/* modify block data */
						struct sensor_iir_yrandom_param* block = (struct sensor_iir_yrandom_param*)data;
	
						/* modify bypass */
						header->bypass = 1;
					}
					break;
	
				default:
					break;
				}
			}
		}
#endif
		return rtn;
}

LOCAL unsigned long _ov13850_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_ov13850_Resolution_Trim_Tab);
	return (unsigned long) s_ov13850_Resolution_Trim_Tab;
}

LOCAL unsigned long _ov13850_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov13850_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov13850_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov13850_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov13850_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov13850_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov13850_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		_dw9174_SRCInit(2);
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
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov13850: _ov13850_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

#if 0
LOCAL uint32_t _ov13850_update_otp(void* param_ptr)
{
	uint16_t stream_value = 0;
	uint32_t rtn = SENSOR_FAIL;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("stream_value = 0x%x  OV13850_OTP_CASE = %d\n", stream_value, OV13850_OTP_CASE);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}

	switch(OV13850_OTP_CASE){
		case OV13850_UPDATE_LNC:
			rtn = ov13850_update_otp_lenc();
			break;
		case OV13850_UPDATE_WB:
			rtn = ov13850_update_otp_wb();
			break;

		case OV13850_UPDATE_LNC_WB:
			rtn = ov13850_update_otp_lenc();
			rtn = ov13850_update_otp_wb();
			break;

		case OV13850_UPDATE_VCM:

			break;
		default:
			break;
	}

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	return rtn;
}
#endif

LOCAL unsigned long _ov13850_cfg_otp(unsigned long  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov13850_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_OV13850: _ov13850_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

#if 0
LOCAL uint32_t _ov13850_Oflim_Identify_otp(void* param_ptr)
{
	struct otp_struct current_otp;
	uint32_t i = 0;
	uint32_t temp = 0;
	uint16_t stream_value = 0;
	uint32_t rtn=SENSOR_FAIL;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<=3;i++) {
		temp = ov13850_read_otp_info(i, &current_otp);
		if(OV13850_RAW_PARAM_OFLIM == current_otp.module_integrator_id){
			rtn=SENSOR_SUCCESS;
			current_otp.index = i;
			SENSOR_PRINT("This is OV13850 OFLIM Module ! index = %d\n", current_otp.index);
			break;
		}
	}
	if (i > 3) {
		// no valid wb OTP data
		SENSOR_PRINT("ov13850_check_otp_module_id no valid wb OTP data\n");
		return 1;
	}

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	SENSOR_PRINT("read ov13850 otp  module_id = %x \n", current_otp.module_integrator_id);

	return rtn;
}
#endif

LOCAL uint32_t _ov13850_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV13850: _ov13850_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=OV13850_RAW_PARAM_COM;

	if(OV13850_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov13850_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov13850_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=OV13850_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_ov13850_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_OV13850: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_OV13850: ov13850_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_ov13850_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_OV13850: ov13850_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _ov13850_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov13850_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov13850_Identify(unsigned long param)
{
#define ov13850_PID_VALUE    0xD8
#define ov13850_PID_ADDR     0x300A
#define ov13850_VER_VALUE    0x50
#define ov13850_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_ov13850: mipi raw identify\n");

	pid_value = Sensor_ReadReg(ov13850_PID_ADDR);
	if (ov13850_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov13850_VER_ADDR);
		SENSOR_PRINT("SENSOR_ov13850: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov13850_VER_VALUE == ver_value) {
			SENSOR_PRINT("SENSOR_ov13850: this is ov13850 sensor !");
			ret_value=_ov13850_GetRawInof();
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT_ERR("SENSOR_ov13850: the module is unknow error !");
			}
			Sensor_ov13850_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_HIGH("SENSOR_ov13850: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR_ov13850: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

LOCAL uint32_t OV13850_get_shutter()
{
	// read shutter, in number of line period
	uint32_t shutter = 0;
	
	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);
	
	return shutter;
}

LOCAL unsigned long  OV13850_set_shutter(unsigned long shutter)
{
	// write shutter, in number of line period
	uint16_t temp = 0;
	
	shutter = shutter & 0xffff;
	temp = shutter & 0x0f;
	temp = temp<<4;
	Sensor_WriteReg(0x3502, temp);
	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg(0x3501, temp);
	temp = (shutter>>12) & 0xf;
	Sensor_WriteReg(0x3500, temp);
	
	return 0;
}

LOCAL uint32_t _ov8825_get_VTS(void)
{
	// read VTS from register settings
	uint32_t VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte
	VTS = ((VTS & 0x7f)<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL unsigned long OV13850_set_VTS(unsigned long VTS)
{
	// write VTS to registers
	uint16_t temp = 0;
	
	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);
	temp = (VTS>>8) & 0x7f;
	Sensor_WriteReg(0x380e, temp);
	
	return 0;
}

//sensor gain
LOCAL uint16_t OV13850_get_sensor_gain16()
{
	// read sensor gain, 16 = 1x
	uint16_t gain16 = 0;
	
	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg( 0x350b);
	gain16 = ((gain16 >> 4) + 1)*((gain16 & 0x0f) + 16);
	
	return gain16;
}

LOCAL uint16_t OV13850_set_sensor_gain16(int gain16)
{
	// write sensor gain, 16 = 1x
	int gain_reg = 0; 
	uint16_t temp = 0;
	int i = 0;
	
	if(gain16 > 0x7ff)
	{
		gain16 = 0x7ff;
	}
	
	for(i=0; i<5; i++) {
		if (gain16>31) {
			gain16 = gain16/2;
			gain_reg = gain_reg | (0x10<<i);
		}
		else
			break;
	}
	gain_reg = gain_reg | (gain16 - 16);
	temp = gain_reg & 0xff;
	Sensor_WriteReg(0x350b, temp);
	temp = gain_reg>>8;
	Sensor_WriteReg(0x350a, temp);
	return 0;
}

LOCAL unsigned long _ov13850_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	SENSOR_PRINT("SENSOR_OV13850: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	max_frame_len=_ov13850_GetMaxFrameLine(size_index);
	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+4)> max_frame_len) ? (expsure_line+4) : max_frame_len;

		frame_len_cur = _ov8825_get_VTS();
		
		SENSOR_PRINT("SENSOR_OV13850: frame_len: %d,   frame_len_cur:%d\n", frame_len, frame_len_cur);
		
		if(frame_len_cur != frame_len){
			OV13850_set_VTS(frame_len);
		}
	}
	OV13850_set_shutter(expsure_line);
	
	return ret_value;
}

LOCAL unsigned long _ov13850_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

#ifdef use_sensor_gain
	SENSOR_PRINT("SENSOR_OV5648: param: 0x%x", param);
	value = param & 0xff;
	Sensor_WriteReg(0x350b, value);
	value = (param>>8) & 0x7;
	Sensor_WriteReg(0x350a, value);

#else
#if 1
	real_gain = param>>3;
#else
	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);
#endif
	SENSOR_PRINT("SENSOR_OV13850: real_gain:0x%x, param: 0x%x", real_gain, param);

	value = real_gain & 0xff;
	Sensor_WriteReg(0x350b, value);
	value = (real_gain>>8) & 0x7;
	Sensor_WriteReg(0x350a, value);
#endif
	

	return ret_value;
}

LOCAL unsigned long _ov13850_write_af(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t  slave_addr = DW9714_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 0;
	uint8_t cmd_val[2] = {0x00};

	cmd_val[0] = ((param&0xfff0)>>4) & 0x3f;
	cmd_val[1] = ((param&0x0f)<<4) & 0xf0;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_OV13850: _write_af, ret =  %d, param = %d,  MSL:%x, LSL:%x\n", 
		ret_value, param, cmd_val[0], cmd_val[1]);
	return ret_value;
}

LOCAL unsigned long _ov13850_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_ov13850_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov13850_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_ov13850: BeforeSnapshot mode: 0x%08x",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_ov13850: prv mode equal to capmode");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	preview_maxline = (ret_h << 8) + ret_l;

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	SENSOR_PRINT("SENSOR_ov13850: prv_linetime = %d   cap_linetime = %d\n", prv_linetime, cap_linetime);

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_ov13850: prvline equal to capline");
		//goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	capture_maxline = (ret_h << 8) + ret_l;

	capture_exposure = preview_exposure * prv_linetime/cap_linetime;
	//capture_exposure *= 2;

	if(0 == capture_exposure){
		capture_exposure = 1;
	}
	SENSOR_PRINT("SENSOR_ov13850: capture_exposure = %d   capture_maxline = %d\n", capture_exposure, capture_maxline);

	if(capture_exposure > (capture_maxline - 4)){
		capture_maxline = capture_exposure + 4;
		ret_l = (unsigned char)(capture_maxline&0x0ff);
		ret_h = (unsigned char)((capture_maxline >> 8)&0xff);
		Sensor_WriteReg(0x380e, ret_h);
		Sensor_WriteReg(0x380f, ret_l);
	}
	ret_l = ((unsigned char)capture_exposure&0xf) << 4;
	ret_m = (unsigned char)((capture_exposure&0xfff) >> 4) & 0xff;
	ret_h = (unsigned char)(capture_exposure >> 12);

	Sensor_WriteReg(0x3502, ret_l);
	Sensor_WriteReg(0x3501, ret_m);
	Sensor_WriteReg(0x3500, ret_h);
	usleep(200*1000);

CFG_INFO:
	s_capture_shutter = _ov13850_get_shutter();
	s_capture_VTS = _ov13850_get_VTS();
	_ov13850_ReadGain(capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov13850_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov13850: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t)param);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov13850_flash(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov13850: param=%d", param);

	/* enable flash, disable in _ov13850_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT("end");
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov13850_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov13850: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov13850_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov13850: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(100*1000);

	return 0;
}

LOCAL uint32_t _ov13850_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

LOCAL uint32_t _ov13850_set_shutter(int shutter)
{
	// write shutter, in number of line period
	int temp;

	shutter = shutter & 0xffff;

	temp = shutter & 0x0f;
	temp = temp<<4;
	Sensor_WriteReg(0x3502, temp);

	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg(0x3501, temp);

	temp = shutter>>12;
	Sensor_WriteReg(0x3500, temp);

	return 0;
}

LOCAL int _ov13850_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg(0x350b);

	return gain16;
}

LOCAL int _ov13850_set_gain16(int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg(0x350b, temp);

	temp = gain16>>8;
	Sensor_WriteReg(0x350a, temp);

	return 0;
}

static void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_ov13850_set_gain16(capture_gain16);

	// write capture shutter
	/*if (capture_shutter > (capture_VTS - 4)) {
		capture_VTS = capture_shutter + 4;
		OV5640_set_VTS(capture_VTS);
	}*/
	_ov13850_set_shutter(capture_shutter);
}

static unsigned long _ov13850_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_ov13850_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR_ov13850: _ov13850_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_ov13850_gain/2,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_ov13850_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_ov13850_gain,s_capture_VTS,s_capture_shutter *4);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL unsigned long _ov13850_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("0x%x", ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _ov13850_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL uint32_t _ov13850_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL uint32_t _ov13850_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);

	temp = VTS>>8;
	Sensor_WriteReg(0x380e, temp);

	return 0;
}
LOCAL unsigned long _ov13850_ReadGain(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;

	s_ov13850_gain=(int)gain;

	SENSOR_PRINT("SENSOR_ov13850: _ov13850_ReadGain gain: 0x%x", s_ov13850_gain);

	return rtn;
}

LOCAL unsigned long _dw9174_SRCInit(unsigned long mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;	
	int i = 0;
	
	slave_addr = DW9714_VCM_SLAVE_ADDR;
	SENSOR_PRINT("SENSOR_HI542: _DW9714A_SRCInit: mode = %d\n", mode);
	switch (mode) {
		case 1:
		break;
		
		case 2:
		{
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xa1;
			cmd_val[1] = 0x0e;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x90;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}
