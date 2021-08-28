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

void elly_spellbg_toe(Boss*, int);

void elly_curvature(Boss*, int);
void elly_lhc(Boss*, int);
void elly_eigenstate(Boss*, int);
void elly_eigenstate(Boss*, int);
void elly_broglie(Boss*, int);
void elly_ricci(Boss*, int);
void elly_theory(Boss*, int);
