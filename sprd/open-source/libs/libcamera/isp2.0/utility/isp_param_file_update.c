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
/*----------------------------------------------------------------------------*
 **				Dependencies					*
 **---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "isp_log.h"

#include "isp_param_file_update.h"
/**---------------------------------------------------------------------------*
 **				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif


int read_mode(FILE* fp, unsigned char * buf, int* plen)
{
	unsigned char* param_buf = buf;

	unsigned char line_buff[1024];
	int i;

	while (!feof(fp))
	{
		unsigned char c[16];
		int n = 0;

		if (fgets(line_buff, 1024, fp) == NULL)
		{
			break;
		}

		if (strstr(line_buff, "{") != NULL)
		{
			continue;
		}

		if (strstr(line_buff, "/*") != NULL)
		{
			continue;
		}

		if (strstr(line_buff, "};") != NULL)
		{
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++)
		{
			*param_buf ++ = c[i];
		}
	}

	*plen = (long)param_buf - (long)(buf);

	return 0;
}

uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,SENSOR_ID_E sensor_id)
{
	const char *sensor_name = sensor_info_ptr->name;
	struct sensor_raw_info* sensor_raw_info_ptr = *(sensor_info_ptr->raw_info_ptr);

	FILE* fp = NULL;
	unsigned char filename[80];

	unsigned char line_buff[512];

	int mode_flag[16] = {0};

	unsigned char mode_name_prefix[80];
	unsigned char mode_name[16][80];

	unsigned char mode_info_array[80];
	unsigned char mode_info[16][80];

	int i;


	sprintf(filename, "/data/sensor_%s_raw_param_v3.c", sensor_name);

	sprintf(mode_name_prefix, "static uint8_t s_%s_tune_info_", sensor_name);

	sprintf(mode_name[0], "static uint8_t s_%s_tune_info_common[]=", sensor_name);
	sprintf(mode_name[1], "static uint8_t s_%s_tune_info_prv_0[]=", sensor_name);
	sprintf(mode_name[2], "static uint8_t s_%s_tune_info_prv_1[]=", sensor_name);
	sprintf(mode_name[3], "static uint8_t s_%s_tune_info_prv_2[]=", sensor_name);
	sprintf(mode_name[4], "static uint8_t s_%s_tune_info_prv_3[]=", sensor_name);
	sprintf(mode_name[5], "static uint8_t s_%s_tune_info_cap_0[]=", sensor_name);
	sprintf(mode_name[6], "static uint8_t s_%s_tune_info_cap_1[]=", sensor_name);
	sprintf(mode_name[7], "static uint8_t s_%s_tune_info_cap_2[]=", sensor_name);
	sprintf(mode_name[8], "static uint8_t s_%s_tune_info_cap_3[]=", sensor_name);
	sprintf(mode_name[9], "static uint8_t s_%s_tune_info_video_0[]=", sensor_name);
	sprintf(mode_name[10], "static uint8_t s_%s_tune_info_video_1[]=", sensor_name);
	sprintf(mode_name[11], "static uint8_t s_%s_tune_info_video_2[]=", sensor_name);
	sprintf(mode_name[12], "static uint8_t s_%s_tune_info_video_3[]=", sensor_name);


	sprintf(mode_info_array, "static struct sensor_raw_info s_%s_mipi_raw_info=", sensor_name);

	sprintf(mode_info[0], "{s_%s_tune_info_common, sizeof(s_%s_tune_info_common)},", sensor_name, sensor_name);
	sprintf(mode_info[1], "{s_%s_tune_info_prv_0, sizeof(s_%s_tune_info_prv_0)},", sensor_name, sensor_name);
	sprintf(mode_info[2], "{s_%s_tune_info_prv_1, sizeof(s_%s_tune_info_prv_1)},", sensor_name, sensor_name);
	sprintf(mode_info[3], "{s_%s_tune_info_prv_2, sizeof(s_%s_tune_info_prv_2)},", sensor_name, sensor_name);
	sprintf(mode_info[4], "{s_%s_tune_info_prv_3, sizeof(s_%s_tune_info_prv_3)},", sensor_name, sensor_name);
	sprintf(mode_info[5], "{s_%s_tune_info_cap_0, sizeof(s_%s_tune_info_cap_0)},", sensor_name, sensor_name);
	sprintf(mode_info[6], "{s_%s_tune_info_cap_1, sizeof(s_%s_tune_info_cap_1)},", sensor_name, sensor_name);
	sprintf(mode_info[7], "{s_%s_tune_info_cap_2, sizeof(s_%s_tune_info_cap_2)},", sensor_name, sensor_name);
	sprintf(mode_info[8], "{s_%s_tune_info_cap_3, sizeof(s_%s_tune_info_cap_3)},", sensor_name, sensor_name);
	sprintf(mode_info[9], "{s_%s_tune_info_video_0, sizeof(s_%s_tune_info_video_0)},", sensor_name, sensor_name);
	sprintf(mode_info[10], "{s_%s_tune_info_video_1, sizeof(s_%s_tune_info_video_1)},", sensor_name, sensor_name);
	sprintf(mode_info[11], "{s_%s_tune_info_video_2, sizeof(s_%s_tune_info_video_2)},", sensor_name, sensor_name);
	sprintf(mode_info[12], "{s_%s_tune_info_video_3, sizeof(s_%s_tune_info_video_3)},", sensor_name, sensor_name);


	ISP_LOGI("ISP try to read %s", filename);
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		return -1;
	}


	while (!feof(fp))
	{
		if (fgets(line_buff, 512, fp) == NULL)
		{
			break;
		}
	

		if (strstr(line_buff, mode_name_prefix) != NULL)
		{
			for (i=0; i<16; i++)
			{
				if (strstr(line_buff, mode_name[i]) != NULL)
				{
					read_mode(fp, sensor_raw_info_ptr->mode_ptr[i].addr, &(sensor_raw_info_ptr->mode_ptr[i].len));

					break;
				}
			}
		}
		else if (strstr(line_buff, mode_info_array) != NULL)
		{
			while (!feof(fp))
			{
				if (fgets(line_buff, 512, fp) == NULL)
				{
					break;
				}

				for (i=0; i<16; i++)
				{
					if (strstr(line_buff, mode_info[i]) != NULL)
					{
						mode_flag[i] = 1;
						break;
					}
				}
			}
		}
	}


	for (i=0; i<16; i++)
	{
		if (mode_flag[i] == 0)
		{
			sensor_raw_info_ptr->mode_ptr[i].addr = NULL;
			sensor_raw_info_ptr->mode_ptr[i].len = 0;
		}
	}

	fclose(fp);


	return 0;
}


/**----------------------------------------------------------------------------*
**				Compiler Flag					*
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

// End

