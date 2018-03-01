/*
 * Copyright (C) 2014 ARM Limited. All rights reserved.
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

#ifndef FORMAT_CHOOSER_H_
#define FORMAT_CHOOSER_H_

#include <hardware/hardware.h>


#define GRALLOC_ARM_INTFMT_EXTENSION_BIT_START     32

/* This format will be use AFBC */
#define	    GRALLOC_ARM_INTFMT_AFBC                 (1ULL << (GRALLOC_ARM_INTFMT_EXTENSION_BIT_START+0))

/* This format uses AFBC split block mode */
#define	    GRALLOC_ARM_INTFMT_AFBC_SPLITBLK        (1ULL << (GRALLOC_ARM_INTFMT_EXTENSION_BIT_START+1))

/* Internal format masks */
#define	    GRALLOC_ARM_INTFMT_FMT_MASK             0x00000000ffffffffULL
#define	    GRALLOC_ARM_INTFMT_EXT_MASK             0xffffffff00000000ULL

/* Index of the internal formats */
typedef enum
{
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_FIRST,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGBA_8888=GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_FIRST,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGBX_8888,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGB_888,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGB_565,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_BGRA_8888,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_sRGB_A_8888,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_sRGB_X_8888,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_YV12,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_Y8,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_Y16,

	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGBA_8888_AFBC,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGBX_8888_AFBC,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGB_888_AFBC,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGB_565_AFBC,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_BGRA_8888_AFBC,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_YV12_AFBC,

	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGBA_8888_AFBC_SPLITBLK,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGBX_8888_AFBC_SPLITBLK,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGB_888_AFBC_SPLITBLK,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_RGB_565_AFBC_SPLITBLK,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_BGRA_8888_AFBC_SPLITBLK,
	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_YV12_AFBC_SPLITBLK,

	/* Add more internal formats here */

	GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_LAST
} gralloc_arm_internal_index_format;

typedef enum
{
	/* Having an invalid format catches lists which are initialized with not all entries. */
	GRALLOC_ARM_HAL_FORMAT_INDEXED_INVALID,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_565,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_sRGB_A_8888,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_sRGB_X_8888,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_YV12,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_Y8,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_Y16,
	GRALLOC_ARM_HAL_FORMAT_INDEXED_LAST
} gralloc_arm_hal_index_format;

#define MAX_COMPATIBLE 3
#define DEFAULT_WEIGHT_SUPPORTED         50
#define DEFAULT_WEIGHT_MOST_PREFERRED    100
#define DEFAULT_WEIGHT_UNSUPPORTED       -1

typedef struct
{
	/* The internal extended format exported outside of gralloc */
	uint64_t internal_extended_format;

	/* Swizzled versions of the requested format for this internal format */
	gralloc_arm_hal_index_format comp_format_list[MAX_COMPATIBLE];
} internal_fmt_info;

uint64_t gralloc_select_format(int req_format, int usage);

struct hwblk
{
	uint32_t usage;
	int16_t weights[GRALLOC_ARM_HAL_FORMAT_INDEXED_LAST][GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_LAST];
};


typedef struct
{
	void (*blk_init)(struct hwblk *format_matrix, int16_t **array);
	struct hwblk hwblkconf;
} blkinit;


extern void initialize_blk_conf();
extern const internal_fmt_info translate_internal_indexed[GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_LAST];
extern blkinit blklist[];
extern uint32_t blklist_array_size;

extern "C"
{
	int gralloc_get_internal_format(int hal_format);
	void *gralloc_get_internal_info(int *blkconf_size, int *gpu_conf);
}

#endif /* FORMAT_CHOOSER_H_ */
