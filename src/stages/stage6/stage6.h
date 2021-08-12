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

extern struct stage6_spells_s {
	// this struct must contain only fields of type AttackInfo
	// order of fields affects the visual spellstage number, but not its real internal ID

	union {
		AttackInfo scythe_first;
		struct {
			AttackInfo occams_razor;
			AttackInfo orbital_clockwork;
			AttackInfo wave_theory;
		} scythe;
	};

	union {
		AttackInfo baryon_first;
		struct {
			AttackInfo many_world_interpretation;
			AttackInfo wave_particle_duality;
			AttackInfo spacetime_curvature;
			AttackInfo higgs_boson_uncovered;
		} baryon;
	};

	struct {
		AttackInfo curvature_domination;
	} extra;

	struct {
		AttackInfo theory_of_everything;
	} final;

	// required for iteration
	AttackInfo null;
} stage6_spells;

extern StageProcs stage6_procs;
extern StageProcs stage6_spell_procs;

// this hackery is needed for spell practice
#define STG6_SPELL_NEEDS_SCYTHE(s) ((s) >= &stage6_spells.scythe_first && ((s) - &stage6_spells.scythe_first) < sizeof(stage6_spells.scythe)/sizeof(AttackInfo))
#define STG6_SPELL_NEEDS_BARYON(s) ((s) >= &stage6_spells.baryon_first && ((s) - &stage6_spells.baryon_first) < sizeof(stage6_spells.baryon)/sizeof(AttackInfo)+1)
