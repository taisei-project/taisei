/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "util/callchain.h"

typedef enum CutsceneID {
	// NOTE: These IDs are used for progress tracking, do not reorder!
	// Only ever append to the end of this enum.
	// Removed cutscenes must be replaced with a stub.

	CUTSCENE_ID_INTRO = 0,
	CUTSCENE_ID_REIMU_BAD_END = 1,
	CUTSCENE_ID_REIMU_GOOD_END = 2,
	CUTSCENE_ID_REIMU_EXTRA_INTRO = 3,
	CUTSCENE_ID_MARISA_BAD_END = 4,
	CUTSCENE_ID_MARISA_GOOD_END = 5,
	CUTSCENE_ID_MARISA_EXTRA_INTRO = 6,
	CUTSCENE_ID_YOUMU_BAD_END = 7,
	CUTSCENE_ID_YOUMU_GOOD_END = 8,
	CUTSCENE_ID_YOUMU_EXTRA_INTRO = 9,

	NUM_CUTSCENE_IDS,
} CutsceneID;

void cutscene_enter(CallChain next, CutsceneID id);
const char *cutscene_get_name(CutsceneID id);
