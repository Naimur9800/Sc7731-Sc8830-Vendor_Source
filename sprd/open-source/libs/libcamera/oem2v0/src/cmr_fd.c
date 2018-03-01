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

#define LOG_TAG "cmr_fd"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "SprdOEMCamera.h"
#include "../../arithmetic/inc/FaceFinder.h"


typedef int (*face_finder_int)(int width, int height, MallocFun Mfp, FreeFun Ffp);
typedef int (*face_finder_function)(unsigned char *src, struct face_finder_data **ppDstFaces, int *pDstFaceNum ,int skip);
typedef int (*face_finder_finalize)(FreeFun Ffp);

struct face_finder_ops {
	face_finder_int      init;
	face_finder_function function;
	face_finder_finalize finalize;
};

struct class_fd {
	struct ipm_common               common;
	cmr_handle                      thread_handle;
	cmr_uint                        is_busy;
	cmr_uint                        is_inited;
	cmr_int                         ops_init_ret;
	void                            *alloc_addr;
	cmr_uint                        mem_size;
	cmr_uint                        frame_cnt;
	cmr_uint                        frame_total_num;
	struct ipm_frame_in             frame_in;
	struct ipm_frame_out            frame_out;
	ipm_callback                    frame_cb;
};

struct fd_start_parameter {
	void                            *frame_data;
	ipm_callback                    frame_cb;
	cmr_handle                      caller_handle;
	void                            *private_data;
};

static cmr_int fd_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *out_class_handle);
static cmr_int fd_close(cmr_handle class_handle);
static cmr_int fd_transfer_frame(cmr_handle class_handle,struct ipm_frame_in *in, struct ipm_frame_out *out);
static cmr_int fd_pre_proc(cmr_handle class_handle);
static cmr_int fd_post_proc(cmr_handle class_handle);
static cmr_int fd_start(cmr_handle class_handle, struct fd_start_parameter *param);
static cmr_uint check_size_data_invalid(struct img_size *fd_size);
static cmr_int fd_call_init(struct class_fd *class_handle, const struct img_size *fd_size);
static cmr_uint fd_is_busy(struct class_fd *class_handle);
static void fd_set_busy(struct class_fd *class_handle, cmr_uint is_busy);
static cmr_int fd_thread_create(struct class_fd *class_handle);
static cmr_int fd_thread_proc(struct cmr_msg *message, void *private_data);


static struct face_finder_ops fd_face_finder_ops = {

#if defined(CONFIG_CAMERA_FACE_DETECT)
	FaceFinder_Init,
	(face_finder_function)FaceFinder_Function,
	FaceFinder_Finalize
#else
	NULL,
	NULL,
	NULL
#endif
};


static struct class_ops fd_ops_tab_info = {
	fd_open,
	fd_close,
	fd_transfer_frame,
	fd_pre_proc,
	fd_post_proc,
};

struct class_tab_t fd_tab_info = {
	&fd_ops_tab_info,
};

#define CMR_FD_LIMIT_SIZE         (320 * 240)
#define CMR_EVT_FD_START          (1 << 16)
#define CMR_EVT_FD_EXIT           (1 << 17)
#define CMR_EVT_FD_INIT           (1 << 18)
#define CMR__EVT_FD_MASK_BITS     (cmr_u32)(CMR_EVT_FD_START | \
					CMR_EVT_FD_EXIT | \
					CMR_EVT_FD_INIT)

#define CAMERA_FD_MSG_QUEUE_SIZE  5
#define IMAGE_FORMAT              "YVU420_SEMIPLANAR"

#define CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return CMR_CAMERA_INVALID_PARAM; \
		} \
	} while(0)

