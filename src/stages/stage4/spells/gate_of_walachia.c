/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "common_tasks.h"
#include "spells.h"
#include "../kurumi.h"

#include "global.h"

TASK(kurumi_walachia_slave_move_turn, { cmplx *vel; int duration; cmplx offset; }) {
	for(int i = 0; i < ARGS.duration; i++, YIELD) {
		*ARGS.vel -= 0.02 * ARGS.offset;
		*ARGS.vel *= cdir(0.02);
	}
}

TASK(kurumi_walachia_slave_move, { cmplx *pos; cmplx *vel; cmplx direction; }) {
	INVOKE_SUBTASK_DELAYED(40, kurumi_walachia_slave_move_turn, ARGS.vel, 60, ARGS.direction);
	
	for(int i = 0;; i++, YIELD) {
		*ARGS.pos += 2 * *ARGS.vel * (sin(i / 10.0) + 1.5);
	}

}	

TASK(kurumi_walachia_slave, { cmplx pos; cmplx direction; int lifetime; }) {
	cmplx pos = ARGS.pos;
	cmplx vel = ARGS.direction;

	INVOKE_SUBTASK(stage4_boss_slave_visual, &pos, .interval = 1);

	INVOKE_SUBTASK(kurumi_walachia_slave_move, &pos, &vel, ARGS.direction);

	int step = difficulty_value(16, 14, 12, 10);
	for(int i = 0; i < ARGS.lifetime; i += WAIT(step)) {
		float r = cimag(pos)/VIEWPORT_H;

		for(int j = 1; j >= -1; j -= 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = pos + j * 10.0 * I * ARGS.direction,
				.color = RGB(r,0,0),
				.move = move_accelerated(j * 2.0 * I * ARGS.direction, -0.01*vel)
			);
		}

		play_sfx("shot1");
	}
}

DEFINE_EXTERN_TASK(kurumi_walachia) {
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	int slave_count = difficulty_value(5, 7, 9, 11);
	int duration = 400;


	for(int run = 0;; run++) {
		INVOKE_SUBTASK(common_charge, .pos = b->pos, .time = 40, .color = RGBA(1.0, 0, 0, 0));
		for(int i = 0; i < slave_count; i++) {
			INVOKE_SUBTASK(kurumi_walachia_slave,
				.pos = b->pos,
				.direction = cdir(M_TAU / slave_count * i + 0.16 * run),
				.lifetime = duration + 200);
		}
		WAIT(duration);
	}
}
