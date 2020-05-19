/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "global.h"

void hina_wheel(Boss *h, int time) {
	int t = time % 400;
	TIMER(&t);

	static int dir = 0;

	if(time < 0)
		return;

	GO_TO(h, VIEWPORT_W/2+VIEWPORT_H/2*I, 0.02);

	AT(0) {
		aniplayer_queue(&h->ani,"guruguru",0);
	}
	if(time < 60) {
		if(time == 0) {
			if(global.diff > D_Normal) {
				dir = 1 - 2 * (tsrand()%2);
			} else {
				dir = 1;
			}
		}

		return;
	}

	FROM_TO_SND("shot1_loop", 0, 400, 5-imin(global.diff, D_Normal)) {
		int i;
		float speed = 10;
		if(time > 500)
			speed = 1+9*exp(-(time-500)/100.0);

		float d = imax(0, global.diff - D_Normal);

		for(i = 1; i < 6+d; i++) {
			float a = dir * 2*M_PI/(5+d)*(i+(1 + 0.4 * d)*time/100.0+(1 + 0.2 * d)*frand()*time/1700.0);
			PROJECTILE(
				.proto = pp_crystal,
				.pos = h->pos,
				.color = RGB(log(1+time*1e-3),0,0.2),
				.rule = linear,
				.args = { speed*cexp(I*a) }
			);
		}
	}
}
