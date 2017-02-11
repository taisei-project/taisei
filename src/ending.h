/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef ENDING_H
#define ENDING_H

#include "resource/texture.h"

enum {
	ENDING_FADE_OUT = 200,
	ENDING_FADE_TIME = 60,
};

typedef struct EndingEntry EndingEntry;
struct EndingEntry {
	char *msg;
	Texture *tex;

	int time;
};

typedef struct Ending Ending;
struct Ending {
	EndingEntry *entries;
	int count;
	int duration;

	int pos;
};

void add_ending_entry(Ending *e, int time, char *msg, char *tex);

void create_ending(Ending *e);
void ending_loop(void);
void free_ending(Ending *e);

#endif
