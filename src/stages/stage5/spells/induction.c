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

void iku_induction(Boss *b, int t) {
	// thwarf safespots
	cmplx ofs = global.diff > D_Normal ? 10*I : 0;

	GO_TO(b, VIEWPORT_W/2+200.0*I + ofs, 0.03);

	if(t < 0) {
		return;
	}

	TIMER(&t);
	AT(0) {
		aniplayer_queue(&b->ani, "dashdown_wait", 0);
	}

	FROM_TO_SND("shot1_loop", 0, 18000, 8) {
		play_sound("redirect");

		int i,j;
		int c = 6;
		int c2 = 6-(global.diff/4);
		for(i = 0; i < c; i++) {
			for(j = 0; j < 2; j++) {
				Color *clr = RGBA(1-1/(1+0.1*(_i%c2)), 0.5-0.1*(_i%c2), 1.0, 0.0);
				float shift = 0.6*(_i/c2);
				float a = -0.0002*(global.diff-D_Easy);
				if(global.diff == D_Hard)
					a += 0.0005;
				PROJECTILE(
					.proto = pp_ball,
					.pos = b->pos,
					.color = clr,
					.rule = iku_induction_bullet,
					.args = {
						2*cexp(2.0*I*M_PI/c*i+I*M_PI/2+I*shift),
						(0.01+0.001*global.diff)*I*(1-2*j)+a
					},
					.max_viewport_dist = 400*(global.diff>=D_Hard),
				);
			}
		}

	}
}
