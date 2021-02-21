/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../cirno.h"

#include "global.h"

void stage1_spell_benchmark_proc(Boss *b, int t) {
	if(t < 0) {
		return;
	}

	int N = 5000; // number of particles on the screen

	if(t == 0) {
		aniplayer_queue(&b->ani, "(9)", 0);
	}
	double speed = 10;
	int c = N*speed/VIEWPORT_H;
	for(int i = 0; i < c; i++) {
		double x = rng_range(0, VIEWPORT_W);
		double plrx = creal(global.plr.pos);
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
