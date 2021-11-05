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

#define NUM_PROFILES 3
#define NUM_EXPRESSIONS 4
#define DESCRIPTION_WIDTH (SCREEN_W / 3 + 80)

typedef enum {
	PROFILE_REIMU,
	PROFILE_MARISA,
	PROFILE_YOUMU,
} CharProfiles;

typedef struct CharacterProfile {
	const char *name;
	const char *fullname;
	const char *title;
	const char *description;
	const char *background;
} CharacterProfile;

typedef struct CharProfileContext {
	int8_t char_draw_order[NUM_PROFILES];
	int8_t prev_selected_char;
	int8_t face;
} CharProfileContext;

static const struct CharacterProfile profiles[NUM_PROFILES] = {
	[PROFILE_REIMU] = {
		.name = "reimu",
		.fullname = "Hakurei Reimu",
		.title = "Shrine Maiden of Fantasy",
		.description = "Species: Human\nArea of Study: Literature (Fiction)\n\nThe incredibly particular shrine maiden.\n\nShe’s taking a break from her busy novel-reading schedule to take care of an incident.\n\nMostly, she has a vague feeling of sentimentality for a bygone era.\n\nRegardless, when the residents of Yōkai Mountain show up sobbing at her door, she has no choice but to put her book down and spring into action.",
		.background = "reimubg",
	},
	[PROFILE_MARISA] = {
		.name = "marisa",
		.fullname = "Kirisame Marisa",
		.title = "Ordinary Magician",
		.description = "Species: Human\nArea of Study: Eclecticism\n\nThe confident, hyperactive witch.\n\nCurious as ever about the limits of magic and the nature of reality, and when she hears about “eldritch lunacy,” her curiosity gets the better of her.\n\nBut maybe this “grimoire” is best left unread?\n\nNot that a warning like that would stop her anyways. So, off she goes.",
		.background = "marisa_bombbg",
	},
	[PROFILE_YOUMU] = {
		.name = "youmu",
		.fullname = "Konpaku Youmu",
		.title = "Chuunibyou Swordswoman",
		.description = "Species: Half-Human, Half-Phantom\nArea of Study: Fencing\n\nThe swordswoman of the somewhat-dead.\n\n“While you were playing games, I was studying the blade” … is probably what she’d say.\n\nEntirely too serious about everything, and utterly terrified of (half of) her own existence.\n\nStill, she’s been given an important mission by Lady Yuyuko. Hopefully she doesn’t let it get to her head.",
		.background = "youmu_bombbg1",
	},
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(cirno, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(hina, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(scuttle, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(wriggle, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(kurumi, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(iku, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(elly, normal), */
};

MenuData *create_charprofile_menu(void);
