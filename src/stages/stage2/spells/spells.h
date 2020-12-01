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

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_amulet_of_harm, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_wheel_of_fortune, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_bad_pick, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_monty_hall_danmaku, BossAttack);

#endif // IGUARD_stages_stage2_spells_spells_h
