/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "audio/audio.h"
#include "coroutine/taskdsl.h"
#include "dynarray.h"
#include "entity.h"
#include "move.h"
#include "projectile.h"
#include "random.h"
#include "stages/stagex/yumemi.h"
#include "taisei.h"

#include "../stagex.h"
#include "global.h"
#include "util/glm.h"
#include "common_tasks.h"
#include "stage.h"


static cmplx hilbert_shift(int turn, int k) {
	int offset = 1-(turn & 2);
	int dir = 1-2 * (turn & 1);
	return offset * cdir(M_TAU/4 * k * dir);
}

TASK(hilbert_curve, { cmplx center; cmplx size; int levels; int lerp_time; int lifetime; }) {
	int length = 1<<(2*ARGS.levels);

	cmplx positions[length];
	int turns[length];
	cmplx new_shifts[length];

	auto l = TASK_BIND(create_chain_laser(ARGS.lifetime, RGBA(0.9,0.4,0.9,0.2), length, positions));
	laser_make_static(l);
	l->unclearable = true;
	l->width_exponent = 0;

	int turn_table[4][4] = {
		{1, 0, 0, 3},
		{0, 1, 1, 2},
		{3, 2, 2, 1},
		{2, 3, 3, 0},
	};

	cmplx a = ARGS.size;

	for(int k = 0; k < 4; k++) {
		for(int i = 0; i < length/4; i++) {
			int idx = k * (length / 4) + i;
			positions[idx] = ARGS.center + a * hilbert_shift(0, k);
			new_shifts[idx] = 0;
			turns[idx] = turn_table[0][k];
		}
	}

	a /= 2;
	int reduced_length = length / 4;

	for(int l = 0; l < ARGS.levels-1; l++) {
		play_sfx("laser1");
		reduced_length /= 4;
		for(int i = 0; i < length/reduced_length/4; i++) {
			for(int k = 0; k < 4; k++) {
				for(int j = 0; j < reduced_length; j++) {
					int idx = (i*4+k)*reduced_length + j;
					int turn = turns[idx];
					turns[idx] = turn_table[turn][k];

					new_shifts[idx] = a * hilbert_shift(turn, k);
					new_shifts[idx] -= 0.1*(positions[idx] + new_shifts[idx] - ARGS.center);
				}
			}
		}
		a /= 2/0.9;

		for(int t = 0; t < ARGS.lerp_time; t++) {
			for(int i = 0; i < length; i++) {
				positions[i] += new_shifts[i] / ARGS.lerp_time;
			}

			YIELD;
		}
	}

	STALL;
}

TASK(data_packet_bullet, { cmplx pos; cmplx target; bool vert; }) {
	cmplx corner = ARGS.vert ? re(ARGS.pos) + I * im(ARGS.target) : re(ARGS.target) + I * im(ARGS.pos);

	real speed = 2;
	play_sfx("shot2");

	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_bullet,
		.color = RGB(0,0,0.8),
		.pos = ARGS.pos,
		.move = move_linear(speed * cnormalize(corner-ARGS.pos)),
	));

	int delay = cabs(corner-ARGS.pos) / speed;

	WAIT(delay);
	play_sfx("redirect");
	cmplx new_velocity = speed * cnormalize(ARGS.target-corner);
	if(cabs(new_velocity) == 0) {
		new_velocity = p->move.velocity * I;
	}
	p->move = move_linear(new_velocity);
}

DEFINE_EXTERN_TASK(stagex_spell_hilbert_curve) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	int levels = 6;
	int lerp_time = 100;
	int packet_interval = 100;

	for(;;) {
		int num_packets = 5 + 3 * rng_chance(0.01); // let’s mess with people

		int laser_lifetime = levels * lerp_time + num_packets * packet_interval;


		boss->move = move_towards(0, rng_range(-200,200) + VIEWPORT_W/2 + 100 * I, 0.01);
		INVOKE_TASK(hilbert_curve, (VIEWPORT_W + I * VIEWPORT_H)/2, 1.5*VIEWPORT_W/2*(-1 + I*1), levels, lerp_time, .lifetime = laser_lifetime);
		WAIT(levels * lerp_time-40);

		for(int r = 0; r < num_packets; r++) {
			play_sfx("shot_special1");
			cmplx target = global.plr.pos;
			for(int c = 0; c < 4; c++) {
				INVOKE_TASK_DELAYED(5 * c, data_packet_bullet, .pos = boss->pos, .target = target, .vert = true);
				INVOKE_TASK_DELAYED(5 * c, data_packet_bullet, .pos = boss->pos, .target = target, .vert = false);
			}
			WAIT(packet_interval);
			boss->move = move_towards(0, rng_range(-200,200) + VIEWPORT_W/2 + 100 * I, 0.01);
		}
	}
}
