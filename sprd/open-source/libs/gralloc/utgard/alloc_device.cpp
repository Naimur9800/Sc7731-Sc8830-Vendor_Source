/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
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

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/ioctl.h>

#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"

#if GRALLOC_ARM_UMP_MODULE
#include <ump/ump.h>
#include <ump/ump_ref_drv.h>
#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
#include <linux/ion.h>
#include <ion/ion.h>
#endif

#include "ion_sprd.h"

#if GRALLOC_SIMULATE_FAILURES
#include <cutils/properties.h>

/* system property keys for controlling simulated UMP allocation failures */
#define PROP_MALI_TEST_GRALLOC_FAIL_FIRST     "mali.test.gralloc.fail_first"
#define PROP_MALI_TEST_GRALLOC_FAIL_INTERVAL  "mali.test.gralloc.fail_interval"

static int __ump_alloc_should_fail()
{

	static unsigned int call_count  = 0;
	unsigned int        first_fail  = 0;
	int                 fail_period = 0;
	int                 fail        = 0;

	++call_count;

	/* read the system properties that control failure simulation */
	{
		char prop_value[PROPERTY_VALUE_MAX];

		if (property_get(PROP_MALI_TEST_GRALLOC_FAIL_FIRST, prop_value, "0") > 0)
		{
			sscanf(prop_value, "%11u", &first_fail);
		}

		if (property_get(PROP_MALI_TEST_GRALLOC_FAIL_INTERVAL, prop_value, "0") > 0)
		{
			sscanf(prop_value, "%11u", &fail_period);
		}
	}

	/* failure simulation is enabled by setting the first_fail property to non-zero */
	if (first_fail > 0)
	{
		LOGI("iteration %u (fail=%u, period=%u)\n", call_count, first_fail, fail_period);

		fail = (call_count == first_fail) ||
		       (call_count > first_fail && fail_period > 0 && 0 == (call_count - first_fail) % fail_period);

		if (fail)
		{
			AERR("failed ump_ref_drv_allocate on iteration #%d\n", call_count);
		}
	}

	return fail;
}
#endif


static int gralloc_alloc_buffer(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
#if GRALLOC_ARM_DMA_BUF_MODULE
	{
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
		ion_user_handle_t ion_hnd;
		unsigned char *cpu_ptr;
		int shared_fd;
		int ret;
		int ion_heap_mask = 0;
		int ion_flag = 0;

		if (usage & (GRALLOC_USAGE_VIDEO_BUFFER|GRALLOC_USAGE_CAMERA_BUFFER))
		{ 
			ion_heap_mask = ION_HEAP_ID_MASK_MM;
		}
		else if(usage & GRALLOC_USAGE_OVERLAY_BUFFER)
		{
			ion_heap_mask = ION_HEAP_ID_MASK_OVERLAY;
		}
		else
		{
			ion_heap_mask = ION_HEAP_ID_MASK_SYSTEM;
		}

		if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
		{
			ion_flag = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
		}

		ret = ion_alloc(m->ion_client, size, 0, ion_heap_mask, ion_flag, &(ion_hnd));

		if (ret != 0)
		{
			AERR("Failed to ion_alloc from ion_client:%d", m->ion_client);
			return -1;
		}

		ret = ion_share(m->ion_client, ion_hnd, &shared_fd);

		if (ret != 0)
		{
			AERR("ion_share( %d ) failed", m->ion_client);

			if (0 != ion_free(m->ion_client, ion_hnd))
			{
				AERR("ion_free( %d ) failed", m->ion_client);
			}

			return -1;
		}

		cpu_ptr = (unsigned char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);

		if (MAP_FAILED == cpu_ptr)
		{
			AERR("ion_map( %d ) failed", m->ion_client);

			if (0 != ion_free(m->ion_client, ion_hnd))
			{
				AERR("ion_free( %d ) failed", m->ion_client);
			}

			close(shared_fd);
			return -1;
		}
		private_handle_t *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION, usage, size, (uint64_t)cpu_ptr, private_handle_t::LOCK_STATE_MAPPED);

		if (NULL != hnd)
		{
			if(ion_heap_mask == ION_HEAP_CARVEOUT_MASK || ion_heap_mask == ION_HEAP_ID_MASK_OVERLAY)
			{
				hnd->flags=(private_handle_t::PRIV_FLAGS_USES_ION)|(private_handle_t::PRIV_FLAGS_USES_PHY);
			}
			hnd->share_fd = shared_fd;
			hnd->ion_hnd = ion_hnd;
			*pHandle = hnd;
			ion_invalidate_fd(m->ion_client,hnd->share_fd);
			return 0;
		}
		else
		{
			AERR("Gralloc out of mem for ion_client:%d", m->ion_client);
		}

		close(shared_fd);
		ret = munmap(cpu_ptr, size);

		if (0 != ret)
		{
			AERR("munmap failed for base:%p size: %lu", cpu_ptr, (unsigned long)size);
		}

		ret = ion_free(m->ion_client, ion_hnd);

		if (0 != ret)
		{
			AERR("ion_free( %d ) failed", m->ion_client);
		}

		return -1;
	}
