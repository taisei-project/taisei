/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <time.h>

#ifdef TAISEI_BUILDCONF_HAVE_TIMESPEC
typedef struct timespec SystemTime;
#else
typedef struct SystemTime {
	time_t tv_sec;
	long tv_nsec;
} SystemTime;
#endif

void get_system_time(SystemTime *time) attr_nonnull(1);
