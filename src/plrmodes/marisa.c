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

void marisa_common_shot(Player *plr, int dmg) {
	if(!(global.frames % 4)) {
		play_sound("generic_shot");
	}

	if(!(global.frames % 6)) {
		Color c = rgb(1, 1, 1);

		PROJECTILE("marisa", plr->pos + 10 - 15.0*I, c, linear, { -20.0*I },
			.type = PlrProj+dmg,
			.color_transform_rule = proj_clrtransform_particle,
		);

		PROJECTILE("marisa", plr->pos - 10 - 15.0*I, c, linear, { -20.0*I },
			.type = PlrProj+dmg,
			.color_transform_rule = proj_clrtransform_particle,
		);
	}
}

void marisa_common_slave_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	r_mat_push();
	r_mat_translate(creal(e->pos), cimag(e->pos), -1);
	// render_rotate_deg(global.frames * 3, 0, 0, 1);
	draw_sprite(0, 0, "part/smoothdot");
	r_mat_pop();
}

void marisa_common_masterspark_draw(int t) {
	ShaderProgram *mshader = get_shader_program("masterspark");
	glUseProgram(mshader->gl_handle);
	glUniform1f(uniloc(mshader,"t"),t);
	r_draw_quad();
	r_shader_standard();
}
