/* --------------------------------------------------------------------------
 * Confidential
 *                    SiliconFile Technologies Inc.
 *
 *                   Copyright (c) 2015 SFT Limited
 *                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : AF_Interface.c
 *
 * $Revision: v1.00
 * $Author 	: silencekjj 
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: AP TShark2
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__AF_INTERFACE_C__
#define	__AF_INTERFACE_C__
/* ------------------------------------------------------------------------ */
#include "../inc/AF_Interface.h"
#include "stdio.h"
#include "malloc.h"
#include "string.h"
/* ------------------------------------------------------------------------ */

// AF_Debug.c

/* ------------------------------------------------------------------------ */

void AF_Start_Debug(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;	


	// After Capture
	// This Function is working!!.

	af_ops->cb_set_afm_bypass(handle, 1);	//Turn Off
	SFT_AF_LOGE("DBG_INTERFACE Debug_Start");
	AF_Main_ISR(afCtl, afv, fv, ISR_START_ALGORITHM);
	af_ops->cb_set_afm_mode(handle, 1); 	// 1:Continue, 0:single
	af_ops->cb_set_afm_bypass(handle, 0);	//Turn On
//	AF_SetFvTh(handle);
#ifdef FILTER_SPRD
		af_cxt->sprd_filter.bypass 			= 0; //0;On, 1:Off
		af_cxt->sprd_filter.mode			= 1;		
		af_cxt->sprd_filter.source_pos		= 0;		
		af_cxt->sprd_filter.shift			= 0;
		af_cxt->sprd_filter.skip_num		= 0;
		af_cxt->sprd_filter.skip_num_clear	= 0;
		af_cxt->sprd_filter.format			= 7;
		af_cxt->sprd_filter.iir_bypass		= 1;
		af_cxt->sprd_filter.IIR_c[ 0] 		= 2;  
		af_cxt->sprd_filter.IIR_c[ 1] 		= 89;
		af_cxt->sprd_filter.IIR_c[ 2] 		= -39;
		af_cxt->sprd_filter.IIR_c[ 3] 		= 64;
		af_cxt->sprd_filter.IIR_c[ 4] 		= 128;
		af_cxt->sprd_filter.IIR_c[ 5] 		= 64;
		af_cxt->sprd_filter.IIR_c[ 6] 		= 117;
		af_cxt->sprd_filter.IIR_c[ 7] 		= -55;
		af_cxt->sprd_filter.IIR_c[ 8] 		= 64;
		af_cxt->sprd_filter.IIR_c[ 9] 		= -128;
		af_cxt->sprd_filter.IIR_c[10] 		= 64;

		af_ops->cb_set_sp_afm_cfg(handle);
#endif
	if(afv->bSystem & SYSTEM_FIRST_INIT){
		afv->bSystem = 0;
			afv->bCAFStatus[0] = STATE_CAF_WAIT_LOCK;
			//AF_SetFvTh(handle);
			afv->bCac[473] = AF_CAF_Calc_FPS_Time(afCtl, afv, fv, 3500); // bCAFRestartCnt
			afv->bCac[469] = 2;	
		return;
	}
	AF_SetFvTh(handle, 0);
	afCtl->wLCDSize[AXIS_X] = 960;
	afCtl->wLCDSize[AXIS_Y] = 540;
	SFT_AF_LOGE("DBG_INTERFACE AF_Start_Debug");
	afv->bSystem |= SYSTEM_FIRST_INIT;
}

void AF_Stop_Debug(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;

	// Finished Capture
	SFT_AF_LOGE("DBG_INTERFACE AF_Stop_Debug");
//	if(afCtl->bCtl[1] & ENB1_CAF)
//		AF_CAF_Restart(afCtl, afv, fv, 3000);
}

void AF_Start(sft_af_handle_t handle,BYTE bTouch)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
	BYTE bTransfer[2] = {0,};

