/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "../stagex.h"
#include "../yumemi.h"

#include "stage.h"
#include "global.h"
#include "common_tasks.h"

static int idxmod(int i, int n) {   // TODO move into utils?
	return (n + i) % n;
}

TASK(rule90, { cmplx origin; int duration; }) {
	/*
	 * Rule 90 automaton with a twist
	 */

	enum { N = 65 };  // NOTE: must be odd (or it'll halt quickly)
	bool state[N] = { 0 };
	// initialize with 1 bit in the middle to get a nice Sierpinski triangle pattern
	state[N/2] = 1;

	static Color color0 = { 0.2, 0.25, 1.0, 1.0 };
	static Color color1 = { 1.2, 0.25, 0.8, 1.0 };

	int delay = 3;
	int ncycles = ARGS.duration / delay;

	for(int cycle = 0; cycle < ncycles; ++cycle, WAIT(3)) {
		bool next_state[N] = { 0 };

		for(int i = 0; i < N; ++i) {
			next_state[i] = state[idxmod(i - 1, N)] ^ state[idxmod(i + 1, N)];
		}

		Color clr = color0;
		float f = cycle / (ncycles - 1.0f);
		f = smoothstep(0, 1, f);
		f = smoothstep(0, 1, f);
		f = smoothstep(0, 1, f);
		f = smoothstep(0, 1, f);
		color_lerp(&clr, &color1, f);

		for(int i = 0; i < N; ++i) {
			if(!state[i]) {
				continue;
			}

			play_sfx_loop("shot1_loop");;

			cmplx dir = cdir((i + 0.5) / N * M_TAU + M_PI/2);
			cmplx v = dir;

			attr_unused Projectile *cell = PROJECTILE(
				.proto = pp_thickrice,
				.color = &clr,
				.pos = ARGS.origin - 100 * dir,
				.move = move_accelerated(v, 0.04 * v),
				.max_viewport_dist = VIEWPORT_W/2,
			);
		}

		memcpy(state, next_state, sizeof(state));
	}

}

TASK(slave, { cmplx origin; int type; }) {
	auto slave = stagex_host_yumemi_slave(ARGS.origin, ARGS.type);

	INVOKE_SUBTASK(common_move,
		.pos = &slave->pos,
		.move_params = move_towards(ARGS.origin - 64i, 0.025),
		.ent = ENT_BOX(slave).as_generic
	);

	common_charge(60, &slave->pos, 0, RGBA(0.2, 0.3, 1.0, 0));
	INVOKE_SUBTASK(rule90, slave->pos, 180);
	WAIT(60);
	common_charge(120, &slave->pos, 0, RGBA(1, 0.3, 0.2, 0));
	stagex_yumemi_slave_laser_sweep(slave, ARGS.type ? 1 : -1, global.plr.pos);
}

DEFINE_EXTERN_TASK(stagex_spell_sierpinski) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	cmplx origin = VIEWPORT_W/2 + 260*I;
	boss->move = move_towards(boss->move.velocity, origin, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	Rect bounds = viewport_bounds(128);
	bounds.top += 64;
	bounds.bottom = VIEWPORT_H / 2 + 64;

	int type = 0;

	for(;;) {
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W/3, bounds);
		INVOKE_SUBTASK(slave, boss->pos, type);
		type = !type;
		WAIT(200);
	}
}
