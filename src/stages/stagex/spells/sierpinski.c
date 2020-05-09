/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "../stagex.h"
#include "stage.h"
#include "global.h"

static int idxmod(int i, int n) {   // TODO move into utils?
	return (n + i) % n;
}

TASK(splitter, {
	BoxedProjectile cell;
	int delay;
	cmplx v;
	Color *color;
}) {
	Projectile *cell = TASK_BIND(ARGS.cell);
	cmplx v = ARGS.v;
	Color c = *ARGS.color;

	WAIT(ARGS.delay);

#if 0
	PROJECTILE(
		.proto = pp_ball,
		.color = &c,
		.pos = cell->pos,
		.move = move_asymptotic_halflife(v, v + 2 * cabs(v), 80),
	);

	PROJECTILE(
		.proto = pp_ball,
		.color = &c,
		.pos = cell->pos,
		.move = move_asymptotic_halflife(v, v - 2 * cabs(v), 80),
	);
#else
	PROJECTILE(
		.proto = pp_ball,
		.color = &c,
		.pos = cell->pos,
		.move = move_accelerated(v, 0.05 * v),
	);
#endif

	kill_projectile(cell);
}

DEFINE_EXTERN_TASK(stagex_spell_sierpinski) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 260*I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	/*
	 * Rule 90 automaton with a twist
	 */

	enum { N = 129 };  // NOTE: must be odd (or it'll halt quickly)
	bool state[N] = { 0 };
	state[N/2] = 1;  // initialize with 1 bit in the middle to get a nice Sierpinski triangle pattern

	const float b = 1;
	Color colors[] = {
		{ 0.2, 0.25, 1.0, 0.0 },
		{ 1.0, 0.45, 0.1, 0.0 },
		{ 1.0, 0.65, 0.1, 0.0 },
	};
	color_mul_scalar(&colors[0], b);
	color_mul_scalar(&colors[1], b);


	for(int step = 0;; step++) {
		bool next_state[N] = { 0 };

		for(int i = 0; i < N; ++i) {
			next_state[i] = state[idxmod(i - 1, N)] ^ state[idxmod(i + 1, N)];
		}

		int sum = 0;

		for(int i = 0; i < N; ++i) {
			if(!state[i]) {
				continue;
			}
			sum++;

			cmplx dir = cdir((i + 0.5) / N * M_TAU + M_PI/2+0.0001*step);

			cmplx v = dir*3;


			Projectile *cell = PROJECTILE(
				.proto = pp_flea,
				.color = &colors[0],
				.pos = boss->pos,
				.move = move_linear(v),
			);
			/*
			Projectile *cell2 = PROJECTILE(
				.proto = pp_flea,
				.color = &colors[1],
				.pos = boss->pos+rad*conj(dir),
				.move = {conj(v), 0, 1.006*cdir(-slip/wait)},
			);*/

			/*if(
				next_state[idxmod(i - 1, N)] ^ next_state[idxmod(i + 1, N)] ^
				     state[idxmod(i - 1, N)] ^      state[idxmod(i + 1, N)] ^
				next_state[idxmod(i - 3, N)] ^ next_state[idxmod(i + 3, N)] ^
				     state[idxmod(i - 3, N)] ^      state[idxmod(i + 3, N)]
			) {
				INVOKE_TASK(splitter, ENT_BOX(cell), 160, v, &colors[1]);
			}*/
		}

		for(int i = 0; i < sqrt(N-sum)/6; i++) {
			cmplx souldir = rng_dir();
			if(i%2==0) {
				PROJECTILE(
					.proto = rng_chance(0.8) ? pp_soul : pp_bigball,
					.color = &colors[1],
					.pos = boss->pos,
					.move = move_accelerated(souldir, 0.01*I*souldir),
				);
			} else {
				PROJECTILE(
					.proto = pp_ball,
					.color = &colors[2],
					.pos = boss->pos,
					.move = move_accelerated(-souldir, 0.01*I*souldir),
				);
			}
		}

		memcpy(state, next_state, sizeof(state));
		WAIT(3);
	}
}