//	af_ops->cb_af_move_start_notice(handle);
//	if(bTouch == 1){
//		AF_Main_ISR(afCtl, afv, fv, ISR_START_TOUCH);
//		SFT_AF_LOGE("CAF_DBGG Touch %x", afCtl->bCtl[1]);
//	}else if(bTouch == 0){
//		AF_Main_ISR(afCtl, afv, fv, ISR_START_SAF);
//		SFT_AF_LOGE("CAF_DBGG SAF %x", afCtl->bCtl[1]);
//	}
		
		
	SFT_AF_LOGE("DBG_INTERFACE AF_Start111 %d", bTouch );
	if(bTouch){ // Touch AF
		AF_Main_ISR(afCtl, afv, fv, ISR_START_TOUCH);
	//	AF_Main_ISR(afCtl, afv, fv, ISR_START_SAF);
		SFT_AF_LOGE("DBG_INTERFACE AF_Start Touch");
		SFT_AF_LOGE("CAF_Touch 10 %x", afCtl->bCtl[1]);
		//if(afv->bCAFStatus[2] & STATUS1_CAF_RECORD){
		//	SFT_AF_LOGE("ROC_DBG AF_Start Touch");
		//	SFT_AF_LOGE("ROC_DBG Scan %x, %x, %x", afv->bCAFStatus[0], afv->bCAFStatus[1], afv->bCAFStatus[2]);

		//}
	}else{		// Single AF
		SFT_AF_LOGE("DBG_INTERFACE AF_Start SAF");
		AF_Main_ISR(afCtl, afv, fv, ISR_START_SAF);
	}
	//AF_Running(handle);
	//afv->bState = STATE_NOTHING;
//	SFT_AF_LOGE("CAF_DBGG APP Msg %d", bTouch);
//	switch(bTouch){
//		case AF_CMD_LONG_PRESS_CAPTURE:
//	//		af_ops->cb_af_move_start_notice(handle);
//	//		AF_Main_ISR(afCtl, afv, fv, ISR_START_SAF);
//	//		afv->bSystem |= SYSTEM_WAIT_NOTICE;
//			//AF_Running(handle);
//		SFT_AF_LOGE("CAF_DBGG APP Long Press");
//			break;
//		case AF_CMD_TOUCH_AF:				
//	//		af_ops->cb_af_move_start_notice(handle);
//	//		AF_Main_ISR(afCtl, afv, fv, ISR_START_TOUCH);
//	//		afv->bSystem |= SYSTEM_WAIT_NOTICE;
//			//AF_Running(handle);
//		SFT_AF_LOGE("CAF_DBGG APP Touch");
//			break;
//		case AF_CMD_SHORT_PRESS_CAPTURE:
//	//		afv->bSystem |= SYSTEM_PUSH_CAPTURE;
//		SFT_AF_LOGE("CAF_DBGG APP Short Press");
//			break;
//		case AF_CMD_TOUCH_AF_TAP:			
//	//		af_ops->cb_af_move_start_notice(handle);
//	//		AF_Main_ISR(afCtl, afv, fv, ISR_START_TOUCH);
//	//		afv->bSystem |= SYSTEM_WAIT_NOTICE;
//	//		afv->bSystem |= SYSTEM_PUSH_CAPTURE;
//			//AF_Running(handle);
//		SFT_AF_LOGE("CAF_DBGG APP Touch with Tap");
//			break;
//		case AF_CMD_CAPTURE:				
//	//		afv->bSystem |= SYSTEM_PUSH_CAPTURE;
//		SFT_AF_LOGE("CAF_DBGG APP Capture");
//			break;
//	}
	



//	else if(bTouch == 3)
//		afv->bSystem |= SYSTEM_PUSH_SAF;
	// Start AF
//	if(afv->bStatus[0] & FLAG0_AF_FINISHED){
//		AF_Start_Capture(afCtl, afv, fv, bTouch);
//		return;
//	}else{
//		AF_Start_Capture(afCtl, afv, fv, bTouch);

	//	if(afCtl->bCtl[1] & ENB1_CAF){
	//		return;
	//	}else if(afCtl->bCtl[0] & ENB0_SAF)
	//		AF_Running(handle);
		
		//	if(afCtl->bCtl[7] & ENB7_DEBUG_REG_READ){
		//		// Open AF_Regs.txt
		//		fp = fopen("/data/AF_Regs.txt", "rb");
		//	
		//		if(fp){
		//			SFT_AF_LOGE("AF_Regs File Open OK!!!");
		//	
		//			//fseek(fp, 0, SEEK_END);
		//			//nSize = ftell(fp);
		//			//rewind(fp);
		//			i = sizeof(AFDATA);
		//			tmp = (void*)malloc(i);
		//			
		//			SFT_AF_LOGE("AF_Regs Size = %d", i);
		//			//if(tmp == NULL){
		//			//	SFT_AF_LOGE("File tmp Buff error");
		//			//	goto END_FILE;
		//			//}
		//			fread(tmp, 1, i, fp);
		//			memcpy(&tmpCtl, tmp, i);
		//			*afCtl = tmpCtl;
		//			//&afCtl = (AFDATA)tmp;
		//			SFT_AF_LOGE("AF_Regs %x", afCtl->bCtl[0]);
		//			SFT_AF_LOGE("AF_Regs File Open Success");
		//	
		//			fclose(fp);
		//			free(tmp);
		//		}
		//	}
	

}

