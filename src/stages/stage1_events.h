/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage1_events_h
#define IGUARD_stages_stage1_events_h

#include "taisei.h"

#include "boss.h"

void cirno_pfreeze_bg(Boss*, int);
void cirno_benchmark(Boss*, int);

Boss *stage1_spawn_cirno(cmplx pos);

DECLARE_EXTERN_TASK(stage1_main, NO_ARGS);

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_perfect_freeze, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_crystal_rain, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_snow_halation, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_icicle_cascade, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_crystal_blizzard, BossAttack);

#endif // IGUARD_stages_stage1_events_h
