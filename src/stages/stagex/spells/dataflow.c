/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

TASK(dataline_bit, { BoxedLaser l; real speed; }) {
	Laser *l = NOT_NULL(ENT_UNBOX(ARGS.l));

	play_sfx_loop("shot1_loop");

	auto p = TASK_BIND(PROJECTILE(
		.pos = laser_pos_at(l, l->timeshift),
		.proto = pp_ball,
		.color = l->color,
		.flags = PFLAG_NOMOVE | PFLAG_NOAUTOREMOVE | PFLAG_NOSPAWNEFFECTS | PFLAG_MANUALANGLE,
	));

	real speed = ARGS.speed;

	for(real t = 0; (l = ENT_UNBOX(ARGS.l)) && t < l->deathtime; t += speed, YIELD) {
		p->pos = laser_pos_at(l, l->timeshift - t);
	}

	assert(!projectile_in_viewport(p));
	kill_projectile(p);
}

TASK(dataline_flipper, { bool *gate; int duration; }) {
	int timings[] = {
		// 15, 20, 15, 25, 15, 15, 20, 15, 25
		15, 30, 15, 30, 30, 60
	};

	// int i = countof(timings) * rng_real();

	// pick a random even starting index, so that uptimes are always shorter than downtimes
	int i = 2 * (int)((countof(timings) / 2) * rng_real());
	static_assert(!(countof(timings) & 1));  // must be even for that to work

	assert(!*ARGS.gate);

	for(int t = 0; t < ARGS.duration;) {
		*ARGS.gate = !*ARGS.gate;
		t += WAIT(timings[i]);
		i = (i + 1) % countof(timings);
	}

	*ARGS.gate = false;
}

TASK(dataline, { cmplx pos; }) {
	cmplx direction;
	Color color;

	if(im(ARGS.pos)) {
		assume(im(ARGS.pos) == VIEWPORT_H);
		direction = -I;
		color = RGBA(1, 0, 0, 0);
	} else {
		direction = I;
		color = RGBA(0, 0, 1, 0);
	}

	int lifetime = 60 * 12;
	real slide_speed = 0.5;

	real maxlen = cabs(VIEWPORT_W + VIEWPORT_H*I);  // approximation of max visible laser length
	real step = 8;  // distance stepped per laser-time unit
	real timespan = maxlen / step + lifetime * slide_speed;

	real transmission_rate = 0.5;  // in laser-time units per frame

	// this should be large enough that by the end of the laser's life, all transmitted "bits" have gone offscreen
	int transmission_latency = maxlen / (step * transmission_rate - slide_speed);
	transmission_latency += 60;  // asspull

	int transmission_delay = 60;
	int transmission_duration = lifetime - transmission_delay - transmission_latency;
	assert(transmission_duration > 0);

	// init laser_rule_dynamic() manually instead of using create_dynamic_laser(),
	// because we want to precompute the whole path instead of tracking laser->pos over time

	int histsize = ceil(timespan);
	cmplx history_data[histsize];

	LaserRuleDynamicTaskData td = {
		.history = RING_BUFFER_INIT(history_data, histsize)
	};

	auto l = TASK_BIND(create_laser(ARGS.pos, timespan, lifetime, color, laser_rule_dynamic(THIS_TASK, &td)));
	l->width_exponent = 0;

	// path generation: randomized 45-degree bends with fixed-length diagonals

	cmplx p = ARGS.pos;
	cmplx forward = step * direction;
	cmplx v = forward;
	cmplx turns[] = { forward * cdir(M_PI/4), forward * cdir(-M_PI/4) };
	int turn_steps = 0;
	int forward_steps = 0;

	for(int i = 0; i < histsize; ++i) {
		ringbuf_push(&td.history, p);

		if(v == forward) {
			if(forward_steps) {
				--forward_steps;
			} else if(rng_chance(0.05)) {
				v = turns[rng_bool()];
				turn_steps = 10;
			}
		} else {
			if(!--turn_steps) {
				v = forward;
				forward_steps = 10;
			}
		}

		p += v;
	}

	// behold,
	// a static dynamic laser.
	laser_make_static(l);

	cmplx slide_dir = -slide_speed * direction;
	slide_dir += 0.25 * (re(ARGS.pos) > VIEWPORT_W*0.5 ? -1 : 1);

	bool on = false;
	INVOKE_SUBTASK_DELAYED(transmission_delay, dataline_flipper,
		.gate = &on,
		.duration = transmission_duration
	);

	play_sfx("boon");

	for(;;) {
		laser_charge(l, global.frames - l->birthtime, 30, 3);
		l->collision_active = false;

		YIELD;
		td.offset += slide_dir;

		if(on) {
			INVOKE_TASK(dataline_bit, ENT_BOX(l), transmission_rate);
		}
	}
}

DEFINE_EXTERN_TASK(stagex_spell_dataflow) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(boss->move.velocity, BOSS_DEFAULT_GO_POS, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	auto bounds = viewport_bounds(160);
	bounds.bottom = VIEWPORT_H/2;

	real spread = 160;

	for(;;) {
		INVOKE_SUBTASK_DELAYED(60, common_charge,
			.color = RGBA(0.2, 0.2, 1, 0),
			.time = 120,
			.anchor = &boss->pos,
			.sound = COMMON_CHARGE_SOUNDS,
		);
		common_charge(120, &boss->pos, 0, RGBA(1, 0.2, 0.2, 0));

		cmplx tx_base = re(global.plr.pos);
		cmplx rx_base = re(boss->pos) + VIEWPORT_H*I;

		INVOKE_TASK(dataline, rx_base);
		INVOKE_TASK(dataline, rx_base + spread);
		INVOKE_TASK(dataline, rx_base - spread);
		INVOKE_TASK_DELAYED(60, dataline, tx_base);
		INVOKE_TASK_DELAYED(60, dataline, tx_base + spread);
		INVOKE_TASK_DELAYED(60, dataline, tx_base - spread);

		WAIT(180);
		boss->move.attraction_point = common_wander(boss->pos, 90, bounds);
	}
}
