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
#include "stagedraw.h"

#define SHOT_BASIC_DAMAGE 60
#define SHOT_BASIC_DELAY 6

#define SHOT_HOMING_DAMAGE 120
#define SHOT_HOMING_DELAY 6

#define SHOT_SPREAD_DAMAGE 40
#define SHOT_SPREAD_DELAY 12

#define SHOT_ORBS_SPIRIT_DAMAGE 90
#define SHOT_ORBS_SPIRIT_SPAWN_DELAY 11
#define SHOT_ORBS_DELAY_BASE 45
#define SHOT_ORBS_DELAY_PER_POWER -5
#define SHOT_ORBS_LIFETIME_BASE 100
#define SHOT_ORBS_LIFETIME_PER_POWER 10

#define BOMB_SLICE_DAMAGE 100
#define BOMB_SLICE_PERIOD 2

typedef struct YoumuBController {
	struct {
		Sprite *blast_huge_halo;
		Sprite *blast_huge_rays;
		Sprite *petal;
		Sprite *smoothdot;
		Sprite *stardust;
		Sprite *youmu_slice;
	} sprites;
	ShaderProgram *shot_shader;

	Player *plr;
	YoumuBombBGData bomb_bg;
} YoumuBController;

static void youmu_homing_trail(YoumuBController *ctrl, Projectile *p, cmplx v, int to) {
	uint32_t tmp = p->ent.spawn_id;
	float u = M_PI * 2.0f * (float)(splitmix32(&tmp) / (double)UINT32_MAX);

	RNG_ARRAY(R, 5);

	PARTICLE(
		.sprite_ptr = ctrl->sprites.stardust,
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
		.sprite_ptr = ctrl->sprites.smoothdot,
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
	// TODO: pick animation frame based on slice direction?
	AniPlayer *player_ani = args[0].as_ptr;
	Sprite *player_frame = aniplayer_get_frame(player_ani);

	float lifetime = p->timeout;
	float tt = t / lifetime;
	float f = 0.0f;

	if(tt > 0.1) {
		f = fminf(1.0f, (tt - 0.1f) / 0.2f);
	}

	if(tt > 0.5f) {
		f = 1.0f + (tt - 0.5f) / 0.5f;
	}

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	sp.scale.x *= f;
	r_draw_sprite(&sp);

	float slicelen = 500.0f;
	cmplx slicepos = p->pos - (tt > 0.1f) * slicelen * I*cdir(p->angle) * (5 * powf(tt - 0.1f, 1.1f) - 0.5f);

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = player_frame,
		.pos.as_cmplx = slicepos,
		.shader_params = &spbuf.shader_params,
	});
}

TASK(youmu_haunting_shot_basic, { YoumuBController *ctrl; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		play_sfx_loop("generic_shot");

		cmplx v = -20 * I;

		for(int side = -1; side < 2; side += 2) {
			cmplx origin = plr->pos + 10*side + 5*I;
			youmu_common_shot(origin, move_linear(v), SHOT_BASIC_DAMAGE, ctrl->shot_shader);
		}

		WAIT(SHOT_BASIC_DELAY);
	}
}

TASK(youmu_burst_shot, { YoumuBController *ctrl; int num_shots; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	ShaderProgram *shader = ctrl->shot_shader;

	int nshots = ARGS.num_shots;
	real base_speed = 8;

	for(int shot = 1; shot <= nshots; ++shot) {
		real np = shot / (real)nshots;

		Color *clr = color_mul_scalar(RGB(0.7 + 0.3 * (1-np), 0.8 + 0.2 * sqrt(1-np), 1.0), 0.5);
		real spread = 0.5 * (1 + 0.25 * sin(global.frames/10.0));
		real speed = base_speed * (1 - 0.25 * (1 - np));
		real boost = 3 * (1 - pow(1 - np, 2));

		for(int side = -1; side < 2; side += 2) {
			cmplx vel = speed * cnormalize(side * shot * spread - base_speed * I);

			PROJECTILE(
				.color = clr,
				.damage = SHOT_SPREAD_DAMAGE,
				.move = move_asymptotic_simple(vel, boost),
				.pos = plr->pos,
				.proto = pp_hghost,
				.shader_ptr = shader,
				.type = PROJ_PLAYER,
			);
		}

		YIELD;
	}
}

TASK(youmu_haunting_shot_spread, { YoumuBController *ctrl; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		if(plr->inputflags & INFLAG_FOCUS) {
			continue;
		}

		INVOKE_TASK(youmu_burst_shot, ctrl, 2 * player_get_effective_power(plr) / 100);
		WAIT(SHOT_SPREAD_DELAY);
	}
}

TASK(youmu_homing_shot, { YoumuBController *ctrl; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_hghost,
		.pos = plr->pos,
		.color = RGB(0.75, 0.9, 1),
		.type = PROJ_PLAYER,
		.damage = SHOT_HOMING_DAMAGE,
		.shader_ptr = ctrl->shot_shader,
	));

	real speed = 10;
	real aim_strength = 0;
	real aim_strength_accel = 0.02;
	p->move = move_linear(-I * speed);
	cmplx target = VIEWPORT_W * 0.5;

	for(int i = 0; i < 60; ++i) {
		target = plrutil_homing_target(p->pos, target);
		cmplx aimdir = cnormalize(target - p->pos - p->move.velocity*10);
		p->move.velocity += aim_strength * aimdir;
		p->move.velocity *= speed / cabs(p->move.velocity);
		aim_strength += aim_strength_accel;

		youmu_homing_trail(ctrl, p, 0.5 * p->move.velocity, 12);
		YIELD;
	}
}

