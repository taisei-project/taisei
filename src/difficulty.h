/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "color.h"
#include "resource/resource.h"

typedef enum {
	D_Any = 0,
	D_Easy,
	D_Normal,
	D_Hard,
	D_Lunatic,
	D_Extra // reserved for later
} Difficulty;

#define NUM_SELECTABLE_DIFFICULTIES D_Lunatic

const char *difficulty_name(Difficulty diff)
	attr_pure attr_returns_nonnull;

const char *difficulty_sprite_name(Difficulty diff)
	attr_pure attr_returns_nonnull;

const Color *difficulty_color(Difficulty diff)
	attr_pure attr_returns_nonnull;

void difficulty_preload(ResourceGroup *rg);

#define difficulty_value(easy, normal, hard, lunatic) ({ \
    typeof((easy)+(normal)+(hard)+(lunatic)) val; \
    switch(global.diff) { \
        case D_Easy:    val = easy;    break; \
        case D_Normal:  val = normal;  break; \
        case D_Hard:    val = hard;    break; \
        case D_Lunatic: val = lunatic; break; \
        default: UNREACHABLE; \
    } \
    val; \
})
