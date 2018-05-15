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
#include "marisa.h"

PlayerCharacter character_marisa = {
	.id = PLR_CHAR_MARISA,
	.lower_name = "marisa",
	.proper_name = "Marisa",
	.full_name = "Kirisame Marisa",
	.title = "Black Magician",
	.dialog_sprite_name = "dialog/marisa",
	.player_sprite_name = "player/marisa",
	.ending = {
		.good = good_ending_marisa,
		.bad = bad_ending_marisa,
	},
};

double marisa_common_property(Player *plr, PlrProperty prop) {
	switch(prop) {
		case PLR_PROP_BOMB_TIME:
			return 300;

		case PLR_PROP_COLLECT_RADIUS:
			return (plr->inputflags & INFLAG_FOCUS) ? 60 : 30;

		case PLR_PROP_SPEED:
			// NOTE: For equivalents in Touhou units, divide these by 1.25.
			return (plr->inputflags & INFLAG_FOCUS) ? 2.75 : 6.25;

		case PLR_PROP_POC:
			return VIEWPORT_H / 3.5;

		case PLR_PROP_DEATHBOMB_WINDOW:
			return 12;
	}

	UNREACHABLE;
}

void marisa_common_shot(Player *plr, float dmg) {
	play_loop("generic_shot");

	if(!(global.frames % 6)) {
		Color *c = RGB(1, 1, 1);

		for(int i = -1; i < 2; i += 2) {
			PROJECTILE(
				.proto = pp_marisa,
				.pos = plr->pos + 10 * i - 15.0*I,
				.color = c,
				.rule = linear,
				.args = { -20.0*I },
				.type = PlrProj,
				.damage = dmg,
				.shader = "sprite_default",
			);
		}
	}
}

void marisa_common_slave_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite = "hakkero",
		.shader = "marisa_hakkero",
		.pos = { creal(e->pos), cimag(e->pos) },
		.rotation.angle = t * 0.05,
		.color = RGB(0.2, 0.4, 0.5),
	});
}

void marisa_common_masterspark_draw(int t) {
	ShaderProgram *prog_saved = r_shader_current();
	r_shader("masterspark");
	r_uniform_float("t", t);
	r_draw_quad();
	r_shader_ptr(prog_saved);
}
