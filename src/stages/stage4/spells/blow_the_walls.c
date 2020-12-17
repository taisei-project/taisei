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

static int blowwall_slave(Enemy *e, int t) {
	float re, im;

	if(t < 0)
		return 1;

	e->pos += e->args[0]*t;

	if(creal(e->pos) <= 0)
		e->pos = I*cimag(e->pos);
	if(creal(e->pos) >= VIEWPORT_W)
		e->pos = VIEWPORT_W + I*cimag(e->pos);
	if(cimag(e->pos) <= 0)
		e->pos = creal(e->pos);
	if(cimag(e->pos) >= VIEWPORT_H)
		e->pos = creal(e->pos) + I*VIEWPORT_H;

	re = creal(e->pos);
	im = cimag(e->pos);

	if(re <= 0 || im <= 0 || re >= VIEWPORT_W || im >= VIEWPORT_H) {
		int i, c;
		float f;
		ProjPrototype *type;

		c = 20 + global.diff*40;

		for(i = 0; i < c; i++) {
			f = frand();

			if(f < 0.3) {
				type = pp_soul;
			} else if(f < 0.6) {
				type = pp_bigball;
			} else {
				type = pp_plainball;
			}

			PROJECTILE(
				.proto = type,
				.pos = e->pos,
				.color = RGBA(1.0, 0.1, 0.1, 0.0),
				.rule = asymptotic,
				.args = { (1+3*f)*cexp(2.0*I*M_PI*frand()), 4 },
			);
		}

		play_sound("shot_special1");
		return ACTION_DESTROY;
	}

	return 1;
}

static void bwlaser(Boss *b, float arg, int slave) {
	create_lasercurve2c(b->pos, 50, 100, RGBA(1.0, 0.5+0.3*slave, 0.5+0.3*slave, 0.0), las_accel, 0, (0.1+0.1*slave)*cexp(I*arg));

	if(slave) {
		play_sound("laser1");
		create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, blowwall_slave, 0.2*cexp(I*arg));
	} else {
		// FIXME: needs a better sound
		play_sound("shot2");
		play_sound("shot_special1");
	}
}

void kurumi_blowwall(Boss *b, int time) {
	int t = time % 600;
	TIMER(&t);

	if(time == EVENT_DEATH)
		enemy_kill_all(&global.enemies);

	if(time < 0) {
		return;
	}

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.04)

	AT(0) {
		aniplayer_queue(&b->ani,"muda",0);
	}

	AT(50)
		bwlaser(b, 0.4, 1);

	AT(100)
		bwlaser(b, M_PI-0.4, 1);

	FROM_TO(200, 300, 50)
		bwlaser(b, -M_PI*frand(), 1);

	FROM_TO(300, 500, 10)
		bwlaser(b, M_PI/10*_i, 0);

}
