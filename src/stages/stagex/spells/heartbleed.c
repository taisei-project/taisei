/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

static real heartbeat(real x) {
	// Not meant to be realistic, just tuned to look nice

	real c = cos(0.5 * x + 0.3);
	real c2 = c * c;
	real c4 = c2 * c2;

	real y = sin(x) * c4;
	y = y * y * sin(3 * x);

	return 4*y;
}

static cmplx heart(real t) {
	// Thanks to lao for getting the shape exactly right
	// https://www.desmos.com/calculator/7cpftseqb9

	t = lerp(-M_PI, M_PI, fmod(t, 1.0));

	const real a = 3.4;
	const real b = 3.0;
	const real c = 1.6;
	const real d = -0.3;
	const real h = 0.74;

	real sin_t  = sin(t);
	real sin_2t = sin(2.0 * t);

	// sin(-x) = -sin(x)
	real sin_abs_t  = t < 0 ? -sin_t  : sin_t;
	real sin_abs_2t = t < 0 ? -sin_2t : sin_2t;

	return 0.2 * CMPLX(
		a * sin_t + h * sin_2t,
		2.5 - (b * sin_abs_t + d * sin_abs_2t + c*fabs(t)));
}

typedef struct LaserRuleHeartData {
	cmplx scale;
	cmplx cwscale;
	real frequency;
	real phase;
} LaserRuleHeartData;

static cmplx laser_rule_heart_impl(Laser *l, real t, void *ruledata) {
	LaserRuleHeartData *rd = ruledata;
	cmplx h = heart(t * rd->frequency + rd->phase);
	return l->pos + cwmul(h, rd->cwscale) * rd->scale;
}

static LaserRule laser_rule_heart(cmplx scale, cmplx cwscale, real frequency, real phase) {
	LaserRuleHeartData rd = {
		.scale = scale,
		.cwscale = cwscale,
		.frequency = frequency,
		.phase = phase,
	};
	return MAKE_LASER_RULE(laser_rule_heart_impl, rd);
}

static IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_heart,
	laser_rule_heart_impl, LaserRuleHeartData)

TASK(heartbleed_half, { BoxedLaser ref; cmplx scale; cmplx cwscale; }) {
	int ltime = 60;
	int span = 2*ltime/3;

	auto ref = NOT_NULL(ENT_UNBOX(ARGS.ref));
	auto half = TASK_BIND(create_laser(ref->pos, span, ltime, RGB(1, 0, 0.25),
		laser_rule_heart(ARGS.scale, ARGS.cwscale, 0.5/ltime, 0)));

	half->width = 20;

	for(;(ref = ENT_UNBOX(ARGS.ref)); YIELD) {
		half->pos = ref->pos;
	}
}

TASK(heartbleed_halves, { BoxedLaser ref; cmplx scale; }) {
	INVOKE_TASK(heartbleed_half, ARGS.ref, ARGS.scale,  1+I);
	INVOKE_TASK(heartbleed_half, ARGS.ref, ARGS.scale, -1+I);

	play_sfx("laser1");
	WAIT(60);
	play_sfx("boon");
}

TASK(drip, { BoxedProjectile p; BoxedLaser l; real t0; real t1; }) {
	auto p = TASK_BIND(ARGS.p);
	p->flags |= PFLAG_NOMOVE | PFLAG_MANUALANGLE;
	p->angle = M_PI/2;

	Laser *l;
	int slide_time = 60;
	cmplx prev_pos = p->pos;
	real lt = ARGS.t0;
	cmplx v = 0;

	p->move = move_asymptotic_halflife(0, rng_range(7.5, 8.5)*I, 60);
	real slide_rate = rng_range(0.008, 0.01);

	for(int t = 0; t < slide_time && (l = ENT_UNBOX(ARGS.l)); ++t) {
		p->pos = laser_pos_at(l, lt) + v;
		v += I * 0.15;
		p->move.velocity = p->pos - prev_pos;
		approach_asymptotic_p(&lt, ARGS.t1, slide_rate, 1e-4);
		prev_pos = p->pos;
		YIELD;
	}

	p->flags &= ~PFLAG_NOMOVE;
}

TASK(colorshift, { BoxedProjectile p; }) {
	auto p = TASK_BIND(ARGS.p);

	Color target_color = RGBA(0, 1.0, 0.4, 1);

	WAIT(60);

	do {
		p->color = color_lerp(p->color, target_color, 0.015f);
		YIELD;
	} while(!color_equals(p->color, target_color));
}

typedef struct HeartbeatTraceData {
	MoveParams *hmove;
	int num_spawned;
} HeartbeatTraceData;

static void *heartbeat_trace_callback(Laser *l, const LaserTraceSample *sample, void *userdata) {
	HeartbeatTraceData *tdata = userdata;
	auto hmove = tdata->hmove;

	cmplx v = l->pos - sample->pos;

	auto p = PROJECTILE(
		.proto = pp_flea,
		.pos = sample->pos,
		.color = RGBA(1.0, 0.0, 0, 0),
		.move = move_asymptotic_halflife(hmove->velocity + 0.03*v, hmove->velocity - 0.03*v, 60),
		.max_viewport_dist = 32,
	);

	if(projectile_in_viewport(p)) {
		tdata->num_spawned++;
		INVOKE_TASK(colorshift, ENT_BOX(p));
	}

	return NULL;
}

