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
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define DBG_OTP
#ifdef DBG_OTP
#define DEBUG_OTP_STR     "OV5670_OTP_debug: L %d, %s: "
#define DEBUG_OTP_ARGS    __LINE__,__FUNCTION__

#define OTP_PRINT(format,...) ALOGE(DEBUG_OTP_STR format, DEBUG_OTP_ARGS, ##__VA_ARGS__)
#else
#define OTP_PRINT
#endif

#define OV5670_RAW_PARAM_Sunny    0x0001
#define OV5670_RAW_PARAM_Truly    0x0002

static uint16_t RG_Ratio_Typical = 0x17d;
static uint16_t BG_Ratio_Typical = 0x164;

struct otp_struct {
	int module_integrator_id;
	int lens_id;
	int production_year;
	int production_month;
	int production_day;
	int rg_ratio;
	int bg_ratio;
};

LOCAL void ov5670_otp_read_begin(void)
{
	// set reg 0x5002[1] to '0'
	int temp1;
	temp1 = Sensor_ReadReg(0x5002);
	Sensor_WriteReg(0x5002, (temp1 &(~0x02)));
}

LOCAL void ov5670_otp_read_end(void)
{
	// set reg 0x5002[1] to '1'
	int temp1;
	temp1 = Sensor_ReadReg(0x5002);
	Sensor_WriteReg(0x5002, (0x02 | temp1));
}

