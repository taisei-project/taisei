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
#include "util/glm.h"

static cmplx youmu_homing_target(cmplx org, cmplx fallback) {
	return plrutil_homing_target(org, fallback);
}

static void youmu_homing_trail(Projectile *p, cmplx v, int to) {
	uint32_t tmp = p->ent.spawn_id;
	float u = M_PI * 2.0f * (float)(splitmix32(&tmp) / (double)UINT32_MAX);

	RNG_ARRAY(R, 5);

	PARTICLE(
		.sprite = "stardust",
		.pos = p->pos + vrng_range(R[0], 3, 12) * vrng_dir(R[1]),
		.color = RGBA(0.0, 0.3 * vrng_real(R[2]), 0.3, 0.0),
		// .draw_rule = pdraw_timeout_fade(1, 0),
		.draw_rule = pdraw_timeout_scalefade_exp(0.001*I, vrng_range(R[3], 0.5, 1.0)*(1+I), 2, 0, 2),
		.move = move_linear(-v),
		.timeout = to,
		.scale = 0.5,
		.angle = vrng_angle(R[4]),
		.flags = PFLAG_NOREFLECT | PFLAG_PLRSPECIALPARTICLE | PFLAG_MANUALANGLE,
		.layer = LAYER_PARTICLE_MID,
	);

	PARTICLE(
		.sprite = "smoothdot",
		.pos = p->pos,
		.color = color_mul(RGBA(0.2, 0.24, 0.3, 0.2), &p->color),
		.move = move_asymptotic_simple(-0.5*v*cdir(0.2*sin(u+3*creal(p->pos)/VIEWPORT_W*M_TAU) + 0.2*cos(u+3*cimag(p->pos)/VIEWPORT_H*M_TAU)), 2),
		.draw_rule = pdraw_timeout_scalefade_exp(0.5+0.5*I, 3+7*I, 1, 0, 2),
		.timeout = to,
		.flags = PFLAG_NOREFLECT,
		.layer = LAYER_PARTICLE_LOW,
	);
}

static void youmu_particle_slice_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	float lifetime = p->timeout;
	float tt = t/lifetime;
	float f = 0;

	if(tt > 0.1) {
		f = min(1,(tt-0.1)/0.2);
	}

	if(tt > 0.5) {
		f = 1+(tt-0.5)/0.5;
	}

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	sp.scale.x *= f;
	r_draw_sprite(&sp);

	float slicelen = 500;
	cmplx slicepos = p->pos-(tt>0.1)*slicelen*I*cdir(p->angle)*(5*pow(tt-0.1,1.1)-0.5);

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = aniplayer_get_frame(&global.plr.ani),
		.pos = { creal(slicepos), cimag(slicepos) },
	});
}

