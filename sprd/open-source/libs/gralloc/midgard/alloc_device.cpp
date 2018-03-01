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

#include "alloc_device_allocator_specific.h"
#if MALI_AFBC_GRALLOC == 1
#include "gralloc_buffer_priv.h"
#endif

#define AFBC_PIXELS_PER_BLOCK                    16
#define AFBC_BODY_BUFFER_BYTE_ALIGNMENT          1024
#define AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY  16

static int gralloc_align_tile_buffer(int *usage, int width, int height, int pixel_size,
		int* stride, size_t* size)
{
#ifdef GPU_USE_TILE_ALIGN
	/*
	 *  Aligned tile need 16 * 16 pixel aligned, for GPU read/write.
	 *  If CPU access buffer, should disable tile alignment feature.
	 * */
	if ((*usage & GRALLOC_USAGE_HW_TILE_ALIGN) &&
	     (((*usage & GRALLOC_USAGE_SW_READ_OFTEN) != GRALLOC_USAGE_SW_READ_OFTEN) &&
             ((*usage & GRALLOC_USAGE_SW_WRITE_OFTEN) != GRALLOC_USAGE_SW_WRITE_OFTEN)) &&
	     (*usage & (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_2D)))
	{
		*stride = GRALLOC_ALIGN(width, 16);
		*size = GRALLOC_ALIGN(height, 16) * (*stride) * pixel_size;
	}
#else
	{
		*usage &= ~GRALLOC_USAGE_HW_TILE_ALIGN;
	}
#endif
	return 0;
}

static int gralloc_alloc_framebuffer_locked(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle, int* stride, int* byte_stride)
{
	private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);

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
	/* framebufferSize is used for allocating the handle to the framebuffer and refers 
	 *                 to the size of the actual framebuffer.
	 * alignedFramebufferSize is used for allocating a possible internal buffer and 
	 *                        thus need to consider internal alignment requirements. */
	const size_t framebufferSize = m->finfo.line_length * m->info.yres;
	const size_t alignedFramebufferSize = GRALLOC_ALIGN(m->finfo.line_length, 64) * m->info.yres;

	*stride = m->info.xres;

	if (numBuffers == 1)
	{
		// If we have only one buffer, we never use page-flipping. Instead,
		// we return a regular buffer which will be memcpy'ed to the main
		// screen when post is called.
		int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
		AWAR( "fallback to single buffering. Virtual Y-res too small %d", m->info.yres );
		*byte_stride = GRALLOC_ALIGN(m->finfo.line_length, 64);
		return alloc_backend_alloc(dev, alignedFramebufferSize, newUsage, pHandle);
	}

	if (bufferMask >= ((1LU<<numBuffers)-1))
	{
		// We ran out of buffers.
		return -ENOMEM;
	}

#ifdef FBIOGET_DMABUF
	uintptr_t framebufferVaddr = (uintptr_t)m->framebuffer->base;
	// find a free slot
	for (uint32_t i=0 ; i<numBuffers ; i++)
	{
		if ((bufferMask & (1LU<<i)) == 0)
		{
			m->bufferMask |= (1LU<<i);
			break;
		}
		framebufferVaddr += framebufferSize;
	}

	// The entire framebuffer memory is already mapped, now create a buffer object for parts of this memory
	private_handle_t* hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER, usage, size,
			(void*)framebufferVaddr, 0, dup(m->framebuffer->fd),
			(framebufferVaddr - (uintptr_t)m->framebuffer->base), 0);

	/*
	 * Perform allocator specific actions. If these fail we fall back to a regular buffer
	 * which will be memcpy'ed to the main screen when fb_post is called.
	 */
	if (alloc_backend_alloc_framebuffer(m, hnd) == -1)
	{
		delete hnd;
		int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
		AERR( "Fallback to single buffering. Unable to map framebuffer memory to handle:%p", hnd );
		*byte_stride = GRALLOC_ALIGN(m->finfo.line_length, 64);
		return alloc_backend_alloc(dev, alignedFramebufferSize, newUsage, pHandle);
	}

	*pHandle = hnd;
	*byte_stride = m->finfo.line_length;
#else/*FBIOGET_DMABUF*/
	*byte_stride = m->finfo.line_length;
	return alloc_backend_alloc(dev, framebufferSize, usage, pHandle);