#endif

#if GRALLOC_ARM_UMP_MODULE
	MALI_IGNORE(dev);
	{
		ump_handle ump_mem_handle;
		void *cpu_ptr;
		ump_secure_id ump_id;
		ump_alloc_constraints constraints;

		size = round_up_to_page_size(size);

		if ((usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN)
		{
			constraints =  UMP_REF_DRV_CONSTRAINT_USE_CACHE;
		}
		else
		{
			constraints = UMP_REF_DRV_CONSTRAINT_NONE;
		}

#ifdef GRALLOC_SIMULATE_FAILURES

		/* if the failure condition matches, fail this iteration */
		if (__ump_alloc_should_fail())
		{
			ump_mem_handle = UMP_INVALID_MEMORY_HANDLE;
		}
		else
#endif
		{
			ump_mem_handle = ump_ref_drv_allocate(size, constraints);

			if (UMP_INVALID_MEMORY_HANDLE != ump_mem_handle)
			{
				cpu_ptr = ump_mapped_pointer_get(ump_mem_handle);

				if (NULL != cpu_ptr)
				{
					ump_id = ump_secure_id_get(ump_mem_handle);

					if (UMP_INVALID_SECURE_ID != ump_id)
					{
						private_handle_t *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_UMP, usage, size, cpu_ptr,
						        private_handle_t::LOCK_STATE_MAPPED, ump_id, ump_mem_handle);

						if (NULL != hnd)
						{
							*pHandle = hnd;
							return 0;
						}
						else
						{
							AERR("gralloc_alloc_buffer() failed to allocate handle. ump_handle = %p, ump_id = %d", ump_mem_handle, ump_id);
						}
					}
					else
					{
						AERR("gralloc_alloc_buffer() failed to retrieve valid secure id. ump_handle = %p", ump_mem_handle);
					}

					ump_mapped_pointer_release(ump_mem_handle);
				}
				else
				{
					AERR("gralloc_alloc_buffer() failed to map UMP memory. ump_handle = %p", ump_mem_handle);
				}

				ump_reference_release(ump_mem_handle);
			}
			else
			{
				AERR("gralloc_alloc_buffer() failed to allocate UMP memory. size:%d constraints: %d", size, constraints);
			}
		}

		return -1;
	}
#endif

}

static int gralloc_alloc_framebuffer_locked(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

	// allocate the framebuffer
	if (m->framebuffer == NULL)
	{
		// initialize the framebuffer, the framebuffer is mapped once and forever.
		int err = init_frame_buffer_locked(m);

		if (err < 0)
		{
			return err;
		}
	}

	const uint32_t bufferMask = m->bufferMask;
	const uint32_t numBuffers = m->numBuffers;
	const size_t bufferSize = m->finfo.line_length * m->info.yres;

	if (numBuffers == 1)
	{
		// If we have only one buffer, we never use page-flipping. Instead,
		// we return a regular buffer which will be memcpy'ed to the main
		// screen when post is called.
		int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
		AERR("fallback to single buffering. Virtual Y-res too small %d", m->info.yres);
		return gralloc_alloc_buffer(dev, bufferSize, newUsage, pHandle);
	}

	if (bufferMask >= ((1LU << numBuffers) - 1))
	{
		// We ran out of buffers.
		return -ENOMEM;
	}

	uint64_t vaddr = m->framebuffer->base;

	// find a free slot
	for (uint32_t i = 0 ; i < numBuffers ; i++)
	{
		if ((bufferMask & (1LU << i)) == 0)
		{
			m->bufferMask |= (1LU << i);
			break;
		}

		vaddr = vaddr + bufferSize;
	}

	// The entire framebuffer memory is already mapped, now create a buffer object for parts of this memory
	private_handle_t *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER, usage, size, vaddr,
	        0, dup(m->framebuffer->fd), vaddr - m->framebuffer->base);
