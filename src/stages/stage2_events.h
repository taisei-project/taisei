/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage2_events_h
#define IGUARD_stages_stage2_events_h

#include "taisei.h"

#include "boss.h"

void hina_spell_bg(Boss*, int);
void hina_amulet(Boss*, int);
void hina_bad_pick(Boss*, int);
void hina_wheel(Boss*, int);
void hina_monty(Boss*, int);

void stage2_events(void);
Boss* stage2_spawn_hina(complex pos);

#endif // IGUARD_stages_stage2_events_h
