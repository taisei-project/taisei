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

static int kurumi_burstslave(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH)
		e->args[1] = e->args[0];
	AT(EVENT_DEATH) {
		free_ref(e->args[2]);
		return 1;
	}


	if(t == 600 || REF(e->args[2]) == NULL)
		return ACTION_DESTROY;

	e->pos += 2*e->args[1]*(sin(t/10.0)+1.5);

	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;

		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + i*10.0*I*e->args[0],
				.color = RGB(r,0,0),
				.rule = accelerated,
				.args = { i*2.0*I*e->args[0], -0.01*e->args[1] }
			);
		}

		play_sfx("shot1");
	}

	FROM_TO(40, 100,1) {
		e->args[1] -= e->args[0]*0.02;
		e->args[1] *= cexp(0.02*I);
	}

	return 1;
}

void kurumi_slaveburst(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time == EVENT_DEATH)
		enemy_kill_all(&global.enemies);

	if(time < 0)
		return;

	AT(0) {
		int i;
		int n = 3+2*global.diff;
		for(i = 0; i < n; i++) {
			create_enemy3c(b->pos, ENEMY_IMMUNE, kurumi_slave_visual, kurumi_burstslave, cexp(I*2*M_PI/n*i+0.2*I*time/500), 0, add_ref(b));
		}
	}
}
