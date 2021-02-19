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

void iku_atmospheric(Boss *b, int time) {
	if(time < 0) {
		GO_TO(b, VIEWPORT_W/2+200.0*I, 0.06);
		return;
	}

	int t = time % 500;
	TIMER(&t);

	GO_TO(b,VIEWPORT_W/2+tanh(sin(time/100))*(200-100*(global.diff==D_Easy))+I*VIEWPORT_H/3+I*(cos(t/200)-1)*50,0.03);

	FROM_TO(0, 500, 23-2*global.diff) {
		tsrand_fill(4);
		cmplx p1 = VIEWPORT_W*afrand(0) + VIEWPORT_H/2*I*afrand(1);
		cmplx p2 = p1 + (120+20*global.diff)*cexp(0.5*I-afrand(2)*I)*(1-2*(afrand(3) > 0.5));

		int i;
		int c = 6+global.diff;

		for(i = -c*0.5; i <= c*0.5; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = p1+(p2-p1)/c*i,
				.color = RGBA(1-1/(1+fabs(0.1*i)), 0.5-0.1*abs(i), 1, 0),
				.rule = accelerated,
				.args = {
					0, (0.004+0.001*global.diff)*cexp(I*carg(p2-p1)+I*M_PI/2+0.2*I*i)
				},
			);
		}

		play_sound("shot_special1");
		play_sound("redirect");
	}

	FROM_TO(0, 500, 7-global.diff) {
		if(global.diff >= D_Hard) {
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = VIEWPORT_W*frand(),
				.color = RGB(0,0.3,0.7),
				.rule = accelerated,
				.args = { 0, 0.01*I }
			);
		}
		else {
			PROJECTILE(
				.proto = pp_rice,
				.pos = VIEWPORT_W*frand(),
				.color = RGB(0,0.3,0.7),
				.rule = linear,
				.args = { 2*I }
			);
		}
	}
}
