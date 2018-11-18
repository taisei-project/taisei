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
#include "youmu.h"
#include "renderer/api.h"

#define MYON (global.plr.slaves.first)

static Color* myon_color(Color *c, float f, float opacity, float alpha) {
	// *RGBA_MUL_ALPHA(0.8+0.2*f, 0.9-0.4*sqrt(f), 1.0-0.2*f*f, a);
	*c = *RGBA_MUL_ALPHA(0.8+0.2*f, 0.9-0.4*sqrt(f), 1.0-0.35*f*f, opacity);
	c->a *= alpha;
	return c;
}

static int myon_particle_rule(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	myon_color(&p->color, clamp(creal(p->args[3]) + t / p->timeout, 0, 1), 0.5 * (1 - sqrt(t / p->timeout)), 0);

	p->pos += p->args[0];
	p->angle += 0.03 * (1 - 2 * (p->birthtime & 1));

	return ACTION_NONE;
}

static complex myon_tail_dir(void) {
	double angle = carg(MYON->args[0]);
	complex dir = cexp(I*(0.1 * sin(global.frames * 0.05) + angle));
	float f = abs(global.plr.focus) / 30.0;
	return f * f * dir;
}

static int myon_flare_particle_rule(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	// wiggle wiggle
	p->pos += 0.05 * (MYON->pos - p->pos) * cexp(I * sin((t - global.frames * 2) * 0.1) * M_PI/8);
	p->args[0] = 3 * myon_tail_dir();

	int r = myon_particle_rule(p, t);
	myon_color(&p->color, creal(p->args[3]), pow(1 - min(1, t / (double)p->timeout), 2), 0.95);
	return r;
}

static void myon_draw_trail(Projectile *p, int t) {
	float fadein = clamp(t/10.0, p->args[2], 1);
	float s = min(1, 1 - t / (double)p->timeout);
	float a = p->color.r*fadein;
	Color c;
	myon_color(&c, creal(p->args[3]), a * s * s, 0);
	youmu_common_draw_proj(p, &c, fadein * (2-s) * p->args[1]);
}

static void spawn_stardust(complex pos, float myon_color_f, int timeout, complex v) {
	PARTICLE(
		.sprite = "stardust",
		.pos = pos+5*frand()*cexp(2.0*I*M_PI*frand()),
		.draw_rule = myon_draw_trail,
		.rule = myon_particle_rule,
		.timeout = timeout,
		.args = { v, 0.2 + 0.1 * frand(), 0, myon_color_f },
		.angle = M_PI*2*frand(),
		.flags = PFLAG_NOREFLECT,
		.layer = LAYER_PARTICLE_LOW | 1,
	);
}

static void myon_spawn_trail(Enemy *e, int t) {
	float a = global.frames * 0.07;
	complex pos = e->pos + 3 * (cos(a) + I * sin(a));

	complex stardust_v = 3 * myon_tail_dir() * cexp(I*M_PI/16*sin(1.33*t));
	float f = abs(global.plr.focus) / 30.0;
	stardust_v = f * stardust_v + (1 - f) * -I;

	if(player_should_shoot(&global.plr, true)) {
		PARTICLE(
			.sprite = "smoke",
			.pos = pos+10*frand()*cexp(2.0*I*M_PI*frand()),
			.draw_rule = myon_draw_trail,
			.rule = myon_particle_rule,
			.timeout = 60,
			.args = { -I*0.0*cexp(I*M_PI/16*sin(t)), -0.2, 0, f },
			.flags = PFLAG_NOREFLECT,
			.angle = M_PI*2*frand(),
		);

		PARTICLE(
			.sprite = "flare",
			.pos = pos+5*frand()*cexp(2.0*I*M_PI*frand()),
			.draw_rule = Shrink,
			.rule = myon_particle_rule,
			.timeout = 10,
			.args = { cexp(I*M_PI*2*frand())*0.5, 0.2, 0, f },
			.flags = PFLAG_NOREFLECT,
			.angle = M_PI*2*frand(),
		);
	}

	PARTICLE(
		.sprite = "myon",
		.pos = pos,
		.rule = myon_flare_particle_rule,
		.timeout = 40,
		.args = { f * stardust_v, 0, 0, f },
		.draw_rule = Shrink,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.angle = M_PI*2*frand(),
		.layer = LAYER_PARTICLE_LOW,
	);

	spawn_stardust(pos, f, 60, stardust_v);
}

