/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "boss.h"

Boss *stage3_spawn_wriggle(cmplx pos);
void stage3_draw_wriggle_spellbg(Boss *boss, int time);

void wriggle_slave_visual(Enemy *e, int time, bool render);
int wriggle_spell_slave(Enemy *e, int time);
