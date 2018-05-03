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

static void reimu_spirit_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"yinyang",
		"proj/ofuda",
		"proj/needle",
		"part/ofuda_glow",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"sprite_yinyang",
	NULL);

	//preload_resources(RES_SFX, flags | RESF_OPTIONAL,
	//NULL);
}

static int reimu_spirit_needle(Projectile *p, int t) {
	int r = linear(p, t);

	if(t < 0) {
		return r;
	}

	PARTICLE(
		.sprite_ptr = p->sprite,
		.color = multiply_colors(p->color, rgba(0.75, 0.5, 1, 0.5)),
		.timeout = 12,
		.pos = p->pos,
		.args = { p->args[0] * 0.8, 0, 0+3*I },
		.rule = linear,
		.draw_rule = ScaleFade,
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
		.blend = BLEND_ADD,
	);

	return r;
}

static int reimu_spirit_homing_trail(Projectile *p, int t) {
	int r = linear(p, t);

	if(t < 0) {
		return r;
	}

	// p->angle = creal(p->args[1]) + cimag(p->args[1]) * t;

	return r;
}

static int reimu_spirit_homing(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_NONE;
	}

	p->args[3] = plrutil_homing_target(p->pos, p->args[3]);
	double v = cabs(p->args[0]);
	complex aimdir = cexp(I*carg(p->args[3] - p->pos));

	double aim = (0.5 * pow(1 - t / p->timeout, 4));

	p->args[0] += v * aim * aimdir;
	p->args[0] = v * cexp(I*carg(p->args[0]));
	p->angle = carg(p->args[0]);
	p->pos += p->args[0];

	PARTICLE(
		.sprite = "ofuda_glow",
		// .color = rgba(0.5 + 0.5 + psin(global.frames * 0.75), psin(t*0.5), 1, 0.5),
		.color = hsla(t * 0.1, 0.5, 0.75, 0.35),
		.timeout = 12,
		.pos = p->pos,
		.args = { p->args[0] * (0.2 + 0.6 * frand()), t*0.1+p->angle + I * 0.1, 1+2*I },
		.angle = p->angle,
		.rule = reimu_spirit_homing_trail,
		.draw_rule = ScaleFade,
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
		.blend = BLEND_ADD,
	);

	return ACTION_NONE;
}

static void reimu_spirit_bomb(Player *p) {
}

static void reimu_spirit_shot(Player *p) {
	reimu_common_shot(p, 175);
}

static void reimu_spirit_slave_shot(Enemy *e, int t) {
	int st = global.frames + 2;

	if(st % 3) {
		return;
	}

	if(global.plr.inputflags & INFLAG_FOCUS) {
		PROJECTILE(
			.proto = pp_needle,
			.pos = e->pos - 25.0*I,
			.color = rgba(1, 1, 1, 0.5),
			.rule = reimu_spirit_needle,
			.args = { -20.0*I },
			.type = PlrProj+30,
			.shader = "sprite_default",
		);
	} else if(!(st % 6)) {
		PROJECTILE(
			.proto = pp_ofuda,
			.pos = e->pos,
			.color = rgba(1, 1, 1, 0.5),
			.rule = reimu_spirit_homing,
			.args = { -10.0*I, 0, 0, creal(e->pos) },
			.type = PlrProj+30,
			.timeout = 60,
			.shader = "sprite_default",
		);
	}
}

static int reimu_spirit_slave(Enemy *e, int t) {
	TIMER(&t);

	AT(EVENT_BIRTH) {
		e->pos = global.plr.pos;
		return 1;
	}

	GO_TO(e, global.plr.pos + e->args[!!(global.plr.inputflags & INFLAG_FOCUS)], 0.005 * cabs(e->args[0]));

	if(player_should_shoot(&global.plr, true)) {
		reimu_spirit_slave_shot(e, t);
	}

	return 1;
}

static void reimu_spirit_respawn_slaves(Player *plr, short npow) {
	Enemy *e = plr->slaves, *tmp;
	double dmg = 56;

	while(e != 0) {
		tmp = e;
		e = e->next;
		if(tmp->hp == ENEMY_IMMUNE)
			delete_enemy(&plr->slaves, tmp);
	}

	if(npow / 100 == 1) {
		create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, 50.0*I, -50.0*I, -0.1*I, dmg);
	}

	if(npow >= 200) {
		create_enemy_p(&plr->slaves, 30.0*I+15, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, +40, +20-30*I,  1-0.1*I, dmg);
		create_enemy_p(&plr->slaves, 30.0*I-15, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, -40, -20-30*I, -1-0.1*I, dmg);
	}

	if(npow / 100 == 3) {
		create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, 50.0*I, -50.0*I, -0.1*I, dmg);
	}

	if(npow >= 400) {
		create_enemy_p(&plr->slaves,  30, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, +80, +40-20*I,  2-0.1*I, dmg);
		create_enemy_p(&plr->slaves, -30, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, -80, -40-20*I, -2-0.1*I, dmg);
	}

	for(e = plr->slaves; e; e = e->next) {
		e->ent.draw_layer = LAYER_PLAYER_SLAVE;
	}
}

static void reimu_spirit_init(Player *plr) {
	reimu_spirit_respawn_slaves(plr, plr->power);
}

static void reimu_spirit_power(Player *plr, short npow) {
	if(plr->power / 100 != npow / 100) {
		reimu_spirit_respawn_slaves(plr, npow);
	}
}

PlayerMode plrmode_reimu_a = {
	.name = "Spirit Sign",
	.character = &character_reimu,
	.dialog = &dialog_reimu,
	.shot_mode = PLR_SHOT_REIMU_SPIRIT,
	.procs = {
		.property = reimu_common_property,
		.init = reimu_spirit_init,
		.bomb = reimu_spirit_bomb,
		.shot = reimu_spirit_shot,
		.power = reimu_spirit_power,
		.preload = reimu_spirit_preload,
	},
};
