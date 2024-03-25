/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "laser.h"
#include "util.h"
#include "dynarray.h"

typedef struct LaserInternalData {
	DYNAMIC_ARRAY(LaserSegment) segments;
} LaserInternalData;

extern LaserInternalData lintern;

// Should be set slightly larger than the
// negated lowest distance threshold value in the sdf_apply shader
#define LASER_SDF_RANGE 4.01f

void laserintern_init(void);
void laserintern_shutdown(void);
