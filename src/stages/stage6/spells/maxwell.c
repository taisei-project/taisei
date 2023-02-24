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

TASK(maxwell_laser, { cmplx pos; cmplx dir; }) {
	Laser *l = TASK_BIND(create_laser(ARGS.pos, 200, 10000, RGBA(0, 0.2, 1, 0.0), maxwell_laser, ARGS.dir*VIEWPORT_H*0.005, 0, 0, 0));

	INVOKE_SUBTASK(laser_charge, ENT_BOX(l), .charge_delay = 200, .target_width = 15);

	real phase = 0;
	real amplitude = 0;
	real phase_speed = difficulty_value(0.12, 0.14, 0.16, 0.13);
	real target_amplitude = difficulty_value(6.045, 6.435, 6.825, 7.215);

	INVOKE_SUBTASK_DELAYED(60, common_easing_animated, &amplitude, target_amplitude, 39, glm_ease_quad_in);

	for(;;) {
		phase += -phase_speed;
		l->args[2] = amplitude + I * phase;
		YIELD;
	}

}

DEFINE_EXTERN_TASK(stage6_spell_maxwell) {
	Boss *boss = stage6_elly_init_scythe_attack(&ARGS);
	EllyScythe *scythe = NOT_NULL(ENT_UNBOX(ARGS.scythe));
	scythe->move = move_towards(boss->pos, 0.03);
	BEGIN_BOSS_ATTACK(&ARGS.base);
	scythe->spin = 0.7;

	WAIT(40);
	int num_lasers = 24;

	for(int i = 0; i < num_lasers; i++) {
		INVOKE_TASK(maxwell_laser, .pos = boss->pos, .dir = cdir(M_TAU/num_lasers*i));

		WAIT(5);
	}

	WAIT(90);
	elly_clap(boss, 50);
	play_sfx("laser1");

	STALL;
}