TASK(youmu_haunting_shot_homing, { YoumuBController *ctrl; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		if(plr->inputflags & INFLAG_FOCUS) {
			continue;
		}

		INVOKE_TASK(youmu_homing_shot, ctrl);
		WAIT(SHOT_HOMING_DELAY);
	}
}

TASK(youmu_orb_homing_spirit_expire, { BoxedProjectile p; }) {
	Projectile *p = NOT_NULL(ENT_UNBOX(ARGS.p));

	PARTICLE(
		.sprite_ptr = p->sprite,
		.shader_ptr = p->shader,
		.color = &p->color,
		.timeout = 30,
		.draw_rule = pdraw_timeout_scalefade(1+I, 0.1+I, 1, 0),
		.pos = p->pos,
		.move = p->move,
		.angle = p->angle,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
		.layer = LAYER_PLAYER_SHOT,
	);
}

static int youmu_orb_homing_spirit_timeout(Projectile *orb) {
	assume(orb != NULL);
	return orb->timeout - projectile_time(orb);
}

TASK(youmu_orb_homing_spirit, { YoumuBController *ctrl; cmplx pos; cmplx velocity; real damage; real spin; BoxedProjectile orb; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	int timeout = youmu_orb_homing_spirit_timeout(ENT_UNBOX(ARGS.orb));

	if(timeout <= 0) {
		return;
	}

	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_hghost,
		.pos = ARGS.pos,
		.color = color_mul_scalar(RGB(0.75, 0.9, 1), 0.5),
		.type = PROJ_PLAYER,
		.timeout = timeout,
		.damage = ARGS.damage,
		.shader_ptr = ctrl->shot_shader,
		.flags = PFLAG_NOAUTOREMOVE
	));

	INVOKE_TASK_AFTER(&p->events.killed, youmu_orb_homing_spirit_expire, ENT_BOX(p));

	real speed = cabs(ARGS.velocity);
	real speed_target = speed;
	real aim_strength = -0.1;
	p->move = move_accelerated(ARGS.velocity, -0.06 * ARGS.velocity);
	p->move.retention = cdir(ARGS.spin);

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

		cmplx target;

		if(orb) {
			target = orb->pos;
		} else {
			target = plrutil_homing_target(p->pos, creal(global.plr.pos) - 128*I);
		}

		cmplx aimdir = cnormalize(target - p->pos - p->move.velocity);
		capproach_asymptotic_p(&p->move.retention, 0.8, 0.1, 1e-3);
		p->move.acceleration *= 0.95;
		p->move.acceleration += aim_strength * aimdir;

		if(aim_peaked) {
			if(!orb) {
				if(approach_p(&aim_strength, 0.00, 0.001) == 0) {
					p->flags &= ~PFLAG_NOAUTOREMOVE;
				}
			}
		} else {
			approach_p(&aim_strength, 0.1, 0.02);
			if(aim_strength == 0.1) {
				aim_peaked = true;
			}
		}

		real s = fmax(speed, cabs(p->move.velocity));
		p->move.velocity = s * cnormalize(p->move.velocity + aim_strength * s * aimdir);
		approach_asymptotic_p(&speed, speed_target, 0.05, 1e-5);

		youmu_homing_trail(ctrl, p, 0.5 * p->move.velocity, 12);
		YIELD;
	}
}

