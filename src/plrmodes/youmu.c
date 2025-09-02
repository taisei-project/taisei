/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "youmu.h"

#include "global.h"
#include "plrmodes.h"
#include "stagedraw.h"
#include "util/graphics.h"

PlayerCharacter character_youmu = {
	.id = PLR_CHAR_YOUMU,
	.lower_name = "youmu",
	.proper_name = "Yōmu",
	.full_name = "Konpaku Yōmu",
	.title = "Swordswoman Between Worlds",
	.menu_texture_name = "youmu_bombbg1",
	.ending = {
		.good = { CUTSCENE_ID_YOUMU_GOOD_END, ENDING_GOOD_YOUMU },
		.bad = { CUTSCENE_ID_YOUMU_BAD_END, ENDING_BAD_YOUMU },
	},
};

double youmu_common_property(Player *plr, PlrProperty prop) {
	switch(prop) {
		case PLR_PROP_BOMB_TIME:
			return 300;

		case PLR_PROP_COLLECT_RADIUS:
			return (plr->inputflags & INFLAG_FOCUS) ? 60 : 30;

		case PLR_PROP_SPEED:
			// NOTE: For equivalents in Touhou units, divide these by 1.25.
			return (plr->inputflags & INFLAG_FOCUS) ? 2.125 : 6.0;

		case PLR_PROP_POC:
			return VIEWPORT_H / 3.5;

		case PLR_PROP_DEATHBOMB_WINDOW:
			return 12;
	}

	UNREACHABLE;
}

Projectile *youmu_common_shot(cmplx pos, MoveParams move, real dmg, ShaderProgram *shader) {
	return PROJECTILE(
		.damage = dmg,
		.layer = LAYER_PLAYER_SHOT | 0x20,
		.move = move,
		.pos = pos,
		.proto = pp_youmu,
		.shader_ptr = shader,
		.type = PROJ_PLAYER,
	);
}

void youmu_common_init_bomb_background(YoumuBombBGData *bg_data) {
	FBAttachmentConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	cfg.tex_params.type = TEX_TYPE_RGB;
	cfg.tex_params.filter.min = TEX_FILTER_LINEAR;
	cfg.tex_params.filter.mag = TEX_FILTER_LINEAR;
	cfg.tex_params.wrap.s = TEX_WRAP_MIRROR;
	cfg.tex_params.wrap.t = TEX_WRAP_MIRROR;

	bg_data->buffer = stage_add_background_framebuffer("Youmu bomb FB", 0.25, 1, 1, &cfg);
	bg_data->shader = res_shader("youmu_bomb_bg");
	bg_data->uniforms.petals = r_shader_uniform(bg_data->shader, "petals");
	bg_data->uniforms.time = r_shader_uniform(bg_data->shader, "time");
	bg_data->texture = res_texture("youmu_bombbg1");
}

static void capture_frame(Framebuffer *dest, Framebuffer *src) {
	r_state_push();
	r_framebuffer(dest);
	r_shader_standard();
	r_color4(1, 1, 1, 1);
	r_blend(BLEND_NONE);
	draw_framebuffer_tex(src, VIEWPORT_W, VIEWPORT_H);
	r_state_pop();
}

DEFINE_EXTERN_TASK(youmu_common_bomb_background) {
	YoumuBombBGData *bg_data = ARGS.bg_data;
	CoEvent *draw_event = &stage_get_draw_events()->background_drawn;

	for(;;) {
		WAIT_EVENT_OR_DIE(draw_event);

		float t = player_get_bomb_progress(&global.plr);
		float fade = 1.0f;

		if(t < 1.0f / 12.0f) {
			fade = t * 12.0f;
		}

		if(t > 1.0f / 2.0f) {
			fade = 1-(t-1./2)*2;
		}

		if(fade < 0.0f) {
			fade = 0.0f;
		}

		r_state_push();
		r_color4(fade, fade, fade, fade);
		r_shader_ptr(bg_data->shader);
		r_uniform_sampler(bg_data->uniforms.petals, bg_data->texture);
		r_uniform_float(bg_data->uniforms.time, t);
		draw_framebuffer_tex(bg_data->buffer, VIEWPORT_W, VIEWPORT_H);
		r_state_pop();

		capture_frame(bg_data->buffer, r_framebuffer_current());
	}
}
