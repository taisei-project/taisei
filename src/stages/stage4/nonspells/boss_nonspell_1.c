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

MODERNIZE_THIS_FILE_AND_REMOVE_ME

void kurumi_sbreaker(Boss *b, int time) {
	if(time < 0)
		return;

	int dur = 300+50*global.diff;
	int t = time % dur;
	int i;
	TIMER(&t);

	int c = 10+global.diff*2;
	int kt = 40;

	FROM_TO_SND("shot1_loop", 50, dur, 2+(global.diff < D_Hard)) {
		cmplx p = b->pos + 150*sin(_i/8.0)+100.0*I*cos(_i/15.0);

		cmplx n = cexp(2.0*I*M_PI/c*_i);
		PROJECTILE(
			.proto = pp_rice,
			.pos = p,
			.color = RGB(1.0, 0.0, 0.5),
			.rule = kurumi_splitcard,
			.args = {
				2*n,
				0,
				kt,
				1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-1.7*n
			}
		);
	}

	FROM_TO(60, dur, 100) {
		play_sfx("shot_special1");
		aniplayer_queue(&b->ani, "muda", 4);
		aniplayer_queue(&b->ani, "main", 0);

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
