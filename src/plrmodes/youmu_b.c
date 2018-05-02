/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <float.h>

#include "global.h"
#include "plrmodes.h"
#include "youmu.h"

static complex youmu_homing_target(complex org, complex fallback) {
	return plrutil_homing_target(org, fallback);
}

static void youmu_homing_draw_common(Projectile *p, float clrfactor, float scale, float alpha) {
	Color c = p->color;
	color_mul(&c, RGBA(0.7f + 0.3f * clrfactor, 0.9f + 0.1f * clrfactor, 1, 1));

	if(alpha <= 0) {
		return;
	}

	bool special_snowflake_shader_bullshit = p->shader_params.vector[1] != 0;

	if(special_snowflake_shader_bullshit) {
		// FIXME: maybe move this to logic someh-- nah. Don't even bother with this crap.
		float old = p->shader_params.vector[1];
		p->shader_params.vector[1] = alpha;
		youmu_common_draw_proj(p, &c, scale);
		p->shader_params.vector[1] = old;
	} else {
		color_mul_scalar(&c, alpha);
		youmu_common_draw_proj(p, &c, scale);
	}
}

static void youmu_homing_draw_proj(Projectile *p, int t) {
	float a = clamp(1.0f - (float)t / p->args[2], 0, 1);
	youmu_homing_draw_common(p, a, 1, 0.5f);
}

static void youmu_homing_draw_trail(Projectile *p, int t) {
	float a = clamp(1.0f - (float)t / p->timeout, 0, 1);
	youmu_homing_draw_common(p, a, 5 * (1 - a), 0.15f * a);
}

static void youmu_trap_draw_trail(Projectile *p, int t) {
	float a = clamp(1.0f - (float)t / p->timeout, 0, 1);
	youmu_homing_draw_common(p, a, 2 - a, 0.15f * a);
}

static void youmu_trap_draw_child_proj(Projectile *p, int t) {
	float to = p->args[2];
	float a = clamp(1.0 - 3 * ((t - (to - to/3)) / to), 0, 1);
	a = 1 - pow(1 - a, 2);
	youmu_homing_draw_common(p, a, 1 + 2 * pow(1 - a, 2), a);
}

static float youmu_trap_charge(int t) {
	return pow(clamp(t / 60.0, 0, 1), 1.5);
}

static Projectile* youmu_homing_trail(Projectile *p, complex v, int to) {
	return PARTICLE(
		.sprite_ptr = p->sprite,
		.pos = p->pos,
		.color = &p->color,
		.angle = p->angle,
		.rule = linear,
		.timeout = to,
		.draw_rule = youmu_homing_draw_trail,
		.args = { v },
		.flags = PFLAG_NOREFLECT,
		.shader_ptr = p->shader,
		.shader_params = &p->shader_params,
		.layer = LAYER_PARTICLE_LOW,
	);
}

