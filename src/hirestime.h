/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

typedef uint64_t hrtime_t;
typedef int64_t shrtime_t;
#define PRIuTIME PRIu64
#define PRIdTIME PRId64
#define HRTIME_C(value) UINT64_C(value)

// picoseconds. like super duper accurate, man
#define HRTIME_RESOLUTION HRTIME_C(1000000000000)

void time_init(void);
void time_shutdown(void);
hrtime_t time_get(void);
