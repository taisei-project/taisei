/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_trainer_h
#define IGUARD_trainer_h

#include "taisei.h"

typedef struct Trainer Trainer;

struct Trainer {
	struct {
		int lives;
		int bombs;
		int hits;
	} total, stage;

	struct {
		bool enabled;
		bool dot;
		bool extra_bombs;
		bool extra_lives;
		bool invulnerable;
		bool no_powerdown;

		bool stats;
	} settings;
};

void trainer_init(Trainer *trainer);
void trainer_reset_stage_counters(Trainer *trainer);
void trainer_append_bomb_event(Trainer *trainer);
void trainer_append_life_event(Trainer *trainer);
void trainer_append_hit_event(Trainer *trainer);

bool trainer_anything_enabled(Trainer *trainer);

bool trainer_check_config(void);

#endif // IGUARD_trainer_h