#define FD_PTR_NUB  200
static void* fd_ptr[FD_PTR_NUB] = {NULL};
static cmr_int fd_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *out_class_handle)
{
	cmr_int              ret        = CMR_CAMERA_SUCCESS;
	struct class_fd      *fd_handle = NULL;
	struct img_size      *fd_size;

	if (!out || !in || !ipm_handle || !out_class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	fd_handle = (struct class_fd *)malloc(sizeof(struct class_fd));
	if (!fd_handle) {
		CMR_LOGE("No mem!");
		return CMR_CAMERA_NO_MEM;
	}

	cmr_bzero(fd_handle, sizeof(struct class_fd));

	fd_handle->common.ipm_cxt     = (struct ipm_context_t*)ipm_handle;
	fd_handle->common.class_type  = IPM_TYPE_FD;
	fd_handle->common.ops         = &fd_ops_tab_info;
	fd_handle->frame_cb           = in->reg_cb;
	fd_handle->mem_size           = in->frame_size.height * in->frame_size.width * 3 / 2;
	fd_handle->frame_total_num    = in->frame_cnt;
	fd_handle->frame_cnt          = 0;

	CMR_LOGD("mem_size = 0x%ld", fd_handle->mem_size);
	fd_handle->alloc_addr = malloc(fd_handle->mem_size);
	if (!fd_handle->alloc_addr) {
		CMR_LOGE("mem alloc failed");
		goto free_fd_handle;
	}

	ret = fd_thread_create(fd_handle);
	if (ret) {
		CMR_LOGE("failed to create thread.");
		goto free_fd_handle;
	}

	fd_size = &in->frame_size;
	ret = fd_call_init(fd_handle, fd_size);
	if (ret) {
		CMR_LOGE("failed to init fd");
		fd_close(fd_handle);
	} else {
		*out_class_handle = (cmr_handle )fd_handle;
	}

	return ret;

free_fd_handle:
	if (fd_handle->alloc_addr) {
		free(fd_handle->alloc_addr);
	}
	free(fd_handle);
	return ret;
}

static cmr_int fd_close(cmr_handle class_handle)
{
	cmr_int              ret         = CMR_CAMERA_SUCCESS;
	struct class_fd      *fd_handle  = (struct class_fd *)class_handle;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(fd_handle);

	message.msg_type = CMR_EVT_FD_EXIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(fd_handle->thread_handle, &message);
	if (ret) {
		CMR_LOGE("send msg fail");
		goto out;
	}

	if (fd_handle->thread_handle) {
		cmr_thread_destroy(fd_handle->thread_handle);
		fd_handle->thread_handle = 0;
		fd_handle->is_inited     = 0;
	}

	if (fd_handle->alloc_addr) {
		free(fd_handle->alloc_addr);
	}

	free(fd_handle);

out:
	return ret;
}

static cmr_int fd_transfer_frame(cmr_handle class_handle,struct ipm_frame_in *in, struct ipm_frame_out *out)
{
	cmr_int                   ret         = CMR_CAMERA_SUCCESS;
	struct class_fd           *fd_handle  = (struct class_fd *)class_handle;
	cmr_uint                  frame_cnt;
	cmr_u32                    is_busy    = 0;
	struct fd_start_parameter param;

	if (!in || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	frame_cnt   = ++fd_handle->frame_cnt;

	if (frame_cnt < fd_handle->frame_total_num) {
		CMR_LOGD("This is fd 0x%ld frame. need the 0x%ld frame,",frame_cnt, fd_handle->frame_total_num);
		return ret;
	}

	is_busy = fd_is_busy(fd_handle);
	CMR_LOGI("fd is_busy =%d", is_busy);

	if (!is_busy) {
		fd_handle->frame_cnt = 0;
		fd_handle->frame_in  = *in;

		param.frame_data    = (void *)in->src_frame.addr_phy.addr_y;
		param.frame_cb      = fd_handle->frame_cb;
		param.caller_handle = in->caller_handle;
		param.private_data  = in->private_data;

		memcpy(fd_handle->alloc_addr, (void *)in->src_frame.addr_vir.addr_y, fd_handle->mem_size);

		ret = fd_start(class_handle,&param);
		if (ret) {
			CMR_LOGE("send msg fail");
			goto out;
		}

		if (fd_handle->frame_cb) {
			if (out != NULL) {
				cmr_bzero(out,sizeof(struct ipm_frame_out));
			}
		} else {
			if (out != NULL) {
				out = &fd_handle->frame_out;
			} else {
				CMR_LOGE("sync err,out parm can't NULL.");
			}
		}
	}
out:
	return ret;
}

static cmr_int fd_pre_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int              ret = CMR_CAMERA_SUCCESS;

	/*no need to do*/

	return ret;
}
static cmr_int fd_post_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int              ret = CMR_CAMERA_SUCCESS;

	/*no need to do*/

	return ret;
}