static void myon_draw_proj_trail(Projectile *p, int t) {
	float time_progress = t / p->timeout;
	float s = 2 * time_progress;
	float a = min(1, s) * (1 - time_progress);
	Color c = p->color;
	color_mul_scalar(&c, a);
	youmu_common_draw_proj(p, &c, s * p->args[1]);
}

static int myon_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	linear(p, t);

	//p->pos = global.plr.slaves->pos - global.plr.slaves->args[0] / cabs(global.plr.slaves->args[0]) * t * cabs(p->args[0]);
	//p->angle = carg(-global.plr.slaves->args[0]);

	// spawn_stardust(p->pos, multiply_colors(p->color, myon_color(abs(global.plr.focus) / 30.0, 0.1)), 20, p->args[0]*0.1);

	Color *c = COLOR_COPY(&p->color);
	color_mul_scalar(c, 0.075);
	c->a = 0;

	PARTICLE(
		.sprite = "boss_shadow",
		.pos = p->pos,
		// .color = derive_color(p->color, CLRMASK_A, rgba(0, 0, 0, 0.075)),
		.color = c,
		.draw_rule = myon_draw_proj_trail,
		.rule = linear,
		.timeout = 10,
		.args = { p->args[0]*0.8, 0.6 },
		.flags = PFLAG_NOREFLECT,
		.angle = p->angle,
	);

	p->shader_params.vector[0] = pow(1 - min(1, t / 10.0), 2);

	return ACTION_NONE;
}

static void myon_proj_draw(Projectile *p, int t) {
	youmu_common_draw_proj(p, &p->color, 1);
}

static Projectile* youmu_mirror_myon_proj(char *tex, complex pos, double speed, double angle, double aoffs, double upfactor, float dmg) {
	complex dir = cexp(I*(M_PI/2 + aoffs)) * upfactor + cexp(I * (angle + aoffs)) * (1 - upfactor);
	dir = dir / cabs(dir);

	// float f = ((global.plr.inputflags & INFLAG_FOCUS) == INFLAG_FOCUS);
	float f = smoothreclamp(abs(global.plr.focus) / 30.0, 0, 1, 0, 1);
	Color c, intermediate = { 1.0, 1.0, 1.0, 1.0 };

	if(f < 0.5) {
		c = *RGB(0.4, 0.6, 0.6);
		color_lerp(
			&c,
			&intermediate,
			f * 2
		);
	} else {
		Color mc;
		c = intermediate;
		color_lerp(
			&c,
			myon_color(&mc, f, 1, 1),
			(f - 0.5) * 2
		);
	}

	return PROJECTILE(
		.color = &c,
		.sprite = tex,
		.pos = pos,
		.rule = myon_proj,
		.args = { speed*dir },
		.draw_rule = myon_proj_draw,
		.type = PlrProj,
		.layer = LAYER_PLAYER_SHOT | 0x10,
		.damage = dmg,
		.shader = "sprite_youmu_myon_shot",
	);
}

