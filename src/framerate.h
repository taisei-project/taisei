/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "hirestime.h"

typedef struct {
    hrtime_t frametimes[120]; // size = number of frames to average
    double fps; // average fps over the last X frames
    hrtime_t frametime; // average frame time over the last X frames;
    hrtime_t last_update_time; // internal; last time the average was recalculated
} FPSCounter;

uint32_t get_effective_frameskip(void);
void fpscounter_reset(FPSCounter *fps);
void fpscounter_update(FPSCounter *fps);