static int youmu_homing(Projectile *p, int t) { // a[0]: velocity, a[1]: aim (r: linear, i: accelerated), a[2]: timeout, a[3]: initial target
	if(t == EVENT_DEATH || t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	if(t > creal(p->args[2])) {
		return ACTION_DESTROY;
	}

	p->args[3] = youmu_homing_target(p->pos, p->args[3]);

	double v = cabs(p->args[0]);
	complex aimdir = cexp(I*carg(p->args[3] - p->pos));

	p->args[0] += creal(p->args[1]) * aimdir;
	p->args[0] = v * cexp(I*carg(p->args[0])) + cimag(p->args[1]) * aimdir;

	p->angle = carg(p->args[0]);
	p->pos += p->args[0];

	Projectile *trail = youmu_homing_trail(p, 0.5 * p->args[0], 12);
	trail->args[2] = p->args[2];

	p->shader_params.vector[0] = cimag(p->args[2]);
	trail->shader_params.vector[0] = cimag(p->args[2]);

	return 1;
}

static Projectile* youmu_trap_trail(Projectile *p, complex v, int t, bool additive) {
	Projectile *trail = youmu_homing_trail(p, v, t);
	trail->draw_rule = youmu_trap_draw_trail;
	// trail->args[3] = global.frames - p->birthtime;
	trail->shader_params.vector[0] = p->shader_params.vector[0];

	if(additive) {
		trail->color.a = 0;
	}

	return trail;
}

static int youmu_trap(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		PARTICLE(
			.proto = pp_blast,
			.pos = p->pos,
			.timeout = 15,
			.draw_rule = Blast,
			.flags = PFLAG_REQUIREDPARTICLE,
			.layer = LAYER_PARTICLE_LOW,
		);
		return ACTION_ACK;
	}

	// FIXME: replace this with timeout?
	double expiretime = creal(p->args[1]);

	if(t > expiretime) {
		return ACTION_DESTROY;
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	float charge = youmu_trap_charge(t);
	p->shader_params.vector[0] = charge;

	if(!(global.plr.inputflags & INFLAG_FOCUS)) {
		PARTICLE(
			.proto = pp_blast,
			.pos = p->pos,
			.timeout = 20,
			.draw_rule = Blast,
			.flags = PFLAG_REQUIREDPARTICLE,
			.layer = LAYER_PARTICLE_LOW,
		);

		PARTICLE(
			.proto = pp_blast,
			.pos = p->pos,
			.timeout = 23,
			.draw_rule = Blast,
			.flags = PFLAG_REQUIREDPARTICLE,
			.layer = LAYER_PARTICLE_LOW,
		);

		int cnt = rint(creal(p->args[2]) * (0.25 + 0.75 * charge));
		int dmg = cimag(p->args[2]);
		complex aim = p->args[3];

		for(int i = 0; i < cnt; ++i) {
			int dur = 55 + 20 * nfrand();
			float a = (i / (float)cnt) * M_PI * 2;
			complex dir = cexp(I*(a));

			PROJECTILE("youmu", p->pos, RGBA(1, 1, 1, 0.85), youmu_homing,
				.args = { 5 * (1 + charge) * dir, (1 + charge) * aim, dur + charge*I, creal(p->pos) - VIEWPORT_H*I },
				.type = PlrProj,
				.damage = dmg,
				.draw_rule = youmu_trap_draw_child_proj,
				.shader = "sprite_youmu_charged_shot",
				.shader_params = &(ShaderCustomParams){{ 0, 1 }},
			);
		}

		// TODO: dedicated sound for this?
		play_sound("enemydeath");
		play_sound("hit");

		return ACTION_DESTROY;
	}

	p->angle = global.frames + t;
	p->pos += p->args[0] * (0.01 + 0.99 * max(0, (10 - t) / 10.0));

	youmu_trap_trail(p, cexp(I*p->angle), 30 * (1 + charge), true);
	youmu_trap_trail(p, cexp(I*-p->angle), 30, false);
	return 1;
}

static void youmu_particle_slice_draw(Projectile *p, int t) {
	double lifetime = p->timeout;
	double tt = t/lifetime;
	double f = 0;
	if(tt > 0.1) {
		f = min(1,(tt-0.1)/0.2);
	}
	if(tt > 0.5) {
		f = 1+(tt-0.5)/0.5;
	}
	
	r_mat_push();
	r_mat_translate(creal(p->pos), cimag(p->pos),0);
	r_mat_rotate_deg(p->angle/M_PI*180,0,0,1);
	r_mat_scale(f,1,1);
	//draw_texture(0,0,"part/youmu_slice");
	ProjDrawCore(p, &p->color);
	r_mat_pop();

	double slicelen = 500;
	complex slicepos = p->pos-(tt>0.1)*slicelen*I*cexp(I*p->angle)*(5*pow(tt-0.1,1.1)-0.5);
	draw_sprite_batched_p(creal(slicepos), cimag(slicepos), aniplayer_get_frame(&global.plr.ani));
}

static int youmu_particle_slice_logic(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	double lifetime = p->timeout;
	double tt = t/lifetime;
	double a = 0;

	if(tt > 0.) {
		a = min(1,(tt-0.)/0.2);
	}
	if(tt > 0.5) {
		a = max(0,1-(tt-0.5)/0.5);
	}

	p->color = *RGBA(a, a, a, 0);

	complex phase = cexp(p->angle*I);
	if(t%5 == 0) {
		tsrand_fill(4);
		PARTICLE(
			.sprite = "petal",
			.pos = p->pos-400*phase,
			.rule = accelerated,
			.draw_rule = Petal,
			.color = RGBA(0.1, 0.1, 0.5, 0),
			.args = {
				phase,
				phase*cexp(0.1*I),
				afrand(1) + afrand(2)*I,
				afrand(3) + 360.0*I*afrand(0)
			},
			.layer = LAYER_PARTICLE_HIGH | 0x2,
		);
	}

	return ACTION_NONE;
}

static void YoumuSlash(Enemy *e, int t, bool render) {
}


static int youmu_slash(Enemy *e, int t) {
	if(t > creal(e->args[0]))
		return ACTION_DESTROY;
	if(t < 0)
		return 1;

	if(global.frames - global.plr.recovery > 0) {
		return ACTION_DESTROY;
	}

	TIMER(&t);
	FROM_TO(0,10000,3) {
	complex pos = cexp(I*_i)*(100+10*_i*_i*0.01);
		PARTICLE(
			.sprite = "youmu_slice",
			.color = RGBA(1, 1, 1, 0),
			.pos = e->pos+pos,
			.draw_rule = youmu_particle_slice_draw,
			.rule = youmu_particle_slice_logic,
			.flags = PFLAG_NOREFLECT,
			.timeout = 100,
			.angle = carg(pos),
			.layer = LAYER_PARTICLE_HIGH | 0x1,
		);
	}

	return 1;
}

static int youmu_asymptotic(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);
	p->args[1] *= 0.8;
	p->pos += p->args[0] * (p->args[1] + 1);

	youmu_homing_trail(p, cexp(I*p->angle), 5);
	return 1;
}

