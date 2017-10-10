/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "boss.h"

void scuttle_spellbg(Boss*, int t);
void stage3_boss_spellbg(Boss*, int t);
void scuttle_dance(Boss*, int t);
void scuttle_acid(Boss*, int t);
void stage3_boss_a1(Boss*, int t);
void stage3_boss_a2(Boss*, int t);
void stage3_boss_a3(Boss*, int t);
void stage3_boss_extra(Boss*, int t);

void stage3_events(void);
Boss* stage3_spawn_scuttle(complex pos);
Boss* stage3_spawn_wriggle_ex(complex pos);
