/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../kurumi.h"

#include "global.h"

void kurumi_breaker(Boss *b, int time) {
	int t = time % 400;
	int i;

	int c = 10+global.diff*2;
	int kt = 20;

	if(time < 0)
		return;

	GO_TO(b, VIEWPORT_W/2 + VIEWPORT_W/3*sin(time/220) + I*cimag(b->pos), 0.02);

	TIMER(&t);

	FROM_TO_SND("shot1_loop", 50, 400, 50-7*global.diff) {
		cmplx p = b->pos + 150*sin(_i) + 100.0*I*cos(_i);

		for(i = 0; i < c; i++) {
			cmplx n = cexp(2.0*I*M_PI/c*i);
			PROJECTILE(
				.proto = pp_rice,
				.pos = p,
				.color = RGB(1,0,0.5),
				.rule = kurumi_splitcard,
				.args = {
					3*n,
					0,
					kt,
					1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-2.6*n
			});
		}
	}

	FROM_TO(60, 400, 100) {
		play_sfx("shot_special1");
		aniplayer_queue(&b->ani,"muda",1);
		aniplayer_queue(&b->ani,"main",0);
		for(i = 0; i < 20; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.5, 0.0, 0.5, 0.0),
				.rule = asymptotic,
				.args = { cexp(2.0*I*M_PI/20.0*i), 3 },
			);
		}
	}
}
