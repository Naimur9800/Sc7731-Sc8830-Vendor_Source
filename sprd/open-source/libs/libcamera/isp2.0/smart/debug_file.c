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

#include "debug_file.h"
#include <stdio.h>
#include <string.h>
#include "isp_log.h"
/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
#define DEBUG_SUCCESS 	0
#define DEBUG_ERROR	1
#define UNUSED(param)  (void)(param)
struct awbl_debug_file
{
	char		file_name[32];
	char		open_mode[4];
	char		file_buffer[DEBUG_FILE_BUF_SIZE];
	uint32_t	buffer_size;
	uint32_t	buffer_used_size;
	FILE		*pf;
};

/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/
static void _debugFlush(struct awbl_debug_file *debug_file)
{
	if (NULL == debug_file)
		return;

	if (NULL == debug_file->pf)
		return;

	if (debug_file->buffer_used_size > 0)
	{
		fwrite(debug_file->file_buffer, 1, debug_file->buffer_used_size,
			debug_file->pf);
		debug_file->buffer_used_size = 0;
		debug_file->file_buffer[0] = 0;
	}
}


/**---------------------------------------------------------------------------*
** 				Public Function Prototypes				*
**---------------------------------------------------------------------------*/
void _debugPrintf(struct awbl_debug_file *debug_file, char *str)
{
	uint32_t len = strlen(str) + 1;

	if (NULL == debug_file)
		return;

	if (NULL == debug_file->pf)
		return;

	if (len + debug_file->buffer_used_size >= debug_file->buffer_size)
	{
		_debugFlush(debug_file);
	}

	strcat(debug_file->file_buffer, str);
	debug_file->buffer_used_size = strlen(debug_file->file_buffer) + 1;

}

void _debugOpenFile(struct awbl_debug_file *debug_file)
{
	if (NULL == debug_file)
		return;

	debug_file->pf = fopen(debug_file->file_name, debug_file->open_mode);
}

void _debugCloseFile(struct awbl_debug_file *debug_file)
{
	if (NULL == debug_file)
		return;

	if (NULL != debug_file->pf)
	{
		fclose(debug_file->pf);
		debug_file->pf = NULL;
	}
}

void _initDebugFile(struct awbl_debug_file *debug_file,
			uint32_t level, uint32_t level_thres,
			const char *file_name,
			const char *open_mode)
{
	if (NULL == debug_file)
		return;

	if (level >= level_thres)
	{
		debug_file->buffer_size = DEBUG_FILE_BUF_SIZE;
		debug_file->buffer_used_size = 0;
		memset(debug_file->file_buffer, 0, sizeof(debug_file->file_buffer));
		debug_file->pf = NULL;
		strcpy(debug_file->open_mode, open_mode);
		strcpy(debug_file->file_name, file_name);
		_debugOpenFile(debug_file);
	}
}

void _deinitDebugFile(struct awbl_debug_file *debug_file, uint32_t level)
{
	UNUSED(level);
	if (NULL == debug_file)
		return;

	if (NULL != debug_file->pf)
	{
		_debugFlush(debug_file);
		_debugCloseFile(debug_file);
		debug_file->buffer_used_size = 0;
	}
}

static void _flush(struct awbl_debug_file *debug_file)
{
	if (NULL == debug_file || NULL == debug_file->pf)
		return;

	if (debug_file->buffer_used_size > 0)
	{
		fwrite(debug_file->file_buffer, 1, debug_file->buffer_used_size,
			debug_file->pf);
		debug_file->buffer_used_size = 0;
		debug_file->file_buffer[0] = 0;
	}
}

uint32_t debug_file_open(debug_handle_t handle, uint32_t debug_level,
			uint32_t debug_level_thres)
{
	struct awbl_debug_file *debug_file = NULL;

	if (0 == handle)
		return DEBUG_ERROR;

	if (debug_level > debug_level_thres) {

		debug_file = (struct awbl_debug_file *)handle;
		debug_file->buffer_size = DEBUG_FILE_BUF_SIZE;
		debug_file->buffer_used_size = 0;
		memset(debug_file->file_buffer, 0, sizeof(debug_file->file_buffer));
		debug_file->pf = fopen(debug_file->file_name, debug_file->open_mode);
	}

	return (NULL != debug_file->pf) ? DEBUG_SUCCESS : DEBUG_ERROR;
}

void debug_file_close(debug_handle_t handle)
{
	struct awbl_debug_file *debug_file = (struct awbl_debug_file *)handle;

	if (NULL == debug_file)
		return;

	if (NULL != debug_file->pf)
	{
		_flush(debug_file);

		fclose(debug_file->pf);
		debug_file->pf = NULL;
	}
}

debug_handle_t debug_file_init(const char file_name[], const char open_mode[])
{
	struct awbl_debug_file *debug_file = NULL;

	debug_file = (struct awbl_debug_file *)malloc(sizeof(struct awbl_debug_file));
	if (NULL == debug_file) {
		return NULL;
	}

	debug_file->buffer_size = DEBUG_FILE_BUF_SIZE;
	debug_file->buffer_used_size = 0;
	memset(debug_file->file_buffer, 0, sizeof(debug_file->file_buffer));
	debug_file->pf = NULL;

	strcpy(debug_file->open_mode, open_mode);
	strcpy(debug_file->file_name, file_name);

	return (debug_handle_t)debug_file;

}

void debug_file_deinit(debug_handle_t handle)
{
	if (0 != handle) {

		debug_file_close(handle);
		free((struct awbl_debug_file *)handle);
		handle = 0;
	}
}

void debug_file_print(debug_handle_t handle, char *str)
{
	struct awbl_debug_file *debug_file = (struct awbl_debug_file *)handle;
	uint32_t len = 0;

	ISP_LOGV("handle=%p", handle);

	if (NULL == debug_file)
		return;

	ISP_LOGV("file=%p", debug_file->pf);

	if (NULL == debug_file->pf)
		return;

	ISP_LOGV("%s", str);

	strcat(str, "\n");

	len = strlen(str) + 1;

	if (len + debug_file->buffer_used_size >= debug_file->buffer_size)
	{
		_flush(debug_file);
	}

	strcat(debug_file->file_buffer, str);
	debug_file->buffer_used_size = strlen(debug_file->file_buffer) + 1;
}

void debug_file_clear(debug_handle_t handle)
{
	struct awbl_debug_file *debug_file = (struct awbl_debug_file *)handle;
	uint32_t len = 0;

	if (NULL == debug_file)
		return;

	if (NULL == debug_file->pf)
		return;

	memset(debug_file->file_buffer, 0, debug_file->buffer_size);
	fwrite(debug_file->file_buffer, 1, debug_file->buffer_size, debug_file->pf);
	fseek(debug_file->pf, 0, SEEK_SET);
	debug_file->buffer_used_size = 0;
}
