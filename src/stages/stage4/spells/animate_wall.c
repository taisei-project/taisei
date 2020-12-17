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

static int aniwall_bullet(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t > creal(p->args[1])) {
		if(global.diff > D_Normal) {
			tsrand_fill(2);
			p->args[0] += 0.1*(0.1-0.2*afrand(0) + 0.1*I-0.2*I*afrand(1))*(global.diff-2);
			p->args[0] += 0.002*cexp(I*carg(global.plr.pos - p->pos));
		}

		p->pos += p->args[0];
	}

	p->color.r = cimag(p->pos)/VIEWPORT_H;
	return ACTION_NONE;
}

static int aniwall_slave(Enemy *e, int t) {
	float re, im;

	if(t < 0)
		return 1;

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

	if(cabs(e->args[1]) <= 0.1) {
		if(re == 0 || re == VIEWPORT_W) {

			e->args[1] = 1;
			e->args[2] = 10.0*I;
		}

		e->pos += e->args[0]*t;
	} else {
		if((re <= 0) + (im <= 0) + (re >= VIEWPORT_W) + (im >= VIEWPORT_H) == 2) {
			float sign = 1;
			sign *= 1-2*(re > 0);
			sign *= 1-2*(im > 0);
			sign *= 1-2*(cimag(e->args[2]) == 0);
			e->args[2] *= sign*I;
		}

		e->pos += e->args[2];

		if(!(t % 7-global.diff-2*(global.diff > D_Normal))) {
			cmplx v = e->args[2]/cabs(e->args[2])*I*sign(creal(e->args[0]));
			if(cimag(v) > -0.1 || global.diff >= D_Normal) {
				play_sound("shot1");
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos+I*v*20*nfrand(),
					.color = RGB(1,0,0),
					.rule = aniwall_bullet,
					.args = { 1*v, 40 }
				);
			}
		}
	}

	return 1;
}

void kurumi_aniwall(Boss *b, int time) {
	TIMER(&time);

	AT(EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
	}

	GO_TO(b, VIEWPORT_W/2 + VIEWPORT_W/3*sin(time/200) + I*cimag(b->pos),0.03)

	if(time < 0)
		return;

	AT(0) {
		aniplayer_queue(&b->ani, "muda", 0);
		play_sound("laser1");
		create_lasercurve2c(b->pos, 50, 80, RGBA(1.0, 0.8, 0.8, 0.0), las_accel, 0, 0.2*cexp(0.4*I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, kurumi_slave_static_visual, aniwall_slave, 0.2*cexp(0.4*I));
		create_lasercurve2c(b->pos, 50, 80, RGBA(1.0, 0.8, 0.8, 0.0), las_accel, 0, 0.2*cexp(I*M_PI - 0.4*I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, kurumi_slave_static_visual, aniwall_slave, 0.2*cexp(I*M_PI - 0.4*I));
	}
}
