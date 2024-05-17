/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "entity.h"

Boss *stage1_spawn_cirno(cmplx pos);
void stage1_draw_cirno_spellbg(Boss *boss, int time);
void stage1_cirno_wander(Boss *boss, real dist, real lower_bound);