#endif/*FBIOGET_DMABUF*/

	return 0;
}

static int gralloc_alloc_framebuffer(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle, int* stride, int* byte_stride)
{
	private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
	pthread_mutex_lock(&m->lock);
	int err = gralloc_alloc_framebuffer_locked(dev, size, usage, pHandle, stride, byte_stride);
	pthread_mutex_unlock(&m->lock);
	return err;
}

/*
 * Computes the strides and size for an RGB buffer
 *
 * width               width of the buffer in pixels
 * height              height of the buffer in pixels
 * pixel_size          size of one pixel in bytes
 *
 * pixel_stride (out)  stride of the buffer in pixels
 * byte_stride  (out)  stride of the buffer in bytes
 * size         (out)  size of the buffer in bytes
 * afbc         (in)   if buffer should be allocated for afbc
 */
static void get_rgb_stride_and_size(int width, int height, int pixel_size,
                                    int* pixel_stride, int* byte_stride, size_t* size, bool afbc)
{
	int stride;

	stride = width * pixel_size;

	/* Align the lines to 64 bytes.
	 * It's more efficient to write to 64-byte aligned addresses because it's the burst size on the bus */
	stride = GRALLOC_ALIGN(stride, 64);

	if (size != NULL)
	{
		*size = stride * height;
	}

	if (byte_stride != NULL)
	{
		*byte_stride = stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = stride / pixel_size;
	}

	if (afbc)
	{
		int w_aligned = GRALLOC_ALIGN( width, AFBC_PIXELS_PER_BLOCK );
		int h_aligned = GRALLOC_ALIGN( height, AFBC_PIXELS_PER_BLOCK );
		int nblocks = w_aligned / AFBC_PIXELS_PER_BLOCK * h_aligned / AFBC_PIXELS_PER_BLOCK;

		if ( size != NULL )
		{
			*size = w_aligned * h_aligned * pixel_size +
					GRALLOC_ALIGN( nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT );
		}
	}
}

/*
 * Computes the strides and size for an YV12 buffer
 *
 * width               width of the buffer in pixels
 * height              height of the buffer in pixels
 *
 * pixel_stride (out)  stride of the buffer in pixels
 * byte_stride  (out)  stride of the buffer in bytes
 * size         (out)  size of the buffer in bytes
 * afbc         (in)   if buffer should be allocated for afbc
 */
static bool get_yv12_stride_and_size(int width, int height,
                                     int* pixel_stride, int* byte_stride, size_t* size, bool afbc)
{
	int luma_stride;

	/* Android assumes the width and height are even withou checking, so we check here */
	if (width % 2 != 0 || height % 2 != 0)
	{
		return false;
	}

	if ( afbc && size != NULL )
	{
		width = GRALLOC_ALIGN( width, AFBC_PIXELS_PER_BLOCK );
		height = GRALLOC_ALIGN( height, AFBC_PIXELS_PER_BLOCK );
	}

	/* Android assumes the buffer should be aligned to 16. */
	luma_stride = GRALLOC_ALIGN(width, 16);

	if (size != NULL)
	{
		/*int chroma_stride = GRALLOC_ALIGN(luma_stride / 2, 16); */
		/* Simplification of ((height * luma_stride ) + 2 * ((height / 2) * chroma_stride)). */
		/* *size = height * (luma_stride + chroma_stride); */
		
		/* mali GPU hardware requires u/v-plane 16byte-alignment */
		*size = GRALLOC_ALIGN(height * luma_stride, 16) + GRALLOC_ALIGN(height/2 * GRALLOC_ALIGN(luma_stride/2,16), 16) + height/2 * GRALLOC_ALIGN(luma_stride/2,16);
	}

	if (byte_stride != NULL)
	{
		*byte_stride = luma_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = luma_stride;
	}

	if ( afbc && size != NULL )
	{
		int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;

		/* Just append header block size, width/height alignment done above */
		*size += GRALLOC_ALIGN( nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT );
	}

	return true;
}
/*
 * Computes the strides and size for an YUV buffer which format is used in SPRD's camera/video 
 *
 * width               width of the buffer in pixels
 * height              height of the buffer in pixels
 *
 * pixel_stride (out)  stride of the buffer in pixels
 * byte_stride  (out)  stride of the buffer in bytes
 * size         (out)  size of the buffer in bytes
 * afbc         (in)   if buffer should be allocated for afbc
 */

