/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "marisa.h"

#include "audio/audio.h"
#include "global.h"
#include "plrmodes.h"
#include "stagedraw.h"
#include "util/graphics.h"

PlayerCharacter character_marisa = {
	.id = PLR_CHAR_MARISA,
	.lower_name = "marisa",
	.proper_name = "Marisa",
	.full_name = "Kirisame Marisa",
	.title = "Unbelievably Ordinary Magician",
	.menu_texture_name = "marisa_bombbg",
	.ending = {
		.good = { CUTSCENE_ID_MARISA_GOOD_END, ENDING_GOOD_MARISA },
		.bad = { CUTSCENE_ID_MARISA_BAD_END, ENDING_BAD_MARISA },
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

DEFINE_EXTERN_TASK(marisa_common_shot_forward) {
	Player *plr = TASK_BIND(ARGS.plr);
	ShaderProgram *bolt_shader = res_shader("sprite_particle");

	real damage = ARGS.damage;
	int delay = ARGS.delay;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		play_sfx_loop("generic_shot");

		for(int i = -1; i < 2; i += 2) {
			PROJECTILE(
				.proto = pp_marisa,
				.pos = plr->pos + 10 * i - 15.0*I,
				.move = move_linear(-20*I),
				.type = PROJ_PLAYER,
				.damage = damage,
				.shader_ptr = bolt_shader,
			);
		}

		WAIT(delay);
	}
}

static void draw_masterspark_ring(int t, float width) {
	float sy = sqrt(t / 500.0) * 6;
	float sx = sy * width / 800;

	if(sx == 0 || sy == 0) {
		return;
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite = "masterspark_ring",
		.shader = "sprite_default",
		.pos = { 0, -t*t*0.4 + 2 },
		.color = RGBA(0.5, 0.5, 0.5, 0.0),
		.scale = { .x = sx, .y = sy * sy * 1.5 },
	});
}

static void draw_masterspark_beam(cmplx origin, cmplx size, float angle, int t, float alpha) {
	r_mat_mv_push();
	r_mat_mv_translate(re(origin), im(origin), 0);
	r_mat_mv_rotate(angle, 0, 0, 1);

	r_shader("masterspark");
	r_uniform_float("t", t);

	r_mat_mv_push();
	r_mat_mv_translate(0, im(size) * -0.5, 0);
	r_mat_mv_scale(alpha * re(size), im(size), 1);
	r_draw_quad();
	r_mat_mv_pop();

	for(int i = 0; i < 4; i++) {
		draw_masterspark_ring(t % 20 + 10 * i, alpha * re(size));
	}

	r_mat_mv_pop();
}

void marisa_common_masterspark_draw(int numBeams, MarisaBeamInfo *beamInfos, float alpha) {
	r_state_push();

	float blur = 1.0 - alpha;
	int pp_quality = config_get_int(CONFIG_POSTPROCESS);

	if(pp_quality < 1 || (pp_quality < 2 && blur == 0)) {
		Framebuffer *main_fb = r_framebuffer_current();
		FBPair *aux = stage_get_fbpair(FBPAIR_FG_AUX);

		r_framebuffer(aux->back);
		r_clear(BUFFER_COLOR, RGBA(0, 0, 0, 0), 1);
		for(int i = 0; i < numBeams; i++) {
			draw_masterspark_beam(beamInfos[i].origin, beamInfos[i].size, beamInfos[i].angle, beamInfos[i].t, alpha);
		}

		fbpair_swap(aux);
		r_framebuffer(main_fb);
		r_shader_standard();
		r_color4(1, 1, 1, 1);
		draw_framebuffer_tex(aux->front, VIEWPORT_W, VIEWPORT_H);
	} else if(blur == 0) {
		for(int i = 0; i < numBeams; i++) {
			draw_masterspark_beam(beamInfos[i].origin, beamInfos[i].size, beamInfos[i].angle, beamInfos[i].t, alpha);
		}
	} else {
		Framebuffer *main_fb = r_framebuffer_current();
		FBPair *aux = stage_get_fbpair(FBPAIR_FG_AUX);

		r_framebuffer(aux->back);
		r_clear(BUFFER_COLOR, RGBA(0, 0, 0, 0), 1);

		for(int i = 0; i < numBeams; i++) {
			draw_masterspark_beam(beamInfos[i].origin, beamInfos[i].size, beamInfos[i].angle, beamInfos[i].t, alpha);
		}

		if(pp_quality > 1) {
			r_shader("blur25");
		} else {
			r_shader("blur5");
		}

		r_uniform_vec2("blur_resolution", VIEWPORT_W, VIEWPORT_H);
		r_uniform_vec2("blur_direction", blur, 0);

		fbpair_swap(aux);
		r_framebuffer(aux->back);
		r_clear(BUFFER_COLOR, RGBA(0, 0, 0, 0), 1);
		draw_framebuffer_tex(aux->front, VIEWPORT_W, VIEWPORT_H);

		r_uniform_vec2("blur_direction", 0, blur);

		fbpair_swap(aux);
		r_framebuffer(main_fb);
		draw_framebuffer_tex(aux->front, VIEWPORT_W, VIEWPORT_H);
	}

	r_state_pop();
}
