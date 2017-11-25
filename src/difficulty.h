/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "color.h"

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
const char* difficulty_tex(Difficulty diff);
Color difficulty_color(Difficulty diff);
void difficulty_preload(void);
