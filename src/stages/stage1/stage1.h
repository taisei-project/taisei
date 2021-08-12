/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stageinfo.h"

#if defined(DEBUG) && !defined(SPELL_BENCHMARK)
	#define SPELL_BENCHMARK
#endif

extern struct stage1_spells_s {
	// this struct must contain only fields of type AttackInfo
	// order of fields affects the visual spellstage number, but not its real internal ID

	struct {
		AttackInfo perfect_freeze;
	} mid;

	struct {
		AttackInfo crystal_rain;
		AttackInfo snow_halation;
		AttackInfo icicle_cascade;
	} boss;

	struct {
		AttackInfo crystal_blizzard;
	} extra;

	// required for iteration
	AttackInfo null;
} stage1_spells;

extern StageProcs stage1_procs;
extern StageProcs stage1_spell_procs;

#ifdef SPELL_BENCHMARK
extern AttackInfo stage1_spell_benchmark;
#endif
