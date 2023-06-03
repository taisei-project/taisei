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

static cmplx maxwell_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->unclearable = true;
		return 0;
	}

	cmplx direction = l->args[0];
	real amplitude = creal(l->args[2]);
	real phase = cimag(l->args[2]);

	return l->pos + direction * t * (1 + I * amplitude * 0.02 * sin(0.1 * t + phase));
}

TASK(maxwell_laser, { cmplx pos; cmplx dir; real phase; real phase_speed; }) {
	Laser *l = TASK_BIND(create_laser(ARGS.pos, 200, 10000, RGBA(0, 0.2, 1, 0.0), maxwell_laser, ARGS.dir*VIEWPORT_H*0.005, 0, 0, 0));

	INVOKE_SUBTASK(laser_charge, ENT_BOX(l), .charge_delay = 200, .target_width = 15);

	real phase = ARGS.phase;
	real phase_speed = ARGS.phase_speed;
	real amplitude = 0;
	real target_amplitude = difficulty_value(6.045, 6.435, 6.825, 7.215);

	INVOKE_SUBTASK_DELAYED(60, common_easing_animated, &amplitude, target_amplitude, 39, glm_ease_quad_in);

	real steepness = difficulty_value(3, 4, 5, 6);
	real fcorrection = 1.0 / tanh(steepness);

	for(real t = 0;; t += 1.0) {
		if(t == 200) {
			play_sfx_ex("laser1", 1, true);
		}

		real f = tanh(steepness * sin(t / 120.0)) * fcorrection;
		l->color.b = 0.5 + 0.5 * f;
		l->color.r = 0.5 - 0.5 * f;

		phase += phase_speed * f;
		l->args[2] = f * amplitude + I * phase;
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
	real phase_speed = difficulty_value(0.12, 0.14, 0.16, 0.16);
	int delay = difficulty_value(20, 17, 16, 16);

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
