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

static int scythe_newton(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	TIMER(&t);

	FROM_TO(0, 100, 1)
		e->pos -= 0.2*I*_i;

	AT(100) {
		e->args[1] = 0.2*I;
//		e->args[2] = 1;
	}

	FROM_TO(100, 10000, 1) {
		e->pos = VIEWPORT_W/2+I*VIEWPORT_H/2 + 400*cos(_i*0.04)*cexp(I*_i*0.01);
	}


	FROM_TO(100, 10000, 3) {
		Projectile *p;
		for(p = global.projs.first; p; p = p->next) {
			if(
				p->type == PROJ_ENEMY &&
				cabs(p->pos-e->pos) < 50 &&
				cabs(global.plr.pos-e->pos) > 50 &&
				p->args[2] == 0 &&
				p->sprite != get_sprite("proj/apple")
			) {
				e->args[3] += 1;
				//p->args[0] /= 2;
				play_sfx_ex("redirect",4,false);
				p->birthtime=global.frames;
				p->pos0=p->pos;
				p->args[0] = (2+0.125*global.diff)*cexp(I*2*M_PI*frand());
				p->color = *RGBA_MUL_ALPHA(frand(), 0, 1, 0.8);
				p->args[2] = 1;
				spawn_projectile_highlight_effect(p);
			}
		}
	}

	scythe_common(e, t);
	return 1;
}

static int newton_apple(Projectile *p, int t) {
	int r = accelerated(p, t);

	if(t >= 0) {
		p->angle += M_PI/16 * sin(creal(p->args[2]) + t / 30.0);
	}

	return r;
}

void elly_newton(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_newton;
		elly_clap(b,100);
	}

	AT(EVENT_DEATH) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_reset;
	}

	FROM_TO(30 + 60 * (D_Lunatic - global.diff), 10000000, 30 - 6 * global.diff) {
		Sprite *apple = get_sprite("proj/apple");
		Color c;

		switch(tsrand() % 3) {
			case 0: c = *RGB(0.8, 0.0, 0.1); break;
			case 1: c = *RGB(0.4, 0.6, 0.0); break;
			case 2: c = *RGB(0.8, 0.6, 0.0); break;
		}

		cmplx apple_pos = clamp(creal(global.plr.pos) + nfrand() * 64, apple->w*0.5, VIEWPORT_W - apple->w*0.5);

		PROJECTILE(
			.pos = apple_pos,
			.proto = pp_apple,
			.rule = newton_apple,
			.args = {
				0, 0.05*I, M_PI*2*frand()
			},
			.color = &c,
			.layer = LAYER_BULLET | 0xffff, // force them to render on top of the other bullets
		);

		play_sfx("shot3");
	}

	FROM_TO(0, 100000, 20+10*(global.diff>D_Normal)) {
		float a = 2.7*_i+carg(global.plr.pos-b->pos);
		int x, y;
		float w = global.diff/2.0+1.5;

		play_sfx("shot_special1");

		for(x = -w; x <= w; x++) {
			for(y = -w; y <= w; y++) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = b->pos+(x+I*y)*25*cexp(I*a),
					.color = RGB(0, 0.5, 1),
					.rule = linear,
					.args = { (2+(_i==0))*cexp(I*a) }
				);
			}
		}
	}
}


