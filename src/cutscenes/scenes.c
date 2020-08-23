/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "scenes.h"

CutscenePhase *g_cutscenes[NUM_CUTSCENE_IDS] = {
	[CUTSCENE_ID_REIMU_BAD_END] = (CutscenePhase[]) {
		#include "reimu_bad_end.inc.h"
	},
};
