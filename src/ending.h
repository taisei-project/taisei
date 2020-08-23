/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_ending_h
#define IGUARD_ending_h

#include "taisei.h"

#include "eventloop/eventloop.h"

enum {
	ENDING_FADE_OUT = 200,
	ENDING_FADE_TIME = 60,
};

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
	ENDING(ENDING_GOOD_REIMU) \

#define BAD_ENDINGS \
	ENDING(ENDING_BAD_MARISA) \
	ENDING(ENDING_BAD_YOUMU) \
	ENDING(ENDING_BAD_REIMU) \

INLINE bool ending_is_good(EndingID end) {
	#define ENDING(e) (end == (e)) ||
	return (
		GOOD_ENDINGS
	false);
	#undef ENDING
}

INLINE bool ending_is_bad(EndingID end) {
	#define ENDING(e) (end == (e)) ||
	return (
		BAD_ENDINGS
	false);
	#undef ENDING
}

typedef struct Ending Ending;

void ending_enter(CallChain next);
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

#endif // IGUARD_ending_h