#if GRALLOC_ARM_UMP_MODULE
	hnd->ump_id = m->framebuffer->ump_id;

	/* create a backing ump memory handle if the framebuffer is exposed as a secure ID */
	if ((int)UMP_INVALID_SECURE_ID != hnd->ump_id)
	{
		hnd->ump_mem_handle = (int)ump_handle_create_from_secure_id(hnd->ump_id);

		if ((int)UMP_INVALID_MEMORY_HANDLE == hnd->ump_mem_handle)
		{
			AINF("warning: unable to create UMP handle from secure ID %i\n", hnd->ump_id);
		}
	}

#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
	{
#ifdef FBIOGET_DMABUF
		struct fb_dmabuf_export fb_dma_buf;

		if (ioctl(m->framebuffer->fd, FBIOGET_DMABUF, &fb_dma_buf) == 0)
		{
			AINF("framebuffer accessed with dma buf (fd 0x%x)\n", (int)fb_dma_buf.fd);
			hnd->share_fd = fb_dma_buf.fd;
		}

#endif
	}
#endif

	*pHandle = hnd;

	return 0;
}

static int gralloc_alloc_framebuffer(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	pthread_mutex_lock(&m->lock);
	int err = gralloc_alloc_framebuffer_locked(dev, size, usage, pHandle);
	pthread_mutex_unlock(&m->lock);
	return err;
}

