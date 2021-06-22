/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage3_scuttle_h
#define IGUARD_stages_stage3_scuttle_h

#include "taisei.h"

#include "boss.h"

Boss *stage3_spawn_scuttle(cmplx pos);
void stage3_draw_scuttle_spellbg(Boss *boss, int time);

#endif // IGUARD_stages_stage3_scuttle_h
