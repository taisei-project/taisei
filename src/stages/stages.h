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

typedef struct StagesExports {
	struct {
		StageProcs *procs;
		AttackInfo *spells;
	} stage1, stage2, stage3, stage4, stage5, stage6, stagex;

#ifdef TAISEI_BUILDCONF_TESTING_STAGES
	struct {
		StageProcs *dps_single, *dps_multi, *dps_boss;
		AttackInfo *benchmark_spell;
	} testing;
#endif
} StagesExports;

#ifdef TAISEI_BUILDCONF_DYNSTAGE
__attribute__((visibility("default")))
#endif
extern StagesExports stages_exports;
