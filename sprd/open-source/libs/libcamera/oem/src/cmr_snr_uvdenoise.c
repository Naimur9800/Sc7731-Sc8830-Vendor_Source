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
#define LOG_TAG "snr_uvdenoise"

#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <arm_neon.h>
#include <semaphore.h>
#include "cmr_ipm.h"
//#include "cmr_snr_uvdenoise.h"
#include "isp_app.h"
#include "cmr_common.h"
#include "cmr_msg.h"
#include "cmr_oem.h"
#include "snr_uvdenoise_params.h"
#include "sensor_drv_u.h"
#include "snr_uv_interface.h"


#define CAMERA_CNRDE_MSG_QUEUE_SIZE       5
#define THREAD_CNRDE                      1

#define CMR_EVT_CNRDE_BASE                (CMR_EVT_IPM_BASE + 0X400)
#define CMR_EVT_CNRDE_INIT                (CMR_EVT_CNRDE_BASE + 0)
#define CMR_EVT_CNRDE_START               (CMR_EVT_CNRDE_BASE + 1)
#define CMR_EVT_CNRDE_EXIT                (CMR_EVT_CNRDE_BASE + 2)

#define CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return -CMR_CAMERA_INVALID_PARAM; \
		} \
	} while(0)

typedef int (*proc_func) (void *param);
typedef void (*proc_cb) (cmr_handle class_handle, void *param);

struct class_snr_uvde {
	struct ipm_common common;
	sem_t denoise_sem_lock;
	cmr_handle thread_handles[THREAD_CNRDE];
	cmr_uint is_inited;
};

struct snr_uvde_start_param {
	proc_func func_ptr;
	proc_cb cb;
	cmr_handle caller_handle;
	void *param;
};

struct snr_uv_denoise_handle_param {
	int width;
	int height;
	uint8 *snr_uv_in;
	uint8 *snr_uv_out;
	//int8 *mask;
	thr *curthr;
};


static struct class_ops snr_uvde_ops_tab_info;