static int youmu_mirror_myon(Enemy *e, int t) {
	if(t == EVENT_BIRTH)
		e->pos = e->pos0 + global.plr.pos;
	if(t < 0)
		return 1;

	myon_spawn_trail(e, t);

	Player *plr = &global.plr;
	float rad = cabs(e->pos0);

	double nfocus = plr->focus / 30.0;

	if(!(plr->inputflags & INFLAG_SHOT)) {
		nfocus = 0.0;
		e->pos0 = -rad * I;
	} else if(!(plr->inputflags & INFLAG_FOCUS)) {
		if((plr->inputflags & INFLAGS_MOVE)) {
			e->pos0 = rad * -plr->lastmovedir;
		} else {
			e->pos0 = e->pos - plr->pos;
			e->pos0 *= rad / cabs(e->pos0);
		}
	}

	complex target = plr->pos + e->pos0;
	complex v = cexp(I*carg(target - e->pos)) * min(10, 0.07 * max(0, cabs(target - e->pos) - VIEWPORT_W * 0.5 * nfocus));
	float s = sign(creal(e->pos) - creal(global.plr.pos));

	if(!s) {
		s = sign(sin(t/10.0));
	}

	float rot = clamp(0.005 * cabs(global.plr.pos - e->pos) - M_PI/6, 0, M_PI/8);
	v *= cexp(I*rot*s);
	e->pos += v;

	if(!(plr->inputflags & INFLAG_SHOT) || !(plr->inputflags & INFLAG_FOCUS)) {
		e->args[0] = plr->pos - e->pos;
	}

	e->args[1] += (e->args[0] - e->args[1]) * 0.1;

	if(player_should_shoot(&global.plr, true)) {
		int v1 = -10;
		int v2 = -10;

		double r1 = (psin(global.frames * 2.0) * 0.5 + 0.5) * 0.1;
		double r2 = (psin(global.frames * 1.2) * 0.5 + 0.5) * 0.1;

		double a = carg(e->args[0]);
		double f = smoothreclamp(0.5 + 0.5 * (1.0 - nfocus), 0, 1, 0, 1);
		double u = 0; // smoothreclamp(1 - nfocus, 0, 1, 0, 1);

		r1 *= f;
		r2 *= f;

		int p = plr->power / 100;
		int dmg_center = 180 - rint(160 * (1 - pow(1 - 0.25 * p, 2)));
		int dmg_side = 41 - 3 * p;

		if(plr->power >= 100 && !((global.frames+0) % 6)) {
			youmu_mirror_myon_proj("youmu",  e->pos, v2, a,  r1*1, u, dmg_side);
			youmu_mirror_myon_proj("youmu",  e->pos, v2, a, -r1*1, u, dmg_side);
		}

		if(plr->power >= 200 && !((global.frames+3) % 6)) {
			youmu_mirror_myon_proj("youmu", e->pos, v1, a,  r2*2, 0, dmg_side);
			youmu_mirror_myon_proj("youmu", e->pos, v1, a, -r2*2, 0, dmg_side);
		}

		if(plr->power >= 300 && !((global.frames+0) % 6)) {
			youmu_mirror_myon_proj("youmu",  e->pos, v2, a,  r1*3, 0, dmg_side);
			youmu_mirror_myon_proj("youmu",  e->pos, v2, a, -r1*3, 0, dmg_side);
		}

		if(plr->power >= 400 && !((global.frames+3) % 6)) {
			youmu_mirror_myon_proj("youmu", e->pos, v1, a,  r2*4, u, dmg_side);
			youmu_mirror_myon_proj("youmu", e->pos, v1, a, -r2*4, u, dmg_side);
		}

		if(!((global.frames+3) % 6)) {
			youmu_mirror_myon_proj("youmu", e->pos, v1, a, 0, 0, dmg_center);
		}
	}

	return 1;
}

static int youmu_mirror_self_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	complex v0 = p->args[0];
	complex v1 = p->args[1];
	double f = creal(p->args[2]) ? clamp(t / p->args[2], 0, 1) : 1;
	complex v = v1*f + v0*(1-f);

	complex diff = p->pos0 + v * t - p->pos;
	p->pos += diff;
	p->angle = carg(diff ? diff : v);

	return 1;
}

static Projectile* youmu_mirror_self_shot(Player *plr, complex ofs, complex vel, float dmg, double turntime) {
	return PROJECTILE("youmu", plr->pos + ofs, 0,
		.type = PlrProj,
		.damage = dmg,
		.shader = "sprite_default",
		.layer = LAYER_PLAYER_SHOT | 0x20,
		.draw_rule = myon_proj_draw,
		.rule = youmu_mirror_self_proj,
		.args = {
			vel*0.2*cexp(I*M_PI*0.5*sign(creal(ofs))), vel, turntime,
		},
	);
}

static void youmu_mirror_shot(Player *plr) {
	play_loop("generic_shot");

	int p = plr->power / 100;

	if(!(global.frames % 6)) {
		int dmg = 105 - 10 * p;
		youmu_mirror_self_shot(plr, +10 - I*20, -20.0*I, dmg, 0);
		youmu_mirror_self_shot(plr, -10 - I*20, -20.0*I, dmg, 0);
	}

	if(!((global.frames) % 6)) {
		for(int i = 0; i < p; ++i) {
			int dmg = 21;
			double spread = M_PI/64 * (1 + 0.5 * smoothreclamp(psin(global.frames/10.0), 0, 1, 0, 1));

			youmu_mirror_self_shot(plr, (+10 + I*10), -(20.0-i)*I*cexp(-I*(1+i)*spread), dmg, 20);
			youmu_mirror_self_shot(plr, (-10 + I*10), -(20.0-i)*I*cexp(+I*(1+i)*spread), dmg, 20);
		}
	}
}

