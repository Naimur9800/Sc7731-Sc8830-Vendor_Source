LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

sc8830like:=0
isp_use2.0:=0
MINICAMERA?=2
TARGET_BOARD_CAMERA_READOTP_METHOD?=0


ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
sc8830like=1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
isp_use2.0=1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
isp_use2.0=1
endif

ifeq ($(strip $(sc8830like)),1)
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/vsp/inc	\
	$(LOCAL_PATH)/vsp/src \
	$(LOCAL_PATH)/jpeg/inc \
	$(LOCAL_PATH)/jpeg/src \
	$(LOCAL_PATH)/common/inc \
	$(LOCAL_PATH)/hal1.0/inc \
	$(LOCAL_PATH)/tool/auto_test/inc \
	$(LOCAL_PATH)/tool/mtrace \
	$(LOCAL_PATH)/hal3/inc \
	$(LOCAL_PATH)/hal3 \
	external/skia/include/images \
	external/skia/include/core\
        external/jhead \
        external/sqlite/dist \
	system/media/camera/include \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/video\
	$(TOP)/vendor/sprd/open-source/libs/libmemoryheapion 

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/open-source/libs/gralloc/midgard
else
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/open-source/libs/gralloc/utgard
endif

# start: sepreate the oem folder to oem and oem2v0 according to isp version
# oem for tshark/sharkl and oem2v0 for tshark2.
ifeq ($(strip $(isp_use2.0)),1)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/oem2v0/inc \
    $(LOCAL_PATH)/oem2v0/isp_calibration/inc
else
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/oem/inc \
    $(LOCAL_PATH)/oem/isp_calibration/inc
endif
# end.

LOCAL_SRC_FILES:= \
	hal1.0/src/SprdCameraParameters.cpp \
	common/src/cmr_msg.c \
	vsp/src/jpg_drv_sc8830.c \
	jpeg/src/jpegcodec_bufmgr.c \
	jpeg/src/jpegcodec_global.c \
	jpeg/src/jpegcodec_table.c \
	jpeg/src/jpegenc_bitstream.c \
	jpeg/src/jpegenc_frame.c \
	jpeg/src/jpegenc_header.c \
	jpeg/src/jpegenc_init.c \
	jpeg/src/jpegenc_interface.c \
	jpeg/src/jpegenc_malloc.c \
	jpeg/src/jpegenc_api.c \
	jpeg/src/jpegdec_bitstream.c \
	jpeg/src/jpegdec_frame.c \
	jpeg/src/jpegdec_init.c \
	jpeg/src/jpegdec_interface.c \
	jpeg/src/jpegdec_malloc.c \
	jpeg/src/jpegdec_dequant.c	\
	jpeg/src/jpegdec_out.c \
	jpeg/src/jpegdec_parse.c \
	jpeg/src/jpegdec_pvld.c \
	jpeg/src/jpegdec_vld.c \
	jpeg/src/jpegdec_api.c  \
	jpeg/src/exif_writer.c  \
	jpeg/src/jpeg_stream.c \
	tool/mtrace/mtrace.c

# start: sepreate the oem folder to oem and oem2v0 according to isp version
# oem for tshark/sharkl and oem2v0 for tshark2.
ifeq ($(strip $(isp_use2.0)),1)
LOCAL_SRC_FILES+= \
    oem2v0/src/SprdOEMCamera.c \
	oem2v0/src/cmr_common.c \
	oem2v0/src/cmr_oem.c \
	oem2v0/src/cmr_setting.c \
	oem2v0/src/cmr_mem.c \
	oem2v0/src/cmr_scale.c \
	oem2v0/src/cmr_rotate.c \
	oem2v0/src/cmr_grab.c \
	oem2v0/src/jpeg_codec.c \
	oem2v0/src/cmr_exif.c \
	oem2v0/src/sensor_cfg.c \
	oem2v0/src/cmr_preview.c \
	oem2v0/src/cmr_snapshot.c \
	oem2v0/src/cmr_sensor.c \
	oem2v0/src/cmr_ipm.c \
	oem2v0/src/cmr_focus.c \
	oem2v0/src/sensor_drv_u.c \
	oem2v0/isp_calibration/src/isp_calibration.c \
	oem2v0/isp_calibration/src/isp_cali_interface.c
