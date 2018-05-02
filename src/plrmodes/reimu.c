/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "reimu.h"

PlayerCharacter character_reimu = {
	.id = PLR_CHAR_REIMU,
	.lower_name = "reimu",
	.proper_name = "Reimu",
	.full_name = "Hakurei Reimu",
	.title = "Shrine Maiden",
	.dialog_sprite_name = "dialog/reimu",
	.player_sprite_name = "player/reimu",
	.ending = {
		.good = good_ending_reimu,
		.bad = bad_ending_reimu,
	},
};

double reimu_common_property(Player *plr, PlrProperty prop) {
	switch(prop) {
		case PLR_PROP_BOMB_TIME:
			return 300;

		case PLR_PROP_COLLECT_RADIUS:
			return (plr->inputflags & INFLAG_FOCUS) ? 60 : 30;

		case PLR_PROP_SPEED:
			return (plr->inputflags & INFLAG_FOCUS) ? 2.0 : 4.5;

		case PLR_PROP_POC:
			return VIEWPORT_H / 3.5;

		case PLR_PROP_DEATHBOMB_WINDOW:
			return 12;
	}

	UNREACHABLE;
}

void reimu_yinyang_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite = "yinyang",
		.shader = "sprite_yinyang",
		.pos = { creal(e->pos), cimag(e->pos) },
		.rotation.angle = global.frames * 6 * DEG2RAD,
		.color = rgb(0.95, 0.75, 1.0),
	});
}