void  AF_Running(sft_af_handle_t handle)
{ // 0 : Fail, 1 : Success
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
	BYTE 	bTransfer[4] = {0,};
//	return;


//	if(afv->bSystem & SYSTEM_PUSH_CAPTURE){
//		// Check AFing...
//		if(afv->bStatus[0] & FLAG0_AF_FINISHED){
//			TAKE_PICTURE:
//			AF_Capture_Func(afCtl, afv, fv);
//			if(afv->bStatus[0] & FLAG0_AF_FAIL)
//				af_ops->cb_af_finish_notice(handle, 0);
//			else
//				af_ops->cb_af_finish_notice(handle, 1);
//			af_ops->cb_unlock_ae(handle);
//			af_ops->cb_unlock_awb(handle);
//		}else{
//			if( afv->bCAFStatus[0] > STATE_CAF_LOCK )
//				goto GOTO_KEEP_GOING; 
//			if(afv->bState)
//				goto GOTO_KEEP_GOING;
//			goto TAKE_PICTURE;
//		}
//
//	}else if(afv->bSystem & SYSTEM_FINISH_CAPTURE){	
//	if(afv->bSystem & SYSTEM_FINISH_CAPTURE){
//		SFT_AF_LOGE("CAF_DBG Finish Capture");
//		AF_SetFvTh(handle);
//		af_ops->cb_set_active_win( handle, 0x001F);
//	}

//	if(AF_Capture_Func(afCtl, afv, fv))
//		return;
//
//	if(afv->bSystem & SYSTEM_WAIT_CAPTURE)
//		return;

	// AFing...
//	SFT_AF_LOGE("CAF_DBG SAF_Over In %x %x", afCtl->wMtValue, afv->bCAFStatus[0]);
	AF_ISR(handle);
#ifdef SFT_LOG_ON_INTERFACE
	SFT_AF_LOGE("######## ISR_END %x-%x %x %x ########", afCtl->wMtValue, afv->bState, afv->bCAFStatus[0], afv->bCAFStatus[1]);
	//SFT_AF_LOGE("######## ISR_END %x-%x ########", afv->bSetStep[FIRST_MIN_STEP], afv->bSetStep[FIRST_MAX_STEP]);
	//SFT_AF_LOGE("######## ISR_END %x-%x %x-%x########", afCtl->wRange[0], afCtl->wRange[1], afv->bSetStep[FIRST_MIN_STEP], afv->bSetStep[FIRST_MAX_STEP]);
#endif






//	if(afv->bSystem & SYSTEM_PUSH_TOUCH)
//		rtn = AF_ISR(handle, ISR_START_TOUCH);
//	else if(afv->bSystem & SYSTEM_PUSH_SAF)
//		rtn = AF_ISR(handle, ISR_START_SAF);
//	else
//		rtn = AF_ISR(handle, ISR_RUNING_AF);
//
//	if(rtn){ // FINISHED_AF
//		if(rtn == ISR_FINISHED_AF_FAIL)
//			af_ops->cb_af_finish_notice(handle, 0);
//		if(rtn == ISR_FINISHED_AF_PASS)
//			af_ops->cb_af_finish_notice(handle, 1);
//
//		af_ops->cb_unlock_ae(handle);
//		af_ops->cb_unlock_awb(handle);
//	}
//	else
//		af_ops->cb_af_move_start_notice(handle);

//	if(rtn){
//		if(afv->bSystem & SYSTEM_WAIT_NOTICE){
//			if(rtn == ISR_FINISHED_AF_FAIL)
//				af_ops->cb_af_finish_notice(handle, 0);
//			if(rtn == ISR_FINISHED_AF_PASS)
//				af_ops->cb_af_finish_notice(handle, 1);
//			afv->bSystem &= 0xF7; // Clear Notice
//		}
//		if(afv->bSystem & SYSTEM_PUSH_CAPTURE){
//			// af_ops->cb_af_finish_capture(handle);
//			afv->bSystem &= 0xF7;
//		}
//	}

//	if(afv->bState == STATE_OVER)
//		SFT_AF_LOGE("CAF_DBG SAF_Over Out %x %x", afCtl->wMtValue, afv->bCAFStatus[0]);
}

