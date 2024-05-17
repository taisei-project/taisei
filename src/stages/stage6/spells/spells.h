/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stages/common_imports.h"  // IWYU pragma: export
#include "../elly.h"  // IWYU pragma: export

#define ELLY_TOE_TARGET_POS (VIEWPORT_W/2+VIEWPORT_H/2*I)

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_newton, ScytheAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_kepler, ScytheAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_maxwell, ScytheAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_eigenstate, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_broglie, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_ricci, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_lhc, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_forgotten, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_toe, BossAttack);
