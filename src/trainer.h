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
	int trainer_lives_stage;
	int trainer_bombs_stage;
	int trainer_hits_stage;

	int trainer_lives_total;
	int trainer_bombs_total;
	int trainer_hits_total;
};

void trainer_init(Trainer *tnr);
void trainer_append_bomb_event(Trainer *tnr);
void trainer_append_life_event(Trainer *tnr);
void trainer_append_hit_event(Trainer *tnr);

bool trainer_enabled(void);
bool trainer_bombs_enabled(void);
bool trainer_invulnerable_enabled(void);
bool trainer_lives_enabled(void);
bool trainer_no_powerdown_enabled(void);

#endif // IGUARD_trainer_h