static void youmu_orb_explode(YoumuBController *ctrl, Projectile *orb) {
	PARTICLE(
		.sprite_ptr = ctrl->sprites.blast_huge_rays,
		.pos = orb->pos,
		.timeout = 20,
		.color = RGBA(0.1, 0.5, 0.1, 0.0),
		.draw_rule = pdraw_timeout_scalefade_exp(0.01*(1+I), 1, 1, 0, 2),
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOMOVE,
		.angle = rng_angle(),
		.layer = LAYER_PARTICLE_LOW,
	);

	PARTICLE(
		.sprite_ptr = ctrl->sprites.blast_huge_halo,
		.pos = orb->pos,
		.timeout = 30,
		.color = RGBA(0.1, 0.1, 0.5, 0.0),
		.draw_rule = pdraw_timeout_scalefade_exp(1, 0.01*(1+I), 1, 0, 2),
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOMOVE,
		.angle = rng_angle(),
		.layer = LAYER_PARTICLE_LOW,
	);

	PARTICLE(
		.sprite_ptr = ctrl->sprites.blast_huge_halo,
		.pos = orb->pos,
		.timeout = 40,
		.color = RGBA(0.5, 0.1, 0.1, 0.0),
		.draw_rule = pdraw_timeout_scalefade_exp(0.8, -0.3*(1+I), 1, 0, 2),
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOMOVE,
		.angle = rng_angle(),
		.layer = LAYER_PARTICLE_LOW,
	);

	// TODO sound effect;
	kill_projectile(orb);
}

