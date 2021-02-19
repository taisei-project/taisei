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

static int scythe_kepler(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	TIMER(&t);

	AT(20) {
		e->args[1] = 0.2*I;
	}

	FROM_TO(20, 10000, 1) {
		GO_TO(e, global.boss->pos + 100*cexp(I*_i*0.01),0.03);
	}

	scythe_common(e, t);
	return 1;
}

static ProjPrototype* kepler_pick_bullet(int tier) {
	switch(tier) {
		case 0:  return pp_soul;
		case 1:  return pp_bigball;
		case 2:  return pp_ball;
		default: return pp_flea;
	}
}

static int kepler_bullet(Projectile *p, int t) {
	TIMER(&t);
	int tier = round(creal(p->args[1]));

	AT(EVENT_DEATH) {
		if(tier != 0) {
			free_ref(p->args[2]);
		}
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	cmplx pos = p->pos0;

	if(tier != 0) {
		Projectile *parent = (Projectile *) REF(creal(p->args[2]));

		if(parent == 0) {
			p->pos += p->args[3];
			return 1;
		}
		pos = parent->pos;
	} else {
		pos += t*p->args[2];
	}

	cmplx newpos = pos + tanh(t/90.)*p->args[0]*cexp((1-2*(tier&1))*I*t*0.5/cabs(p->args[0]));
	cmplx vel = newpos-p->pos;
	p->pos = newpos;
	p->args[3] = vel;

	if(t%(30-5*global.diff) == 0) {
		p->args[1]+=1*I;
		int tau = global.frames-global.boss->current->starttime;
		cmplx phase = cexp(I*0.2*tau*tau);
		int n = global.diff/2+3+(frand()>0.3);
		if(global.diff == D_Easy)
			n=7;

		play_sfx("redirect");
		if(tier <= 1+(global.diff>D_Hard) && cimag(p->args[1])*(tier+1) < n) {
			PROJECTILE(
				.proto = kepler_pick_bullet(tier + 1),
				.pos = p->pos,
				.color = RGB(0.3 + 0.3 * tier, 0.6 - 0.3 * tier, 1.0),
				.rule = kepler_bullet,
				.args = {
					cabs(p->args[0])*phase,
					tier+1,
					add_ref(p)
				}
			);
		}
	}
	return ACTION_NONE;
}

void elly_kepler(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_kepler;

		PROJECTILE(
			.proto = pp_soul,
			.pos = b->pos,
			.color = RGB(0.3,0.8,1),
			.rule = kepler_bullet,
			.args = { 50+10*global.diff }
		);

		elly_clap(b, 20);
	}

	AT(EVENT_DEATH) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_reset;
	}

	FROM_TO(0, 100000, 20) {
		int c = 2;
		play_sfx("shot_special1");
		for(int i = 0; i < c; i++) {
			cmplx n = cexp(I*2*M_PI/c*i+I*0.6*_i);

			PROJECTILE(
				.proto = kepler_pick_bullet(0),
				.pos = b->pos,
				.color = RGB(0.3,0.8,1),
				.rule = kepler_bullet,
				.args = { 50*n, 0, (1.4+0.1*global.diff)*n }
			);
		}
	}
}
