/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage6_extra_h
#define IGUARD_stages_stage6_extra_h

#include "taisei.h"
#include "global.h"

#define FERMIONTIME 1000
#define HIGGSTIME 1700
#define YUKAWATIME 2200
#define SYMMETRYTIME (HIGGSTIME+200)
#define BREAKTIME (YUKAWATIME+400)
#define BROGLIE_PERIOD (420 - 30 * global.diff)

#define LASER_EXTENT (4+1.5*global.diff-D_Normal)
#define LASER_LENGTH 60

#define ELLY_TOE_TARGET_POS (VIEWPORT_W/2+VIEWPORT_H/2*I)

cmplx wrap_around(cmplx*);

int fall_over;

#endif // IGUARD_stages_stage6_extra_h
