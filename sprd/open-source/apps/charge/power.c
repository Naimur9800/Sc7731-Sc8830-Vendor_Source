/*
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
#include <hardware_legacy/power.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include "common.h"

#define LOG_TAG "power"
#include <utils/Log.h>


enum {
	ACQUIRE_PARTIAL_WAKE_LOCK = 0,
	RELEASE_WAKE_LOCK,
	REQUEST_STATE,
	OUR_FD_COUNT
};

const char * const OLD_PATHS[] = {
	"/sys/android_power/acquire_partial_wake_lock",
	"/sys/android_power/release_wake_lock",
	"/sys/android_power/request_state"
};

const char * const NEW_PATHS[] = {
	"/sys/power/wake_lock",
	"/sys/power/wake_unlock",
	"/sys/power/state"
};

static int g_initialized = 0;
static int g_fds[OUR_FD_COUNT];
static int g_error = 1;

static const char *off_state = "mem";
static const char *on_state = "on";

	static int
open_file_descriptors(const char * const paths[])
{
	int i;
	for (i=0; i<OUR_FD_COUNT; i++) {
		int fd = open(paths[i], O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "fatal error opening \"%s\"\n", paths[i]);
			g_error = errno;
			return -1;
		}
		g_fds[i] = fd;
	}

	g_error = 0;
	return 0;
}

	static inline void
initialize_fds(void)
{
	if (g_initialized == 0) {
		if(open_file_descriptors(NEW_PATHS) < 0) {
			open_file_descriptors(OLD_PATHS);
			on_state = "wake";
			off_state = "standby";
		}
		g_initialized = 1;
	}
}

	int
set_screen_state(int on)
{

	LOGI("*** set_screen_state %d", on);

	initialize_fds();

	if (g_error) return g_error;

	char buf[32];
	int len;
	if(on)
		len = sprintf(buf, "%s", on_state);
	else
		len = sprintf(buf, "%s", off_state);
	len = write(g_fds[REQUEST_STATE], buf, len);
	if(len < 0) {
		LOGE("Failed setting last user activity: g_error=%d\n", g_error);
	}
	return 0;
}
