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
#include "renderer/api.h"

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

void marisa_common_shot(Player *plr, int dmg) {
	if(!(global.frames % 4)) {
		play_sound("generic_shot");
	}

	if(!(global.frames % 6)) {
		Color c = rgb(1, 1, 1);

		PROJECTILE("marisa", plr->pos + 10 - 15.0*I, c, linear, { -20.0*I },
			.type = PlrProj+dmg,
			.shader = "sprite_default",
		);

		PROJECTILE("marisa", plr->pos - 10 - 15.0*I, c, linear, { -20.0*I },
			.type = PlrProj+dmg,
			.shader = "sprite_default",
		);
	}
}

void marisa_common_slave_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	draw_sprite_batched(creal(e->pos), cimag(e->pos), "part/smoothdot");
}

void marisa_common_masterspark_draw(int t) {
	ShaderProgram *prog_saved = r_shader_current();
	r_shader("masterspark");
	r_uniform_float("t", t);
	r_draw_quad();
	r_shader_ptr(prog_saved);
}
