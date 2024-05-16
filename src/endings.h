/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

typedef enum EndingID {
	// WARNING: Reordering this will break current progress files.

	ENDING_BAD_MARISA,
	ENDING_BAD_YOUMU,
	ENDING_GOOD_MARISA,
	ENDING_GOOD_YOUMU,
	ENDING_BAD_REIMU,
	ENDING_GOOD_REIMU,
	NUM_ENDINGS,
} EndingID;

#define GOOD_ENDINGS \
	ENDING(ENDING_GOOD_MARISA) \
	ENDING(ENDING_GOOD_YOUMU) \
	ENDING(ENDING_GOOD_REIMU)

#define BAD_ENDINGS \
	ENDING(ENDING_BAD_MARISA) \
	ENDING(ENDING_BAD_YOUMU) \
	ENDING(ENDING_BAD_REIMU)