TASK(youmu_orb_update, { YoumuBController *ctrl; BoxedProjectile orb; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	Projectile *orb = TASK_BIND(ARGS.orb);

	for(;;) {
		float tf = glm_ease_bounce_out(1.0f - projectile_timeout_factor(orb));
		orb->color.g = tf * tf;
		orb->color.b = tf;
		orb->scale = 0.5 + 0.5 * tf;
		orb->angle += 0.2;

		if(!(plr->inputflags & INFLAG_FOCUS)) {
			youmu_orb_explode(ctrl, orb);
			break;
		}

		YIELD;
	}
}

TASK(youmu_orb_death, { BoxedProjectile orb; BoxedTask control_task; }) {
	CANCEL_TASK(ARGS.control_task);
	Projectile *orb = NOT_NULL(ENT_UNBOX(ARGS.orb));

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

TASK(youmu_orb_shot, { YoumuBController *ctrl; int lifetime; real spirit_damage; int spirit_spawn_delay; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	Projectile *orb = TASK_BIND(PROJECTILE(
		.proto = pp_youhoming,
		.pos = plr->pos,
		.color = RGB(1, 1, 1),
		.type = PROJ_PLAYER,
		.timeout = ARGS.lifetime,
		.move = move_asymptotic(-30.0*I, -0.7*I, 0.8),
		.flags = PFLAG_MANUALANGLE | PFLAG_NOCOLLISION | PFLAG_NOAUTOREMOVE,
	));

	INVOKE_TASK(youmu_orb_update, ctrl, ENT_BOX(orb));
	INVOKE_TASK_AFTER(&orb->events.killed, youmu_orb_death, ENT_BOX(orb), THIS_TASK);

	real pdmg = ARGS.spirit_damage;
	cmplx v = 5 * I;
	int spawn_delay = ARGS.spirit_spawn_delay;

	for(;;) {
		WAIT(spawn_delay);
		INVOKE_TASK(youmu_orb_homing_spirit, ctrl, orb->pos, v, pdmg,  0.1, ENT_BOX(orb));
		INVOKE_TASK(youmu_orb_homing_spirit, ctrl, orb->pos, v, pdmg, -0.1, ENT_BOX(orb));
	}
}

TASK(youmu_haunting_shot_orbs, { YoumuBController *ctrl; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		if(!(plr->inputflags & INFLAG_FOCUS)) {
			continue;
		}

		int power_rank = player_get_effective_power(plr) / 100;

		INVOKE_TASK(youmu_orb_shot,
			.ctrl = ctrl,
			.lifetime = SHOT_ORBS_LIFETIME_BASE + power_rank * SHOT_ORBS_LIFETIME_PER_POWER,
			.spirit_damage = SHOT_ORBS_SPIRIT_DAMAGE,
			.spirit_spawn_delay = SHOT_ORBS_SPIRIT_SPAWN_DELAY
		);

		WAIT(SHOT_ORBS_DELAY_BASE + power_rank * SHOT_ORBS_DELAY_PER_POWER);
	}
}

TASK(youmu_haunting_bomb_slice_petal, { YoumuBController *ctrl; cmplx pos; cmplx vel; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Projectile *p = TASK_BIND(PARTICLE(
		.sprite_ptr = ctrl->sprites.petal,
		.pos = ARGS.pos,
		.draw_rule = pdraw_petal_random(),
		.move = move_accelerated(ARGS.vel, ARGS.vel * cdir(0.1)),
		.layer = LAYER_PARTICLE_HIGH | 0x2,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
	));

	real transition_time = 40;

	for(real t = 0; t <= transition_time; ++t) {
		p->color = *color_mul_scalar(RGBA(0.2, 0.2, 1, 0), fmin(1, t / transition_time));
		YIELD;
	}
}

TASK(youmu_haunting_bomb_slice, { YoumuBController *ctrl; cmplx pos; real angle; }) {
	YoumuBController *ctrl = ARGS.ctrl;

	Projectile *p = TASK_BIND(PARTICLE(
		.sprite_ptr = ctrl->sprites.youmu_slice,
		.color = RGBA(1, 1, 1, 0),
		.pos = ARGS.pos,
		.draw_rule = {
			.func = youmu_particle_slice_draw,
			.args[0].as_ptr = &ctrl->plr->ani,
		},
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOMOVE,
		.timeout = 100,
		.angle = ARGS.angle,
		.layer = LAYER_PARTICLE_HIGH | 0x1,
	));

	cmplx petal_dir = cdir(ARGS.angle);

	for(int t = 0;; ++t) {
		real lifetime = p->timeout;
		real tt = t/lifetime;
		real a = 0;

		if(tt > 0.5) {
			a = fmax(0, 1 - (tt - 0.5) / 0.5);
		} else {
			a = fmin(1, tt / 0.2);
		}

		p->color = *RGBA(a, a, a, 0);

		if(t % 5 == 0) {
			INVOKE_TASK(youmu_haunting_bomb_slice_petal, ctrl, p->pos - 400 * petal_dir, petal_dir);
		}

		if(t % BOMB_SLICE_PERIOD == 0) {
			Ellipse e = {
				.origin = p->pos,
				.axes = CMPLX(512, 64),
				.angle = p->angle + M_PI * 0.5,
			};

			// FIXME: this may still be too slow for lasers in some cases
			stage_clear_hazards_in_ellipse(e, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
			ent_area_damage_ellipse(e, &(DamageInfo) { BOMB_SLICE_DAMAGE, DMG_PLAYER_BOMB }, NULL, NULL);
		}

		YIELD;
	}
}

TASK(youmu_haunting_bomb, { YoumuBController *ctrl; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	cmplx origin = plr->pos;

	INVOKE_SUBTASK(youmu_common_bomb_background, ENT_BOX(plr), &ctrl->bomb_bg);

	YIELD;

	int i = 0;
	do {
		cmplx ofs = cdir(i) * (100 + 10 * i * i * 0.01);
		real angle = carg(ofs) + M_PI/2;
		INVOKE_TASK(youmu_haunting_bomb_slice, ctrl, origin + ofs, angle);
		WAIT(3);
		++i;
	} while(player_is_bomb_active(plr));
}

TASK(youmu_haunting_bomb_handler, { YoumuBController *ctrl; }) {
	YoumuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	BoxedTask bomb_task = { 0 };

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.bomb_used);
		CANCEL_TASK(bomb_task);
		play_sfx("bomb_youmu_b");
		bomb_task = cotask_box(INVOKE_SUBTASK(youmu_haunting_bomb, ctrl));
	}
}

TASK(youmu_haunting_controller, { BoxedPlayer plr; }) {
	YoumuBController *ctrl = TASK_MALLOC(sizeof(*ctrl));
	ctrl->plr = TASK_BIND(ARGS.plr);
	ctrl->sprites.blast_huge_halo = res_sprite("part/blast_huge_halo");
	ctrl->sprites.blast_huge_rays = res_sprite("part/blast_huge_rays");
	ctrl->sprites.petal = res_sprite("part/petal");
	ctrl->sprites.smoothdot = res_sprite("part/smoothdot");
	ctrl->sprites.stardust = res_sprite("part/stardust");
	ctrl->sprites.youmu_slice = res_sprite("part/youmu_slice");
	ctrl->shot_shader = res_shader("sprite_particle");

	youmu_common_init_bomb_background(&ctrl->bomb_bg);

	INVOKE_SUBTASK(youmu_haunting_shot_basic, ctrl);
	INVOKE_SUBTASK(youmu_haunting_shot_homing, ctrl);
	INVOKE_SUBTASK(youmu_haunting_shot_spread, ctrl);
	INVOKE_SUBTASK(youmu_haunting_shot_orbs, ctrl);
	INVOKE_SUBTASK(youmu_haunting_bomb_handler, ctrl);

	STALL;
}

static void youmu_haunting_init(Player *plr) {
	INVOKE_TASK(youmu_haunting_controller, ENT_BOX(plr));
}

static void youmu_haunting_preload(void) {
	const int flags = RESF_DEFAULT;

	res_preload_multi(RES_SPRITE, flags,
		"proj/youmu",
		"part/youmu_slice",
	NULL);

	res_preload_multi(RES_TEXTURE, flags,
		"youmu_bombbg1",
	NULL);

	res_preload_multi(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_youmu_b",
	NULL);
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
		.init = youmu_haunting_init,
		.preload = youmu_haunting_preload,
	},
};
