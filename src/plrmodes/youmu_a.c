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
#include "renderer/api.h"

#define MYON (global.plr.slaves.first)

static Color *myon_color(Color *c, float f, float opacity, float alpha) {
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

static cmplx myon_tail_dir(void) {
	double angle = carg(MYON->args[0]);
	cmplx dir = cexp(I*(0.1 * sin(global.frames * 0.05) + angle));
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

static void myon_draw_trail_func(Projectile *p, int t, ProjDrawRuleArgs args) {
	float focus_factor = args[0].as_float[0];
	float scale = args[0].as_float[1];

	float fadein = clamp(t/10.0, 0, 1);
	float s = 1 - projectile_timeout_factor(p);

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);

	float a = spbuf.color.r * fadein;
	myon_color(&spbuf.color, focus_factor, a * s * s, 0);
	sp.scale.as_cmplx *= fadein * (2-s) * scale;

	r_draw_sprite(&sp);
}

static ProjDrawRule myon_draw_trail(float scale, float focus_factor) {
	return (ProjDrawRule) {
		.func = myon_draw_trail_func,
		.args[0].as_float = { focus_factor, scale },
	};
}

static void spawn_stardust(cmplx pos, float myon_color_f, int timeout, cmplx v) {
	RNG_ARRAY(R, 4);

	PARTICLE(
		.sprite = "stardust",
		.pos = pos + vrng_range(R[0], 0, 5) * vrng_dir(R[1]),
		.draw_rule = myon_draw_trail(vrng_range(R[2], 0.2, 0.3), myon_color_f),
		.rule = myon_particle_rule,
		.timeout = timeout,
		.args = { v, 0, 0, myon_color_f },
		.angle = vrng_angle(R[3]),
		.flags = PFLAG_NOREFLECT,
		.layer = LAYER_PARTICLE_LOW | 1,
	);
}

static void myon_spawn_trail(Enemy *e, int t) {
	cmplx pos = e->pos + 3 * cdir(global.frames * 0.07);
	cmplx stardust_v = 3 * myon_tail_dir() * cdir(M_PI/16*sin(1.33*t));
	real f = abs(global.plr.focus) / 30.0;
	stardust_v = f * stardust_v + (1 - f) * -I;

	if(player_should_shoot(&global.plr, true)) {
		RNG_ARRAY(R, 7);

		PARTICLE(
			.sprite = "smoke",
			.pos = pos + vrng_range(R[0], 0, 10) * vrng_dir(R[1]),
			.draw_rule = myon_draw_trail(0.2, f),
			.rule = myon_particle_rule,
			.timeout = 60,
			.args = { 0, -0.2, 0, f },
			.flags = PFLAG_NOREFLECT,
			.angle = vrng_angle(R[2]),
		);

		PARTICLE(
			.sprite = "flare",
			.pos = pos + vrng_range(R[3], 0, 5) * vrng_dir(R[4]),
			.draw_rule = pdraw_timeout_scale(2, 0.01),
			.rule = myon_particle_rule,
			.timeout = 10,
			.args = { 0.5 * vrng_dir(R[5]), 0.2, 0, f },
			.flags = PFLAG_NOREFLECT,
			.angle = vrng_angle(R[6]),
		);
	}

	PARTICLE(
		.sprite = "myon",
		.pos = pos,
		.rule = myon_flare_particle_rule,
		.timeout = 40,
		.args = { f * stardust_v, 0, 0, f },
		.draw_rule = pdraw_timeout_scale(2, 0.01),
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.angle = rng_angle(),
		.layer = LAYER_PARTICLE_LOW,
	);

	spawn_stardust(pos, f, 60, stardust_v);
}

static void myon_draw_proj_trail(Projectile *p, int t, ProjDrawRuleArgs args) {
	float time_progress = projectile_timeout_factor(p);
	float s = 2 * time_progress;
	float a = min(1, s) * (1 - time_progress);

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	color_mul_scalar(&spbuf.color, a);
	sp.scale.as_cmplx *= s;
	r_draw_sprite(&sp);
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
		.timeout = 10,
		.move = move_linear(p->args[0]*0.8),
		.flags = PFLAG_NOREFLECT,
		.angle = p->angle,
		.scale = 0.6,
	);

	p->opacity = 1.0f - powf(1.0f - fminf(1.0f, t / 10.0f), 2.0f);

	return ACTION_NONE;
}

