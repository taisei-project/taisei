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

// FIXME: We probably need a better way to store shot-specific state.
//        See also MarisaA.
static struct {
	uint prev_inputflags;
	bool respawn_slaves;
} reimu_spirit_state;

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

	Color c = p->color;
	color_mul(&c, RGBA_MUL_ALPHA(0.75, 0.5, 1, 0.5));
	c.a = 0;

	PARTICLE(
		.sprite_ptr = p->sprite,
		.color = &c,
		.timeout = 12,
		.pos = p->pos,
		.args = { p->args[0] * 0.8, 0, 0+3*I },
		.rule = linear,
		.draw_rule = ScaleFade,
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
	);

	return r;
}

#define REIMU_SPIRIT_HOMING_SCALE 0.75

static void reimu_spirit_homing_draw(Projectile *p, int t) {
	r_mat_push();
	r_mat_translate(creal(p->pos), cimag(p->pos), 0);
	r_mat_rotate(p->angle + M_PI/2, 0, 0, 1);
	r_mat_scale(REIMU_SPIRIT_HOMING_SCALE, REIMU_SPIRIT_HOMING_SCALE, 1);
	ProjDrawCore(p, &p->color);
	r_mat_pop();
}

static Projectile* reimu_spirit_spawn_ofuda_particle(Projectile *p, int t, double vfactor) {
	Color *c = HSLA_MUL_ALPHA(t * 0.1, 0.6, 0.7, 0.45);
	c->a = 0;

	return PARTICLE(
		.sprite = "ofuda_glow",
		// .color = rgba(0.5 + 0.5 + psin(global.frames * 0.75), psin(t*0.5), 1, 0.5),
		.color = c,
		.timeout = 12,
		.pos = p->pos,
		.args = { p->args[0] * (0.6 + 0.4 * frand()) * vfactor, 0, (1+2*I) * REIMU_SPIRIT_HOMING_SCALE },
		.angle = p->angle,
		.rule = linear,
		.draw_rule = ScaleFade,
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
	);
}

static int reimu_spirit_homing_impact(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	Projectile *trail = reimu_spirit_spawn_ofuda_particle(p, global.frames, 1);
	trail->rule = NULL;
	trail->timeout = 6;
	trail->angle = p->angle;
	trail->ent.draw_layer = LAYER_PLAYER_FOCUS - 1; // TODO: add a layer for "super high" particles?
	trail->args[2] *= 1.5 * (1 - t/p->timeout);
	p->angle += 0.2;

	return ACTION_NONE;
}

static Projectile* reimu_spirit_spawn_homing_impact(Projectile *p, int t) {
	return PARTICLE(
		.proto = p->proto,
		.color = &p->color,
		.timeout = 32,
		.pos = p->pos,
		.args = { 0, 0, (1+2*I) * REIMU_SPIRIT_HOMING_SCALE },
		.angle = p->angle,
		.rule = reimu_spirit_homing_impact,
		.draw_rule = ScaleFade,
		.layer = LAYER_PARTICLE_HIGH,
		.flags = PFLAG_NOREFLECT,
	);
}

static inline double reimu_spirit_homing_aimfactor(double t, double maxt) {
	t = clamp(t, 0, maxt);
	double q = pow(1 - t / maxt, 3);
	return 4 * q * (1 - q);
}

static int reimu_spirit_homing(Projectile *p, int t) {
	if(t < 0) {
		if(t == EVENT_DEATH && projectile_in_viewport(p)) {
			reimu_spirit_spawn_homing_impact(p, t);
		}

		return ACTION_ACK;
	}

	p->args[3] = plrutil_homing_target(p->pos, p->args[3]);
	double v = cabs(p->args[0]);

	complex aimdir = cexp(I*carg(p->args[3] - p->pos));
	double aim = reimu_spirit_homing_aimfactor(t, p->args[1]);

	p->args[0] += v * 0.25 * aim * aimdir;
	p->args[0] = v * cexp(I*carg(p->args[0]));
	p->angle = carg(p->args[0]);

	double s = 1;// max(pow(2*t/creal(p->args[1]), 2), 0.1); //(0.25 + 0.75 * (1 - aim));
	p->pos += p->args[0] * s;
	reimu_spirit_spawn_ofuda_particle(p, t, 0.5);

	return ACTION_NONE;
}

static const Color *reimu_spirit_orb_color(Color *c, int i) {
	*c = *RGBA((0.2 + (i==0))/1.2, (0.2 + (i==1))/1.2, (0.2 + 1.5*(i==2))/1.2, 0.0);
	return c;
}