else
LOCAL_SRC_FILES+= \
    oem/src/SprdOEMCamera.c \
	oem/src/cmr_common.c \
	oem/src/cmr_oem.c \
	oem/src/cmr_setting.c \
	oem/src/cmr_mem.c \
	oem/src/cmr_scale.c \
	oem/src/cmr_rotate.c \
	oem/src/cmr_grab.c \
	oem/src/jpeg_codec.c \
	oem/src/cmr_exif.c \
	oem/src/sensor_cfg.c \
	oem/src/cmr_preview.c \
	oem/src/cmr_snapshot.c \
	oem/src/cmr_sensor.c \
	oem/src/cmr_ipm.c \
	oem/src/cmr_focus.c \
	oem/src/sensor_drv_u.c \
	oem/isp_calibration/src/isp_calibration.c \
	oem/isp_calibration/src/isp_cali_interface.c
endif
# end.

ifeq ($(strip $(isp_use2.0)),1)
include $(LOCAL_PATH)/isp2.0/isp2_0.mk
LOCAL_SRC_FILES+= \
	sensor/ov5640/sensor_ov5640_mipi.c \
	sensor/ov5640/sensor_ov5640_mipi_raw.c \
	sensor/ov5670/sensor_ov5670_mipi_raw.c \
	sensor/gc2155/sensor_gc2155_mipi.c \
	sensor/ov8825/sensor_ov8825_mipi_raw.c \
	sensor/hi544/sensor_hi544_mipi_raw.c \
	sensor/hi255/sensor_hi255.c \
	sensor/sensor_gc0310_mipi.c \
	sensor/ov5648/sensor_ov5648_mipi_raw.c \
	sensor/ov13850/sensor_ov13850_mipi_raw.c \
	sensor/ov8858/sensor_ov8858_mipi_raw.c \
	sensor/ov2680/sensor_ov2680_mipi_raw.c \
	sensor/s5k4h5yc/sensor_s5k4h5yc_mipi_raw.c \
	sensor/s5k5e3yx/sensor_s5k5e3yx_mipi_raw.c \
	sensor/s5k4h5yc/sensor_s5k4h5yc_mipi_raw_jsl.c \
	sensor/s5k3l2xx/sensor_s5k3l2xx_mipi_raw.c \
	sensor/s5k4h5yc/packet_convert.c

else
include $(LOCAL_PATH)/isp1.0/isp1_0.mk
LOCAL_SRC_FILES+= \
	tool/auto_test/src/SprdCameraHardware_autest_Interface.cpp \
	sensor/sensor_ov8825_mipi_raw.c \
	sensor/sensor_autotest_ov8825_mipi_raw.c\
	sensor/sensor_ov13850_mipi_raw.c \
	sensor/sensor_ov5648_mipi_raw.c \
	sensor/sensor_ov5670_mipi_raw.c \
	sensor/sensor_ov2680_mipi_raw.c \
	sensor/sensor_ov8858_mipi_raw.c \
	sensor/sensor_imx179_mipi_raw.c \
	sensor/sensor_imx219_mipi_raw.c \
	sensor/sensor_hi544_mipi_raw.c \
	sensor/sensor_ov5640_mipi.c \
	sensor/sensor_autotest_ov5640_mipi_yuv.c \
	sensor/sensor_ov5640.c \
	sensor/sensor_autotest_ov5640_ccir_yuv.c \
	sensor/sensor_autotest_ccir_yuv.c \
	sensor/sensor_JX205_mipi_raw.c \
	sensor/sensor_gc2035.c \
	sensor/sensor_gc2155.c \
	sensor/sensor_gc2155_mipi.c \
	sensor/sensor_gc0308.c \
	sensor/sensor_gc0310_mipi.c \
	sensor/sensor_hm2058.c \
	sensor/sensor_ov8865_mipi_raw.c \
	sensor/sensor_gt2005.c \
	sensor/sensor_hi702_ccir.c \
	sensor/sensor_pattern.c \
	sensor/sensor_ov7675.c\
	sensor/sensor_hi253.c\
	sensor/sensor_hi255.c\
	sensor/sensor_s5k4ecgx_mipi.c \
	sensor/sensor_sp2529_mipi.c \
	sensor/sensor_s5k4ecgx.c \
	sensor/sensor_sr352.c \
	sensor/sensor_sr352_mipi.c \
	sensor/sensor_sr030pc50_mipi.c \
	sensor/sensor_s5k4h5yb_mipi_raw.c \
	sensor/sensor_s5k5e3yx_mipi_raw.c


endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
		LOCAL_C_INCLUDES += \
			$(LOCAL_PATH)/arithmetic/omron/inc
        ifeq ($(strip $(isp_use2.0)),1)
            LOCAL_SRC_FILES+= oem2v0/src/cmr_fd_omron.c
        else
            LOCAL_SRC_FILES+= oem/src/cmr_fd_omron.c
        endif
	else
        ifeq ($(strip $(isp_use2.0)),1)
            LOCAL_SRC_FILES+= oem2v0/src/cmr_fd.c
        else
            LOCAL_SRC_FILES+= oem/src/cmr_fd.c
        endif
	endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
    ifeq ($(strip $(isp_use2.0)),1)
        LOCAL_SRC_FILES+= oem2v0/src/cmr_hdr.c
    else
        LOCAL_SRC_FILES+= oem/src/cmr_hdr.c
    endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
    ifeq ($(strip $(isp_use2.0)),1)
        LOCAL_SRC_FILES+= oem2v0/src/cmr_uvdenoise.c
    else
        LOCAL_SRC_FILES+= oem/src/cmr_uvdenoise.c
    endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
    ifeq ($(strip $(isp_use2.0)),1)
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/oem2v0/inc/ydenoise_paten
        LOCAL_SRC_FILES+= oem2v0/src/cmr_ydenoise.c
    else
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/oem/inc/ydenoise_paten
        LOCAL_SRC_FILES+= oem/src/cmr_ydenoise.c
    endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SNR_UV_DENOISE)),true)
    ifeq ($(strip $(isp_use2.0)),1)
#        LOCAL_SRC_FILES+= oem2v0/src/cmr_snr_uvdenoise.c
    else
        LOCAL_SRC_FILES+= oem/src/cmr_snr_uvdenoise.c
    endif
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_HAL_VERSION)),1.0)
LOCAL_SRC_FILES += hal1.0/src/SprdCameraHardwareInterface.cpp
else
LOCAL_SRC_FILES+= \
        hal3/SprdCamera3Factory.cpp \
        hal3/SprdCamera3Hal.cpp \
        hal3/SprdCamera3HWI.cpp \
        hal3/SprdCamera3Channel.cpp \
	hal3/SprdCamera3Mem.cpp \
	hal3/SprdCamera3OEMIf.cpp \
	hal3/SprdCamera3Setting.cpp \
	hal3/SprdCamera3Stream.cpp \
	test.cpp
endif
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS := -fno-strict-aliasing -D_VSP_ -DJPEG_ENC -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -DCONFIG_CAMERA_2M -DANDROID_4100

ifeq ($(strip $(TARGET_BOARD_CAMERA_HAL_VERSION)),1.0)
LOCAL_CFLAGS += -DCONFIG_CAMERA_HAL_VERSION_1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),0)
LOCAL_CFLAGS += -DCONFIG_SP7731GEA_BOARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),1)
LOCAL_CFLAGS += -DCONFIG_SP9630EA_BOARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_VERSION_V3
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_VERSION_V4
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SMALL_PREVSIZE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_CTRL)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_CTRL
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),13M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_13M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),8M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_8M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),5M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_5M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),3M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_3M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_2M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),5M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_5M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),3M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_3M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_2M
endif

ifeq ($(strip $(TARGET_BOARD_NO_FRONT_SENSOR)),true)
LOCAL_CFLAGS += -DCONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_USE_ISP)),true)
else
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP
endif

LOCAL_CFLAGS += -DCONFIG_CAMERA_PREVIEW_YV12

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_BIG_PREVIEW_AND_RECORD_SIZE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_BIG_PREVIEW_RECORD_SIZE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FORCE_ZSL_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FORCE_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANDROID_ZSL_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ANDROID_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_FLICKER)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AFL_AUTO_DETECTION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAF)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_CAF
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_AF_ALG_SPRD)),true)
LOCAL_CFLAGS += -DCONFIG_NEW_AF
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ROTATION_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ROTATION_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_SHAKE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ANTI_SHAKE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DMA_COPY)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_DMA_COPY
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_INTERFACE)),mipi)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_INTERFACE)),ccir)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_INTERFACE)),mipi)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_INTERFACE)),ccir)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_720P)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_720P
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_CIF)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_CIF
endif

ifeq ($(strip $(CAMERA_DISP_ION)),true)
LOCAL_CFLAGS += -DUSE_ION_MEM
endif

