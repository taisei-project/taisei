/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "global.h"

#include "boss.h"

#include "stages/stage6/elly.h"

DECLARE_EXTERN_TASK(stage6_boss_nonspell_baryons_common, { BoxedEllyBaryons baryons; });

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_boss_nonspell_1, ScytheAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_boss_nonspell_2, ScytheAttack);
DECLARE_EXTERN_TASK(stage6_boss_paradigm_shift, { BossAttackTaskArgs base; BoxedEllyScythe scythe; BoxedEllyBaryons baryons; });
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_boss_nonspell_4, BaryonsAttack);


void elly_baryonattack2(Boss*, int);
void elly_baryon_explode(Boss*, int);
