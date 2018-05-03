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

static int reimu_ofuda_trail(Projectile *p, int t) {
	int r = linear(p, t);

	if(t < 0) {
		return r;
	}

	float g = color_component(p->color, CLR_G) * 0.95;
	p->color = derive_color(p->color, CLRMASK_G, rgba(0, g, 0, 0));

	return r;
}

static int reimu_ofuda(Projectile *p, int t) {
	int r = linear(p, t);

	if(t < 0) {
		return r;
	}

	PARTICLE(
		// .sprite_ptr = p->sprite,
		.sprite = "hghost",
		.color = p->color,
		.timeout = 12,
		.pos = p->pos + p->args[0] * 0.3,
		.args = { p->args[0] * 0.5, 0, 1+2*I },
		.rule = reimu_ofuda_trail,
		.draw_rule = ScaleFade,
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
	);

	return r;
}

void reimu_common_shot(Player *plr, int dmg) {
	if(!(global.frames % 4)) {
		play_sound("generic_shot");
	}

	if(!(global.frames % 3)) {
		int i = 1 - 2 * (bool)(global.frames % 6);
		PROJECTILE(
			.proto = pp_ofuda,
			.pos = plr->pos + 10 * i - 15.0*I,
			.color = rgba(1, 1, 1, 0.5),
			.rule = reimu_ofuda,
			.args = { -20.0*I },
			.type = PlrProj+dmg,
			.shader = "sprite_default",
		);
	}
}

void reimu_common_draw_yinyang(Enemy *e, int t, Color c) {
	r_draw_sprite(&(SpriteParams) {
		.sprite = "yinyang",
		.shader = "sprite_yinyang",
		.pos = { creal(e->pos), cimag(e->pos) },
		.rotation.angle = global.frames * 6 * DEG2RAD,
		// .color = rgb(0.95, 0.75, 1.0),
		.color = c,
	});
}