static Projectile* youmu_mirror_myon_proj(ProjPrototype *proto, cmplx pos, double speed, double angle, double aoffs, double upfactor, float dmg) {
	cmplx dir = cexp(I*(M_PI/2 + aoffs)) * upfactor + cexp(I * (angle + aoffs)) * (1 - upfactor);
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
		.proto = proto,
		.pos = pos,
		.rule = myon_proj,
		.args = { speed*dir },
		.type = PROJ_PLAYER,
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

	double followfactor = 0.1;
	double nfocus = plr->focus / 30.0;

	if(plr->inputflags & INFLAG_FOCUS) {
		e->args[3] = 1;
	} else if(e->args[3] == 1) {
		nfocus = 0.0;
		e->pos0 = -rad * I;
		followfactor *= 3;

		if(plr->inputflags & INFLAGS_MOVE) {
			e->args[3] = 0;
		}
	}

	if(e->args[3] == 0) {
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
	}

	cmplx target = plr->pos + e->pos0;
	cmplx v = cexp(I*carg(target - e->pos)) * min(10, followfactor * max(0, cabs(target - e->pos) - VIEWPORT_W * 0.5 * nfocus));
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

	e->args[1] += (e->args[0] - e->args[1]) * 0.5;

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
			youmu_mirror_myon_proj(pp_youmu,  e->pos, v2, a,  r1*1, u, dmg_side);
			youmu_mirror_myon_proj(pp_youmu,  e->pos, v2, a, -r1*1, u, dmg_side);
		}

		if(plr->power >= 200 && !((global.frames+3) % 6)) {
			youmu_mirror_myon_proj(pp_youmu, e->pos, v1, a,  r2*2, 0, dmg_side);
			youmu_mirror_myon_proj(pp_youmu, e->pos, v1, a, -r2*2, 0, dmg_side);
		}

		if(plr->power >= 300 && !((global.frames+0) % 6)) {
			youmu_mirror_myon_proj(pp_youmu,  e->pos, v2, a,  r1*3, 0, dmg_side);
			youmu_mirror_myon_proj(pp_youmu,  e->pos, v2, a, -r1*3, 0, dmg_side);
		}

		if(plr->power >= 400 && !((global.frames+3) % 6)) {
			youmu_mirror_myon_proj(pp_youmu, e->pos, v1, a,  r2*4, u, dmg_side);
			youmu_mirror_myon_proj(pp_youmu, e->pos, v1, a, -r2*4, u, dmg_side);
		}

		if(!((global.frames+3) % 6)) {
			youmu_mirror_myon_proj(pp_youmu, e->pos, v1, a, 0, 0, dmg_center);
		}
	}

	return 1;
}

static int youmu_mirror_self_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	cmplx v0 = p->args[0];
	cmplx v1 = p->args[1];
	double f = creal(p->args[2]) ? clamp(t / p->args[2], 0, 1) : 1;
	cmplx v = v1*f + v0*(1-f);

	cmplx diff = p->pos0 + v * t - p->pos;
	p->pos += diff;
	p->angle = carg(diff ? diff : v);

	return 1;
}

