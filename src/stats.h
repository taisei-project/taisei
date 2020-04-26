/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stats_h
#define IGUARD_stats_h

#include "taisei.h"

typedef struct Stats Stats;

struct Stats {
	struct {
		int lives;
		int bombs;
		int continues;
	} total, stage;

	struct {
		bool enabled;
	} settings;
};

void stats_init(Stats *stats);

void stats_append_life(Stats *stats);
void stats_append_bomb(Stats *stats);
void stats_append_continue(Stats *stats);

void stats_stage_reset(Stats *stats);

#endif // IGUARD_stats_h