void AF_Stop(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;

	// AF Stop
//	afCtl->bCtl[0] &= 0xFE;
//	AF_Main_ISR(afCtl, afv, fv, ISR_STOP_ALGORITHM);

	// Close Filter
//	af_ops->cb_set_afm_bypass(handle, 1);	
	SFT_AF_LOGE("DBG_INTERFACE AF_Stop");
	if(afv->bCAFStatus[2] & STATUS1_CAF_RECORD_TOUCH){
		SFT_AF_LOGE("DBG_INTERFACE AF_Stop_Touch");
		AF_SetFvTh(handle, 1);
		afv->bCAFStatus[2] &= 0xFB;
	}
}

void AF_Prv_Start_Notice(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;

//	if(afv->bSystem & SYSTEM_WAIT_CAPTURE)
//		afv->bSystem = SYSTEM_FINISH_CAPTURE;
	AF_SetFvTh(handle, 0);
	af_ops->cb_set_active_win( handle, 0x03FF);
	afv->bCAFStatus[0] = STATE_CAF_WAIT_LOCK;

	SFT_AF_LOGE("DBG_INTERFACE Start_Preview");
}

void AF_Deinit_Notice(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;

	AF_SoftLanding(handle);
}

void AF_Set_Mode(sft_af_handle_t handle, uint32_t mode)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
	BYTE bTransfer[2] = {0,}, bChangeCenter=0;
	WORD	wSize[2];
	struct win_coord	region[25], winPos;

	// mode 
	SFT_AF_LOGE("DBG_INTERFACE Mode %d", mode);

	if(!afv->bFrameCount){
		if(afv->bCompass[0] == 0xFF)
			afv->bCompass[0] = mode;
		else if(afv->bCompass[1] == 0xFF)
			afv->bCompass[1] = mode;

		if((afv->bCompass[0] != 0xFF) && (afv->bCompass[1] != 0xFF)){
			if((!afv->bCompass[0]) && (!afv->bCompass[1])){
				bTransfer[0] = 0x03;
				bTransfer[1] = afCtl->wRange[0]>>8;
				af_ops->cb_sp_write_i2c(handle, 0x0c, &bTransfer[0], 2); 
				bTransfer[0] = 0x04;
				bTransfer[1] = afCtl->wRange[0] & 0xFF;
				af_ops->cb_sp_write_i2c(handle, 0x0c, &bTransfer[0], 2);  
			}
		}
	}

	if(mode == 4){
		SFT_AF_LOGE("DBG_INTERFACE Mode Camcoder Mode %d", mode);
		AF_Mode_Record(afCtl, afv, 1);
		AF_CAF_Restart(afCtl, afv, fv, 0);
		afv->bCAFStatus[0] = STATE_CAF_WAIT_STABLE;
		if(afv->bCAFStatus[2] & STATUS1_CAF_LOCK_AF){
			afv->bCAFStatus[2] &= 0xFB;
			bChangeCenter = 1;
		}
	}else if(mode == 3){
		SFT_AF_LOGE("DBG_INTERFACE Mode Capture Mode %d", mode);
		if(afv->bCAFStatus[2] & STATUS1_CAF_RECORD){
		//	afCtl->bCtl[1] |= 0x01;
		//	afCtl->bWaitFirstStart 	= 23;
		//	afv->bCAFStatus[2] |= STATUS1_CAF_FIRST_TIME;
			AF_CAF_Restart(afCtl, afv, fv, 0);
		}
		af_ops->cb_unlock_ae(handle);
		af_ops->cb_unlock_awb(handle);
		AF_Mode_Record(afCtl, afv, 0);
	}
	if(bChangeCenter){
		af_ops->cb_set_active_win(handle, 0x003F);
		af_ops->cb_get_isp_size(handle, &wSize[1], &wSize[0]);	//handle, width, height
		winPos.start_x 	= 0;	
		winPos.end_x 	= wSize[1];	
		winPos.start_y 	= 0;	
		winPos.end_y 	= wSize[0];	
		AF_Touch_Initial(afCtl, afv, winPos, &region[0], &wSize[0]);
		af_ops->cb_set_afm_win(handle, region);
	}
}
#endif

/* ------------------------------------------------------------------------ */
/* end of this file															*/
/* ------------------------------------------------------------------------ */