static Projectile* youmu_mirror_self_shot(Player *plr, cmplx ofs, cmplx vel, float dmg, double turntime) {
	return PROJECTILE(
		.proto = pp_youmu,
		.pos = plr->pos + ofs,
		.type = PROJ_PLAYER,
		.damage = dmg,
		.shader = "sprite_default",
		.layer = LAYER_PLAYER_SHOT | 0x20,
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

static void youmu_mirror_bomb_damage_callback(EntityInterface *victim, cmplx victim_origin, void *arg) {
	cmplx ofs_dir = rng_dir();
	victim_origin += ofs_dir * rng_range(0, 15);

	RNG_ARRAY(R, 6);

	PARTICLE(
		.sprite = "blast_huge_halo",
		.pos = victim_origin,
		.color = RGBA(vrng_range(R[0], 0.6, 0.7), 0.8, vrng_range(R[1], 0.7, 0.775), vrng_range(R[2], 0, 0.5)),
		.timeout = 30,
		.draw_rule = pdraw_timeout_scalefade(0, 0.5, 1, 0),
		.layer = LAYER_PARTICLE_HIGH | 0x4,
		.angle = vrng_angle(R[3]),
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	if(global.frames & 2) {
		return;
	}

	real t = rng_real();
	RNG_NEXT(R);

	PARTICLE(
		.sprite = "petal",
		.pos = victim_origin,
		.rule = asymptotic,
		.draw_rule = pdraw_petal_random(),
		.color = RGBA(sin(5*t) * t, cos(5*t) * t, 0.5 * t, 0),
		.args = {
			vrng_sign(R[0]) * vrng_range(R[1], 3, 3 + 5 * t) * cdir(M_PI*8*t),
			5+I,
		},
		.layer = LAYER_PARTICLE_PETAL,
	);
}

static int youmu_mirror_bomb_controller(Enemy *e, int t) {
	if(t == EVENT_DEATH || t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	if(!player_is_bomb_active(&global.plr)) {
		return ACTION_DESTROY;
	}

	MYON->pos = e->pos;
	cmplx myonpos = MYON->pos;

	e->pos += e->args[0];
	cmplx aim = (global.plr.pos - e->pos) * 0.01;
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
			.draw_rule = pdraw_timeout_fade(1, 0),
			.color = RGBA(0.9, 0.8, 1.0, 0.0),
			.timeout = 30,
			.args = {
				2 * rng_dir(),
			},
			.flags = _i%2 == 0 ? PFLAG_REQUIREDPARTICLE : 0
		);

		RNG_ARRAY(R, 2);

		PARTICLE(
			.sprite = "stain",
			.pos = e->pos,
			.rule = accelerated,
			.draw_rule = pdraw_timeout_scalefade(0, 3, 1, 0),
			.angle = vrng_angle(R[0]),
			.color = RGBA(0.2, 0.1, 1.0, 0.0),
			.timeout = 50,
			.args = {
				-1*e->args[0]*cdir(0.2*rng_real())/30,
				0.1*e->args[0]*I*sin(t/4.)/30,
			},
			.flags = _i%2 == 0 ? PFLAG_REQUIREDPARTICLE : 0
		);
	}

	// roughly matches the shader effect
	float bombtime = player_get_bomb_progress(&global.plr);
	float envelope = bombtime * (1 - bombtime);
	float range = 200 / (1 + pow(0.08 / envelope, 5));

	ent_area_damage(myonpos, range, &(DamageInfo) { 200, DMG_PLAYER_BOMB }, youmu_mirror_bomb_damage_callback, e);
	stage_clear_hazards_at(myonpos, range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);

	return ACTION_NONE;
}

static bool youmu_mirror_shader(Framebuffer *fb) {
	ShaderProgram *shader = r_shader_get("youmua_bomb");

	double t = player_get_bomb_progress(&global.plr);
	r_shader_ptr(shader);
	r_uniform_float("tbomb", t);

	cmplx myonpos = MYON->pos;
	float f = max(0,1 - 10*t);
	r_uniform_vec2("myon", creal(myonpos)/VIEWPORT_W, 1-cimag(myonpos)/VIEWPORT_H);
	r_uniform_vec4("fill_overlay", f, f, f, f);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();

	return true;
}

static void youmu_mirror_bomb(Player *plr) {
	play_sound("bomb_youmu_b");
	create_enemy_p(&plr->slaves, MYON->pos, ENEMY_BOMB, NULL, youmu_mirror_bomb_controller, -cexp(I*carg(MYON->args[0])) * 30, 0, 0, 0);
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
	.name = "Soul Reflection",
	.description = "Human and phantom act together towards a singular purpose. Your inner duality shall lend you a hand… or a tail.",
	.spellcard_name = "Soul Sign “Reality-Piercing Apparition”",
	.character = &character_youmu,
	.dialog = &dialog_tasks_youmu,
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