static cmr_int snr_uvde_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *class_handle = (struct class_snr_uvde *)private_data;
	cmr_u32 evt = 0;
	struct snr_uvde_start_param *snr_uvde_start_ptr = NULL;

	if (!message || !class_handle) {
		CMR_LOGE("parameter is fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32) message->msg_type;

	switch (evt) {
	case CMR_EVT_CNRDE_INIT:
		ret = cnr_init();
		if (ret) {
			CMR_LOGE("failed to init snr_uv lib%ld", ret);
		}

		break;

	case CMR_EVT_CNRDE_START:
		snr_uvde_start_ptr = (struct snr_uvde_start_param *)message->data;
		if (snr_uvde_start_ptr->func_ptr) {
			ret = snr_uvde_start_ptr->func_ptr(snr_uvde_start_ptr->param);
		}

		if (snr_uvde_start_ptr->cb) {
			snr_uvde_start_ptr->cb(class_handle, (void *)snr_uvde_start_ptr->param);
		}
		break;

	case CMR_EVT_CNRDE_EXIT:
		ret = cnr_destroy();
		if (ret) {
			CMR_LOGE("failed to deinit snr_uv lib%ld", ret);
		}

		break;

	default:
		break;
	}

	return ret;
}

static cmr_int snr_uvde_thread_create(struct class_snr_uvde *class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	cmr_handle *cur_handle_ptr = NULL;
	cmr_int thread_id = 0;

	CHECK_HANDLE_VALID(snr_uvde_handle);
	CMR_MSG_INIT(message);

	if (!class_handle->is_inited) {
		for (thread_id = 0; thread_id < THREAD_CNRDE; thread_id++) {
			cur_handle_ptr = &snr_uvde_handle->thread_handles[thread_id];
			ret = cmr_thread_create(cur_handle_ptr,
						CAMERA_CNRDE_MSG_QUEUE_SIZE, snr_uvde_thread_proc, (void *)class_handle);
			if (ret) {
				CMR_LOGE("send msg failed!");
				ret = CMR_CAMERA_FAIL;
				return ret;
			}
			message.sync_flag = CMR_MSG_SYNC_PROCESSED;
			message.msg_type = CMR_EVT_CNRDE_INIT;
			ret = cmr_thread_msg_send(*cur_handle_ptr, &message);
			if (CMR_CAMERA_SUCCESS != ret) {
				CMR_LOGE("msg send fail");
				ret = CMR_CAMERA_FAIL;
				return ret;
			}

		}

		class_handle->is_inited = 1;
	}

	return ret;
}

static cmr_int snr_uvde_thread_destroy(struct class_snr_uvde *class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	cmr_uint thread_id = 0;

	CHECK_HANDLE_VALID(class_handle);

	if (snr_uvde_handle->is_inited) {
		for (thread_id = 0; thread_id < THREAD_CNRDE; thread_id++) {
			if (snr_uvde_handle->thread_handles[thread_id]) {
				ret = cmr_thread_destroy(snr_uvde_handle->thread_handles[thread_id]);
				if (ret) {
					CMR_LOGE("snr_uvde cmr_thread_destroy fail");
					return CMR_CAMERA_FAIL;
				}
				snr_uvde_handle->thread_handles[thread_id] = 0;
			}
		}
		snr_uvde_handle->is_inited = 0;
	}

	return ret;
}

static cmr_int snr_uvde_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
			 cmr_handle * class_handle)
{
	UNUSED(in);
	UNUSED(out);

	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = NULL;

	if (!ipm_handle || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	snr_uvde_handle = (struct class_snr_uvde *)malloc(sizeof(struct class_snr_uvde));
	if (!snr_uvde_handle) {
		CMR_LOGE("No mem!");
		return CMR_CAMERA_NO_MEM;
	}

	cmr_bzero(snr_uvde_handle, sizeof(struct class_snr_uvde));

	snr_uvde_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
	snr_uvde_handle->common.class_type = IPM_TYPE_SNR_UVDE;
	snr_uvde_handle->common.ops = &snr_uvde_ops_tab_info;
	sem_init(&snr_uvde_handle->denoise_sem_lock, 0, 0);
	ret = snr_uvde_thread_create(snr_uvde_handle);
	if (ret) {
		CMR_LOGE("HDR error: create thread.");
		goto exit;
	}

	*class_handle = (cmr_handle) snr_uvde_handle;
	return ret;

exit:
	if (NULL != snr_uvde_handle)
		free(snr_uvde_handle);
	return CMR_CAMERA_FAIL;

}

static cmr_int snr_uvde_close(cmr_handle class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;

	if (!snr_uvde_handle) {
		return -CMR_CAMERA_INVALID_PARAM;
	}

	ret = snr_uvde_thread_destroy(snr_uvde_handle);

	if (snr_uvde_handle)
		free(snr_uvde_handle);

	return ret;
}

static int snr_uv_proc_func(void* param)
{

	nsecs_t timestamp1 = 0;
	nsecs_t timestamp2 = 0;
	uint32_t snr_uv_duration = 0;
	cmr_int ret = -1;

	struct snr_uv_denoise_handle_param *snr_uvde_param = (struct snr_uv_denoise_handle_param *)param;
	if (NULL == param) {
		CMR_LOGE("error param");
		return -1;
	}
	CMR_LOGI("snr_uvdenoise begin width = %d, height = %d, snr_uvde_param->curthr =%p", snr_uvde_param->width, snr_uvde_param->height,snr_uvde_param->curthr);

	timestamp1 = systemTime(CLOCK_MONOTONIC);

	//camera_flush();
	//cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle);

	ret = cnr(snr_uvde_param->snr_uv_in, snr_uvde_param->width, snr_uvde_param->height, snr_uvde_param->curthr);

	//camera_flush();
	//cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle);

	timestamp2 = systemTime(CLOCK_MONOTONIC);
	timestamp2 -= timestamp1;
	snr_uv_duration = (uint32_t) timestamp2;
	CMR_LOGI("enc_in_param.size.width = %d, enc_in_param.size.height = %d, snr_uv ret is %d, duration is %d", snr_uvde_param->width, snr_uvde_param->height, ret, snr_uv_duration);

exit:
	return 0;
}

static void snr_uv_proc_cb(cmr_handle class_handle, void *param)
{
	UNUSED(param);

	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	CMR_LOGI("%s called! release sem lock", __func__);
	sem_post(&snr_uvde_handle->denoise_sem_lock);
}

static cmr_int snr_uvde_start(cmr_handle class_handle, cmr_uint thread_id, proc_func func_ptr, proc_cb cb, void *param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	cmr_handle *cur_handle_ptr = NULL;
	struct snr_uvde_start_param *snr_uvde_start_ptr = NULL;

	CMR_MSG_INIT(message);

	if (!class_handle) {
		CMR_LOGE("parameter is NULL. fail");
		return -CMR_CAMERA_INVALID_PARAM;
	}

	snr_uvde_start_ptr = (struct snr_uvde_start_param *)malloc(sizeof(struct snr_uvde_start_param));
	if (!snr_uvde_start_ptr) {
		CMR_LOGE("no mem");
		return CMR_CAMERA_NO_MEM;
	}
	memset(snr_uvde_start_ptr, 0, sizeof(struct snr_uvde_start_param));
	cur_handle_ptr = &snr_uvde_handle->thread_handles[thread_id];
	snr_uvde_start_ptr->func_ptr = func_ptr;
	snr_uvde_start_ptr->param = param;
	snr_uvde_start_ptr->cb = cb;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.msg_type = CMR_EVT_CNRDE_START;
	message.alloc_flag = 1;
	message.data = snr_uvde_start_ptr;
	ret = cmr_thread_msg_send(*cur_handle_ptr, &message);
	if (ret) {
		CMR_LOGE("send msg fail");
		if (snr_uvde_start_ptr)
			free(snr_uvde_start_ptr);
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}

static cmr_int snr_uvde_choose_mask(cmr_uint sensor_id, float gain, thr **curthr)
{
	cmr_int ret = -1;
	//int ret2 = 0;

	//thr aThr = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
	//curthr = &aThr;
	float snr_uv_analog_gain = 0.0;
	uint32_t snr_uv_index = 0;

	cmr_s8* sensor_name = sensor_get_cur_name();

	//snr_uv_analog_gain = get_cnr_analog_gain();
	snr_uv_analog_gain = (float)gain;

	//if (get_sensor_5M_model() == SENSOR_5M_S5K4E1GA) {
	if( strcmp(sensor_name, "s5k4e1ga") == 0) {
		CMR_LOGI("snr_uv_lib, SENSOR_5M_S5K4E1GA");
		if (snr_uv_analog_gain >= 1 && snr_uv_analog_gain < 1.5) {
			snr_uv_index = 0;
		} else if (snr_uv_analog_gain >= 1.5 && snr_uv_analog_gain < 2.5) {
			snr_uv_index = 1;
		} else if (snr_uv_analog_gain >= 2.5 && snr_uv_analog_gain < 3.5) {
			snr_uv_index = 2;
		} else if (snr_uv_analog_gain >= 3.5 && snr_uv_analog_gain < 4.5) {
			snr_uv_index = 3;
		} else if (snr_uv_analog_gain >= 4.5 && snr_uv_analog_gain < 5.5) {
			snr_uv_index = 4;
		} else if (snr_uv_analog_gain >= 5.5 && snr_uv_analog_gain < 6.5) {
			snr_uv_index = 5;
		} else if (snr_uv_analog_gain >= 6.5 && snr_uv_analog_gain < 7.5) {
			snr_uv_index = 6;
		} else if (snr_uv_analog_gain >= 7.5 && snr_uv_analog_gain < 8.5) {
			snr_uv_index = 7;
		}else if (snr_uv_analog_gain >= 8.5 && snr_uv_analog_gain < 9.5) {
			snr_uv_index = 8;
		} else if (snr_uv_analog_gain >= 9.5 && snr_uv_analog_gain < 16) {
			snr_uv_index = 9;
		} else {
			CMR_LOGI("---> snr_uv_index error: snr_uv_analog_gain = %f", snr_uv_analog_gain);
		}
		CMR_LOGI("---> snr_uv_index = %d", snr_uv_index);

		s5k4e1ga_thr_array[snr_uv_index].udw[0]=s5k4e1ga_tudw0[snr_uv_index];
		s5k4e1ga_thr_array[snr_uv_index].udw[1]=s5k4e1ga_tudw1[snr_uv_index];
		s5k4e1ga_thr_array[snr_uv_index].udw[2]=s5k4e1ga_tudw2[snr_uv_index];

		s5k4e1ga_thr_array[snr_uv_index].vdw[0]=s5k4e1ga_tvdw0[snr_uv_index];
		s5k4e1ga_thr_array[snr_uv_index].vdw[1]=s5k4e1ga_tvdw1[snr_uv_index];
		s5k4e1ga_thr_array[snr_uv_index].vdw[2]=s5k4e1ga_tvdw2[snr_uv_index];

		s5k4e1ga_thr_array[snr_uv_index].urw[0]=s5k4e1ga_turw0[snr_uv_index];
		s5k4e1ga_thr_array[snr_uv_index].urw[1]=s5k4e1ga_turw1[snr_uv_index];
		s5k4e1ga_thr_array[snr_uv_index].urw[2]=s5k4e1ga_turw2[snr_uv_index];

		s5k4e1ga_thr_array[snr_uv_index].vrw[0]=s5k4e1ga_tvrw0[snr_uv_index];
		s5k4e1ga_thr_array[snr_uv_index].vrw[1]=s5k4e1ga_tvrw1[snr_uv_index];
		s5k4e1ga_thr_array[snr_uv_index].vrw[2]=s5k4e1ga_tvrw2[snr_uv_index];

		//ret2 = snr_uv(in_parm.src_addr_vir.addr_u, in_parm.size.width, in_parm.size.height, curthr);
		*curthr = &s5k4e1ga_thr_array[snr_uv_index];
		CMR_LOGI("---> curthr = %p", *curthr);
	} else if (strcmp(sensor_name, "ov5648")==0){
		CMR_LOGI("snr_uv_lib, SENSOR_5M_OV5648");
		if (snr_uv_analog_gain >= 1 && snr_uv_analog_gain < 1.5) {
			snr_uv_index = 0;
		} else if (snr_uv_analog_gain >= 1.5 && snr_uv_analog_gain < 2.5) {
			snr_uv_index = 1;
		} else if (snr_uv_analog_gain >= 2.5 && snr_uv_analog_gain < 3.5) {
			snr_uv_index = 2;
		} else if (snr_uv_analog_gain >= 3.5 && snr_uv_analog_gain < 4.5) {
			snr_uv_index = 3;
		} else if (snr_uv_analog_gain >= 4.5 && snr_uv_analog_gain < 5.5) {
			snr_uv_index = 4;
		} else if (snr_uv_analog_gain >= 5.5 && snr_uv_analog_gain < 6.5) {
			snr_uv_index = 5;
		} else if (snr_uv_analog_gain >= 6.5 && snr_uv_analog_gain < 7.5) {
			snr_uv_index = 6;
		} else if (snr_uv_analog_gain >= 7.5 && snr_uv_analog_gain < 8.5) {
			snr_uv_index = 7;
		} else if (snr_uv_analog_gain >= 8.5 && snr_uv_analog_gain < 9.5) {
			snr_uv_index = 8;
		} else if (snr_uv_analog_gain >= 9.5 && snr_uv_analog_gain < 10.5) {
			snr_uv_index = 9;
		} else if (snr_uv_analog_gain >= 10.5 && snr_uv_analog_gain < 11.5) {
			snr_uv_index = 10;
		} else if (snr_uv_analog_gain >= 11.5 && snr_uv_analog_gain < 16) {
			snr_uv_index = 11;
		} else {
			CMR_LOGI("---> snr_uv_index error: snr_uv_analog_gain = %f", snr_uv_analog_gain);
		}

		CMR_LOGI("---> snr_uv_index = %d", snr_uv_index);

		ov5648_thr_array[snr_uv_index].udw[0]=ov5648_tudw0[snr_uv_index];
		ov5648_thr_array[snr_uv_index].udw[1]=ov5648_tudw1[snr_uv_index];
		ov5648_thr_array[snr_uv_index].udw[2]=ov5648_tudw2[snr_uv_index];

		ov5648_thr_array[snr_uv_index].vdw[0]=ov5648_tvdw0[snr_uv_index];
		ov5648_thr_array[snr_uv_index].vdw[1]=ov5648_tvdw1[snr_uv_index];
		ov5648_thr_array[snr_uv_index].vdw[2]=ov5648_tvdw2[snr_uv_index];

		ov5648_thr_array[snr_uv_index].urw[0]=ov5648_turw0[snr_uv_index];
		ov5648_thr_array[snr_uv_index].urw[1]=ov5648_turw1[snr_uv_index];
		ov5648_thr_array[snr_uv_index].urw[2]=ov5648_turw2[snr_uv_index];

		ov5648_thr_array[snr_uv_index].vrw[0]=ov5648_tvrw0[snr_uv_index];
		ov5648_thr_array[snr_uv_index].vrw[1]=ov5648_tvrw1[snr_uv_index];
		ov5648_thr_array[snr_uv_index].vrw[2]=ov5648_tvrw2[snr_uv_index];

		//ret2 = snr_uv(in_parm.src_addr_vir.addr_u, in_parm.size.width, in_parm.size.height, curthr);
		*curthr = &ov5648_thr_array[snr_uv_index];
		CMR_LOGI("---> curthr = %p", *curthr);
	}else{
		  CMR_LOGI("this sensor %s uv denoise is unavailable",sensor_name);
		  goto exit;
	}


	ret = 0;
exit:
	return ret;
}

static cmr_int snr_uvde_transfer_frame(cmr_handle class_handle, struct ipm_frame_in *in, struct ipm_frame_out *out)
{

	UNUSED(out);

	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	struct snr_uv_denoise_handle_param snr_uvde_param = {0x00};
	struct camera_context *cxt = (struct camera_context*)(snr_uvde_handle->common.ipm_cxt->init_in.oem_handle);
	cmr_uint sensor_id = 0;
	cmr_uint thread_id = 0;

	if (!in || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}
	sensor_id = snr_uvde_handle->common.ipm_cxt->init_in.sensor_id;
	snr_uvde_param.height = in->src_frame.size.height;
	snr_uvde_param.width = in->src_frame.size.width;
	snr_uvde_param.snr_uv_in = (uint8 *)in->src_frame.addr_vir.addr_u;

	float gain = 0;
	ret = cmr_sensor_get_real_gain(cxt->sn_cxt.sensor_handle, cxt->camera_id, &gain);
	CMR_LOGI("--->	gain = %f, ret = %d", gain,ret);

//	int real_gain = get_cur_real_gain();
//	int gain = real_gain * 100 / 128;

	if (0 != snr_uvde_choose_mask(sensor_id, gain, &snr_uvde_param.curthr))
		goto snr_uvdenoise_exit;


	//int i = 0;
	//for (i = 0; i < 10; i++)
	//    CMR_LOGE("before yde_param.cnr_in[%d] = %d", i, snr_uvde_param.cnr_in[i]);
	snr_uvde_start(snr_uvde_handle, thread_id, snr_uv_proc_func, snr_uv_proc_cb, &snr_uvde_param);
	sem_wait(&snr_uvde_handle->denoise_sem_lock);
	//for (i = 0; i < 10; i++)
	//	CMR_LOGE("after yde_param.cnr_in[%d] = %d", i, snr_uvde_param.cnr_in[i]);

snr_uvdenoise_exit:
	return ret;


}

static cmr_int snr_uvde_pre_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int ret = CMR_CAMERA_SUCCESS;

	/*no need to do */

	return ret;
}

static cmr_int snr_uvde_post_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int ret = CMR_CAMERA_SUCCESS;

	/*no need to do */

	return ret;
}

static struct class_ops snr_uvde_ops_tab_info = {
	.open = snr_uvde_open,
	.close = snr_uvde_close,
	.transfer_frame = snr_uvde_transfer_frame,
	.pre_proc = snr_uvde_pre_proc,
	.post_proc = snr_uvde_post_proc,
};

struct class_tab_t snr_uvde_tab_info = {
	&snr_uvde_ops_tab_info,
};
