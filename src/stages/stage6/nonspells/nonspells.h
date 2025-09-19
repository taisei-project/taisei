/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stages/common_imports.h"  // IWYU pragma: export
#include "../elly.h"  // IWYU pragma: export

DECLARE_EXTERN_TASK(stage6_boss_nonspell_scythe_common, { BoxedEllyScythe scythe; });
DECLARE_EXTERN_TASK(stage6_boss_nonspell_baryons_common, { BoxedEllyBaryons baryons; });

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_boss_nonspell_1, ScytheAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_boss_nonspell_2, ScytheAttack);
DECLARE_EXTERN_TASK(stage6_boss_paradigm_shift, { BossAttackTaskArgs base; BoxedEllyScythe scythe; BoxedEllyBaryons baryons; });
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_boss_nonspell_4, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_boss_nonspell_5, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_boss_baryons_explode, BaryonsAttack);
