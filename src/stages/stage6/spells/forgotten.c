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

static int baryon_curvature(Enemy *e, int t) {
	int num = creal(e->args[2])+0.5;
	int odd = num&1;
	cmplx bpos = global.boss->pos;
	cmplx target = (1-2*odd)*(300+100*sin(t*0.01))*cexp(I*(2*M_PI*(num+0.5*odd)/6+0.6*sqrt(1+t*t/600.)));
	GO_TO(e,bpos+target, 0.1);

	if(global.diff > D_Easy && t % (80-4*global.diff) == 0) {
		tsrand_fill(2);
		cmplx pos = e->pos+60*anfrand(0)+I*60*anfrand(1);

		if(cabs(pos - global.plr.pos) > 100) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = pos,
				.color = RGBA(1.0, 0.4, 1.0, 0.0),
				.rule = linear,
				.args = { cexp(I*carg(global.plr.pos-pos)) },
			);
		}
	}

	return 1;
}

static int curvature_bullet(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
		return ACTION_ACK;
	} else if(t ==	EVENT_BIRTH) {
		return ACTION_ACK;
	}

	if(REF(p->args[1]) == 0) {
		return 0;
	}

	float vx, vy, x, y;
	cmplx v = ((Enemy *)REF(p->args[1]))->args[0]*0.00005;
	vx = creal(v);
	vy = cimag(v);
	x = creal(p->pos-global.plr.pos);
	y = cimag(p->pos-global.plr.pos);

	float f1 = (1+2*(vx*x+vy*y)+x*x+y*y);
	float f2 = (1-vx*vx+vy*vy);
	float f3 = f1-f2*(x*x+y*y);
	p->pos = global.plr.pos + (f1*v+f2*(x+I*y))/f3+p->args[0]/(1+2*(x*x+y*y)/VIEWPORT_W/VIEWPORT_W);

	return ACTION_NONE;
}

static int curvature_orbiter(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
		return ACTION_ACK;
	} else if(t ==	EVENT_BIRTH) {
		return ACTION_ACK;
	}

	const double w = 0.03;
	if(REF(p->args[1]) != 0 && p->args[3] == 0) {
		p->pos = ((Projectile *)REF(p->args[1]))->pos+p->args[0]*cexp(I*t*w);

		p->args[2] = p->args[0]*I*w*cexp(I*t*w);
	} else {
		p->pos += p->args[2];
	}

	return ACTION_NONE;
}

static double saw(double t) {
	return cos(t)+cos(3*t)/9+cos(5*t)/25;
}

static int curvature_slave(Enemy *e, int t) {
	e->args[0] = -(e->args[1] - global.plr.pos);
	e->args[1] = global.plr.pos;
	play_sfx_loop("shot1_loop");

	if(t % (2+(global.diff < D_Hard)) == 0) {
		tsrand_fill(2);
		cmplx pos = VIEWPORT_W*afrand(0)+I*VIEWPORT_H*afrand(1);
		if(cabs(pos - global.plr.pos) > 50) {
			tsrand_fill(2);
			float speed = 0.5/(1+(global.diff < D_Hard));

			PROJECTILE(
				.proto = pp_flea,
				.pos = pos,
				.color = RGB(0.1*afrand(0), 0.6,1),
				.rule = curvature_bullet,
				.args = {
					speed*cexp(2*M_PI*I*afrand(1)),
					add_ref(e)
				}
			);
		}
	}

	if(global.diff >= D_Hard && !(t%20)) {
		play_sfx_ex("shot2",10,false);
		Projectile *p =PROJECTILE(
			.proto = pp_bigball,
			.pos = global.boss->pos,
			.color = RGBA(0.5, 0.4, 1.0, 0.0),
			.rule = linear,
			.args = { 4*I*cexp(I*M_PI*2/5*saw(t/100.)) },
		);

		if(global.diff == D_Lunatic) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = global.boss->pos,
				.color = RGBA(0.2, 0.4, 1.0, 0.0),
				.rule = curvature_orbiter,
				.args = {
					40*cexp(I*t/400),
					add_ref(p)
				},
			);
		}
	}

	return 1;
}


void elly_curvature(Boss *b, int t) {
	TIMER(&t);

	AT(EVENT_BIRTH) {
		set_baryon_rule(baryon_curvature);
		return;
	}

	AT(EVENT_DEATH) {
		set_baryon_rule(baryon_reset);

		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->logic_rule == curvature_slave) {
				e->hp = 0;
			}
		}

		return;
	}

	AT(50) {
		create_enemy2c(b->pos, ENEMY_IMMUNE, 0, curvature_slave, 0, global.plr.pos);
	}

	GO_TO(b, VIEWPORT_W/2+100*I+VIEWPORT_W/3*round(sin(t/200)), 0.04);

}