static void youmu_haunting_power_shot(Player *plr, int p) {
	int d = -2;
	double spread = 0.5 * (1 + 0.25 * sin(global.frames/10.0));
	double speed = 8;

	if(2 * plr->power / 100 < p || (global.frames + d * p) % 12) {
		return;
	}

	float np = (float)p / (2 * plr->power / 100);

	for(int sign = -1; sign < 2; sign += 2) {
		complex dir = cexp(I*carg(sign*p*spread-speed*I));

		PROJECTILE(
			.sprite = "hghost",
			.pos =  plr->pos,
			.rule = youmu_asymptotic,
			.color = RGB(0.7 + 0.3 * (1-np), 0.8 + 0.2 * sqrt(1-np), 1.0),
			.draw_rule = youmu_homing_draw_proj,
			.args = { speed * dir * (1 - 0.25 * (1 - np)), 3 * (1 - pow(1 - np, 2)), 60, },
			.type = PlrProj,
			.damage = 30,
			.shader = "sprite_default",
		);
	}
}

static void youmu_haunting_shot(Player *plr) {
	youmu_common_shot(plr);

	if(player_should_shoot(plr, true)) {
		if(plr->inputflags & INFLAG_FOCUS) {
			int pwr = plr->power / 100;

			if(!(global.frames % (45 - 4 * pwr))) {
				int pcnt = 11 + pwr * 4;
				int pdmg = 120 - 18 * 4 * (1 - pow(1 - pwr / 4.0, 1.5));
				complex aim = 0.75;

				PROJECTILE("youhoming", plr->pos, RGB(1, 1, 1), youmu_trap,
					.args = { -30.0*I, 120, pcnt+pdmg*I, aim },
					.type = PlrProj,
					.damage = 1000,
					.shader = "sprite_youmu_charged_shot",
					.shader_params = &(ShaderCustomParams){{ 0, 1 }},
				);
			}
		} else {
			if(!(global.frames % 6)) {
				PROJECTILE("hghost", plr->pos, RGB(0.75, 0.9, 1), youmu_homing,
					.args = { -10.0*I, 0.1 + 0.2*I, 60, VIEWPORT_W*0.5 },
					.type = PlrProj,
					.damage = 120,
					.shader = "sprite_default",
				);
			}

			for(int p = 1; p <= 2*PLR_MAX_POWER/100; ++p) {
				youmu_haunting_power_shot(plr, p);
			}
		}
	}
}

static void youmu_haunting_bomb(Player *plr) {
	play_sound("bomb_youmu_b");
	create_enemy_p(&plr->slaves, global.plr.pos, ENEMY_BOMB, YoumuSlash, youmu_slash, 280,0,0,0);
}

static void youmu_haunting_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"proj/youmu",
		"part/youmu_slice",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"sprite_youmu_charged_shot",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"youmu_bombbg1",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_youmu_b",
	NULL);
}

PlayerMode plrmode_youmu_b = {
	.name = "Haunting Sign",
	.character = &character_youmu,
	.dialog = &dialog_youmu,
	.shot_mode = PLR_SHOT_YOUMU_HAUNTING,
	.procs = {
		.property = youmu_common_property,
		.bomb = youmu_haunting_bomb,
		.bombbg = youmu_common_bombbg,
		.shot = youmu_haunting_shot,
		.preload = youmu_haunting_preload,
		.think = player_placeholder_bomb_logic,
	},
};
