/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stageinfo.h"

extern StageProcs stagex_procs;
extern StageProcs stagex_spell_procs;

extern struct stagex_spells_s {
	// this struct must contain only fields of type AttackInfo
	// order of fields affects the visual spellstage number, but not its real internal ID

	struct {
		AttackInfo infinity_network;
		AttackInfo sierpinski;
	} boss;

	// required for iteration
	AttackInfo null;
} stagex_spells;

Boss *stagex_spawn_yumemi(cmplx pos);
void stagex_draw_yumemi_portrait_overlay(SpriteParams *sp);

DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_infinity_network, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_sierpinski, BossAttack);
