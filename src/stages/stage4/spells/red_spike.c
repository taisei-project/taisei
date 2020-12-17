/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "spells.h"
#include "../kurumi.h"

#include "global.h"

static int kurumi_spikeslave(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH)
		e->args[1] = e->args[0];
	AT(EVENT_DEATH) {
		free_ref(e->args[2]);
		return 1;
	}


	if(t == 300+50*global.diff || REF(e->args[2]) == NULL)
		return ACTION_DESTROY;

	e->pos += e->args[1];
	e->args[1] *= cexp(0.01*I*e->args[0]);

	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;

		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 10.0*I*e->args[0],
				.color = RGB(r,0,0),
				.rule = linear,
				.args = { i*1.5*I*e->args[1] }
			);
		}

		play_sound("shot1");
	}

	return 1;
}

void kurumi_redspike(Boss *b, int time) {
	int t = time % 500;

	if(time == EVENT_DEATH)
		enemy_kill_all(&global.enemies);

	if(time < 0)
		return;

	TIMER(&t);

	FROM_TO(0, 500, 60) {
		create_enemy3c(b->pos, ENEMY_IMMUNE, kurumi_slave_visual, kurumi_spikeslave, 1-2*(_i&1), 0, add_ref(b));
	}

	if(global.diff < D_Hard) {
		FROM_TO(0, 500, 150-50*global.diff) {
			int i;
			int n = global.diff*8;
			for(i = 0; i < n; i++) {
				PROJECTILE(
					.proto = pp_bigball,
					.pos = b->pos,
					.color = RGBA(1.0, 0.0, 0.0, 0.0),
					.rule = asymptotic,
					.args = {
						(1+0.1*(global.diff == D_Normal))*3*cexp(2.0*I*M_PI/n*i+I*carg(global.plr.pos-b->pos)),
						3
					},
				);
			}

			play_sound("shot_special1");
		}
	} else {
		AT(80) {
			aniplayer_queue(&b->ani, "muda", 0);
		}

		AT(499) {
			aniplayer_queue(&b->ani, "main", 0);
		}

		FROM_TO_INT(80, 500, 40,200,2+2*(global.diff == D_Hard)) {
			tsrand_fill(2);
			cmplx offset = 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1));
			cmplx n = cexp(I*carg(global.plr.pos-b->pos-offset));
			PROJECTILE(
				.proto = pp_rice,
				.pos = b->pos+offset,
				.color = RGBA(1, 0, 0, 0),
				.rule = accelerated,
				.args = { -1*n, 0.05*n },
			);
			play_sound_ex("warp",0,false);
		}
	}
}
