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
#include "stagedraw.h"

static Framebuffer *bomb_buffer;

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
			// NOTE: For equivalents in Touhou units, divide these by 1.25.
			return (plr->inputflags & INFLAG_FOCUS) ? 2.5 : 5.625;

		case PLR_PROP_POC:
			return VIEWPORT_H / 3.5;

		case PLR_PROP_DEATHBOMB_WINDOW:
			return 12;
	}

	UNREACHABLE;
}

static int reimu_ofuda_trail(Projectile *p, int t) {
	int r = linear(p, t);

	if(t < 0) {
		return r;
	}

	p->color.g *= 0.95;

	return r;
}

int reimu_common_ofuda(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	p->pos += p->args[0];

	PARTICLE(
		// .sprite_ptr = p->sprite,
		.sprite = "hghost",
		.color = &p->color,
		.timeout = 12,
		.pos = p->pos + p->args[0] * 0.3,
		.args = { p->args[0] * 0.5, 0, 1+2*I },
		.rule = reimu_ofuda_trail,
		.draw_rule = ScaleFade,
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
	);

	return ACTION_NONE;
}

void reimu_common_shot(Player *plr, int dmg) {
	play_loop("generic_shot");

	if(!(global.frames % 3)) {
		int i = 1 - 2 * (bool)(global.frames % 6);
		PROJECTILE(
			.proto = pp_ofuda,
			.pos = plr->pos + 10 * i - 15.0*I,
			.color = RGBA_MUL_ALPHA(1, 1, 1, 0.5),
			.rule = reimu_common_ofuda,
			.args = { -20.0*I },
			.type = PlrProj,
			.damage = dmg,
			.shader = "sprite_default",
		);
	}
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
	r_texture(1, "runes");
	r_uniform_int("runes", 1);
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
	bomb_buffer = stage_add_foreground_framebuffer(1, 1, &cfg);
}
