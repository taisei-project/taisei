/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "boss.h"

#include "stages/stage6/draw.h"
#include "stages/stage6/elly.h"

#define ELLY_TOE_TARGET_POS (VIEWPORT_W/2+VIEWPORT_H/2*I)

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_newton, ScytheAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_kepler, ScytheAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_maxwell, ScytheAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_eigenstate, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_broglie, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_ricci, BaryonsAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage6_spell_lhc, BaryonsAttack);

void elly_spellbg_toe(Boss*, int);

void elly_curvature(Boss*, int);
void elly_theory(Boss*, int);