static void reimu_spirit_bomb_orb_visual(Projectile *p, int t) {
	complex pos = p->pos;

	for(int i = 0; i < 3; i++) {
		complex offset = (10 + pow(t, 0.5)) * cexp(I * (2 * M_PI / 3*i + sqrt(1 + t * t / 300.0)));

		Color c;
		r_draw_sprite(&(SpriteParams) {
			.sprite = "proj/glowball",
			.shader = "sprite_bullet",
			.pos = { creal(pos+offset), cimag(pos+offset) },
			.color = reimu_spirit_orb_color(&c, i),

//			.shader_params = &(ShaderCustomParams) {.vector = {0.3,0,0,0}},
		});
	}
}

static int reimu_spirit_bomb_orb(Projectile *p, int t) {
	int index = creal(p->args[1]) + 0.5;
	if(t == EVENT_BIRTH) {
		if(index == 0)
			global.shake_view = 4;
		p->args[3] = global.plr.pos;
	}

	if(t == EVENT_DEATH) {
		global.shake_view = 20;
		global.shake_view_fade = 0.6;

		double damage = 2000;
		double range = 300;

		ent_area_damage(p->pos, range, &(DamageInfo){damage, DMG_PLAYER_BOMB});
		int count = 21;
		double offset = frand();
		for(int i = 0; i < count; i++) {
			Color c;
			PARTICLE(
				.sprite_ptr = get_sprite("proj/glowball"),
				.shader = "sprite_bullet",
				.color = reimu_spirit_orb_color(&c, i%3),
				.timeout = 60,
				.pos = p->pos,
				.args = { cexp(I * 2 * M_PI / count * (i + offset)) * 15 },
				.angle = 2*M_PI*frand(),
				.rule = linear,
				.draw_rule = Fade,
				.flags = PFLAG_NOREFLECT,
			);
		}
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	if(!player_is_bomb_active(&global.plr) > 0) {
		return ACTION_DESTROY;
	}

	double circletime = 100+20*index;

	complex target_circle = global.plr.pos + 10 * sqrt(t) * p->args[0]*(1 + 0.1 * sin(0.2*t));
	p->args[0] *= cexp(I*0.1);
                                                                            
	double circlestrength = 1 / (1 + exp(t-circletime));
	
	p->args[3] = plrutil_homing_target(p->pos, p->args[3]);
	complex target_homing = p->args[3];


	complex homing = target_homing - p->pos;
	p->pos += 0.3 * (circlestrength * (target_circle - p->pos) + 0.2 * (1-circlestrength) * (homing + 2*homing/(cabs(homing)+0.01)));

	for(int i = 0; i < 3 && circlestrength < 0.3; i++) {
		Color c;
		PARTICLE(
			.sprite_ptr = get_sprite("part/stain"),
			.color = reimu_spirit_orb_color(&c, i),
			.pos = p->pos + 10 * cexp(I*2*M_PI/3*(i+t*0.1)),
			.angle = 2*M_PI*frand(),
			.timeout = 60,
			.draw_rule = ScaleFade,
			.args = { 0, 0.8, 0.8, 0.8},
		);
	}
	
	return ACTION_NONE;
}

static void reimu_spirit_bomb(Player *p) {
	int count = 6;
	for(int i = 0; i < count; i++) {
		PROJECTILE(
			.pos = p->pos,
			.draw_rule = reimu_spirit_bomb_orb_visual,
			.rule = reimu_spirit_bomb_orb,
			.args = { cexp(I*2*M_PI/count*i), i, 0, 0},
			.type = PlrProj,
			.damage = 0,
			.size = 10 + 10*I,
		);
	}
}

static void reimu_spirit_bomb_bg(Player *p) {
	double speed;
	double t = player_get_bomb_progress(p, &speed);
	float alpha = 0;
	if(t > 0)
		alpha = min(1,10*t);
	if(t > 0.7)
		alpha *= 1-pow((t-0.7)/0.3,4);
	
	reimu_common_bomb_bg(p, alpha);
}

static void reimu_spirit_shot(Player *p) {
	reimu_common_shot(p, 100 - 17 * (p->power / 100));
}

static void reimu_spirit_slave_shot(Enemy *e, int t) {
	int st = t;

	if(st % 3) {
		return;
	}

	if(global.plr.inputflags & INFLAG_FOCUS) {
		PROJECTILE(
			.proto = pp_needle,
			.pos = e->pos - 25.0*I,
			.color = RGBA_MUL_ALPHA(1, 1, 1, 0.5),
			.rule = reimu_spirit_needle,
			.args = { -20.0*I },
			.type = PlrProj,
			.damage_type = DMG_PLAYER_SHOT,
			.damage = cimag(e->args[2]),
			.shader = "sprite_default",
		);
	} else if(!(st % 12)) {
		complex v = -10 * I * cexp(I*cimag(e->args[0]));

		PROJECTILE(
			.proto = pp_ofuda,
			.pos = e->pos,
			.color = RGBA_MUL_ALPHA(1, 0.9, 0.95, 0.7),
			.rule = reimu_spirit_homing,
			.draw_rule = reimu_spirit_homing_draw,
			.args = { v , 60, 0, e->pos + v * VIEWPORT_H * VIEWPORT_W /*creal(e->pos)*/ },
			.type = PlrProj,
			.damage_type = DMG_PLAYER_SHOT,
			.damage = creal(e->args[2]),
			// .timeout = 60,
			.shader = "sprite_default",
			.flags = PFLAG_NOCOLLISIONEFFECT,
		);
	}
}

static int reimu_spirit_slave(Enemy *e, int t) {
	TIMER(&t);

	AT(EVENT_BIRTH) {
		e->pos = global.plr.pos;
		return ACTION_NONE;
	}

	if(t < 0) {
		return ACTION_NONE;
	}

	if(player_should_shoot(&global.plr, true)) {
		reimu_spirit_slave_shot(e, t);
	}

	if(creal(e->args[3]) != 0) {
		int death_begin_time = creal(e->args[3]);
		int death_duration = cimag(e->args[3]);
		double death_progress = (t - death_begin_time) / (double)death_duration;

		e->pos = global.plr.pos * death_progress + e->pos0 * (1 - death_progress);

		if(death_progress >= 1) {
			return ACTION_DESTROY;
		}

		return ACTION_NONE;
	}

	double speed = 0.005 * min(1, t / 12.0);

	if(global.plr.inputflags & INFLAG_FOCUS) {
		GO_TO(e, global.plr.pos + cimag(e->args[1]) * cexp(I*(creal(e->args[0]) + t * creal(e->args[1]))), speed * cabs(e->args[1]));
	} else {
		GO_TO(e, global.plr.pos + e->pos0, speed * cabs(e->pos0));
	}

	return ACTION_NONE;
}

static int reimu_spirit_yinyang_flare(Projectile *p, int t) {
	double a = p->angle;
	int r = linear(p, t);
	p->angle = a;
	return r;
}

static void reimu_spirit_yinyang_focused_visual(Enemy *e, int t, bool render) {
	if(!render && player_should_shoot(&global.plr, true)) {
		PARTICLE(
			.sprite = "stain",
			.color = RGBA(1, 0.0 + 0.5 * frand(), 0, 0),
			.timeout = 12 + 2 * nfrand(),
			.pos = e->pos,
			.args = { -5*I * (1 + frand()), 0, 0.5 + 0*I },
			.angle = 2*M_PI*frand(),
			.rule = reimu_spirit_yinyang_flare,
			.draw_rule = ScaleFade,
			.layer = LAYER_PARTICLE_HIGH,
			.flags = PFLAG_NOREFLECT,
		);
	}

	if(render) {
		reimu_common_draw_yinyang(e, t, RGB(1.0, 0.8, 0.8));
	}
}

static void reimu_spirit_yinyang_unfocused_visual(Enemy *e, int t, bool render) {
	if(!render && player_should_shoot(&global.plr, true)) {
		PARTICLE(
			.sprite = "stain",
			.color = RGBA(1, 0.25, 0.0 + 0.5 * frand(), 0),
			.timeout = 12 + 2 * nfrand(),
			.pos = e->pos,
			.args = { -5*I * (1 + frand()), 0, 0.5 + 0*I },
			.angle = 2*M_PI*frand(),
			.rule = reimu_spirit_yinyang_flare,
			.draw_rule = ScaleFade,
			.layer = LAYER_PARTICLE_HIGH,
			.flags = PFLAG_NOREFLECT,
		);
	}

	if(render) {
		reimu_common_draw_yinyang(e, t, RGB(0.95, 0.75, 1.0));
	}
}

static inline Enemy* reimu_spirit_spawn_slave(Player *plr, complex pos, complex a0, complex a1, complex a2, complex a3, EnemyVisualRule visual) {
	Enemy *e = create_enemy_p(&plr->slaves, pos, ENEMY_IMMUNE, visual, reimu_spirit_slave, a0, a1, a2, a3);
	e->ent.draw_layer = LAYER_PLAYER_SLAVE;
	return e;
}

static void reimu_spirit_kill_slaves(EnemyList *slaves) {
	for(Enemy *e = slaves->first, *next; e; e = next) {
		next = e->next;

		if(e->hp == ENEMY_IMMUNE && creal(e->args[3]) == 0) {
			// delete_enemy(slaves, e);
			// e->args[3] = 1;
			e->args[3] = global.frames - e->birthtime + 3 * I;
			e->pos0 = e->pos;
		}
	}
}

static void reimu_spirit_respawn_slaves(Player *plr, short npow, complex param) {
	double dmg_homing = 100; // every 12 frames
	double dmg_needle = 80;  // every 3 frames
	complex dmg = dmg_homing + I * dmg_needle;
	EnemyVisualRule visual;

	if(plr->inputflags & INFLAG_FOCUS) {
		visual = reimu_spirit_yinyang_focused_visual;
	} else {
		visual = reimu_spirit_yinyang_unfocused_visual;
	}

	reimu_spirit_kill_slaves(&plr->slaves);

	switch(npow / 100) {
		case 0:
			break;
		case 1:
			reimu_spirit_spawn_slave(plr, 50.0*I, 0                     , +0.10 + 60*I, dmg, 0, visual);
			break;
		case 2:
			reimu_spirit_spawn_slave(plr, +40,    0           +M_PI/24*I, +0.10 + 60*I, dmg, 0, visual);
			reimu_spirit_spawn_slave(plr, -40,    M_PI        -M_PI/24*I, +0.10 + 60*I, dmg, 0, visual);
			break;
		case 3:
			reimu_spirit_spawn_slave(plr, 50.0*I, 0*2*M_PI/3            , +0.10 + 60*I, dmg, 0, visual);
			reimu_spirit_spawn_slave(plr, +40,    1*2*M_PI/3  +M_PI/24*I, +0.10 + 60*I, dmg, 0, visual);
			reimu_spirit_spawn_slave(plr, -40,    2*2*M_PI/3  -M_PI/24*I, +0.10 + 60*I, dmg, 0, visual);
			break;
		case 4:
			reimu_spirit_spawn_slave(plr, +40,    0           +M_PI/32*I, +0.10 + 60*I, dmg, 0, visual);
			reimu_spirit_spawn_slave(plr, -40,    M_PI        -M_PI/32*I, +0.10 + 60*I, dmg, 0, visual);
			reimu_spirit_spawn_slave(plr, +80,    0           +M_PI/16*I, -0.05 + 80*I, dmg, 0, visual);
			reimu_spirit_spawn_slave(plr, -80,    M_PI        -M_PI/16*I, -0.05 + 80*I, dmg, 0, visual);
			break;
		default:
			UNREACHABLE;
	}
}

static void reimu_spirit_init(Player *plr) {
	memset(&reimu_spirit_state, 0, sizeof(reimu_spirit_state));
	reimu_spirit_state.prev_inputflags = plr->inputflags;
	reimu_spirit_respawn_slaves(plr, plr->power, 0);
	reimu_common_bomb_buffer_init();
}

static void reimu_spirit_think(Player *plr) {
	if((bool)(reimu_spirit_state.prev_inputflags & INFLAG_FOCUS) ^ (bool)(plr->inputflags & INFLAG_FOCUS)) {
		reimu_spirit_state.respawn_slaves = true;
	}

	if(reimu_spirit_state.respawn_slaves) {
		if(plr->slaves.first) {
			reimu_spirit_kill_slaves(&plr->slaves);
		} else {
			reimu_spirit_respawn_slaves(plr, plr->power, 0);
			reimu_spirit_state.respawn_slaves = false;
		}
	}

	reimu_spirit_state.prev_inputflags = plr->inputflags;
}

static void reimu_spirit_power(Player *plr, short npow) {
	if(plr->power / 100 != npow / 100) {
		reimu_spirit_respawn_slaves(plr, npow, 0);
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
		.think = reimu_spirit_think,
		.bomb = reimu_spirit_bomb,
		.bombbg = reimu_spirit_bomb_bg,
		.shot = reimu_spirit_shot,
		.power = reimu_spirit_power,
		.preload = reimu_spirit_preload,
	},
};
