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

extern struct stage3_spells_s {
	// this struct must contain only fields of type AttackInfo
	// order of fields affects the visual spellstage number, but not its real internal ID

	struct {
		AttackInfo deadly_dance;
	} mid;

	struct {
		AttackInfo moonlight_rocket;
		AttackInfo moths_to_a_flame;
		AttackInfo firefly_storm;
	} boss;

	struct {
		AttackInfo light_singularity;
	} extra;

	// required for iteration
	AttackInfo null;
} stage3_spells;

extern StageProcs stage3_procs;
