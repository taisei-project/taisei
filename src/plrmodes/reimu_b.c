/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "reimu.h"

static void reimu_spirit_preload(void) {
	const int flags = RESF_DEFAULT;

	//preload_resources(RES_TEXTURE, flags,
	//NULL);

	//preload_resources(RES_SHADER, flags,
	//NULL);

	//preload_resources(RES_SFX, flags | RESF_OPTIONAL,
	//NULL);
}


static void reimu_spirit_bomb(Player *p) {
}
static void reimu_spirit_shot(Player *p) {
}
static void reimu_spirit_power(Player *p, short npow) {
}

PlayerMode plrmode_reimu_b = {
	.name = "Spirit Sign",
	.character = &character_reimu,
	.shot_mode = PLR_SHOT_REIMU_SPIRIT,
	.procs = { 
		.bomb = reimu_spirit_bomb,
		.shot = reimu_spirit_shot,
		.power = reimu_spirit_power,
		.preload = reimu_spirit_preload,

	},
};
