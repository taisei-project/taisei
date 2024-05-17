/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "reimu.h"

#include "global.h"
#include "plrmodes.h"
#include "stagedraw.h"
#include "util/graphics.h"

static Framebuffer *bomb_buffer;

PlayerCharacter character_reimu = {
	.id = PLR_CHAR_REIMU,
	.lower_name = "reimu",
	.proper_name = "Reimu",
	.full_name = "Hakurei Reimu",
	.title = "Shrine Maiden of Fantasy",
	.menu_texture_name = "reimubg",
	.ending = {
		.good = { CUTSCENE_ID_REIMU_GOOD_END, ENDING_GOOD_REIMU },
		.bad = { CUTSCENE_ID_REIMU_BAD_END, ENDING_BAD_REIMU },
	},
};

double reimu_common_property(Player *plr, PlrProperty prop) {
	switch(prop) {
		case PLR_PROP_BOMB_TIME:
			return 300;

		case PLR_PROP_COLLECT_RADIUS:
			return (plr->inputflags & INFLAG_FOCUS) ? 60 : 30;

		case PLR_PROP_SPEED:
			// NOTE: For equivalents in Touhou units, divide these by 1.25.
			return (plr->inputflags & INFLAG_FOCUS) ? 2.5 : 5.625;

		case PLR_PROP_POC:
			return VIEWPORT_H / 3.5;

		case PLR_PROP_DEATHBOMB_WINDOW:
			return 12;
	}

	UNREACHABLE;
}

TASK(reimu_ofuda_trail, { BoxedProjectile trail; }) {
	Projectile *trail = TASK_BIND(ARGS.trail);

	for(;;) {
		trail->color.g *= 0.95;
		YIELD;
	}
}

Projectile *reimu_common_ofuda_swawn_trail(Projectile *p) {
	Projectile *trail = PARTICLE(
		// .sprite_ptr = p->sprite,
		.sprite_ptr = res_sprite("proj/hghost"),
		.color = &p->color,
		.timeout = 12,
		.pos = p->pos + p->move.velocity * 0.3,
		.move = move_linear(p->move.velocity * 0.5),
		.draw_rule = pdraw_timeout_scalefade(1, 2, 1, 0),
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
	);

	// TODO maybe replace this with something better looking and less task-hungry
	INVOKE_TASK(reimu_ofuda_trail, ENT_BOX(trail));
	return trail;
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

void reimu_common_bomb_bg(Player *p, float alpha) {
	if(alpha <= 0)
		return;

	r_state_push();
	r_color(HSLA_MUL_ALPHA(global.frames / 30.0, 0.2, 0.9, alpha));

	r_shader("reimu_bomb_bg");
	r_uniform_sampler("runes", "runes");
	r_uniform_float("zoom", VIEWPORT_H / sqrt(VIEWPORT_W*VIEWPORT_W + VIEWPORT_H*VIEWPORT_H));
	r_uniform_vec2("aspect", VIEWPORT_W / (float)VIEWPORT_H, 1);
	r_uniform_float("time", 9000 + 3 * global.frames / 60.0);
	draw_framebuffer_tex(bomb_buffer, VIEWPORT_W, VIEWPORT_H);

	r_state_pop();
	capture_frame(bomb_buffer, r_framebuffer_current());
}

void reimu_common_bomb_buffer_init(void) {
	FBAttachmentConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	cfg.tex_params.type = TEX_TYPE_RGB;
	cfg.tex_params.filter.min = TEX_FILTER_LINEAR;
	cfg.tex_params.filter.mag = TEX_FILTER_LINEAR;
	cfg.tex_params.wrap.s = TEX_WRAP_MIRROR;
	cfg.tex_params.wrap.t = TEX_WRAP_MIRROR;
	bomb_buffer = stage_add_background_framebuffer("Reimu bomb FB", 0.25, 1, 1, &cfg);
}