ifeq ($(strip $(CAMERA_SENSOR_OUTPUT_ONLY)),true)
LOCAL_CFLAGS += -DCONFIG_SENSOR_OUTPUT_ONLY
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_HDR_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_UV_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_Y_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SNR_UV_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SNR_UV_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_FLASH_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_AUTOFOCUS_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_720P_PREVIEW)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_NO_720P_PREVIEW
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ADAPTER_IMAGE)),180)
LOCAL_CFLAGS += -DCONFIG_CAMERA_IMAGE_180
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_PRE_ALLOC_CAPTURE_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_PRE_ALLOC_CAPTURE_MEM
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phya)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYA
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyb)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYB
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyc)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYC
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyab)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYAB
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phya)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYA
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phyb)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYB
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phyab)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYAB
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_CCIR_PCLK)),source0)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR_PCLK_SOURCE0
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_CCIR_PCLK)),source1)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR_PCLK_SOURCE1
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_CCIR_PCLK)),source0)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR_PCLK_SOURCE0
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_CCIR_PCLK)),source1)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR_PCLK_SOURCE1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAPTURE_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_EXPOSURE_METERING)),true)
LOCAL_CFLAGS += -DCONFIG_EXPOSURE_METERING_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISO_NOT_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISO_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_LOW_CAPTURE_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_LOW_CAPTURE_MEM
endif

ifeq ($(strip $(TARGET_BOARD_MULTI_CAP_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_MULTI_CAP_MEM
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FULL_SCREEN_DISPLAY)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FULL_SCREEN_DISPLAY
endif

LOCAL_CFLAGS += -DCONFIG_READOTP_METHOD=$(TARGET_BOARD_CAMERA_READOTP_METHOD)

LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

ifdef CAMERA_SENSOR_TYPE_BACK
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_BACK=\"$(CAMERA_SENSOR_TYPE_BACK)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_BACK=\"\\0\"
endif
ifdef CAMERA_SENSOR_TYPE_FRONT
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_FRONT=\"$(CAMERA_SENSOR_TYPE_FRONT)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_FRONT=\"\\0\"
endif
ifdef AT_CAMERA_SENSOR_TYPE_BACK
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_BACK=\"$(AT_CAMERA_SENSOR_TYPE_BACK)\"
else
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_BACK=\"\\0\"
endif
ifdef AT_CAMERA_SENSOR_TYPE_FRONT
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_FRONT=\"$(AT_CAMERA_SENSOR_TYPE_FRONT)\"
else
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_FRONT=\"\\0\"
endif

ifeq ($(strip $(TARGET_BOOTLOADER_BOARD_NAME)),ss_sharklt8)
LOCAL_CFLAGS += -DCONFIG_NEW_AF
endif

ifeq ($(strip $(TARGET_BOOTLOADER_BOARD_NAME)),sp9838aea_5mod)
LOCAL_CFLAGS += -DCONFIG_NEW_AF
endif

ifeq ($(strip $(TARGET_BOOTLOADER_BOARD_NAME)),sp9830iec_4m_h100)
LOCAL_CFLAGS += -DCONFIG_NEW_AF
endif

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_CFLAGS += -DCONFIG_GPU_MIDGARD
endif

ifeq ($(strip $(TARGET_VCM_BU64241GWZ)),true)
LOCAL_CFLAGS += -DCONFIG_VCM_BU64241GWZ
endif


#ifeq ($(strip $(MINICAMERA)),2)
#LOCAL_CFLAGS += -DMINICAMERA=2
#else
#LOCAL_CFLAGS += -DMINICAMERA=1
#endif

LOCAL_MODULE_TAGS := optional

ifeq ($(strip $(sc8830like)),1)
LOCAL_SHARED_LIBRARIES :=  libutils libmemoryheapion libcamera_client libcutils libhardware libcamera_metadata
LOCAL_SHARED_LIBRARIES += libui libbinder

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
		LOCAL_STATIC_LIBRARIES +=libeUdnDt libeUdnCo
	else
		LOCAL_SHARED_LIBRARIES += libface_finder
	endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_SHARED_LIBRARIES += libmorpho_easy_hdr
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libuvdenoise
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libynoise
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SNR_UV_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libcnr
endif


ifeq ($(strip $(isp_use2.0)),1)
LOCAL_SHARED_LIBRARIES += libae libspaf libawb liblsc libcalibration
ifeq ($(strip $(TARGET_ARCH)),arm)
LOCAL_SHARED_LIBRARIES += libAF libsft_af_ctrl libdeflicker
endif
else
LOCAL_SHARED_LIBRARIES += libawb libaf liblsc
endif

#ALC_S
ifeq ($(strip $(isp_use2.0)),1)
ifeq ($(strip $(TARGET_ARCH)),arm)
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AWB)),true)
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AWB
LOCAL_SHARED_LIBRARIES += libAl_Awb libAl_Awb_Sp
endif
endif
endif
#ALC_E

include $(BUILD_SHARED_LIBRARY)

#ALC_S
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AWB)),true)

