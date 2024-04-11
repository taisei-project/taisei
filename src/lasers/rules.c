/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "rules.h"

#include "global.h"

static cmplx laser_rule_linear_impl(Laser *l, real t, void *ruledata) {
	LaserRuleLinearData *rd = ruledata;
	return l->pos + t * rd->velocity;
}

LaserRule laser_rule_linear(cmplx velocity) {
	LaserRuleLinearData rd = {
		.velocity = velocity,
	};
	return MAKE_LASER_RULE(laser_rule_linear_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_linear,
	laser_rule_linear_impl, LaserRuleLinearData)

static cmplx laser_rule_accelerated_impl(Laser *l, real t, void *ruledata) {
	LaserRuleAcceleratedData *rd = ruledata;
	return l->pos + t * (rd->velocity + t * rd->half_accel);
}

LaserRule laser_rule_accelerated(cmplx velocity, cmplx accel) {
	LaserRuleAcceleratedData rd = {
		.velocity = velocity,
		.half_accel = accel * 0.5,
	};
	return MAKE_LASER_RULE(laser_rule_accelerated_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_accelerated,
	laser_rule_accelerated_impl, LaserRuleAcceleratedData)

static cmplx laser_rule_sine_impl(Laser *l, real t, void *ruledata) {
	LaserRuleSineData *rd = ruledata;
	cmplx line_vel = rd->velocity;
	cmplx line_dir = line_vel / cabs(line_vel);
	cmplx line_normal = im(line_dir) - I*re(line_dir);
	cmplx sine_ofs = line_normal * rd->amplitude * sin(rd->frequency * t + rd->phase);
	return l->pos + t * line_vel + sine_ofs;
}

LaserRule laser_rule_sine(cmplx velocity, cmplx amplitude, real frequency, real phase) {
	LaserRuleSineData rd = {
		.velocity = velocity,
		.amplitude = amplitude,
		.frequency = frequency,
		.phase = phase,
	};
	return MAKE_LASER_RULE(laser_rule_sine_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_sine,
	laser_rule_sine_impl, LaserRuleSineData)

static cmplx laser_rule_sine_expanding_impl(Laser *l, real t, void *ruledata) {
	LaserRuleSineExpandingData *rd = ruledata;
	real angle = carg(rd->velocity);
	real speed = cabs(rd->velocity);
	real s = (rd->frequency * t + rd->phase);
	return l->pos + cdir(angle + rd->amplitude * sin(s)) * t * speed;
}

LaserRule laser_rule_sine_expanding(cmplx velocity, real amplitude, real frequency, real phase) {
	LaserRuleSineExpandingData rd = {
		.velocity = velocity,
		.amplitude = amplitude,
		.frequency = frequency,
		.phase = phase,
	};
	return MAKE_LASER_RULE(laser_rule_sine_expanding_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_sine_expanding,
	laser_rule_sine_expanding_impl, LaserRuleSineExpandingData)

static cmplx laser_rule_arc_impl(Laser *l, real t, void *ruledata) {
	LaserRuleArcData *rd = ruledata;
	return l->pos + rd->radius * cdir(rd->turn_speed * (t + rd->time_ofs));
}

LaserRule laser_rule_arc(cmplx radius, real turnspeed, real timeofs) {
	LaserRuleArcData rd = {
		.radius = radius,
		.turn_speed = turnspeed,
		.time_ofs = timeofs,
	};
	return MAKE_LASER_RULE(laser_rule_arc_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_arc,
	laser_rule_arc_impl, LaserRuleArcData)

static cmplx laser_rule_dynamic_impl(Laser *l, real t, void *ruledata) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	LaserRuleDynamicData *rd = ruledata;
	assert(cotask_unbox(rd->control_task));
	LaserRuleDynamicTaskData *td = rd->task_data;

	assert(td->history.num_elements > 0);

	real tbase = (global.frames - l->birthtime) * l->speed;
	real tofsraw = t - tbase;
	real tofs = clamp(tofsraw, 1 - td->history.num_elements, 0);

	int i0 = floor(tofs);
	int i1 = ceil(tofs);
	real ifract = (tofs - floor(tofs));

	cmplx v0 = *NOT_NULL(ringbuf_peek_ptr(&td->history, -i0));
	cmplx v1 = *NOT_NULL(ringbuf_peek_ptr(&td->history, -i1));
	return clerp(v0, v1, ifract);
}

static LaserRule laser_rule_dynamic(
	BoxedTask control_task, LaserRuleDynamicTaskData *task_data
) {
	LaserRuleDynamicData rd = {
		.control_task = control_task,
		.task_data = task_data,
	};
	return MAKE_LASER_RULE(laser_rule_dynamic_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_dynamic,
	laser_rule_dynamic_impl, LaserRuleDynamicData)

TASK(laser_dynamic, {
	Laser **out_laser;
	cmplx pos;
	float timespan;
	float deathtime;
	const Color *color;
}) {
	int histsize = ceilf(ARGS.timespan) + 2;
	assume(histsize > 2);

	cmplx *history_data = TASK_MALLOC(sizeof(*history_data) * histsize);
	LaserRuleDynamicTaskData td = {
		.history = RING_BUFFER_INIT(history_data, histsize)
	};

	auto l = TASK_BIND(create_laser(
		ARGS.pos, ARGS.timespan, ARGS.deathtime, ARGS.color,
		laser_rule_dynamic(THIS_TASK, &td)
	));

	*ARGS.out_laser = l;
	ringbuf_push(&td.history, l->pos);  // HACK: this works around some edge cases but we can probably
	ringbuf_push(&td.history, l->pos);  //       make the sampling function more robust instead?
	ringbuf_push(&td.history, l->pos);
	YIELD;

	for(;global.frames - l->birthtime <= l->deathtime; YIELD) {
		move_update(&l->pos, &td.move);
		ringbuf_push(&td.history, l->pos);
	}

	STALL;
}

Laser *create_dynamic_laser(cmplx pos, float time, float deathtime, const Color *color) {
	Laser *l;
	INVOKE_TASK(laser_dynamic, &l, pos, time, deathtime, color);
	return l;
}

/*
 * Legacy rules for compatibility (TODO remove)
 */

static cmplx laser_rule_compat_impl(Laser *l, real t, void *ruledata) {
	LaserRuleCompatData *rd = ruledata;
	return rd->oldrule(l, t);
}

LaserRule laser_rule_compat(LegacyLaserPosRule oldrule, cmplx a0, cmplx a1, cmplx a2, cmplx a3) {
	LaserRuleCompatData rd = {
		.oldrule = oldrule,
		.args = { a0, a1, a2, a3 },
	};

	return MAKE_LASER_RULE(laser_rule_compat_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_compat,
	laser_rule_compat_impl, LaserRuleCompatData)

DIAGNOSTIC(ignored "-Wdeprecated-declarations")

cmplx las_linear(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	return l->pos + l->args[0]*t;
}

cmplx las_accel(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	return l->pos + l->args[0]*t + 0.5*l->args[1]*t*t;
}

cmplx las_sine(Laser *l, float t) {               // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// this is actually shaped like a sine wave

	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx line_vel = l->args[0];
	cmplx line_dir = line_vel / cabs(line_vel);
	cmplx line_normal = im(line_dir) - I*re(line_dir);
	cmplx sine_amp = l->args[1];
	real sine_freq = re(l->args[2]);
	real sine_phase = re(l->args[3]);

	cmplx sine_ofs = line_normal * sine_amp * sin(sine_freq * t + sine_phase);
	return l->pos + t * line_vel + sine_ofs;
}

cmplx las_sine_expanding(Laser *l, float t) { // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase

	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx velocity = l->args[0];
	real amplitude = re(l->args[1]);
	real frequency = re(l->args[2]);
	real phase = re(l->args[3]);

	real angle = carg(velocity);
	real speed = cabs(velocity);

	real s = (frequency * t + phase);
	return l->pos + cdir(angle + amplitude * sin(s)) * t * speed;
}

cmplx las_turning(Laser *l, float t) { // [0] = vel0; [1] = vel1; [2] r: turn begin time, i: turn end time
	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx v0 = l->args[0];
	cmplx v1 = l->args[1];
	float begin = re(l->args[2]);
	float end = im(l->args[2]);

	float a = clamp((t - begin) / (end - begin), 0, 1);
	a = 1.0 - (0.5 + 0.5 * cos(a * M_PI));
	a = 1.0 - pow(1.0 - a, 2);

	cmplx v = v1 * a + v0 * (1 - a);

	return l->pos + v * t;
}

cmplx las_circle(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	real turn_speed = re(l->args[0]);
	real time_ofs = im(l->args[0]);
	real radius = re(l->args[1]);

	return l->pos + radius * cdir(turn_speed * (t + time_ofs));
}

