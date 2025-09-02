/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../stage1.h"

#include "stages/common_imports.h"  // IWYU pragma: export
#include "../cirno.h"  // IWYU pragma: export
#include "../misc.h"  // IWYU pragma: export

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_perfect_freeze, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_crystal_rain, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_snow_halation, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_icicle_cascade, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_crystal_blizzard, BossAttack);

#ifdef SPELL_BENCHMARK
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage1_spell_benchmark, BossAttack);
#endif
