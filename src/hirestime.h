/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

typedef uint64_t hrtime_t;
typedef int64_t shrtime_t;
#define PRIuTIME PRIu64
#define PRIdTIME PRId64
#define HRTIME_C(value) UINT64_C(value)

#include <SDL3/SDL_timer.h>
#define HRTIME_RESOLUTION SDL_NS_PER_SECOND

void time_init(void);
void time_shutdown(void);
hrtime_t time_get(void);
