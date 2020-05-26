/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_stages_stage2_spells_spells_h
#define IGUARD_stages_stage2_spells_spells_h

#include "taisei.h"

#include "boss.h"

void hina_bad_pick(Boss*, int);
void hina_wheel(Boss*, int);
void hina_monty(Boss*, int);

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_amulet_of_harm, BossAttack);

#endif // IGUARD_stages_stage2_spells_spells_h