ifeq ($(strip $(TARGET_ARCH)),arm)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := isp2.0/awb/libAl_Awb.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libAl_Awb_Sp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libAl_Awb_Sp.so
LOCAL_MODULE_STEM_64 := libAl_Awb_Sp.so
LOCAL_SRC_FILES_32 :=  isp2.0/awb/libAl_Awb_Sp.so
LOCAL_SRC_FILES_64 :=  isp2.0/awb/libAl_Awb_Sp.so
include $(BUILD_PREBUILT)
else

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := isp2.0/awb/libAl_Awb_Sp.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

endif
#ALC_E

ifeq ($(strip $(sc8830like)),1)

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libisp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libisp.so
LOCAL_MODULE_STEM_64 := libisp.so
ifeq ($(strip $(isp_use2.0)),1)
    LOCAL_SRC_FILES_32 :=  oem2v0/isp/libisp.so
    LOCAL_SRC_FILES_64 :=  oem2v0/isp/libisp_64.so
else
    LOCAL_SRC_FILES_32 :=  oem/isp/libisp.so
    LOCAL_SRC_FILES_64 :=  oem/isp/libisp_64.so
endif
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
ifeq ($(strip $(isp_use2.0)),1)
    LOCAL_PREBUILT_LIBS := oem2v0/isp/libisp.so
else
    LOCAL_PREBUILT_LIBS := oem/isp1.0/libawb.so \
        oem/isp1.0/libaf.so \
        oem/isp1.0/liblsc.so
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
include $(CLEAR_VARS)
ifeq ($(strip $(isp_use2.0)),1)
    LOCAL_PREBUILT_LIBS := oem2v0/isp/libuvdenoise.so
else
    LOCAL_PREBUILT_LIBS := oem/isp/libuvdenoise.so
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
include $(CLEAR_VARS)
ifeq ($(strip $(isp_use2.0)),1)
    LOCAL_PREBUILT_LIBS := oem2v0/isp/libynoise.so
else
    LOCAL_PREBUILT_LIBS := oem/isp/libynoise.so
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SNR_UV_DENOISE)),true)
include $(CLEAR_VARS)
ifeq ($(strip $(isp_use2.0)),1)
#    LOCAL_PREBUILT_LIBS := oem2v0/isp/libcnr.so
else
    LOCAL_PREBUILT_LIBS := oem/isp/libcnr.so
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libmorpho_easy_hdr
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libmorpho_easy_hdr.so
LOCAL_MODULE_STEM_64 := libmorpho_easy_hdr.so
LOCAL_SRC_FILES_32 :=  arithmetic/libmorpho_easy_hdr.so
LOCAL_SRC_FILES_64 :=  arithmetic/lib64/libmorpho_easy_hdr.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := arithmetic/libmorpho_easy_hdr.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
	ifeq ($(strip $(TARGET_ARCH)),arm64)
		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnDt
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libeUdnDt.a
		LOCAL_MODULE_STEM_64 := libeUdnDt.a
		LOCAL_SRC_FILES_32 := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_SRC_FILES_64 := arithmetic/omron/lib64/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)

		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnCo
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libeUdnCo.a
		LOCAL_MODULE_STEM_64 := libeUdnCo.a
		LOCAL_SRC_FILES_32 := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_SRC_FILES_64 := arithmetic/omron/lib64/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)
	else
		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnDt
		LOCAL_SRC_FILES := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnCo
		LOCAL_SRC_FILES := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif # end TARGET_ARCH
	else
		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/libface_finder.so
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif # fd_lib
endif

endif


ifeq ($(strip $(isp_use2.0)),1)

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libae
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libae.so
LOCAL_MODULE_STEM_64 := libae.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/libae.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/libae_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libae.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libawb
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libawb.so
LOCAL_MODULE_STEM_64 := libawb.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/libawb.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/libawb_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libawb.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libcalibration
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libcalibration.so
LOCAL_MODULE_STEM_64 := libcalibration.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/libcalibration.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/libcalibration_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libcalibration.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := isp2.0/sft_af/lib/libAF.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := isp2.0/sft_af/lib/libsft_af_ctrl.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libdeflicker.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libspaf
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libspaf.so
LOCAL_MODULE_STEM_64 := libspaf.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/libspaf.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/libspaf_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/libspaf.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := liblsc
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := liblsc.so
LOCAL_MODULE_STEM_64 := liblsc.so
LOCAL_SRC_FILES_32 :=  oem2v0/isp/liblsc.so
LOCAL_SRC_FILES_64 :=  oem2v0/isp/liblsc_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := oem2v0/isp/liblsc.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

endif

