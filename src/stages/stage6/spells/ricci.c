/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "common_tasks.h"

/* !!! You are entering Akari danmaku code !!! (rejuvenated by lao) */
// FIXME: This awful timing system is an relic of a shameful coroutine-less past; rewrite this.
#define SAFE_RADIUS_DELAY 300
#define SAFE_RADIUS_MIN 60
#define SAFE_RADIUS_MAX 150
#define SAFE_RADIUS_PHASE_VELOCITY 0.015
#define SAFE_RADIUS_PHASE 3*M_PI/2

static real safe_radius_phase(int time, int baryon_idx) {
	return baryon_idx * M_PI / 3 + fmax(0, time - SAFE_RADIUS_DELAY) * SAFE_RADIUS_PHASE_VELOCITY;
}

static int phase_num(real phase) {
	return (int)(phase / M_TAU);
}

static real safe_radius(real phase) {
	real base = 0.5 * (SAFE_RADIUS_MAX + SAFE_RADIUS_MIN);
	real stretch = 0.5 * (SAFE_RADIUS_MAX - SAFE_RADIUS_MIN);

	return base + stretch * 2 * (smooth(0.5 * sin(phase + SAFE_RADIUS_PHASE) + 0.5) - 0.5);
}

TASK(ricci_laser, { BoxedEllyBaryons baryons; int baryon_idx; cmplx offset; Color color; real turn_speed; real timespan; int time_offset; }) {
	Laser *l = TASK_BIND(create_laser(0, ARGS.timespan, 60, &ARGS.color, las_circle,  ARGS.turn_speed + I * ARGS.time_offset, 0, 0, 0));

	real radius = SAFE_RADIUS_MAX * difficulty_value(0.4, 0.47, 0.53, 0.6);

	for(int t = 0; t < l->deathtime + l->timespan; t++) {
		EllyBaryons *baryons = ENT_UNBOX(ARGS.baryons);
		if(baryons == NULL) {
			return;
		}

		l->pos = baryons->poss[ARGS.baryon_idx] + ARGS.offset;
		l->args[1] = radius * sin(M_PI * t / (real)(l->deathtime + l->timespan));
		l->width = clamp(t / 3.0, 0, 15);

		YIELD;
	}
}


TASK(ricci_baryon_laser_spawner, { BoxedEllyBaryons baryons; int baryon_idx; cmplx dir; }) {
	play_sfx("shot3");

	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.color = RGBA(0.0, 0.5, 0.1, 0.0),
		.flags = PFLAG_NOCOLLISION,
	));

	int delay = 30;
	cmplx offset = 14 * ARGS.dir;

	for(int t = 0; t < delay; t++) {
		p->pos = NOT_NULL(ENT_UNBOX(ARGS.baryons))->poss[ARGS.baryon_idx] + offset;
		YIELD;
	}

	// play_sound_ex("shot3",8, false);
	play_sfx("shot_special1");

	real turn_speed = 3*M_TAU/60;

	INVOKE_TASK(ricci_laser,
		.baryons = ARGS.baryons,
		.baryon_idx = ARGS.baryon_idx,
		.offset = offset,
		.color = *RGBA(0.2, 1, 0.5, 0),
		.turn_speed = turn_speed,
		.timespan = 12,
		.time_offset = 0
	);

	INVOKE_TASK(ricci_laser,
		.baryons = ARGS.baryons,
		.baryon_idx = ARGS.baryon_idx,
		.offset = offset,
		.color = *RGBA(1, 0, 0, 0),
		.turn_speed = -turn_speed,
		.timespan = 2.5,
		.time_offset = 0
	);

	INVOKE_TASK(ricci_laser,
		.baryons = ARGS.baryons,
		.baryon_idx = ARGS.baryon_idx,
		.offset = offset,
		.color = *RGBA(0.2, 0.4, 1, 0),
		.turn_speed = turn_speed,
		.timespan = 12,
		.time_offset = 30
	);

	INVOKE_TASK(ricci_laser,
		.baryons = ARGS.baryons,
		.baryon_idx = ARGS.baryon_idx,
		.offset = offset,
		.color = *RGBA(1, 0, 0, 0),
		.turn_speed = -turn_speed,
		.timespan = 2.5,
		.time_offset = 30
	);

	spawn_projectile_highlight_effect(p);
	kill_projectile(p);
}

TASK(ricci_baryons_movement, { BoxedEllyBaryons baryons; BoxedBoss boss; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);
	real baryon_speed = difficulty_value(1.00, 1.25, 1.50, 1.75);

	for(int t = 0;; t++) {
		baryons->center_pos = NOT_NULL(ENT_UNBOX(ARGS.boss))->pos;

		cmplx target = VIEWPORT_W / 2.0 + VIEWPORT_H * I * 2 / 3 - 100 * sin(baryon_speed * t / 200.0) - 25 * I * cos(baryon_speed * t * 3.0 / 500.0);

		if(t < 150) {
			baryons->target_poss[0] = global.plr.pos;
		} else {
			baryons->target_poss[0] += 0.5 * cnormalize(target - baryons->target_poss[0]);
		}

		for(int i = 1; i < NUM_BARYONS; i++) {
			if(i % 2 == 1) {
				baryons->target_poss[i] = baryons->center_pos;
			} else {
				baryons->target_poss[i] = baryons->center_pos + (baryons->poss[0] - baryons->center_pos) * cdir(M_TAU/NUM_BARYONS * i);
			}
		}

		YIELD;
	}

}