static cmr_int fd_start(cmr_handle class_handle, struct fd_start_parameter *param)
{
	cmr_int              ret         = CMR_CAMERA_SUCCESS;
	struct class_fd      *fd_handle  = (struct class_fd *)class_handle;
	cmr_u32              is_busy     = 0;
	CMR_MSG_INIT(message);

	if (!class_handle || !param) {
		CMR_LOGE("parameter is NULL. fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (!param->frame_data) {
		CMR_LOGE("frame_data is NULL. fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	message.data = (void *)malloc(sizeof(struct fd_start_parameter));
	if (NULL == message.data) {
		CMR_LOGE("NO mem, Fail to alloc memory for msg data");
		return CMR_CAMERA_NO_MEM;
	}

	memcpy(message.data, param, sizeof(struct fd_start_parameter));

	message.msg_type = CMR_EVT_FD_START;
	message.alloc_flag = 1;

	if (fd_handle->frame_cb) {
		message.sync_flag = CMR_MSG_SYNC_RECEIVED;
		//message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	} else {
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	}

	ret = cmr_thread_msg_send(fd_handle->thread_handle, &message);
	if (ret) {
		CMR_LOGE("send msg fail");
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}

static cmr_uint check_size_data_invalid(struct img_size *fd_size)
{
	cmr_int              ret = -CMR_CAMERA_FAIL;

	if (NULL != fd_size) {
		if ((fd_size->width) && (fd_size->height)){
			ret= CMR_CAMERA_SUCCESS;
		}
	}

	return ret;
}

static void* fd_malloc(cmr_s32 size)
{
	void*                   ptr = NULL;
	cmr_uint i = 0;
	if (size) {
	   ptr = malloc(size);
	}
	for (i = 0; i <  FD_PTR_NUB; i++) {
		if (fd_ptr[i] == NULL) {
			fd_ptr[i] = ptr;
			break;
		}
	}
	if (i == FD_PTR_NUB) {
		CMR_LOGE("fd_malloc ERR %ld", i);
	}
	return ptr;
}
static void  fd_free(void* ptr)
{
	cmr_uint i = 0;
	if (ptr) {
		for (i = 0; i <  FD_PTR_NUB; i++) {
			if (fd_ptr[i] == ptr) {
				free(ptr);
				ptr = NULL;
				fd_ptr[i] = NULL;
				break;
			}
		}
		if (i == FD_PTR_NUB) {
			CMR_LOGE("fd_free ERR %p", ptr);
		}
	}
	return;
}
static void  fd_freeall(void)
{
	cmr_uint i = 0;
	for (i = 0; i <  FD_PTR_NUB; i++) {
		if (NULL != fd_ptr[i]) {
			free(fd_ptr[i]);
			fd_ptr[i] = NULL;
		}
	}
	return;
}
static cmr_int fd_call_init(struct class_fd *class_handle, const struct img_size *fd_size)
{
	cmr_int              ret = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);

	message.data = malloc(sizeof(struct img_size));
	if (NULL == message.data) {
		CMR_LOGE("NO mem, Fail to alloc memory for msg data");
		ret = CMR_CAMERA_NO_MEM;
		goto out;
	}

	message.alloc_flag = 1;
	memcpy(message.data, fd_size, sizeof(struct img_size));

	message.msg_type = CMR_EVT_FD_INIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(class_handle->thread_handle, &message);
	if (CMR_CAMERA_SUCCESS != ret) {
		CMR_LOGE("msg send fail");
		ret = CMR_CAMERA_FAIL;
		goto free_all;
	}

	return ret | class_handle->ops_init_ret;

free_all:
	free(message.data);
out:
	return ret;
}

static cmr_uint fd_is_busy(struct class_fd *class_handle)
{
	cmr_int              is_busy = 0;

	if (NULL == class_handle) {
		return is_busy;
	}

	is_busy = class_handle->is_busy;

	return is_busy;
}

static void fd_set_busy(struct class_fd *class_handle, cmr_uint is_busy)
{
	if (NULL == class_handle) {
		return;
	}

	class_handle->is_busy = is_busy;
}


static cmr_int fd_thread_create(struct class_fd *class_handle)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(class_handle);

	if (!class_handle->is_inited) {
		ret = cmr_thread_create(&class_handle->thread_handle,
					CAMERA_FD_MSG_QUEUE_SIZE,
					fd_thread_proc,
					(void*)class_handle);
		if (ret) {
			CMR_LOGE("send msg failed!");
			ret = CMR_CAMERA_FAIL;
			goto end;
		}

		class_handle->is_inited = 1;
	} else {
		CMR_LOGI("fd is inited already");
	}

end:
	return ret;
}

static cmr_int fd_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int                   ret           = CMR_CAMERA_SUCCESS;
	struct class_fd           *class_handle = (struct class_fd *)private_data;
	cmr_int                   evt           = 0;
	cmr_uint                  mem_size      = 0;
	void                      *addr         = 0;
	cmr_int                   facesolid_ret = 0;
	MallocFun                 Mfp = fd_malloc;
	FreeFun                   Ffp = fd_free;
	struct face_finder_data   *face_rect_ptr = NULL;
	cmr_int                   face_num = 0;
	cmr_int                   k,min_fd;
	struct fd_start_parameter *start_param;

	if (!message || !class_handle) {
		CMR_LOGE("parameter is fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32)(message->msg_type & CMR__EVT_FD_MASK_BITS);

	switch (evt) {
	case CMR_EVT_FD_INIT: {
			struct img_size *fd_size = message->data;
			if (!check_size_data_invalid(fd_size)) {
				if (fd_face_finder_ops.init && (fd_size->width * fd_size->height >= CMR_FD_LIMIT_SIZE)) {
					CMR_LOGI("[FD] w/h %d %d", fd_size->width, fd_size->height);
					if ( 0 != fd_face_finder_ops.init(fd_size->width, fd_size->height, Mfp, Ffp)) {
						class_handle->ops_init_ret = -CMR_CAMERA_FAIL;
						ret = -CMR_CAMERA_FAIL;
						CMR_LOGE("[FD] init fail.");
					} else {
						CMR_LOGI("[FD] init done.");
					}
				} else {
					class_handle->ops_init_ret = -CMR_CAMERA_FAIL;
				}
			}
		}
		break;

	case CMR_EVT_FD_START:
		start_param = (struct fd_start_parameter *)message->data;

		if (NULL == start_param) {
			CMR_LOGE("parameter fail");
			break;
		}

		fd_set_busy(class_handle, 1);

		/*check memory addr*/
		if (NULL == class_handle->alloc_addr) {
			CMR_LOGE("handle->addr 0x%lx", (cmr_uint)class_handle->alloc_addr);
			break;
		}

		/*start call lib function*/
		CMR_LOGV("fd detect start");

		if (fd_face_finder_ops.function) {
			facesolid_ret = fd_face_finder_ops.function((cmr_u8*)class_handle->alloc_addr,
									&face_rect_ptr,
									(int*)&face_num,
									0);
		}

		if (0 != facesolid_ret) {
			CMR_LOGE("face function fail.");
			fd_set_busy(class_handle, 0);
		} else {
			min_fd = MIN(face_num,FACE_DETECT_NUM);
			int invalid_count = 0;
			for (k = 0; k < min_fd; k++) {
				if (face_rect_ptr->sx < 0 || face_rect_ptr->sy < 0 ||
					face_rect_ptr->ex < 0 || face_rect_ptr->ey < 0 ||
					face_rect_ptr->srx < 0 || face_rect_ptr->sry < 0 ||
					face_rect_ptr->elx < 0 || face_rect_ptr->ely < 0) {
					invalid_count++;
					face_rect_ptr++;
					continue;
				}
				class_handle->frame_out.face_area.range[k] = *face_rect_ptr;
				/*CMR_LOGI("cmr_fd face rect:s(%d,%d) sr(%d,%d) e(%d,%d) el(%d,%d)", face_rect_ptr->sx, face_rect_ptr->sy,
					face_rect_ptr->srx, face_rect_ptr->sry, face_rect_ptr->ex, face_rect_ptr->ey,
					face_rect_ptr->elx, face_rect_ptr->ely);*/

				face_rect_ptr++;
			}
			class_handle->frame_out.face_area.face_count = min_fd - invalid_count;
			CMR_LOGI("valid face num %ld", min_fd - invalid_count);

			/*callback*/
			if (class_handle->frame_cb) {
				class_handle->frame_out.private_data  = start_param->private_data;
				class_handle->frame_out.caller_handle = start_param->caller_handle;
				class_handle->frame_cb(IPM_TYPE_FD, &class_handle->frame_out);
			}

		fd_set_busy(class_handle, 0);
		}
		break;

	case CMR_EVT_FD_EXIT:
		if (fd_face_finder_ops.finalize && !class_handle->ops_init_ret) {
			if ( 0 != fd_face_finder_ops.finalize(Ffp)) {
				CMR_LOGE("[FD] deinit fail");
			} else {
				CMR_LOGI("[FD] deinit done");
			}
			fd_freeall();
		}
		class_handle->ops_init_ret = 0;
		break;

	default:
		break;
	}

	return ret;
}