static bool get_yuv_stride_and_size(int format , int width, int height,
                                     int* pixel_stride, int* byte_stride, size_t* size, bool afbc)
{
	int luma_stride;

	/* Android assumes the width and height are even withou checking, so we check here */
	if (width % 2 != 0 || height % 2 != 0)
	{
		ALOGD("width = %d, height = %d, width or height is not even", width, height);
		return false;
	}

	if ( afbc && size != NULL )
	{
		width = GRALLOC_ALIGN( width, AFBC_PIXELS_PER_BLOCK );
		height = GRALLOC_ALIGN( height, AFBC_PIXELS_PER_BLOCK );
	}

	if( (!pixel_stride)  || (!byte_stride) || (!size) ){
		ALOGD("pixel_stride or byte_stride or size is a null pointer !");
		return false;
	}
	switch (format)
	{
		case HAL_PIXEL_FORMAT_YCbCr_420_SP:
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		case HAL_PIXEL_FORMAT_YCbCr_420_888:
			luma_stride = width;
			*size = GRALLOC_ALIGN(height * luma_stride, 16) + height * luma_stride / 2;
			*byte_stride = luma_stride;
			*pixel_stride = luma_stride;
			break;
		case HAL_PIXEL_FORMAT_YCbCr_420_P:
			luma_stride = width;
			// mali GPU hardware requires u/v-plane 16byte-alignment
			*size = GRALLOC_ALIGN(height * luma_stride, 16) + GRALLOC_ALIGN(height * luma_stride /4 , 16) + GRALLOC_ALIGN(height * luma_stride /4, 16);
			*byte_stride = luma_stride;
			*pixel_stride = luma_stride;
			break;
		default:
			return -EINVAL;
	}


	if ( afbc && size != NULL )
	{
		int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;

		/* Just append header block size, width/height alignment done above */
		*size += GRALLOC_ALIGN( nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT );
	}

	return true;
}