static int alloc_device_alloc(alloc_device_t *dev, int w, int h, int format, int usage, buffer_handle_t *pHandle, int *pStride)
{
	if (!pHandle || !pStride)
	{
		return -EINVAL;
	}

	if(w < 1 || h < 1)
	{
		return -EINVAL;
	}

	size_t size;
	size_t stride;

	ALOGD_IF(mDebug,"alloc buffer start w:%d h:%d format:0x%x usage:0x%x",w,h,format,usage);

	if (format == HAL_PIXEL_FORMAT_YCbCr_420_888 || format == HAL_PIXEL_FORMAT_YCrCb_420_SP || format == HAL_PIXEL_FORMAT_YV12
		|| format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED
	/* HAL_PIXEL_FORMAT_YCbCr_420_SP, HAL_PIXEL_FORMAT_YCbCr_420_P, HAL_PIXEL_FORMAT_YCbCr_422_I are not defined in Android.
 	 * To enable Mali DDK EGLImage support for those formats, firstly, you have to add them in Android system/core/include/system/graphics.h.
 	 * Then, define SUPPORT_LEGACY_FORMAT in the same header file(Mali DDK will also check this definition).
	 */
		|| format == HAL_PIXEL_FORMAT_YCbCr_420_SP || format == HAL_PIXEL_FORMAT_YCbCr_420_P || format == HAL_PIXEL_FORMAT_YCbCr_422_I
	   )
	{
		switch (format)
		{
			case HAL_PIXEL_FORMAT_YCbCr_420_SP:
                            stride = GRALLOC_ALIGN(w, 16);
                            size = GRALLOC_ALIGN(h, 16) * (stride + GRALLOC_ALIGN(stride / 2, 16));
                            break;
			case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                            stride = GRALLOC_ALIGN(w, 16);
                            size = GRALLOC_ALIGN(h, 16) * (stride + GRALLOC_ALIGN(stride / 2, 16));
                            break;
			case HAL_PIXEL_FORMAT_YCbCr_420_888:
			case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
				stride = w;
				// mali GPU hardware requires uv-plane 64byte-alignment
				size = GRALLOC_ALIGN(h * stride, 64) + h * GRALLOC_ALIGN(stride/2,16);
				break;
			case HAL_PIXEL_FORMAT_YCbCr_420_P:
				stride = w;
				// mali GPU hardware requires u/v-plane 64byte-alignment
				size = GRALLOC_ALIGN(h * stride, 64) + GRALLOC_ALIGN(h/2 * GRALLOC_ALIGN(stride/2,16), 64) + h/2 * GRALLOC_ALIGN(stride/2,16);
				break;


			case HAL_PIXEL_FORMAT_YV12:
				stride = GRALLOC_ALIGN(w, 16);
				// mali GPU hardware requires u/v-plane 64byte-alignment
				size = GRALLOC_ALIGN(h * stride, 64) + GRALLOC_ALIGN(h/2 * GRALLOC_ALIGN(stride/2,16), 64) + h/2 * GRALLOC_ALIGN(stride/2,16);
				break;

			default:
				return -EINVAL;
		}
	}
	else if(format == HAL_PIXEL_FORMAT_BLOB)
	{
		stride = GRALLOC_ALIGN(w, 16);
		size = w*h;
	}
	else
	{
		int bpp = 0;

		switch (format)
		{
			case HAL_PIXEL_FORMAT_RGBA_8888:
			case HAL_PIXEL_FORMAT_RGBX_8888:
			case HAL_PIXEL_FORMAT_BGRA_8888:
				bpp = 4;
				break;

			case HAL_PIXEL_FORMAT_RGB_888:
				bpp = 3;
				break;

			case HAL_PIXEL_FORMAT_RGB_565:
#if PLATFORM_SDK_VERSION < 19
			case HAL_PIXEL_FORMAT_RGBA_5551:
			case HAL_PIXEL_FORMAT_RGBA_4444:
#endif
				bpp = 2;
				break;

			default:
				return -EINVAL;
		}

		size_t bpr = GRALLOC_ALIGN(w * bpp, 8);
		size = bpr * h;
		stride = bpr / bpp;

#ifdef GPU_USE_TILE_ALIGN
                /*
                 *  Aligned tile need 16 * 16 pixel aligned, for GPU read/write.
                 *  If CPU access buffer, should disable tile alignment feature.
                 * */
                if ((usage & GRALLOC_USAGE_HW_TILE_ALIGN) &&
                    (((usage & GRALLOC_USAGE_SW_READ_OFTEN) != GRALLOC_USAGE_SW_READ_OFTEN) &&
                     ((usage & GRALLOC_USAGE_SW_WRITE_OFTEN) != GRALLOC_USAGE_SW_WRITE_OFTEN)) &&
                    (usage & (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_2D)))
                {
                    stride = GRALLOC_ALIGN(w, 16);
                    size = GRALLOC_ALIGN(h, 16) * stride * bpp;
                }
                else
#endif
                {
                    usage &= ~GRALLOC_USAGE_HW_TILE_ALIGN;
                }
	}

	int err;

#ifndef MALI_600

	if ((usage & GRALLOC_USAGE_HW_FB) && (w != 1 && h != 1))
	{
		err = gralloc_alloc_framebuffer(dev, size, usage, pHandle);
	}
	else
#endif

	{
		err = gralloc_alloc_buffer(dev, size, usage, pHandle);
		if(err>=0)
		{
			const native_handle_t *p_nativeh  = *pHandle;
			private_handle_t *hnd = (private_handle_t*)p_nativeh;
			hnd->format = format;
			hnd->width = stride;
			hnd->height = h;
			ALOGD_IF(mDebug,"alloc buffer end handle:%p ion_hnd:0x%x",pHandle,hnd->ion_hnd);
		}
	}

	if (err < 0)
	{
		return err;
	}

	/* match the framebuffer format */
	if (usage & GRALLOC_USAGE_HW_FB)
	{
#ifdef GRALLOC_16_BITS
		format = HAL_PIXEL_FORMAT_RGB_565;
#else
		format = HAL_PIXEL_FORMAT_RGBA_8888;
#endif
	}

	private_handle_t *hnd = (private_handle_t *)*pHandle;
	int               private_usage = usage & (GRALLOC_USAGE_PRIVATE_0 |
	                                  GRALLOC_USAGE_PRIVATE_1);

	switch (private_usage)
	{
		case 0:
			hnd->yuv_info = MALI_YUV_BT601_NARROW;
			break;

		case GRALLOC_USAGE_PRIVATE_1:
			hnd->yuv_info = MALI_YUV_BT601_WIDE;
			break;

		case GRALLOC_USAGE_PRIVATE_0:
			hnd->yuv_info = MALI_YUV_BT709_NARROW;
			break;

		case (GRALLOC_USAGE_PRIVATE_0 | GRALLOC_USAGE_PRIVATE_1):
			hnd->yuv_info = MALI_YUV_BT709_WIDE;
			break;
	}

	hnd->width = w;
	hnd->height = h;
	hnd->format = format;
	hnd->stride = stride;

	*pStride = stride;
	return 0;
}