TASK(heartbeat, { BoxedLaser l; MoveParams *hmove; }) {
	auto l = TASK_BIND(ARGS.l);
	auto rd = NOT_NULL(laser_get_ruledata_heart(l));

	real strength = 0;
	real target_strength = 0.1;
	int period = 86;
	int peak = 66;
	real freq = M_TAU / period;

	cmplx orig_cwscale = rd->cwscale;

	for(int t = 0;; ++t) {
		approach_asymptotic_p(&strength, target_strength, 0.1, 1e-8);
		real phase = freq * t;
		real m = heartbeat(phase) * strength;
		real n = heartbeat(phase+0.5) * strength;
		rd->cwscale = cwmul(orig_cwscale, CMPLX(1 + 0.5 * m + n, 1 - m + n));

		if((t % period) == peak) {
			HeartbeatTraceData tdata = {
				.hmove = ARGS.hmove,
			};

			laser_trace(l, 16, heartbeat_trace_callback, &tdata);

			if(tdata.num_spawned > 0) {
				play_sfx("shot_special1");
			}
		}

		YIELD;
	}
}

TASK(heartbleed, { cmplx spawn_pos; cmplx pos; cmplx pos_plrfactor; real size; }) {
	real laser_span = 64;
	cmplx spawn_pos = ARGS.spawn_pos;

	auto h = TASK_BIND(create_laser(spawn_pos, laser_span, 60 * 30, RGBA(1,0,0,0),
		laser_rule_heart(ARGS.size, 1+I, 1.0/(laser_span + 1.0), 0)));

	h->width_exponent = 0;
	h->unclearable = true;
	laser_make_static(h);

	float target_width = 30;
	int charge_time = 120;
	int move_time = 80;

	INVOKE_SUBTASK_DELAYED(move_time, heartbleed_halves, ENT_BOX(h), ARGS.size);
	INVOKE_SUBTASK(laser_charge, ENT_BOX(h), charge_time, target_width);

	cmplx plrpos_tracker = global.plr.pos;

	for(int i = 0; i < move_time; ++i) {
		real f = i / (move_time - 1.0);
		capproach_asymptotic_p(&plrpos_tracker, global.plr.pos, lerp(0.1, 0.0, f), 1e-5);

		cmplx p0 = spawn_pos;
		cmplx p1 = cwmul(plrpos_tracker, ARGS.pos_plrfactor) + ARGS.pos;
		cmplx ptarget = clerp(p0, p1, glm_ease_back_inout(f));

		h->pos = ptarget;
		YIELD;
	}

	WAIT(charge_time - move_time);

	while(h->width < target_width) {
		YIELD;
	}

	assert(h->width == target_width);

	MoveParams lmove = move_asymptotic_halflife(0, cnormalize(global.plr.pos - h->pos), 60);
	INVOKE_SUBTASK_DELAYED(0, common_move_ext, &h->pos, &lmove);
	INVOKE_SUBTASK_DELAYED(120, heartbeat, ENT_BOX(h), &lmove);

	while(h->width >= target_width) {
		real r = 0.5 + 0.25 * rng_sreal();
		real lt_spawn = laser_span * r;
		real lt_tip = laser_span * 0.5;
		cmplx p = laser_pos_at(h, lt_spawn);

		int cnt = 6;

		for(int i = 0; i < cnt; ++i) {
			auto droplet = PROJECTILE(
				.proto = rng_chance(0.5) ? pp_thickrice : pp_droplet,
				.color = h->color,
				.pos = p,
				.flags = PFLAG_NOSPAWNFLARE,
			);

			INVOKE_TASK(drip, ENT_BOX(droplet), ENT_BOX(h), lt_spawn, lt_tip);
		}

		WAIT(6);
	}

	AWAIT_SUBTASKS;
}

DEFINE_EXTERN_TASK(stagex_spell_heartbleed) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(boss->move.velocity, BOSS_DEFAULT_GO_POS, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	auto bounds = viewport_bounds(160);
	bounds.bottom = VIEWPORT_H/2;

	real hsize = 64;
	real ofs = -0.5 * hsize;
	cmplx charge_ofs = -12 - 34i;

	for(;;) {
		boss->move.attraction_point = common_wander(boss->pos, 90, bounds);

		aniplayer_hard_switch(&boss->ani, "dance", 3);
		aniplayer_queue(&boss->ani, "main", 0);

		common_charge(120, &boss->pos, charge_ofs, RGBA(1.5, 0.3, 0.5, 0.5));
		play_sfx("warp");

		cmplx spawn_pos = boss->pos + charge_ofs;

		INVOKE_TASK(heartbleed, spawn_pos,              -ofs,  I, hsize);
		INVOKE_TASK(heartbleed, spawn_pos,    VIEWPORT_W+ofs,  I, hsize);
		INVOKE_TASK(heartbleed, spawn_pos, I*(          -ofs), 1, hsize);
		INVOKE_TASK(heartbleed, spawn_pos, I*(VIEWPORT_H+ofs), 1, hsize);

		WAIT(280);
	}
}
