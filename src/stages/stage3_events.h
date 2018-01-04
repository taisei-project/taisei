/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"

void scuttle_spellbg(Boss*, int t);
void wriggle_spellbg(Boss*, int t);

void scuttle_deadly_dance(Boss*, int t);
void wriggle_moonlight_rocket(Boss*, int t);
void wriggle_night_ignite(Boss*, int t);
void wriggle_firefly_storm(Boss*, int t);
void wriggle_light_singularity(Boss*, int t);

void stage3_events(void);
Boss* stage3_spawn_scuttle(complex pos);
Boss* stage3_spawn_wriggle_ex(complex pos);

#define STAGE3_MIDBOSS_TIME 1765
