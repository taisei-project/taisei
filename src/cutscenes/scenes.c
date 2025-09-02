/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "scenes.h"
#include "intl/intl.h"

Cutscene const g_cutscenes[NUM_CUTSCENE_IDS] = {
	[CUTSCENE_ID_INTRO] = {
		#include "intro.inc.h"
	},
	[CUTSCENE_ID_REIMU_BAD_END] = {
		#include "reimu_bad_end.inc.h"
	},
	[CUTSCENE_ID_REIMU_GOOD_END] = {
		#include "reimu_good_end.inc.h"
	},
	[CUTSCENE_ID_REIMU_EXTRA_INTRO] = {
		#include "reimu_extra_intro.inc.h"
	},
	[CUTSCENE_ID_MARISA_BAD_END] = {
		#include "marisa_bad_end.inc.h"
	},
	[CUTSCENE_ID_MARISA_GOOD_END] = {
		#include "marisa_good_end.inc.h"
	},
	[CUTSCENE_ID_MARISA_EXTRA_INTRO] = {
		#include "marisa_extra_intro.inc.h"
	},
	[CUTSCENE_ID_YOUMU_BAD_END] = {
		#include "youmu_bad_end.inc.h"
	},
	[CUTSCENE_ID_YOUMU_GOOD_END] = {
		#include "youmu_good_end.inc.h"
	},
	[CUTSCENE_ID_YOUMU_EXTRA_INTRO] = {
		#include "youmu_extra_intro.inc.h"
	},
};