static int youmu_split(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	if(!player_is_bomb_active(&global.plr)) {
		return ACTION_DESTROY;
	}

	MYON->pos = e->pos;
	complex myonpos = MYON->pos;

	e->pos += e->args[0];
	complex aim = (global.plr.pos - e->pos) * 0.01;
	double accel_max = 1;

	if(cabs(aim) > accel_max) {
		aim *= accel_max / cabs(aim);
	}

	e->args[0] += aim;
	e->args[0] *= 1.0 - cabs(global.plr.pos - e->pos) * 0.0001;

	TIMER(&t);
	FROM_TO(0, 240, 1) {
		PARTICLE(
			.sprite = "arc",
			.pos = e->pos,
			.rule = linear,
			.draw_rule = Fade,
			.color = RGBA(0.9, 0.8, 1.0, 0.0),
			.timeout = 30,
			.args = {
				2*cexp(2*I*M_PI*frand()),
			},
			.flags = _i%2 == 0 ? PFLAG_REQUIREDPARTICLE : 0
		);
		PARTICLE(
			.sprite = "stain",
			.pos = e->pos,
			.rule = accelerated,
			.draw_rule = GrowFade,
			.angle = 2*M_PI*frand(),
			.color = RGBA(0.2, 0.1, 1.0, 0.0),
			.timeout = 50,
			.args = {
				-1*e->args[0]*cexp(I*0.2*nfrand())/30,
				0.1*e->args[0]*I*sin(t/4.)/30,
				2
			},
			.flags = _i%2 == 0 ? PFLAG_REQUIREDPARTICLE : 0
		);
	}

	float range = 200;
	ent_area_damage(myonpos, range, &(DamageInfo){250, DMG_PLAYER_BOMB});
	stage_clear_hazards_at(myonpos, range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);

	return 1;
}

static void youmu_mirror_shader(Framebuffer *fb) {
	ShaderProgram *shader = r_shader_get("youmua_bomb");

	double t = player_get_bomb_progress(&global.plr,0);
	r_shader_ptr(shader);
	r_uniform_float("tbomb", t);

	complex myonpos = MYON->pos;
	r_uniform_vec2("myon", creal(myonpos)/VIEWPORT_W, 1-cimag(myonpos)/VIEWPORT_H);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();

	float f = max(0,1 - 10*t);
	colorfill(f, f, f, f);
}

static void youmu_mirror_bomb(Player *plr) {
	play_sound("bomb_youmu_b");
	create_enemy_p(&plr->slaves, MYON->pos, ENEMY_BOMB, NULL, youmu_split, -cexp(I*carg(MYON->args[0])) * 30, 0, 0, 0);
}

static void youmu_mirror_init(Player *plr) {
	Enemy *myon = create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, NULL, youmu_mirror_myon, 0, 0, 0, 0);
	myon->ent.draw_layer = LAYER_PLAYER_SLAVE;
	youmu_common_bomb_buffer_init();
}

static void youmu_mirror_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"proj/youmu",
		"part/myon",
		"part/stardust",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"youmu_bombbg1",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"sprite_youmu_myon_shot",
		"youmu_bomb_bg",
		"youmua_bomb",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_youmu_b",
	NULL);
}

static void youmu_mirror_bomb_logic(Player *plr) {
}

PlayerMode plrmode_youmu_a = {
	.name = "Mirror Sign",
	.character = &character_youmu,
	.dialog = &dialog_youmu,
	.shot_mode = PLR_SHOT_YOUMU_MIRROR,
	.procs = {
		.property = youmu_common_property,
		.bomb = youmu_mirror_bomb,
		.bomb_shader = youmu_mirror_shader,
		.bombbg = youmu_common_bombbg,
		.shot = youmu_mirror_shot,
		.init = youmu_mirror_init,
		.preload = youmu_mirror_preload,
		.think = youmu_mirror_bomb_logic,
	},
};
