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
#include "reimu.h"
#include "stagedraw.h"

static Framebuffer *bomb_buffer;

PlayerCharacter character_reimu = {
	.id = PLR_CHAR_REIMU,
	.lower_name = "reimu",
	.proper_name = "Reimu",
	.full_name = "Hakurei Reimu",
	.title = "Shrine Maiden of Fantasy",
	.dialog_base_sprite_name = "dialog/reimu",
	.player_sprite_name = "player/reimu",
	.menu_texture_name = "reimubg",
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
			// NOTE: For equivalents in Touhou units, divide these by 1.25.
			return (plr->inputflags & INFLAG_FOCUS) ? 2.5 : 5.625;

		case PLR_PROP_POC:
			return VIEWPORT_H / 3.5;

		case PLR_PROP_DEATHBOMB_WINDOW:
			return 12;
	}

	UNREACHABLE;
}

Projectile *reimu_common_ofuda_swawn_trail(Projectile *p, ProjectileList *dest) {
	return PARTICLE(
		// .sprite_ptr = p->sprite,
		.sprite_ptr = get_sprite("proj/hghost"),
		.color = &p->color,
		.timeout = 12,
		.pos = p->pos + p->move.velocity * 0.3,
		.move = move_linear(p->move.velocity * 0.5),
		.draw_rule = pdraw_timeout_scalefade(1, 2, 1, 0),
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
		.dest = dest,
	);
}

void reimu_common_draw_yinyang(Enemy *e, int t, const Color *c) {
	r_draw_sprite(&(SpriteParams) {
		.sprite = "yinyang",
		.shader = "sprite_yinyang",
		.pos = { creal(e->pos), cimag(e->pos) },
		.rotation.angle = global.frames * -6 * DEG2RAD,
		// .color = rgb(0.95, 0.75, 1.0),
		.color = c,
	});
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