static int alloc_device_free(alloc_device_t *dev, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		return -EINVAL;
	}

	private_handle_t const *hnd = reinterpret_cast<private_handle_t const *>(handle);

	ALOGD_IF(mDebug,"free buffer start handle:%p ion_hnd:0x%x",handle,hnd->ion_hnd);

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		// free this buffer
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
		const size_t bufferSize = m->finfo.line_length * m->info.yres;
		int index = ((uintptr_t)hnd->base - (uintptr_t)m->framebuffer->base) / bufferSize;
		m->bufferMask &= ~(1 << index);
		close(hnd->fd);

#if GRALLOC_ARM_UMP_MODULE

		if ((int)UMP_INVALID_MEMORY_HANDLE != hnd->ump_mem_handle)
		{
			ump_reference_release((ump_handle)hnd->ump_mem_handle);
		}

#endif
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
	{
#if GRALLOC_ARM_UMP_MODULE

		/* Buffer might be unregistered so we need to check for invalid ump handle*/
		if ((int)UMP_INVALID_MEMORY_HANDLE != hnd->ump_mem_handle)
		{
			ump_mapped_pointer_release((ump_handle)hnd->ump_mem_handle);
			ump_reference_release((ump_handle)hnd->ump_mem_handle);
		}

#else
		AERR("Can't free ump memory for handle:0x%p. Not supported.", hnd);
#endif
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
#if GRALLOC_ARM_DMA_BUF_MODULE
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

		/* Buffer might be unregistered so we need to check for invalid ump handle*/
		if (0 != hnd->base)
		{
			if (0 != munmap((void *)hnd->base, hnd->size))
			{
				AERR("Failed to munmap handle 0x%p", hnd);
			}
		}

		close(hnd->share_fd);

		if (0 != ion_free(m->ion_client, hnd->ion_hnd))
		{
			AERR("Failed to ion_free( ion_client: %d ion_hnd: %p )", m->ion_client, (void *)(uintptr_t)hnd->ion_hnd);
		}

		memset((void *)hnd, 0, sizeof(*hnd));
#else
		AERR("Can't free dma_buf memory for handle:0x%x. Not supported.", (unsigned int)hnd);
#endif

	}

	ALOGD_IF(mDebug,"free buffer end handle:%p",handle);
	delete hnd;

	return 0;
}

static int alloc_device_close(struct hw_device_t *device)
{
	alloc_device_t *dev = reinterpret_cast<alloc_device_t *>(device);

	if (dev)
	{
#if GRALLOC_ARM_DMA_BUF_MODULE
		private_module_t *m = reinterpret_cast<private_module_t *>(device);

		if (0 != ion_close(m->ion_client))
		{
			AERR("Failed to close ion_client: %d", m->ion_client);
		}

		close(m->ion_client);
#endif
		delete dev;
#if GRALLOC_ARM_UMP_MODULE
		ump_close(); // Our UMP memory refs will be released automatically here...
#endif
	}

	return 0;
}

int alloc_device_open(hw_module_t const *module, const char *name, hw_device_t **device)
{
	MALI_IGNORE(name);
	alloc_device_t *dev;

	dev = new alloc_device_t;

	if (NULL == dev)
	{
		return -1;
	}

#if GRALLOC_ARM_UMP_MODULE
	ump_result ump_res = ump_open();

	if (UMP_OK != ump_res)
	{
		AERR("UMP open failed with %d", ump_res);
		delete dev;
		return -1;
	}

#endif

	/* initialize our state here */
	memset(dev, 0, sizeof(*dev));

	/* initialize the procs */
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = const_cast<hw_module_t *>(module);
	dev->common.close = alloc_device_close;
	dev->alloc = alloc_device_alloc;
	dev->free = alloc_device_free;

#if GRALLOC_ARM_DMA_BUF_MODULE
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	m->ion_client = ion_open();

	if (m->ion_client < 0)
	{
		AERR("ion_open failed with %s", strerror(errno));
		delete dev;
		return -1;
	}

#endif

	*device = &dev->common;

	return 0;
}
