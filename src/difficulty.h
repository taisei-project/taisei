/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include "color.h"

struct Color;

typedef enum {
    D_Any = 0,
    D_Easy,
    D_Normal,
    D_Hard,
    D_Lunatic,
    D_Extra // reserved for later
} Difficulty;

#define NUM_SELECTABLE_DIFFICULTIES D_Lunatic

const char* difficulty_name(Difficulty diff);
void difficulty_color(Color *c, Difficulty diff);

#endif
