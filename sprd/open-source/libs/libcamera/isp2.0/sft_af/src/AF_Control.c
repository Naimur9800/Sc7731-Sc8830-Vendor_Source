/* --------------------------------------------------------------------------
 * Confidential
 *                    SiliconFile Technologies Inc.
 *
 *                   Copyright (c) 2015 SFT Limited
 *                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : AF_Control.c
 *
 * $Revision: v1.00
 * $Author 	: silencekjj 
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: DW9804(VCM IC)
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__AF_CONTROL_C__
#define	__AF_CONTROL_C__

/* ------------------------------------------------------------------------ */

#include "../inc/AF_Control.h"

/* ------------------------------------------------------------------------ */

//extern AFDATA afCtl;
//extern AFDATA_RO afv;

/* ------------------------------------------------------------------------ */
void AF_Write_I2C(sft_af_handle_t handle, BYTE id, BYTE addr, BYTE val)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
//	BYTE BUSY = 0;
//	I2CD = id;
//	I2CX = 0xA0;  	// start, ack, write
//	while(BUSY);	
//	
//	I2CD = 0x03;
//	I2CX = 0x20;	// ack, write
//	while(BUSY);	
//	
//	I2CD = addr;
//	I2CX = 0x20;	// ack, write
//	while(BUSY);	
//	
//	I2CD = val;
//	I2CX = 0x60;	// End, ACK
//	while(BUSY);	
	
}

void AF_Write_DAC_I2C(sft_af_handle_t handle, WORD val)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;

//	BYTE	ccc[2] = {0,};

	af_ops->cb_set_motor_pos(handle, val);

//	ccc[0] = 0x03;
//	ccc[1] = val>>8;
//	SFT_AF_LOGE( "CAF_DBG_DY1.....!!! ");
//	af_cxt->af_ctrl_ops.cb_sp_write_i2c(handle, 0x0c, &ccc[0], 2);  
//
//	ccc[0] = 0x04;
//	ccc[1] = val&0xFF;
//	af_ops->cb_sp_write_i2c(handle, 0x0c,  &ccc[0], 2);	
//	SFT_AF_LOGE( "CAF_DBG_DY2.....!!! ");
}

void AF_Read_DAC_I2C(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
//	BYTE BUSY = 0;
//	I2CD = afCtl.bDacId;
//	I2CX = 0xA0;  	// start, ack, write
//	while(BUSY);    	
//
//	I2CD = 0x03;
//	I2CX = 0x20; 	// ack, write
//	while(BUSY);	
//	afv.wMtValueRead = I2CD<<8;
//		
//	I2CD = afCtl.bDacId | 1;
//	I2CX = 0xA0; 	// start, ack, write
//	while(BUSY);	
//	afv.wMtValueRead |= I2CD;
//
//	I2CX = 0x30;   	// ack, read
//	while(BUSY);
//	afv.wMtValueRead = I2CD;
//
//	I2CX = 0x54; 	// stop, read
//	while(BUSY);	
//	afv.wMtValueRead = I2CD | ( afv.wMtValueRead << 8);
}
#endif

/* ------------------------------------------------------------------------ */
/* end of this file															*/
/* ------------------------------------------------------------------------ */
