/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static int baryon_nattack(Enemy *e, int t) {
	if(t < 0)
		return 1;

	TIMER(&t);

	e->pos = global.boss->pos + (e->pos-global.boss->pos)*cexp(0.006*I);

	FROM_TO_SND("shot1_loop",30, 10000, (7 - global.diff)) {
		float a = 0.2*_i + creal(e->args[2]) + 0.006*t;
		float ca = a + t/60.0f;
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos+40*cexp(I*a),
			.color = RGB(cos(ca), sin(ca), cos(ca+2.1)),
			.rule = asymptotic,
			.args = {
				(1+0.2*global.diff)*cexp(I*a),
				3
			}
		);
	}

	return 1;
}

void elly_baryonattack(Boss *b, int t) {
	TIMER(&t);
	AT(0)
		set_baryon_rule(baryon_nattack);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);
}

void elly_baryonattack2(Boss *b, int t) {
	TIMER(&t);
	AT(0) {
		aniplayer_queue(&b->ani, "snipsnip", 0);
		set_baryon_rule(baryon_nattack);
	}
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);

	FROM_TO(100, 100000, 200-5*global.diff) {
		play_sfx("shot_special1");

		if(_i % 2) {
			int cnt = 5;
			for(int i = 0; i < cnt; ++i) {
				float a = M_PI/4;
				a = a * (i/(float)cnt) - a/2;
				cmplx n = cexp(I*(a+carg(global.plr.pos-b->pos)));

				for(int j = 0; j < 3; ++j) {
					PROJECTILE(
						.proto = pp_bigball,
						.pos = b->pos,
						.color = RGB(0,0.2,0.9),
						.rule = asymptotic,
						.args = { n, 2 * j }
					);
				}
			}
		} else {
			int x, y;
			int w = 1+(global.diff > D_Normal);
			cmplx n = cexp(I*carg(global.plr.pos-b->pos));

			for(x = -w; x <= w; x++) {
				for(y = -w; y <= w; y++) {
					PROJECTILE(
						.proto = pp_bigball,
						.pos = b->pos+25*(x+I*y)*n,
						.color = RGB(0,0.2,0.9),
						.rule = asymptotic,
						.args = { n, 3 },
					);
				}
			}
		}
	}
}
