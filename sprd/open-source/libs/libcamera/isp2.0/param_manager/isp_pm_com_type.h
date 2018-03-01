
/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _ISP_PM_COM_TYPE_H_
#define _ISP_PM_COM_TYPE_H_

 #include "isp_type.h"
 
 #ifdef	 __cplusplus
extern	 "C"
{
#endif

#define ISP_TUNE_MODE_MAX 16
#define ISP_TUNE_BLOCK_MAX 256

enum isp_pm_blk_cmd {
	ISP_PM_BLK_ISP_SETTING = 0x0000,
	ISP_PM_BLK_SMART_SETTING,
	ISP_PM_BLK_SCENE_MODE,
	ISP_PM_BLK_SPECIAL_EFFECT,
	ISP_PM_BLK_MEMORY_INIT,
	//for blc
	ISP_PM_BLK_BLC_BASE = 0x0100,
	ISP_PM_BLK_BLC_BYPASS,
	ISP_PM_BLK_BLC_OFFSET,
	ISP_PM_BLK_BLC_MODE,
	ISP_PM_BLK_BLC_OFFSET_GB,

	//for nlc
	ISP_PM_BLK_NLC_BASE = 0x0200,
	ISP_PM_BLK_NLC_BYPASS,
	ISP_PM_BLK_NLC,

	//for lsc
	ISP_PM_BLK_LSC_BASE = 0x0300,
	ISP_PM_BLK_LSC_MEM_ADDR,
	ISP_PM_BLK_LSC_VALIDATE,
	ISP_PM_BLK_LSC_PARAM,
	ISP_PM_BLK_LSC_BYPASS,
	ISP_PM_BLK_LSC_INFO,
	ISP_PM_BLK_LSC_OTP,
	ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE = (1<<0),
	ISP_PM_BLK_LSC_UPDATE_MASK_PARAM = (1<<1),

	//for awb
	ISP_PM_BLK_AWB_BASE =0x0400,
	ISP_PM_BLK_AWBM_BYPASS,
	ISP_PM_BLK_AWBC_BYPASS,
	ISP_PM_BLK_AWB_CT,
	ISP_PM_BLK_AWBM,
	ISP_PM_BLK_AWBC,
	ISP_PM_BLK_AWBM_STATISTIC,
	ISP_PM_BLK_AWB_MASK_AWBC = (1<<0),
	ISP_PM_BLK_AWB_MASK_AWBM = (1<<1),

	//for ae
	ISP_PM_BLK_AE_BASE = 0x0500,
	ISP_PM_BLK_AE_BYPASS,
	ISP_PM_BLK_AE_PARAM,
	ISP_PM_BLK_AEM_STATISTIC,

	//for bpc
	ISP_PM_BLK_BPC_BASE = 0x0600,
	ISP_PM_BLK_BPC_BYPASS,
	ISP_PM_BLK_BPC,
	ISP_PM_BLK_BPC_MODE,
	ISP_PM_BLK_BPC_THRD,
	ISP_PM_BLK_BPC_MAP_ADDR,

	//for bl nr
	ISP_PM_BLK_BL_NR_BASE = 0x0700,
	ISP_PM_BLK_BL_NR_BYPASS,
	ISP_PM_BLK_BL_NR_RANWEI_STRENGTH,
	ISP_PM_BLK_BL_NR_DISWEI_STRENGTH,

	//for grgb
	ISP_PM_BLK_GRGB_BASE = 0x0800,
	ISP_PM_BLK_GRGB_BYPASS,
	ISP_PM_BLK_GRGB_GAIN,

	//for cfa
	ISP_PM_BLK_CFA_BASE = 0x0900,
	ISP_PM_BLK_CFA,
	ISP_PM_BLK_CFA_BYPASS,

	//for cmc
	ISP_PM_BLK_CMC_BASE = 0x0A00,
	ISP_PM_BLK_CMC_BYPASS,
	ISP_PM_BLK_CMC,

	//for gamma
	ISP_PM_BLK_GAMMA_BASE = 0x0B00,
	ISP_PM_BLK_GAMMA_BYPASS,
	ISP_PM_BLK_GAMMA,
	ISP_PM_BLK_GAMMA_CORR1,
	ISP_PM_BLK_GAMMA_CORR2,

	//for ygamma
	ISP_PM_BLK_YGAMMA_BASE = 0x0C00,
	ISP_PM_BLK_YGAMMA_BYPSS,
	ISP_PM_BLK_YGAMMA,

	//for flicker
	ISP_PM_BLK_FLICKER_BASE = 0x0D00,
	ISP_PM_BLK_FLICKER_BYPASS,
	ISP_PM_BLK_FLICKER,

	//for cce
	ISP_PM_BLK_CCE_BASE = 0x0E00,
	ISP_PM_BLK_CCE,

	//for uv div
	ISP_PM_BLK_UV_DIV_BASE = 0x0F00,
	ISP_PM_BLK_UV_DIV_BYPSS,
	ISP_PM_BLK_UV_DIV,

	//for pref
	ISP_PM_BLK_PREF_BASE = 0x1000,
	ISP_PM_BLK_PREF_BYPSS,
	ISP_PM_BLK_PREF,

	//for bright
	ISP_PM_BLK_BRIGHT_BASE = 0x1100,
	ISP_PM_BLK_BRIGHT_BYPASS,
	ISP_PM_BLK_BRIGHT,

	//for contrast
	ISP_PM_BLK_CONTRAST_BASE = 0x1200,
	ISP_PM_BLK_CONTRAST_BYPASS,
	ISP_PM_BLK_CONTRAST,

	//for hist
	ISP_PM_BLK_HIST_BASE = 0x1300,
	ISP_PM_BLK_HIST_BYPASS,
	ISP_PM_BLK_HIST,
	ISP_PM_BLK_HIST_RATIO,
	ISP_PM_BLK_HIST_MODE,
	ISP_PM_BLK_HIST_RANGE,

	//for saturation
	ISP_PM_BLK_SATURATION_BASE = 0x1400,
	ISP_PM_BLK_SATURATION_BYPASS,
	ISP_PM_BLK_SATURATION,

	//for hue
	ISP_PM_BLK_HUE_BASE = 0x1500,
	ISP_PM_BLK_HUE_BYPASS,
	ISP_PM_BLK_HUE,

	//for af
	ISP_PM_BLK_AF_BASE = 0x1600,
	ISP_PM_BLK_AF_BYPASS,
	ISP_PM_BLK_AFM,

	//for edge
	ISP_PM_BLK_EDGE_BASE = 0x1700,
	ISP_PM_BLK_EDGE_BYPASS,
	ISP_PM_BLK_EDGE,
	ISP_PM_BLK_EDGE_STRENGTH,
	ISP_PM_BLK_EDGE_DETAIL_THRD,
	ISP_PM_BLK_EDGE_SMOOTH_THRD,

	//for emboss
	ISP_PM_BLK_EMBOSS_BASE = 0x1800,
	ISP_PM_BLK_EMBOSS_BYPASS,
	ISP_PM_BLK_EMBOSS,
	
	//for fcs
	ISP_PM_BLK_FCS_BASE = 0x1900,
	ISP_PM_BLK_FCS_BYPASS,
	ISP_PM_BLK_FCS,

	//for css
	ISP_PM_BLK_CSS_BASE = 0x1A00,
	ISP_PM_BLK_CSS_BYPASS,
	ISP_PM_BLK_CSS,

	//for hdr
	ISP_PM_BLK_HDR_BASE = 0x1B00,
	ISP_PM_BLK_HDR_BYPASS,
	ISP_PM_BLK_HDR,
	ISP_PM_BLK_HDR_LEVEL,
	ISP_PM_BLK_HDR_INDEX,

	//for global gain
	ISP_PM_BLK_GBL_GAIN_BASE = 0x1C00,
	ISP_PM_BLK_GBL_GAIN_BYPASS,
	ISP_PM_BLK_GBL_GAIN,

	//for channel gain
	ISP_PM_BLK_CHN_GAIN_BASE = 0x1D00,
	ISP_PM_BLK_CHN_GAIN_BYPASS,
	ISP_PM_BLK_CHN_GAIN,
	ISP_PM_BLK_CHN_OFFSET,

	//for flash
	ISP_PM_BLK_FLASH_BASE = 0x1E00,
	ISP_PM_BLK_FLASH,
	ISP_PM_BLK_FLASH_RATION_LUM,
	ISP_PM_BLK_FLASH_RATION_RGB,

	//for pre-global gain
	ISP_PM_BLK_PRE_GBL_GAIN_BASE = 0x1F00,
	ISP_PM_BLK_PRE_GBL_GIAN_BYPASS,
	ISP_PM_BLK_PRE_GBL_GAIN,	

	ISP_PM_BLK_AUTO_CONTRAST_BASE = 0x2000,
	ISP_PM_BLK_AUTO_CONTRAST_BYPASS,
	ISP_PM_BLK_AUTO_CONTRAST,
	ISP_PM_BLK_AUTO_CONTRAST_MODE,
	ISP_PM_BLK_AUTO_CONTRAST_RANGE,
	
	ISP_PM_BLK_YIQ_BASE = 0x2100,
	ISP_PM_BLK_YIQ_AE,
	ISP_PM_BLK_YIQ_AE_BYPASS,
	ISP_PM_BLK_YIQ_AE_SKIP_NUM,
	ISP_PM_BLK_YIQ_AE_MODE,
	ISP_PM_BLK_YIQ_AE_SRC_SEL,
	
	ISP_PM_BLK_SPE_BASE = 0x2200,
	ISP_PM_BLK_SPE_MODE_CHNG,

	//for nawbm
	ISP_PM_BLK_NAWBM_BASE = 0x2300,
	ISP_PM_BLK_NAWBM_BYPASS,

	//for pre_wavelet
	ISP_PM_BLK_PRE_WAVELET_BASE = 0x2400,
	ISP_PM_BLK_PRE_WAVELET_BYPASS,
	ISP_PM_BLK_PRE_WAVELET_STRENGTH_LEVEL,

	//for binning4awb
	ISP_PM_BLK_BINNING4AWB_BASE = 0x2500,
	ISP_PM_BLK_BINNING4AWB_BYPASS,

	//for nbpc
	ISP_PM_BLK_NBPC_BASE = 0x2700,
	ISP_PM_BLK_NBPC_BYPASS,

	//for vst
	ISP_PM_BLK_VST_BASE = 0x2800,
	ISP_PM_BLK_VST_BYPASS,

	//for vst
	ISP_PM_BLK_NLM_BASE = (ISP_PM_BLK_VST_BASE + 0x100),
	ISP_PM_BLK_NLM_BYPASS,
	ISP_PM_BLK_NLM_STRENGTH_LEVEL,

	//for vst
	ISP_PM_BLK_IVST_BASE = (ISP_PM_BLK_NLM_BASE + 0x100),
	ISP_PM_BLK_IVST_BYPASS,

	//for cmc10
	ISP_PM_BLK_CMC10_BASE = (ISP_PM_BLK_IVST_BASE + 0x100),
	ISP_PM_BLK_CMC10_BYPASS,
	ISP_PM_BLK_CMC10,

	//for cmc8
	ISP_PM_BLK_CMC8_BASE = (ISP_PM_BLK_CMC10_BASE + 0x100),
	ISP_PM_BLK_CMC8_BYPASS,

	//for ctm
	ISP_PM_BLK_CTM_BASE = (ISP_PM_BLK_CMC8_BASE + 0x100),
	ISP_PM_BLK_CTM_BYPASS,

	ISP_PM_BLK_PSTRZ_BASE = (ISP_PM_BLK_CTM_BASE + 0x100),
	ISP_PM_BLK_PSTRZ_BYPASS,

	ISP_PM_BLK_RGB_AFM_BASE = (ISP_PM_BLK_PSTRZ_BASE + 0x100),
	ISP_PM_BLK_RGB_AFM_BYPASS,

	ISP_PM_BLK_YIQ_AEM_BASE = (ISP_PM_BLK_RGB_AFM_BASE + 0x100),
	ISP_PM_BLK_YIQ_AEM_BYPASS,

	ISP_PM_BLK_YIQ_AFL_BASE = (ISP_PM_BLK_YIQ_AEM_BASE + 0x100),
	ISP_PM_BLK_YIQ_AFL_BYPASS,
	ISP_PM_BLK_YIQ_AFL_CFG,

	//for yiq_afm
	ISP_PM_BLK_YIQ_AFM_BASE = (ISP_PM_BLK_YIQ_AFL_BASE + 0x100),
	ISP_PM_BLK_YIQ_AFM_BYPASS,

	//for precdn
	ISP_PM_BLK_YUV_PRECDN_BASE = (ISP_PM_BLK_YIQ_AFM_BASE + 0x100),
	ISP_PM_BLK_YUV_PRECDN_BYPASS,
	ISP_PM_BLK_YUV_PRECDN_STRENGTH_LEVEL,
	//for prfyv1
	ISP_PM_BLK_PRFY_BASE = (ISP_PM_BLK_YUV_PRECDN_BASE + 0x100),
	ISP_PM_BLK_PRFY_BYPASS,
	//for brta
	ISP_PM_BLK_BRTA_BASE = (ISP_PM_BLK_PRFY_BASE + 0x100),
	ISP_PM_BLK_BRTA_BYPASS,
	//for cnta
	ISP_PM_BLK_CNTA_BASE = (ISP_PM_BLK_BRTA_BASE + 0x100),
	ISP_PM_BLK_CNTA_BYPASS,
	//for hist
	ISP_PM_BLK_HIST_BASE_V1 = (ISP_PM_BLK_CNTA_BASE + 0x100),
	ISP_PM_BLK_HIST_BYPASS_V1,
	//for hist2
	ISP_PM_BLK_HIST2_BASE = (ISP_PM_BLK_HIST_BASE_V1 + 0x100),
	ISP_PM_BLK_HIST2_BYPASS,
	//for aca
	ISP_PM_BLK_ACA_BASE = (ISP_PM_BLK_HIST2_BASE + 0x100),
	ISP_PM_BLK_ACA_BYPASS,
	//for cdn_v1
	ISP_PM_BLK_UV_CDN_BASE_V1 = (ISP_PM_BLK_ACA_BASE + 0x100),
	ISP_PM_BLK_UV_CDN_BYPASS_V1,

	//for Bilateral Denoise
	ISP_PM_BLK_BDN_BASE  = (ISP_PM_BLK_UV_CDN_BASE_V1 + 0x100),
	ISP_PM_BLK_BDN_BYPASS,
	ISP_PM_BLK_BDN_RADIAL_BYPASS,
	ISP_PM_BLK_BDN_STRENGTH_LEVEL,

	//for pre-CDN in RGB domain
	ISP_PM_BLK_RGB_PRE_CDN_BASE = (ISP_PM_BLK_BDN_BASE + 0x100),
	ISP_PM_BLK_RGB_PRE_CDN_BYPASS,
	ISP_PM_BLK_RGB_PRE_CDN_STRENGTH_LEVEL,

	//for iir nr in yuv domain
	ISP_PM_BLK_IIR_NR_BASE = (ISP_PM_BLK_RGB_PRE_CDN_BASE + 0x100),
	ISP_PM_BLK_IIR_NR_STRENGTH_LEVEL,

	//for IIR yrandom in yuv domain
	ISP_PM_BLK_IIR_YRANDOM_BASE = (ISP_PM_BLK_IIR_NR_BASE + 0x100),
	ISP_PM_BLK_IIR_YRANDOM_BYPASS,

	//for UV post CDN in uv channel in yuv domain
	ISP_PM_BLK_UV_POST_CDN_BASE = (ISP_PM_BLK_IIR_YRANDOM_BASE + 0x100),
	ISP_PM_BLK_UV_POST_CDN_BYPASS,
	ISP_PM_BLK_UV_POST_CDN_STRENGTH_LEVEL,
};

struct isp_pm_param_data {
	uint32_t mod_id;
	uint32_t id;
	uint32_t cmd;
	void* data_ptr;
	uint32_t data_size;
	uint8_t user_data[4];
};

struct isp_pm_block_header {
	char name[8];
	uint32_t block_id;			//block id: blc / nlc/lsc/cmc
	uint32_t version_id;		//version id: version_0, version_1, and so on
	uint32_t param_id;		//cmd/setting/a
	uint32_t size;
	uint32_t bypass;
	uint32_t is_update;	//block param need update
	uint32_t source_flag;
	void* absolute_addr;//
};

struct isp_pm_mode_param {
	char mode_name[8];
	uint32_t mode_id;
	uint32_t block_num;
	struct isp_size resolution;
	uint32_t fps;
	struct isp_pm_block_header header[ISP_TUNE_BLOCK_MAX];
	uint32_t data_area[0];
};

struct isp_pm_memory_init_param {
	struct isp_data_info buffer;
	struct isp_buffer_size_info size_info;
};

#ifdef	 __cplusplus
 	}
#endif

#endif
