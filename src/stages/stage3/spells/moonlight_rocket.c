/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

TASK(aimcircle, { int lifetime; int focustime; }) {
	auto p = TASK_BIND(PARTICLE(
		.sprite_ptr = res_sprite("fairy_circle"),
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOAUTOREMOVE,
		.pos = global.plr.pos,
		.angle_delta = M_TAU/60,
		.color = RGB(2, 0.75, 0.5),
	));

	int spawntime = 30;
	int despawntime = 30;

	for(int t = 0; t < ARGS.lifetime; ++t, YIELD) {
		if(t < ARGS.focustime) {
			capproach_asymptotic_p(&p->pos, global.plr.pos, 0.2, 1e-3);
		} else if(t == ARGS.focustime) {
			p->pos = global.plr.pos;
		}

		float sf = min(t / (float)spawntime, 1.0f);
		float sf2 = min(t / (float)(spawntime * 2), 1.0f);
		float df = min((ARGS.lifetime - t - 1) / (float)despawntime, 1.0f);

		df = glm_ease_back_out(df);
		p->opacity = glm_ease_quad_out(sf) * glm_ease_quad_in(df) * 0.1f;
		sf = glm_ease_quad_in(sf);
		sf = glm_ease_back_out(sf) * 0.9 + sf * 0.1;

		p->scale = (1+I) * (1.0f + 6 * (1 - sf));
		p->angle -= p->angle_delta * 10.0f * (1 - sf2);
	}

	kill_projectile(p);
}

TASK(cancel_event, { CoEvent *event; }) {
	coevent_cancel(ARGS.event);
}

TASK(laser_bullet, { BoxedProjectile p; BoxedLaser l; CoEvent *event; int event_time; }) {
	Laser *l = NOT_NULL(ENT_UNBOX(ARGS.l));

	if(ARGS.event) {
		INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, cancel_event, ARGS.event);
	}

	Projectile *p = TASK_BIND(ARGS.p);

	for(int t = 0; (l = ENT_UNBOX(ARGS.l)); ++t, YIELD) {
		p->pos = l->prule(l, t);

		if(t == 0) {
			p->prevpos = p->pos;
		}

		if(t == ARGS.event_time && ARGS.event) {
			coevent_signal(ARGS.event);
			break;
		}
	}

	kill_projectile(p);
}

TASK(rocket, { BoxedBoss boss; cmplx pos; cmplx dir; Color color; real phase; real accel_rate; int rocket_time; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real dt = ARGS.rocket_time;
	Laser *l = create_lasercurve4c(
		ARGS.pos, dt, dt, &ARGS.color, las_sine_expanding, 2.5*ARGS.dir, M_PI/20, 0.2, ARGS.phase
	);

	Projectile *p = PROJECTILE(
		.proto = pp_ball,
		.color = RGB(1.0, 0.4, 0.6),
		.flags = PFLAG_NOMOVE,
	);

	COEVENTS_ARRAY(phase2, explosion) events;
	TASK_HOST_EVENTS(events);

	INVOKE_TASK(laser_bullet, ENT_BOX(p), ENT_BOX(l), &events.phase2, dt);
	WAIT_EVENT_OR_DIE(&events.phase2);

	p = PROJECTILE(
		.pos = p->pos,
		.proto = pp_plainball,
		.color = RGB(1.0, 0.4, 0.6),
		.flags = PFLAG_NOMOVE,
	);

	play_sfx("redirect");

	cmplx dist = global.plr.pos - p->pos;
	cmplx accel = ARGS.accel_rate * cnormalize(dist);
	dt = sqrt(2 * cabs(dist) / ARGS.accel_rate);
	dt += 2 * rng_f64s();

	l = create_lasercurve2c(p->pos, dt, dt, RGBA(0.4, 0.9, 1.0, 0.0), las_accel, 0, accel);
	l->width = 15;
	INVOKE_TASK(laser_bullet, ENT_BOX(p), ENT_BOX(l), &events.explosion, dt);
	WAIT_EVENT_OR_DIE(&events.explosion);
	// if we get here, p must be still alive and valid

	int cnt = 22;
	real rot = (global.frames - NOT_NULL(boss->current)->starttime) * 0.0037 * global.diff;
	Color *c = HSLA(fmod(rot, M_TAU) / (M_TAU), 1.0, 0.5, 0);
	real boost = difficulty_value(4, 6, 8, 10);

	for(int i = 0; i < cnt; ++i) {
		real f = (real)i/cnt;

		cmplx dir = cdir(M_TAU * f + rot);
		cmplx v = (1.0 + psin(M_TAU * 9 * f)) * dir;

		PROJECTILE(
			.proto = pp_thickrice,
			.pos = p->pos,
			.color = c,
			.move = move_asymptotic_simple(v, boost),
		);
	}

	real to = rng_range(30, 35);
	real scale = rng_range(2, 2.5);

	PARTICLE(
		.proto = pp_blast,
		.pos = p->pos,
		.color = c,
		.timeout = to,
		.draw_rule = pdraw_timeout_scalefade(0.01, scale, 1, 0),
		.angle = rng_angle(),
	);

	// FIXME: better sound
	play_sfx("enemydeath");
	play_sfx("shot1");
	play_sfx("shot_special1");

	kill_projectile(p);
}

TASK(rocket_slave, { BoxedBoss boss; real rot_speed; real rot_initial; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	cmplx dir;

	WriggleSlave *slave = stage3_host_wriggle_slave(boss->pos);
	INVOKE_SUBTASK(wriggle_slave_damage_trail, ENT_BOX(slave));
	INVOKE_SUBTASK(wriggle_slave_follow,
		.slave = ENT_BOX(slave),
		.boss = ENT_BOX(boss),
		.rot_speed = ARGS.rot_speed,
		.rot_initial = ARGS.rot_initial,
		.out_dir = &dir
	);

	int rocket_time = 60;
	int warn_time = 20;
	int rperiod = difficulty_value(220, 200, 180, 160);
	real laccel = difficulty_value(0.15, 0.2, 0.25, 0.3);

	WAIT(rperiod/2);

	for(;;WAIT(rperiod)) {
		play_sfx("laser1");
		INVOKE_TASK_DELAYED(rocket_time - warn_time, aimcircle, 60 + warn_time, warn_time);
		INVOKE_TASK(rocket, ENT_BOX(boss), slave->pos, dir, *RGBA(1.0, 1.0, 0.5, 0.0), 0, laccel, rocket_time);
		INVOKE_TASK(rocket, ENT_BOX(boss), slave->pos, dir, *RGBA(0.5, 1.0, 0.5, 0.0), M_PI, laccel, rocket_time);
	}
}

DEFINE_EXTERN_TASK(stage3_spell_moonlight_rocket) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + VIEWPORT_H*I/2.5, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);

	int nslaves = difficulty_value(2, 3, 4, 5);

	for(int i = 0; i < nslaves; ++i) {
		INVOKE_SUBTASK(rocket_slave, ENT_BOX(boss),  1/70.0,  i*M_TAU/nslaves);
		INVOKE_SUBTASK(rocket_slave, ENT_BOX(boss), -1/70.0, -i*M_TAU/nslaves);
	}

	// keep subtasks alive
	STALL;
}