TASK(ricci_baryons, { BoxedEllyBaryons baryons; BoxedBoss boss; }) {
	auto baryons = TASK_BIND(ARGS.baryons);

	INVOKE_SUBTASK(ricci_baryons_movement, ARGS.baryons, ARGS.boss);

	bool charge_flags[NUM_BARYONS] = {};

	for(int t = 0;; t++) {
		int time = global.frames - NOT_NULL(ENT_UNBOX(ARGS.boss))->current->starttime;

		for(int i = 0; i < NUM_BARYONS; i += 2) {
			real phase = safe_radius_phase(time, i);
			int num = phase_num(phase);

			if(num == 0) {
				continue;
			}

			real phase_norm = fmod(phase, M_TAU) / M_TAU;

			if(phase_norm < 0.15 && !charge_flags[i]) {
				charge_flags[i] = true;

				for(int j = 0; j < 3; ++j) {
					int d = 10 * j;
					INVOKE_SUBTASK_DELAYED(d, common_charge,
						.anchor = &baryons->poss[i],
						.pos = 14 * cdir(M_TAU * (0.25 + j / 3.0)),
						.bind_to_entity = ARGS.baryons.as_generic,
						.time = 250 - d,
						.color = RGBA(0.02, 0.01, 0.1, 0.0),
						.sound = COMMON_CHARGE_SOUNDS,
					);
				}
			} else if(phase_norm > 0.15 && phase_norm < 0.55) {
				int count = 3;
				cmplx dir = cdir(M_TAU * (0.25 + 1.0 / count * t));
				INVOKE_TASK(ricci_baryon_laser_spawner, ARGS.baryons,
					.baryon_idx = i,
					.dir = dir
				);
			} else if(phase_norm > 0.55) {
				charge_flags[i] = false;
			}
		}

		WAIT(10);
	}
}

TASK(ricci_proj, { cmplx pos; cmplx velocity; BoxedEllyBaryons baryons; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = ARGS.pos,
		.color = RGBA(0, 0, 0, 0),
		.flags = PFLAG_NOSPAWNEFFECTS | PFLAG_NOCLEAR,
		.max_viewport_dist = SAFE_RADIUS_MAX,
	));


	for(int t = 0;; t++) {
		cmplx shift = 0;
		p->pos = ARGS.pos + ARGS.velocity * t;

		EllyBaryons *baryons = ENT_UNBOX(ARGS.baryons);
		if(baryons == NULL) {
			return;
		}

		real influence = 0;

		int time = global.frames-global.boss->current->starttime;

		for(int i = 0; i < NUM_BARYONS; i += 2) {
			real base_radius = safe_radius(safe_radius_phase(time, i));

			cmplx d = baryons->poss[i] - p->pos;
			real angular_velocity = 0.01 * difficulty_value(1.0, 1.25, 1.5, 1.75);

			int gaps = phase_num(safe_radius_phase(time, i)) + 5;

			real radius = cabs(d) / (1.0 - 0.15 * sin(gaps * carg(d) + angular_velocity * time));
			real range = 1 / (exp((radius - base_radius) / 50) + 1);
			shift += -1.1 * (base_radius - radius) / radius * d * range;
			influence += range;
		}

		if(p->type == PROJ_DEAD) {
			p->move = move_asymptotic_simple(rng_dir(), 9);
			return;
		}

		p->pos = ARGS.pos + ARGS.velocity * t + shift;

		float a = 0.5 + 0.5 * fmax(0, tanh((time - 80) / 100.)) * clamp(influence, 0.2, 1);
		a *= fmin(1, t / 20.0f);

		p->color.r = 0.5;
		p->color.g = 0;
		p->color.b = cabs(shift)/20.0;
		p->color.a = 0;
		p->opacity = a;

		YIELD;
	}
}

DEFINE_EXTERN_TASK(stage6_spell_ricci) {
	Boss *boss = stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK(ricci_baryons, ARGS.baryons, ENT_BOX(boss));

	boss->move = move_from_towards(boss->pos, BOSS_DEFAULT_GO_POS, 0.03);

	int count = 15;
	real speed = 3;
	int interval = 5;

	WAIT(60);

	for(int t = 0;; t++) {
		real offset = -VIEWPORT_W/(real)count;
		real w = VIEWPORT_W * (1 + 2.0 / count);

		for(int i = 0; i < count; i++) {
			cmplx pos = offset + fmod((i + 0.5 * t) / count, 1) * w + (VIEWPORT_H + 10) * I;

			INVOKE_TASK(ricci_proj, pos, -speed * I, ARGS.baryons);
		}

		WAIT(interval);
	}
}

/* Thank you for visiting Akari danmaku code (tm) rejuvenated by lao */

