LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp2.0/isp_app \
	$(LOCAL_PATH)/isp2.0/isp_tune \
	$(LOCAL_PATH)/isp2.0/calibration \
	$(LOCAL_PATH)/isp2.0/driver/inc \
	$(LOCAL_PATH)/isp2.0/param_manager \
	$(LOCAL_PATH)/isp2.0/ae/inc \
	$(LOCAL_PATH)/isp2.0/awb/inc \
	$(LOCAL_PATH)/isp2.0/af/inc \
	$(LOCAL_PATH)/isp2.0/lsc/inc \
	$(LOCAL_PATH)/isp2.0/anti_flicker/inc \
	$(LOCAL_PATH)/isp2.0/smart \
	$(LOCAL_PATH)/isp2.0/utility \
	$(LOCAL_PATH)/isp2.0/calibration/inc
	
ifeq ($(strip $(TARGET_ARCH)),arm)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp2.0/sft_af/inc
endif

LOCAL_SRC_FILES+= \
	isp2.0/isp_app/isp_app.c \
	isp2.0/isp_tune/isp_video.c \
	isp2.0/calibration/isp_otp_calibration.c \
	isp2.0/utility/isp_param_file_update.c \
	isp2.0/af/af_ctrl.c \
	isp2.0/smart/smart_ctrl.c \
	isp2.0/smart/isp_smart.c \
	isp2.0/smart/debug_file.c \
	isp2.0/isp_tune/isp_param_tune_com.c \
	isp2.0/isp_tune/isp_param_size.c \
	isp2.0/isp_tune/isp_otp.c \
	isp2.0/param_manager/isp_blocks_cfg.c \
	isp2.0/param_manager/isp_pm.c \
	isp2.0/param_manager/isp_com_alg.c \
	isp2.0/driver/src/isp_u_interface.c \
	isp2.0/driver/src/isp_u_capability.c \
	isp2.0/driver/src/isp_u_dev.c \
	isp2.0/driver/src/isp_u_fetch.c \
	isp2.0/driver/src/isp_u_blc.c \
	isp2.0/driver/src/isp_u_2d_lsc.c \
	isp2.0/driver/src/isp_u_awb.c \
	isp2.0/driver/src/isp_u_bpc.c \
	isp2.0/driver/src/isp_u_bdn.c \
	isp2.0/driver/src/isp_u_grgb.c \
	isp2.0/driver/src/isp_u_cfa.c \
	isp2.0/driver/src/isp_u_cmc.c \
	isp2.0/driver/src/isp_u_gamma.c \
	isp2.0/driver/src/isp_u_cce.c \
	isp2.0/driver/src/isp_u_prefilter.c \
	isp2.0/driver/src/isp_u_brightness.c \
	isp2.0/driver/src/isp_u_contrast.c \
	isp2.0/driver/src/isp_u_hist.c \
	isp2.0/driver/src/isp_u_aca.c \
	isp2.0/driver/src/isp_u_afm.c \
	isp2.0/driver/src/isp_u_edge.c \
	isp2.0/driver/src/isp_u_emboss.c \
	isp2.0/driver/src/isp_u_fcs.c \
	isp2.0/driver/src/isp_u_css.c \
	isp2.0/driver/src/isp_u_csa.c \
	isp2.0/driver/src/isp_u_store.c \
	isp2.0/driver/src/isp_u_feeder.c \
	isp2.0/driver/src/isp_u_hdr.c \
	isp2.0/driver/src/isp_u_nlc.c \
	isp2.0/driver/src/isp_u_nawbm.c \
	isp2.0/driver/src/isp_u_pre_wavelet.c \
	isp2.0/driver/src/isp_u_binning4awb.c \
	isp2.0/driver/src/isp_u_pgg.c \
	isp2.0/driver/src/isp_u_comm.c \
	isp2.0/driver/src/isp_u_glb_gain.c \
	isp2.0/driver/src/isp_u_rgb_gain.c \
	isp2.0/driver/src/isp_u_yiq.c \
	isp2.0/driver/src/isp_u_hue.c \
	isp2.0/driver/src/isp_u_nbpc.c \
	isp2.0/driver/src/isp_u_pwd.c\
	isp2.0/driver/src/isp_u_1d_lsc.c\
	isp2.0/driver/src/isp_u_raw_awb.c\
	isp2.0/driver/src/isp_u_raw_aem.c\
	isp2.0/driver/src/isp_u_rgb_gain2.c\
	isp2.0/driver/src/isp_u_nlm.c\
	isp2.0/driver/src/isp_u_cmc8.c\
	isp2.0/driver/src/isp_u_ct.c\
	isp2.0/driver/src/isp_u_hsv.c\
	isp2.0/driver/src/isp_u_csc.c\
	isp2.0/driver/src/isp_u_pre_cdn_rgb.c\
	isp2.0/driver/src/isp_u_pstrz.c\
	isp2.0/driver/src/isp_u_raw_afm.c\
	isp2.0/driver/src/isp_u_yiq_aem.c\
	isp2.0/driver/src/isp_u_anti_flicker.c\
	isp2.0/driver/src/isp_u_yiq_afm.c\
	isp2.0/driver/src/isp_u_arbiter.c \
	isp2.0/driver/src/isp_u_cdn.c \
	isp2.0/driver/src/isp_u_comm_v1.c \
	isp2.0/driver/src/isp_u_dispatch.c \
	isp2.0/driver/src/isp_u_hist2.c \
	isp2.0/driver/src/isp_u_iircnr.c \
	isp2.0/driver/src/isp_u_postcdn.c \
	isp2.0/driver/src/isp_u_pre_cdn.c \
	isp2.0/driver/src/isp_u_raw_sizer.c \
	isp2.0/driver/src/isp_u_ydelay.c \
	isp2.0/driver/src/isp_u_ygamma.c \




ifeq ($(strip $(TARGET_BOARD_USE_ALC_AWB)),true)
LOCAL_SRC_FILES+= 	isp2.0/awb/awb_al_ctrl.c
else
LOCAL_SRC_FILES+= 	isp2.0/awb/awb_ctrl.c
endif
	
ifeq ($(strip $(TARGET_ARCH)),arm)
LOCAL_SRC_FILES += \
	isp2.0/sft_af/sp_af_ctrl.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
LOCAL_SRC_FILES+= \
	isp2.0/driver/src/isp_u_rgb2y.c \
	isp2.0/driver/src/isp_u_uv_prefilter.c \
	isp2.0/driver/src/isp_u_yuv_nlm.c
endif