static int alloc_device_alloc(alloc_device_t* dev, int w, int h, int format, int usage, buffer_handle_t* pHandle, int* pStride)
{
	if (!pHandle || !pStride)
	{
		return -EINVAL;
	}

	size_t size;       // Size to be allocated for the buffer
	int byte_stride;   // Stride of the buffer in bytes
	int pixel_stride;  // Stride of the buffer in pixels - as returned in pStride
	uint64_t internal_format;
	bool alloc_for_afbc=false;

	ALOGD_IF(mDebug,"alloc buffer start w:%d h:%d format:0x%x usage:0x%x",w,h,format,usage);

#if defined(GRALLOC_FB_SWAP_RED_BLUE)
	/* match the framebuffer format */
	if (usage & GRALLOC_USAGE_HW_FB)
	{
#ifdef GRALLOC_16_BITS
		format = HAL_PIXEL_FORMAT_RGB_565;
#else
		format = HAL_PIXEL_FORMAT_BGRA_8888;
#endif
	}
#endif

	internal_format = gralloc_select_format(format, usage);

	alloc_for_afbc = (internal_format & GRALLOC_ARM_INTFMT_AFBC);

	switch (internal_format & GRALLOC_ARM_INTFMT_FMT_MASK)
	{
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_RGBX_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
#if PLATFORM_SDK_VERSION >= 19
		case HAL_PIXEL_FORMAT_sRGB_A_8888:
		case HAL_PIXEL_FORMAT_sRGB_X_8888:
#endif
			get_rgb_stride_and_size(w, h, 4, &pixel_stride, &byte_stride, &size, alloc_for_afbc );
			gralloc_align_tile_buffer(&usage, w, h, 4, &pixel_stride, &size);
			break;
		case HAL_PIXEL_FORMAT_RGB_888:
			get_rgb_stride_and_size(w, h, 3, &pixel_stride, &byte_stride, &size, alloc_for_afbc );
			gralloc_align_tile_buffer(&usage, w, h, 3, &pixel_stride, &size);
			break;
		case HAL_PIXEL_FORMAT_RGB_565:
#if PLATFORM_SDK_VERSION < 19
		case HAL_PIXEL_FORMAT_RGBA_5551:
		case HAL_PIXEL_FORMAT_RGBA_4444:
#endif
			get_rgb_stride_and_size(w, h, 2, &pixel_stride, &byte_stride, &size, alloc_for_afbc );
			gralloc_align_tile_buffer(&usage, w, h, 2, &pixel_stride, &size);
			break;

		/*case HAL_PIXEL_FORMAT_YCrCb_420_SP:*/
		case HAL_PIXEL_FORMAT_YV12:
			if (!get_yv12_stride_and_size(w, h, &pixel_stride, &byte_stride, &size, alloc_for_afbc))
			{
				return -EINVAL;
			}
			break;

			/*
			 * Additional custom formats can be added here
			 * and must fill the variables pixel_stride, byte_stride and size.
			 */
		case  HAL_PIXEL_FORMAT_YCbCr_420_SP:
		case  HAL_PIXEL_FORMAT_YCrCb_420_SP:
		case  HAL_PIXEL_FORMAT_YCbCr_420_888:
		case  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
		case  HAL_PIXEL_FORMAT_YCbCr_420_P:
			if (!get_yuv_stride_and_size(internal_format & GRALLOC_ARM_INTFMT_FMT_MASK, w, h, &pixel_stride, &byte_stride, &size, alloc_for_afbc))
			{
				return -EINVAL;
			}
			break;
		case  HAL_PIXEL_FORMAT_BLOB:
			pixel_stride= w;
			size = w * h;
			byte_stride=size;
			break;
		default:
			return -EINVAL;
	}

	int err;
#if DISABLE_FRAMEBUFFER_HAL != 1
	if (usage & GRALLOC_USAGE_HW_FB)
	{
		err = gralloc_alloc_framebuffer(dev, size, usage, pHandle, &pixel_stride, &byte_stride);
	}
	else
#endif
	{
		err = alloc_backend_alloc(dev, size, usage, pHandle);
	}

	if (err < 0)
	{
		return err;
	}

	private_handle_t *hnd = (private_handle_t *)*pHandle;

#if MALI_AFBC_GRALLOC == 1
	err = gralloc_buffer_attr_allocate( hnd );
	if( err < 0 )
	{
		private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);

		if ( (usage & GRALLOC_USAGE_HW_FB) )
		{
			/*
			 * Having the attribute region is not critical for the framebuffer so let it pass.
			 */
			err = 0;
		}
		else
		{
			alloc_backend_alloc_free( hnd, m );
			return err;
		}
	}
#endif

	hnd->format = format;
	hnd->byte_stride = byte_stride;
	hnd->internal_format = internal_format;

	int private_usage = usage & (GRALLOC_USAGE_PRIVATE_0 |
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
	hnd->stride = pixel_stride;

	*pStride = pixel_stride;
	return 0;
}

static int alloc_device_free(alloc_device_t* dev, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		return -EINVAL;
	}

	private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(handle);
	private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		// free this buffer
		private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
		const size_t bufferSize = m->finfo.line_length * m->info.yres;
		int index = ((uintptr_t)hnd->base - (uintptr_t)m->framebuffer->base) / bufferSize;
		m->bufferMask &= ~(1<<index); 
		close(hnd->fd);
	}

#if MALI_AFBC_GRALLOC == 1
	gralloc_buffer_attr_free( (private_handle_t *) hnd );
#endif
	alloc_backend_alloc_free(hnd, m);

	delete hnd;

	return 0;
}

int alloc_device_open(hw_module_t const* module, const char* name, hw_device_t** device)
{
	alloc_device_t *dev;
	
	dev = new alloc_device_t;
	if (NULL == dev)
	{
		return -1;
	}

	/* initialize our state here */
	memset(dev, 0, sizeof(*dev));

	/* initialize the procs */
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = const_cast<hw_module_t*>(module);
	dev->common.close = alloc_backend_close;
	dev->alloc = alloc_device_alloc;
	dev->free = alloc_device_free;

	if (0 != alloc_backend_open(dev)) {
		delete dev;
		return -1;
	}
	
	*device = &dev->common;

	return 0;
}
