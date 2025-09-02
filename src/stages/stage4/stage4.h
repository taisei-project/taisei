/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stageinfo.h"

extern struct stage4_spells_s {
	// this struct must contain only fields of type AttackInfo
	// order of fields affects the visual spellstage number, but not its real internal ID

	struct {
		AttackInfo gate_of_walachia;
		AttackInfo dry_fountain;
		AttackInfo red_spike;
	} mid;

	struct {
		AttackInfo animate_wall;
		AttackInfo demon_wall;
		AttackInfo bloody_danmaku;
		AttackInfo blow_the_walls;
	} boss;

	struct {
		AttackInfo vlads_army;
	} extra;

	// required for iteration
	AttackInfo null;
} stage4_spells;

extern StageProcs stage4_procs;
extern StageProcs stage4_spell_procs;

void stage4_skip(int t);
