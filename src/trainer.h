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
};

void trainer_init(Trainer *trainer);
void trainer_reset_stage_counters(Trainer *trainer);
void trainer_append_bomb_event(Trainer *trainer);
void trainer_append_life_event(Trainer *trainer);
void trainer_append_hit_event(Trainer *trainer);

bool trainer_enabled(void);
bool trainer_hud_stats_enabled(void);
bool trainer_anything_enabled(void);
bool trainer_bombs_enabled(void);
bool trainer_invulnerable_enabled(void);
bool trainer_lives_enabled(void);
bool trainer_no_powerdown_enabled(void);
bool trainer_focus_dot_enabled(void);

#endif // IGUARD_trainer_h
