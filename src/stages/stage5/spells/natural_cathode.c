/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static cmplx cathode_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	l->args[1] = I*cimag(l->args[1]);

	return l->pos + l->args[0]*t*cexp(l->args[1]*t);
}

void iku_cathode(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2+200.0*I, 0.02);

	TIMER(&t)

	FROM_TO(50, 18000, 70-global.diff*10) {
		aniplayer_hard_switch(&b->ani, (_i&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&b->ani,"main",0);

		int i;
		int c = 7+global.diff/2;

		double speedmod = 1-0.3*(global.diff == D_Lunatic);
		for(i = 0; i < c; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.2, 0.4, 1.0, 0.0),
				.rule = iku_induction_bullet,
				.args = {
					speedmod*2*cexp(2.0*I*M_PI*frand()),
					speedmod*0.01*I*(1-2*(_i&1)),
					1
				},
			);
			if(i < c*3/4)
				create_lasercurve2c(b->pos, 60, 200, RGBA(0.4, 1, 1, 0), cathode_laser, 2*cexp(2.0*I*M_PI*M_PI*frand()), 0.015*I*(1-2*(_i&1)));
		}

		// XXX: better ideas?
		play_sound("shot_special1");
		play_sound("redirect");
		play_sound("shot3");
		play_sound("shot2");
	}
}
