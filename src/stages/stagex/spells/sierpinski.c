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
	return (i >= 0) ? i % n : (n + i) % n;
}

DEFINE_EXTERN_TASK(stagex_spell_sierpinski) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 120*I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	/*
	 * Rule 90 automaton with a twist
	 */

	enum { N = 77 };  // NOTE: must be odd (or it'll halt quickly)
	bool state[N] = { 0 };
	state[N/2] = 1;  // initialize with 1 bit in the middle to get a nice Sierpinski triangle pattern

	for(;;) {
		bool next_state[N] = { 0 };

		for(int i = 0; i < N; ++i) {
			next_state[i] = state[idxmod(i - 1, N)] ^ state[idxmod(i + 1, N)];
		}

		for(int i = 0; i < N; ++i) {
			if(!state[i]) {
				continue;
			}

			real p = ((i + 0.5) / N) * VIEWPORT_W;
			cmplx v = I * 1.0625;

			if(
				next_state[idxmod(i - 1, N)] ^ next_state[idxmod(i + 1, N)] ^
				     state[idxmod(i - 1, N)] ^      state[idxmod(i + 1, N)]
			) {
				PROJECTILE(
					.proto = pp_ball,
					.color = RGBA(0, 0, 1, 0),
					.pos = p,
					.move = move_linear(v),
					.flags = PFLAG_NOCLEAR | PFLAG_NOCLEARBONUS | PFLAG_NOCOLLISION,
					.opacity = 0.2f,
				);

				PROJECTILE(
					.proto = pp_thickrice,
					.color = RGBA(1, 0, 0, 1),
					.pos = p,
					.move = move_asymptotic_halflife(v, v + 2*creal(v * cdir(M_TAU/8)), 200),
				);

				PROJECTILE(
					.proto = pp_thickrice,
					.color = RGBA(1, 0, 0, 1),
					.pos = p,
					.move = move_asymptotic_halflife(v, v + 2*creal(v / cdir(M_TAU/8)), 200),
				);
			} else {
				PROJECTILE(
					.proto = pp_ball,
					.color = RGBA(0, 0, 1, 0),
					.pos = p,
					.move = move_linear(v),
				);
			}
		}

		memcpy(state, next_state, sizeof(state));
		WAIT(16);
	}
}