static int youmu_slice_petal(Projectile *p, int t) {
	int r = accelerated(p, t);

	if(t >= 0) {
		p->color = *color_mul_scalar(RGBA(0.2, 0.2, 1, 0), min(1, t / 40.0));
	}

	return r;
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


	if(t%5 == 0) {
		cmplx phase = cdir(p->angle);

		PARTICLE(
			.sprite = "petal",
			.pos = p->pos-400*phase,
			.rule = youmu_slice_petal,
			.draw_rule = pdraw_petal_random(),
			.args = {
				phase,
				phase*cdir(0.1),
			},
			.layer = LAYER_PARTICLE_HIGH | 0x2,
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		);
	}

	Ellipse e = {
		.origin = p->pos,
		.axes = CMPLX(512, 64),
		.angle = p->angle + M_PI * 0.5,
	};

	// FIXME: this may still be too slow for lasers in some cases
	stage_clear_hazards_in_ellipse(e, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
	ent_area_damage_ellipse(e, &(DamageInfo) { 52, DMG_PLAYER_BOMB }, NULL, NULL);

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
		cmplx pos = cexp(I*_i)*(100+10*_i*_i*0.01);
		PARTICLE(
			.sprite = "youmu_slice",
			.color = RGBA(1, 1, 1, 0),
			.pos = e->pos+pos,
			.draw_rule = youmu_particle_slice_draw,
			.rule = youmu_particle_slice_logic,
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
			.timeout = 100,
			.angle = carg(pos) + M_PI/2,
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
		cmplx dir = cexp(I*carg(sign*p*spread-speed*I));

		PROJECTILE(
			.proto = pp_hghost,
			.pos =  plr->pos,
			.rule = youmu_asymptotic,
			.color = color_mul_scalar(RGB(0.7 + 0.3 * (1-np), 0.8 + 0.2 * sqrt(1-np), 1.0), 0.5),
			.args = { speed * dir * (1 - 0.25 * (1 - np)), 3 * (1 - pow(1 - np, 2)), 60, },
			.type = PROJ_PLAYER,
			.damage = 20,
			.shader = "sprite_default",
		);
	}
}

TASK(youmu_homing_shot, { BoxedPlayer plr; }) {
	Projectile *p = TASK_BIND_UNBOXED(PROJECTILE(
		.proto = pp_hghost,
		.pos = ENT_UNBOX(ARGS.plr)->pos,
		.color = RGB(0.75, 0.9, 1),
		.type = PROJ_PLAYER,
		.damage = 120,
		.shader = "sprite_default",
	));

	real speed = 10;
	real aim_strength = 0;
	real aim_strength_accel = 0.02;
	p->move = move_linear(-I * speed);
	cmplx target = VIEWPORT_W * 0.5;

	for(int i = 0; i < 60; ++i) {
		target = youmu_homing_target(p->pos, target);
		cmplx aimdir = cnormalize(target - p->pos - p->move.velocity*10);
		p->move.velocity += aim_strength * aimdir;
		p->move.velocity *= speed / cabs(p->move.velocity);
		aim_strength += aim_strength_accel;

		youmu_homing_trail(p, 0.5 * p->move.velocity, 12);
		YIELD;
	}
}

TASK(youmu_orb_homing_spirit_expire, { BoxedProjectile p; }) {
	Projectile *p = ENT_UNBOX(ARGS.p);

	PARTICLE(
		.sprite_ptr = p->sprite,
		.shader_ptr = p->shader,
		.color = &p->color,
		.timeout = 30,
		.draw_rule = pdraw_timeout_scalefade(1+I, 0.1+I, 1, 0),
		.pos = p->pos,
		.move = p->move,
		.angle = p->angle,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_PLAYER_SHOT,
	);
}

static int youmu_orb_homing_spirit_timeout(Projectile *orb) {
	return orb->timeout - projectile_time(orb);
}

TASK(youmu_orb_homing_spirit, { cmplx pos; cmplx velocity; cmplx target; real charge; real damage; real spin; BoxedProjectile orb; }) {
	int timeout = youmu_orb_homing_spirit_timeout(ENT_UNBOX(ARGS.orb));

	if(timeout <= 0) {
		return;
	}

	Projectile *p = TASK_BIND_UNBOXED(PROJECTILE(
		.proto = pp_hghost,
		.pos = ARGS.pos,
		.color = color_mul_scalar(RGB(0.75, 0.9, 1), 0.5),
		.type = PROJ_PLAYER,
		.timeout = timeout,
		.damage = ARGS.damage,
		.shader = "sprite_particle",
	));

	INVOKE_TASK_AFTER(&p->events.killed, youmu_orb_homing_spirit_expire, ENT_BOX(p));

	real speed = cabs(ARGS.velocity);
	real speed_target = speed;
	real aim_strength = -0.1;
	p->move = move_accelerated(ARGS.velocity, -0.06 * ARGS.velocity);
	p->move.retention = cdir(ARGS.spin);
	cmplx target = ARGS.target;

	bool aim_peaked = false;
	bool orb_died = false;
	Projectile *orb = NULL;

	for(;;) {
		if(!orb_died) {
			orb = ENT_UNBOX(ARGS.orb);

			if(orb == NULL) {
				orb_died = true;
				p->timeout = 0;
				// p->move.velocity = 0;
				p->move.retention *= 0.9;
				aim_peaked = false;
				aim_strength = -0.2;
				speed *= 1.2;
			}
		}

		if(orb) {
			target = orb->pos;
		} else {
			target = youmu_homing_target(p->pos, creal(global.plr.pos) - 128*I);
		}

		cmplx aimdir = cnormalize(target - p->pos - p->move.velocity);
		capproach_asymptotic_p(&p->move.retention, 0.8, 0.1, 1e-3);
		p->move.acceleration *= 0.95;
		p->move.acceleration += aim_strength * aimdir;

		if(aim_peaked) {
			if(!orb) {
				approach_p(&aim_strength, 0.00, 0.001);
			}
		} else {
			approach_p(&aim_strength, 0.1, 0.02);
			if(aim_strength == 0.1) {
				aim_peaked = true;
			}
		}

		real s = max(speed, cabs(p->move.velocity));
		p->move.velocity = s * cnormalize(p->move.velocity + aim_strength * s * aimdir);
		approach_asymptotic_p(&speed, speed_target, 0.05, 1e-5);

		youmu_homing_trail(p, 0.5 * p->move.velocity, 12);
		YIELD;
	}
}

TASK(youmu_orb_update, { BoxedPlayer plr; BoxedProjectile orb; }) {
	Player *plr = ENT_UNBOX(ARGS.plr);
	Projectile *orb = TASK_BIND(ARGS.orb);

	for(;;) {
		float tf = glm_ease_bounce_out(1.0f - projectile_timeout_factor(orb));
		orb->color.g = tf * tf;
		orb->color.b = tf;
		orb->scale = 0.5 + 0.5 * tf;
		orb->angle += 0.2;

		// TODO events for player input?
		if(!(plr->inputflags & INFLAG_FOCUS)) {
			PARTICLE(
				.sprite = "blast_huge_rays",
				.pos = orb->pos,
				.timeout = 20,
				.color = RGBA(0.1, 0.5, 0.1, 0.0),
				.draw_rule = pdraw_timeout_scalefade_exp(0.01*(1+I), 1, 1, 0, 2),
				.flags = PFLAG_REQUIREDPARTICLE,
				.angle = rng_angle(),
				.layer = LAYER_PARTICLE_LOW,
			);

			PARTICLE(
				.sprite = "blast_huge_halo",
				.pos = orb->pos,
				.timeout = 30,
				.color = RGBA(0.1, 0.1, 0.5, 0.0),
				.draw_rule = pdraw_timeout_scalefade_exp(1, 0.01*(1+I), 1, 0, 2),
				.flags = PFLAG_REQUIREDPARTICLE,
				.angle = rng_angle(),
				.layer = LAYER_PARTICLE_LOW,
			);

			PARTICLE(
				.sprite = "blast_huge_halo",
				.pos = orb->pos,
				.timeout = 40,
				.color = RGBA(0.5, 0.1, 0.1, 0.0),
				.draw_rule = pdraw_timeout_scalefade_exp(0.8, -0.3*(1+I), 1, 0, 2),
				.flags = PFLAG_REQUIREDPARTICLE,
				.angle = rng_angle(),
				.layer = LAYER_PARTICLE_LOW,
			);

			// TODO sound effect;
			kill_projectile(orb);
			break;
		}

		YIELD;
	}
}

TASK(youmu_orb_death, { BoxedProjectile orb; BoxedTask control_task; }) {
	cotask_cancel(cotask_unbox(ARGS.control_task));
	Projectile *orb = ENT_UNBOX(ARGS.orb);

	PARTICLE(
		.proto = pp_blast,
		.pos = orb->pos,
		.timeout = 20,
		.draw_rule = pdraw_blast(),
		.flags = PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_PARTICLE_LOW,
	);

	PARTICLE(
		.proto = pp_blast,
		.pos = orb->pos,
		.timeout = 23,
		.draw_rule = pdraw_blast(),
		.flags = PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_PARTICLE_LOW,
	);
}

TASK(youmu_orb_shot, { BoxedPlayer plr; }) {
	Player *plr = ENT_UNBOX(ARGS.plr);
	int pwr = plr->power / 100;

	Projectile *orb = TASK_BIND_UNBOXED(PROJECTILE(
		.proto = pp_youhoming,
		.pos = plr->pos,
		.color = RGB(1, 1, 1),
		.type = PROJ_PLAYER,
		.damage = 1000,
		.timeout = 100 + 10 * pwr,
		.move = move_asymptotic(-30.0*I, -0.7*I, 0.8),
		.flags = PFLAG_MANUALANGLE,
	));

	INVOKE_TASK(youmu_orb_update, ARGS.plr, ENT_BOX(orb));
	INVOKE_TASK_AFTER(&orb->events.killed, youmu_orb_death, ENT_BOX(orb), THIS_TASK);

	real pdmg = 120 - 18 * 4 * (1 - pow(1 - pwr / 4.0, 1.5));
	cmplx v = 5 * I;

	for(;;) {
		WAIT(11);
		INVOKE_TASK(youmu_orb_homing_spirit, orb->pos, v, 0, 0, pdmg,  0.1, ENT_BOX(orb));
		INVOKE_TASK(youmu_orb_homing_spirit, orb->pos, v, 0, 0, pdmg, -0.1, ENT_BOX(orb));
	}
}

static void youmu_haunting_shot(Player *plr) {
	youmu_common_shot(plr);

	if(player_should_shoot(plr, true)) {
		if(plr->inputflags & INFLAG_FOCUS) {
			int pwr = plr->power / 100;

			if(!(global.frames % (45 - 4 * pwr))) {
				INVOKE_TASK(youmu_orb_shot, ENT_BOX(plr));
			}
		} else {
			if(!(global.frames % 6)) {
				INVOKE_TASK(youmu_homing_shot, ENT_BOX(plr));
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

	preload_resources(RES_TEXTURE, flags,
		"youmu_bombbg1",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_youmu_b",
	NULL);
}

static void youmu_haunting_init(Player *plr) {
	youmu_common_bomb_buffer_init();
}

PlayerMode plrmode_youmu_b = {
	.name = "Haunting Revelation",
	.description = "Ghosts are real, but they’re nothing to be afraid of. Unless, of course, you happen to stand in the way of a determined sword-wielding lady…",
	.spellcard_name = "Aegis Sword “Saigyō Omnidirectional Slash”",
	.character = &character_youmu,
	.dialog = &dialog_tasks_youmu,
	.shot_mode = PLR_SHOT_YOUMU_HAUNTING,
	.procs = {
		.property = youmu_common_property,
		.bomb = youmu_haunting_bomb,
		.bombbg = youmu_common_bombbg,
		.init = youmu_haunting_init,
		.shot = youmu_haunting_shot,
		.preload = youmu_haunting_preload,
	},
};
