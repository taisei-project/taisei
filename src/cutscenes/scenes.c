/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "scenes.h"

Cutscene const g_cutscenes[NUM_CUTSCENE_IDS] = {
	[CUTSCENE_ID_INTRO] = {
		#include "intro.inc.h"
	},

	[CUTSCENE_ID_REIMU_BAD_END] = {
		#include "reimu_bad_end.inc.h"
	},
};
