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

void hina_amulet(Boss *h, int time) {
	int t = time % 200;

	if(time < 0)
		return;

	if(time < 100)
		GO_TO(h, VIEWPORT_W/2 + 200.0*I, 0.02);

	TIMER(&t);

	cmplx d = global.plr.pos - h->pos;
	int loopduration = 200*(global.diff+0.5)/(D_Lunatic+0.5);
	AT(0) {
		aniplayer_queue_frames(&h->ani,"guruguru",loopduration);
	}
	FROM_TO_SND("shot1_loop", 0,loopduration,1) {
		float f = _i/30.0;
		cmplx n = cexp(I*2*M_PI*f+I*carg(d)+0.7*time/200*I)/sqrt(0.5+global.diff);

		float speed = 1.0 + 0.75 * imax(0, global.diff - D_Normal);
		float accel = 1.0 + 1.20 * imax(0, global.diff - D_Normal);

		cmplx p = h->pos+30*log(1+_i/2.0)*n;

		ProjPrototype *t0 = pp_ball;
		ProjPrototype *t1 = global.diff == D_Easy ? t0 : pp_crystal;

		PROJECTILE(.proto = t0, .pos = p, .color = RGB(0.8, 0.0, 0.0), .rule = accelerated, .args = { speed *  2*n*I, accel * -0.01*n });
		PROJECTILE(.proto = t1, .pos = p, .color = RGB(0.8, 0.0, 0.5), .rule = accelerated, .args = { speed * -2*n*I, accel * -0.01*n });
	}
}
