/*
 * Copyright (C) 2014 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <hardware/hardware.h>
#include <cutils/log.h>
#include <hardware/gralloc.h>
#include "format_chooser.h"

#define GRALLOC_ARM_FORMAT_SELECTION_DISABLE 1

static inline int find_format_index(int format)
{
	int index=-1;

	switch ( format )
	{
		case HAL_PIXEL_FORMAT_RGBA_8888:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888;
			break;
		case HAL_PIXEL_FORMAT_RGBX_8888:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888;
			break;
		case HAL_PIXEL_FORMAT_RGB_888:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888;
			break;
		case HAL_PIXEL_FORMAT_RGB_565:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_565;
			break;
		case HAL_PIXEL_FORMAT_BGRA_8888:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888;
			break;
#if PLATFORM_SDK_VERSION >= 19
		case HAL_PIXEL_FORMAT_sRGB_A_8888:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_sRGB_A_8888;
			break;
		case HAL_PIXEL_FORMAT_sRGB_X_8888:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_sRGB_X_8888;
			break;
#endif
		case HAL_PIXEL_FORMAT_YV12:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_YV12;
			break;
#if PLATFORM_SDK_VERSION >= 18
		case HAL_PIXEL_FORMAT_Y8:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_Y8;
			break;
		case HAL_PIXEL_FORMAT_Y16:
			index = GRALLOC_ARM_HAL_FORMAT_INDEXED_Y16;
			break;
#endif
	}

	return index;
}

/*
 * Define GRALLOC_ARM_FORMAT_SELECTION_DISABLE to disable the format selection completely
 */
uint64_t gralloc_select_format(int req_format, int usage)
{
#if defined(GRALLOC_ARM_FORMAT_SELECTION_DISABLE)

	(void) usage;
	return (uint64_t) req_format;

#else
	uint64_t new_format = req_format;
	int intformat_ind;
	int n=0;
	int largest_weight_ind=-1;
	int16_t accum_weights[GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_LAST] = {0};

	ALOGV("gralloc_select_format: req_format=0x%x usage=0x%x\n",req_format,usage);

	if( req_format == 0 )
	{
		return 0;
	}

	if( (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK)) != 0 ||
             usage == 0 )
	{
		return new_format;
	}

#if DISABLE_FRAMEBUFFER_HAL != 1
	/* This is currently a limitation with the display and will be removed eventually
	 *  We can't allocate fbdev framebuffer buffers in AFBC format */
	if( usage & GRALLOC_USAGE_HW_FB )
	{
		return new_format;
	}
#endif

	/* if this format can't be classified in one of the groups we
	 * have pre-defined, ignore it.
	 */
	intformat_ind = find_format_index( req_format );
	if( intformat_ind < 0 )
	{
		return new_format;
	}

	while( blklist[n].blk_init != 0 )
	{
		if( (blklist[n].hwblkconf.usage & usage) != 0 )
		{
			uint32_t m;

			for(m=GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_FIRST; m<GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_LAST; m++)
			{
				if( blklist[n].hwblkconf.weights[intformat_ind][m] != DEFAULT_WEIGHT_UNSUPPORTED )
				{
					accum_weights[m] += blklist[n].hwblkconf.weights[intformat_ind][m];


					if( largest_weight_ind < 0 ||
						accum_weights[m] > accum_weights[largest_weight_ind])
					{
						largest_weight_ind = m;
					}
				}
				else
				{
					/* Format not supported by this hwblk */
					accum_weights[m] = DEFAULT_WEIGHT_UNSUPPORTED;
				}
			}
		}
		n++;
	}

	if( largest_weight_ind < 0 )
	{
		new_format = 0;
	}
	else
	{
		new_format = translate_internal_indexed[largest_weight_ind].internal_extended_format;
	}

	ALOGV("Returned iterated format: 0x%llX", new_format);

	return new_format;
#endif
}

extern "C"
{
void *gralloc_get_internal_info(int *blkconf_size, int *gpu_conf)
{
	void *blkinit_address = NULL;

#if !defined(GRALLOC_ARM_FORMAT_SELECTION_DISABLE)

	if(blkconf_size != NULL && gpu_conf != NULL)
	{
		blkinit_address = (void*) blklist;
		*blkconf_size = blklist_array_size;

/*
 * Tests intended to verify gralloc format selection behavior are GPU version aware in runtime.
 * They need to know what configuration we built for. For now this is simply AFBC on or off. This
 * will likely change in the future to mean something else.
 */
#if MALI_AFBC_GRALLOC == 1
		*gpu_conf = 1;
#else
		*gpu_conf = 0;
#endif /* MALI_AFBC_GRALLOC */
	}

#endif /* GRALLOC_ARM_FORMAT_SELECTION_DISABLE */

	return blkinit_address;
}

int gralloc_get_internal_format(int hal_format)
{
	return find_format_index(hal_format);
}
}
