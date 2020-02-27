/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "youmu.h"
#include "stagedraw.h"

PlayerCharacter character_youmu = {
	.id = PLR_CHAR_YOUMU,
	.lower_name = "youmu",
	.proper_name = "Yōmu",
	.full_name = "Konpaku Yōmu",
	.title = "Swordswoman Between Worlds",
	.dialog_base_sprite_name = "dialog/youmu",
	.player_sprite_name = "player/youmu",
	.menu_texture_name = "youmu_bombbg1",
	.ending = {
		.good = good_ending_youmu,
		.bad = bad_ending_youmu,
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

void youmu_common_shot(Player *plr) {
	play_loop("generic_shot");

	if(!(global.frames % 6)) {
		Color *c = RGB(1, 1, 1);

		PROJECTILE(
			.proto = pp_youmu,
			.pos = plr->pos + 10 - I*20,
			.color = c,
			.rule = linear,
			.args = { -20.0*I },
			.type = PROJ_PLAYER,
			.damage = 120,
			.shader = "sprite_default",
		);

		PROJECTILE(
			.proto = pp_youmu,
			.pos = plr->pos - 10 - I*20,
			.color = c,
			.rule = linear,
			.args = { -20.0*I },
			.type = PROJ_PLAYER,
			.damage = 120,
			.shader = "sprite_default",
		);
	}
}

static Framebuffer *bomb_buffer;

static void capture_frame(Framebuffer *dest, Framebuffer *src) {
	r_state_push();
	r_framebuffer(dest);
	r_shader_standard();
	r_color4(1, 1, 1, 1);
	r_blend(BLEND_NONE);
	draw_framebuffer_tex(src, VIEWPORT_W, VIEWPORT_H);
	r_state_pop();
}

void youmu_common_bombbg(Player *plr) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	float t = player_get_bomb_progress(&global.plr);
	float fade = 1;

	if(t < 1./12)
		fade = t*12;

	if(t > 1./2)
		fade = 1-(t-1./2)*2;

	if(fade < 0)
		fade = 0;

	r_state_push();
	r_color4(fade, fade, fade, fade);
	r_shader("youmu_bomb_bg");
	r_uniform_sampler("petals", "youmu_bombbg1");
	r_uniform_float("time", t);

	draw_framebuffer_tex(bomb_buffer, VIEWPORT_W, VIEWPORT_H);
	r_state_pop();

	capture_frame(bomb_buffer, r_framebuffer_current());
}

void youmu_common_bomb_buffer_init(void) {
	FBAttachmentConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	cfg.tex_params.type = TEX_TYPE_RGB;
	cfg.tex_params.filter.min = TEX_FILTER_LINEAR;
	cfg.tex_params.filter.mag = TEX_FILTER_LINEAR;
	cfg.tex_params.wrap.s = TEX_WRAP_MIRROR;
	cfg.tex_params.wrap.t = TEX_WRAP_MIRROR;
	bomb_buffer = stage_add_background_framebuffer("Youmu bomb FB", 0.25, 1, 1, &cfg);
}
