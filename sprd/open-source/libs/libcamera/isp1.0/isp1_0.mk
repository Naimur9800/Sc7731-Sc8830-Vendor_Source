LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp1.0/inc \
	$(LOCAL_PATH)/isp1.0/uv_denoise/inc

LOCAL_SRC_FILES+= \
	isp1.0/isp_app.c \
	isp1.0/isp_msg.c \
	isp1.0/isp_drv.c \
	isp1.0/isp_ctrl.c \
	isp1.0/isp_smart.c \
	isp1.0/isp_smart_fitting.c \
	isp1.0/aaal/lsc/isp_smart_lsc.c \
	isp1.0/aaal/af/isp_af_ctrl.c \
	isp1.0/aaal/awb/isp_awb_ctrl.c \
	isp1.0/aaal/ae/isp_ae_ctrl_v00.c \
	isp1.0/aaal/ae/isp_ae_ctrl.c \
	isp1.0/aaal/ae/src/isp_alg.c \
	isp1.0/aaal/ae/src/isp_ae_alg_v00.c \
	isp1.0/aaal/ae/src/isp_ae.c \
	isp1.0/isp_app_msg.c \
	isp1.0/isp_video.c \
	isp1.0/isp_param_tune_com.c \
	isp1.0/isp_param_tune_v0000.c \
	isp1.0/isp_param_tune_v0001.c \
	isp1.0/isp_param_size.c \
	isp1.0/isp_param_file_update.c \
	isp1.0/isp_stub_proc.c \
	isp1.0/isp_stub_msg.c \
	isp1.0/isp_otp.c \
	isp1.0/uv_denoise/denoise_app.c
