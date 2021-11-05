/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"
#include "portrait.h"

#define NUM_PROFILES 11
#define LOCKED_PROFILE NUM_PROFILES-1
#define DESCRIPTION_WIDTH (SCREEN_W / 3 + 80)

typedef enum {
	PROFILE_REIMU,
	PROFILE_MARISA,
	PROFILE_YOUMU,
	PROFILE_CIRNO,
	PROFILE_HINA,
	PROFILE_SCUTTLE,
	PROFILE_WRIGGLE,
	PROFILE_KURUMI,
	PROFILE_IKU,
	PROFILE_ELLY,
	PROFILE_LOCKED,
} CharProfiles;

typedef struct CharacterProfile {
	const char *name;
	const char *fullname;
	const char *title;
	const char *description;
	const char *background;
	const char *unlock;
	Sprite *sprite;
	char *faces[11];
} CharacterProfile;

typedef struct CharProfileContext {
	int8_t char_draw_order[NUM_PROFILES];
	int8_t prev_selected_char;
	int8_t face;
} CharProfileContext;

MenuData *create_charprofile_menu(void);