// index: index of otp group. (1, 2, 3)
// return:  0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL int check_otp_info(int index)
{
	int flag, i;
	int address_start = 0x7010;
	int address_end = 0x7010;
	ov5670_otp_read_begin();
	// read otp into buffer
	Sensor_WriteReg(0x3d84, 0xc0); // program disable, manual mode
	//partial mode OTP write start address 
	Sensor_WriteReg(0x3d88, (address_start>>8));
	Sensor_WriteReg(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (address_end>>8));
	Sensor_WriteReg(0x3d8B, (address_end & 0xff));
	Sensor_WriteReg(0x3d81, 0x01); // read otp
	usleep(5*1000);
	flag = Sensor_ReadReg(0x7010);
	//select group
	if(index==1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if(index==2)
	{
		flag = (flag>>4) & 0x03;
	}
	else
	{
		flag = (flag>>2) & 0x03;
	}
	// clear otp buffer
	for(i=address_start;i<=address_end;i++) {
		Sensor_WriteReg(i, 0x00);
	}
	ov5670_otp_read_end();
	if(flag == 0x00) {
		return 0;
	}
	else if(flag & 0x02) {
		return 1;
	}
	else{
		return 2;
	}
}

// index: index of otp group. (1, 2, 3)
// return:  0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL int check_otp_wb(int index)
{
	int flag, i;
	int address_start = 0x7020;
	int address_end = 0x7020;
	OTP_PRINT("check_otp_wb : index = %d\n", index);
	ov5670_otp_read_begin();
	// read otp into buffer
	Sensor_WriteReg(0x3d84, 0xc0); // program disable, manual mode
	//partial mode OTP write start address 
	Sensor_WriteReg(0x3d88, (address_start>>8));
	Sensor_WriteReg(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (address_end>>8));
	Sensor_WriteReg(0x3d8B, (address_end & 0xff));
	Sensor_WriteReg(0x3d81, 0x01); // read OTP
	usleep(5*1000);
	//select group
	flag = Sensor_ReadReg(0x7020);
	if(index==1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if(index==2)
	{
		flag = (flag>>4) & 0x03;
	}
	else
	{
		flag =( flag>>2) & 0x03;
	}
	OTP_PRINT("check_otp_wb : index = %d, flag = %d\n", index, flag);
	// clear otp buffer
	for(i=address_start;i<=address_end;i++) {
		Sensor_WriteReg(i, 0x00);
	}
	ov5670_otp_read_end();
	if(flag == 0x00) {
		return 0;
	}
	else if(flag & 0x02) {
		return 1;
	}
	else{
		return 2;
	}
}

// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return: 0, 
LOCAL int read_otp_info(int index, struct otp_struct *otp_ptr)
{
	int i;
	int address_start;
	int address_end;
	ov5670_otp_read_begin();
	// read otp into buffer
	Sensor_WriteReg(0x3d84, 0xc0); // program disable, manual mode
	//select group
	if(index==1)
	{
		address_start = 0x7011;
		address_end = 0x7015;
	}
	else if(index==2)
	{
		address_start = 0x7016;
		address_end = 0x701a;
	}
	else
	{
		address_start = 0x701b;
		address_end = 0x701f;
	}
	//partial mode OTP write start address 
	Sensor_WriteReg(0x3d88, (address_start>>8));
	Sensor_WriteReg(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (address_end>>8));
	Sensor_WriteReg(0x3d8B, (address_end & 0xff));
	Sensor_WriteReg(0x3d81, 0x01); // load otp into buffer
	usleep(5*1000);
	(*otp_ptr).module_integrator_id = Sensor_ReadReg(address_start);
	(*otp_ptr).lens_id = Sensor_ReadReg(address_start + 1);
	(*otp_ptr).production_year = Sensor_ReadReg(address_start + 2);
	(*otp_ptr).production_month = Sensor_ReadReg(address_start + 3);
	(*otp_ptr).production_day = Sensor_ReadReg(address_start + 4);
	// clear otp buffer
	for(i=address_start;i<=address_end;i++) {
		Sensor_WriteReg(i, 0x00);
	}
	ov5670_otp_read_end();
	return 0;
}

// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of otp_struct
// return:  0, 
LOCAL int read_otp_wb(int index, struct otp_struct *otp_ptr)
{
	int i, temp;
	int address_start = 0;
	int address_end = 0;
	
	OTP_PRINT("read_otp_wb : index = %d\n", index);
	//select group
	if(index==1)
	{
		address_start = 0x7021;
		address_end = 0x7023;
	}
	else if(index==2)
	{
		address_start = 0x7024;
		address_end = 0x7026;
	}
	else if(index==3)
	{
		address_start = 0x7027;
		address_end = 0x7029;
	}
	ov5670_otp_read_begin();
	// read otp into buffer
	Sensor_WriteReg(0x3d84, 0xc0); // program disable, manual mode
	//partial mode OTP write start address 
	Sensor_WriteReg(0x3d88, (address_start>>8));
	Sensor_WriteReg(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (address_end>>8));
	Sensor_WriteReg(0x3d8B, (address_end & 0xff));
	Sensor_WriteReg(0x3d81, 0x01); // load otp into buffer
	usleep(5*1000);
	temp = Sensor_ReadReg(address_start + 2);
	(*otp_ptr).rg_ratio = (Sensor_ReadReg(address_start )<<2) + ((temp>>6) & 0x03);
	(*otp_ptr).bg_ratio = (Sensor_ReadReg(address_start + 1)<<2) + ((temp>>4) & 0x03);

	OTP_PRINT("index = %d, rg_ratio = %d\n", index, (*otp_ptr).rg_ratio);
	OTP_PRINT("index = %d, bg_ratio = %d\n", index, (*otp_ptr).bg_ratio);

	// clear otp buffer
	for(i=address_start;i<=address_end;i++) {
		Sensor_WriteReg(i, 0x00);
	}
	ov5670_otp_read_end();
	return 0;
}

// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
LOCAL int update_awb_gain(int R_gain, int G_gain, int B_gain)
{
	OTP_PRINT("R_gain = 0x%x,  G_gain = 0x%x,  B_gain = 0x%x\n", R_gain, G_gain, B_gain);
	if(R_gain>=0x400) {
		Sensor_WriteReg(0x5032, R_gain>>8);
		Sensor_WriteReg(0x5033, R_gain & 0x00ff);
	}
	if(G_gain>=0x400) {
		Sensor_WriteReg(0x5034, G_gain>>8);
		Sensor_WriteReg(0x5035, G_gain & 0x00ff);
	}
	if(B_gain>=0x400) {
		Sensor_WriteReg(0x5036, B_gain>>8);
		Sensor_WriteReg(0x5037, B_gain & 0x00ff);
	}
	return 0;
}

// call this function after OV5670 initialization
// return value: 0 update success
// 1, no OTP
LOCAL uint32_t update_otp_wb(void* param_ptr)
{
	struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp;
	int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	int rg,bg;
	OTP_PRINT("update_otp_wb!\n");
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<=3;i++) {
		temp = check_otp_wb(i);
		if(temp == 2) {
			otp_index = i;
			break;
		}
	}
	if(i>3) {
		// no valid wb OTP data
		OTP_PRINT("update_otp_wb : no valid wb OTP data!\n");
		return 1;
	}
	read_otp_wb(otp_index, &current_otp);
	rg = current_otp.rg_ratio;
	bg = current_otp.bg_ratio;
	//calculate G gain
	//0x400 = 1x gain
	if(bg < BG_Ratio_Typical) {
		if(rg < RG_Ratio_Typical) {
			// current_otp.bg_ratio < BG_Ratio_typical && 
			// current_otp.rg_ratio < RG_Ratio_typical
			G_gain = 0x400;
			B_gain = 0x400 * BG_Ratio_Typical / bg;
			R_gain = 0x400 * RG_Ratio_Typical / rg; 
		}
		else{
			// current_otp.bg_ratio < BG_Ratio_typical && 
			// current_otp.rg_ratio >= RG_Ratio_typical
			R_gain = 0x400;
			G_gain = 0x400 * rg / RG_Ratio_Typical;
			B_gain = G_gain * BG_Ratio_Typical /bg;
		}
	}
	else{
		if(rg < RG_Ratio_Typical) {
			// current_otp.bg_ratio >= BG_Ratio_typical && 
			// current_otp.rg_ratio < RG_Ratio_typical
			B_gain = 0x400;
			G_gain = 0x400 * bg / BG_Ratio_Typical;
			R_gain = G_gain * RG_Ratio_Typical / rg;
		}
		else{
			// current_otp.bg_ratio >= BG_Ratio_typical && 
			// current_otp.rg_ratio >= RG_Ratio_typical
			G_gain_B = 0x400 * bg / BG_Ratio_Typical;
			G_gain_R = 0x400 * rg / RG_Ratio_Typical;
			if(G_gain_B > G_gain_R ) {
				B_gain = 0x400;
				G_gain = G_gain_B;
				R_gain = G_gain * RG_Ratio_Typical /rg;
			}
			else{
				R_gain = 0x400;
				G_gain = G_gain_R;
				B_gain = G_gain * BG_Ratio_Typical / bg;
			}
		} 
	}
	update_awb_gain(R_gain, G_gain, B_gain);
	return 0;
}

LOCAL uint32_t ov5670_check_otp_module_id(void)
{
	struct otp_struct current_otp;
	int i = 0;
	int otp_index = 0;
	int temp = 0;
	uint16_t stream_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("ov5670_check_otp_module_id:stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<=3;i++) {
		temp = check_otp_wb(i);
		if(temp == 2) {
			otp_index = i;
			break;
		}
	}
	if(i>3) {
		// no valid wb OTP data
		return 1;
	}
	read_otp_wb(otp_index, &current_otp);

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	SENSOR_PRINT("read ov5670 otp  module_id = %x \n", current_otp.module_integrator_id);

	return current_otp.module_integrator_id;
}

LOCAL uint32_t _ov5670_Truly_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV5670: _ov5670_Truly_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov5670_check_otp_module_id();;

	if(OV5670_RAW_PARAM_Truly==param_id){
		SENSOR_PRINT("SENSOR_OV5670: This is Truly module!!\n");
		RG_Ratio_Typical = 0x152;
		BG_Ratio_Typical = 0x137;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov5670_Sunny_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV5670: _ov5670_Sunny_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov5670_check_otp_module_id();

	if(OV5670_RAW_PARAM_Sunny==param_id){
		SENSOR_PRINT("SENSOR_OV5670: This is Sunny module!!\n");
		RG_Ratio_Typical = 386;
		BG_Ratio_Typical = 367;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}
