/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

typedef struct LaserRuleMaxwellData {
	cmplx dir;
	real amplitude;
	real phase;
} LaserRuleMaxwellData;

static cmplx maxwell_laser_rule_impl(Laser *l, real t, void *ruledata) {
	LaserRuleMaxwellData *rd = ruledata;
	return l->pos + rd->dir * t * (1 + I * rd->amplitude * 0.02 * sin(0.1 * t + rd->phase));
}

static LaserRule maxwell_laser_rule(cmplx dir, real amplitude, real phase) {
	LaserRuleMaxwellData rd = {
		.dir = dir,
		.amplitude = amplitude,
		.phase = phase,
	};
	return MAKE_LASER_RULE(maxwell_laser_rule_impl, rd);
}

static IMPL_LASER_RULE_DATAGETTER(maxwell_laser_rule_data, maxwell_laser_rule_impl, LaserRuleMaxwellData)

TASK(maxwell_laser, { cmplx pos; cmplx dir; real phase; real phase_speed; }) {
	Laser *l = TASK_BIND(
		create_laser(ARGS.pos, 200, 10000, RGBA(0, 0.2, 1, 0.0),
			maxwell_laser_rule(ARGS.dir*VIEWPORT_H*0.005, 0, ARGS.phase)));
	l->unclearable = true;
	auto rd = NOT_NULL(maxwell_laser_rule_data(l));

	INVOKE_SUBTASK(laser_charge, ENT_BOX(l), .charge_delay = 200, .target_width = 15);

	real amplitude = 0;
	real phase_speed = ARGS.phase_speed;
	real target_amplitude = difficulty_value(6.045, 6.435, 6.825, 7.215);

	INVOKE_SUBTASK_DELAYED(60, common_easing_animated, &amplitude, target_amplitude, 39, glm_ease_quad_in);

	real steepness = difficulty_value(3, 4, 5, 6);
	real fcorrection = 1.0 / tanh(steepness);

	for(real t = 0;; t += 1.0) {
		if(t == 200) {
			play_sfx_ex("laser1", 1, true);
		}

		real f = -1.0;
		if(global.diff > D_Normal) {
			f = tanh(steepness * sin(t / 120.0)) * fcorrection;
		}

		l->color.b = 0.5 + 0.5 * f;
		l->color.r = 0.5 - 0.5 * f;

		rd->phase += phase_speed * f;
		rd->amplitude = f * amplitude;
		YIELD;
	}
}

DEFINE_EXTERN_TASK(stage6_spell_maxwell) {
	Boss *boss = stage6_elly_init_scythe_attack(&ARGS);
	EllyScythe *scythe = NOT_NULL(ENT_UNBOX(ARGS.scythe));

	cmplx targetpos = ELLY_DEFAULT_POS;
	cmplx ellyofs = I*9;

	boss->move = move_from_towards(boss->pos, targetpos + ellyofs, 0.03);
	scythe->move = move_from_towards(scythe->pos, targetpos + ellyofs, 0.03);
	boss->in_background = true;

	BEGIN_BOSS_ATTACK(&ARGS.base);
	scythe->spin = 0.7;

	WAIT(40);
	aniplayer_queue(&boss->ani, "clapyohands", 1);
	aniplayer_queue(&boss->ani, "holdyohands", 0);
	int num_lasers = 24;

	real phase = 0;
	real phase_speed = difficulty_value(0.12, 0.13, 0.16, 0.16);
	int delay = difficulty_value(1, 4, 16, 16);

	for(int i = 0; i < num_lasers; i++) {
		INVOKE_TASK(maxwell_laser,
			.pos = targetpos,
			.dir = cdir(M_TAU/num_lasers*i - M_PI/2),
			.phase = phase,
			.phase_speed = phase_speed,
		);
		phase += WAIT(delay) * phase_speed;
	}

	WAIT(200);
	boss->in_background = false;

	STALL;
}
