/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "coroutine/taskdsl.h"
#include "dynarray.h"
#include "entity.h"
#include "move.h"
#include "projectile.h"
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

TASK(hilbert_curve, { cmplx center; cmplx size; int levels; }) {

	int length = 1<<(2*ARGS.levels);

	cmplx positions[length];
	int turns[length];
	cmplx new_shifts[length];

	auto l = TASK_BIND(create_chain_laser(1000, RGBA(0.9,0.4,0.9,0.2), length, positions));
	laser_make_static(l);
	l->unclearable = true;

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
	
	int lerp_time = 100;
	
	for(int l = 0; l < ARGS.levels-1; l++) {
		reduced_length /= 4;
		for(int i = 0; i < length/reduced_length/4; i++) {
			for(int k = 0; k < 4; k++) {
				for(int j = 0; j < reduced_length; j++) {
					int idx = (i*4+k)*reduced_length + j;
					int turn = turns[idx];
					turns[idx] = turn_table[turn][k];
					
					new_shifts[idx] = a * hilbert_shift(turn, k);
				}
			}
		}
		a /= 2;
				
		for(int t = 0; t < lerp_time; t++) {
			for(int i = 0; i < length; i++) {
				positions[i] += new_shifts[i] / lerp_time;
			}

			YIELD;
		}
		WAIT(60);
	}

	STALL;
}

DEFINE_EXTERN_TASK(stagex_spell_hilbert_curve) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);



	for(int i = 0;; i++) {
		INVOKE_TASK(hilbert_curve, (VIEWPORT_W + I * VIEWPORT_H)/2, 250*(1 - I*1), 6);
		WAIT(1000);
	}
}
