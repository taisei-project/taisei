/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"

void cirno_perfect_freeze(Boss*, int);
void cirno_crystal_rain(Boss*, int);
void cirno_snow_halation(Boss*, int);
void cirno_icicle_fall(Boss*, int);
void cirno_pfreeze_bg(Boss*, int);
void cirno_crystal_blizzard(Boss*, int);
void cirno_benchmark(Boss*, int);

void stage1_events(void);
Boss* stage1_spawn_cirno(complex pos);
