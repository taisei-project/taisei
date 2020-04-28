/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stages.h"
#include "stages/stage1/stage1.h"
#include "stages/stage2/stage2.h"
#include "stages/stage3/stage3.h"
#include "stages/stage4/stage4.h"
#include "stages/stage5/stage5.h"
#include "stages/stage6/stage6.h"
#include "stages/stagex/stagex.h"

#ifdef TAISEI_BUILDCONF_TESTING_STAGES
#include "stages/dpstest.h"
#endif

StagesExports stages_exports = {
	.stage1 = { &stage1_procs, (AttackInfo*)&stage1_spells },
	.stage2 = { &stage2_procs, (AttackInfo*)&stage2_spells },
	.stage3 = { &stage3_procs, (AttackInfo*)&stage3_spells },
	.stage4 = { &stage4_procs, (AttackInfo*)&stage4_spells },
	.stage5 = { &stage5_procs, (AttackInfo*)&stage5_spells },
	.stage6 = { &stage6_procs, (AttackInfo*)&stage6_spells },
	.stagex = { &stagex_procs, NULL },

#ifdef TAISEI_BUILDCONF_TESTING_STAGES
	.testing = {
		.dps_single = &stage_dpstest_single_procs,
		.dps_multi = &stage_dpstest_multi_procs,
		.dps_boss = &stage_dpstest_boss_procs,
		.benchmark_spell = &stage1_spell_benchmark,
	}
#endif
};
