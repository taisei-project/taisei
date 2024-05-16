/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"
#include "../cirno.h"

#ifdef SPELL_BENCHMARK

#include "global.h"

static void stage1_spell_benchmark_proc(Boss *b, int t) {
	int N = 5000; // number of particles on the screen

	double speed = 10;
	int c = N*speed/VIEWPORT_H;
	for(int i = 0; i < c; i++) {
		double x = rng_range(0, VIEWPORT_W);
		double plrx = re(global.plr.pos);
		x = plrx + sqrt((x-plrx)*(x-plrx)+100)*(1-2*(x<plrx));

		Projectile *p = PROJECTILE(
			.proto = pp_ball,
			.pos = x,
			.color = RGB(0.1, 0.1, 0.5),
			.flags = PFLAG_NOGRAZE,
			.move = move_linear(speed*I),
		);

		if(rng_chance(0.1)) {
			p->flags &= ~PFLAG_NOGRAZE;
		}

		if(t > 700 && rng_chance(0.5))
			projectile_set_prototype(p, pp_plainball);

		if(t > 1200 && rng_chance(0.5))
			p->color = *RGB(1.0, 0.2, 0.8);

		if(t > 350 && rng_chance(0.5))
			p->color.a = 0;
	}
}

DEFINE_EXTERN_TASK(stage1_spell_benchmark) {
	auto b = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	aniplayer_queue(&b->ani, "(9)", 0);

	for(int t = 0;; ++t, YIELD) {
		stage1_spell_benchmark_proc(b, t);
	}
}

#endif
