/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "common_tasks.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static int eigenstate_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t == creal(p->args[2])) {
		p->args[0] += p->args[3];
	}

	return asymptotic(p, t);
}

static int baryon_eigenstate(Enemy *e, int t) {
	if(t < 0)
		return 1;

	e->pos = e->pos0 + 40*sin(0.03*t+M_PI*(creal(e->args[0]) > 0)) + 30*cimag(e->args[0])*I*sin(0.06*t);

	TIMER(&t);

	FROM_TO(100+20*(int)creal(e->args[2]), 100000, 150-12*global.diff) {
		int i, j;
		int c = 9;

		play_sfx("shot_special1");
		play_sfx_delayed("redirect",4,true,60);
		for(i = 0; i < c; i++) {
			cmplx n = cexp(2.0*I*_i+I*M_PI/2+I*creal(e->args[2]));
			for(j = 0; j < 3; j++) {
				PROJECTILE(
					.proto = pp_plainball,
					.pos = e->pos + 60*cexp(2.0*I*M_PI/c*i),
					.color = RGBA(j == 0, j == 1, j == 2, 0.0),
					.rule = eigenstate_proj,
					.args = {
						1*n,
						1,
						60,
						0.6*I*n*(j-1)*cexp(0.4*I-0.1*I*global.diff)
					},
				);
			}
		}
	}

	return 1;
}

void elly_eigenstate(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		aniplayer_queue(&b->ani, "snipsnip", 0);
		set_baryon_rule(baryon_eigenstate);
	}

	AT(EVENT_DEATH) {
		set_baryon_rule(baryon_reset);
	}
}
