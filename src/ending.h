/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "resource/texture.h"

enum {
	ENDING_FADE_OUT = 200,
	ENDING_FADE_TIME = 60,
};

enum {
	// WARNING: Reordering this will break current progress files.

	ENDING_BAD_1,
	ENDING_BAD_2,
	ENDING_GOOD_1,
	ENDING_GOOD_2,
	ENDING_BAD_3,
	ENDING_GOOD_3,
	NUM_ENDINGS,
};

typedef struct EndingEntry EndingEntry;
typedef struct Ending Ending;

void ending_loop(void);
void ending_preload(void);

/*
 * These ending callbacks are referenced in plrmodes/ code
 */
void bad_ending_marisa(Ending *e);
void bad_ending_youmu(Ending *e);
void bad_ending_reimu(Ending *e);
void good_ending_marisa(Ending *e);
void good_ending_youmu(Ending *e);
void good_ending_reimu(Ending *e);
