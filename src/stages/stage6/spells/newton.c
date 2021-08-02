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

TASK(newton_spawn_square, { cmplx pos; cmplx dir; real width; int count; real speed; }) {

	int delay = round(ARGS.width / (ARGS.count-1) / ARGS.speed);
	for(int i = 0; i < ARGS.count; i++) {
		for(int j = 0; j < ARGS.count; j++) {
			real x = ARGS.width * (-0.5 + j / (real) (ARGS.count-1));

			PROJECTILE(
				.proto = pp_ball,
				.pos = ARGS.pos + x * ARGS.dir * I,
				.color = RGB(0, 0.5, 1),
				.move = move_accelerated(ARGS.speed * ARGS.dir, 0*0.01*I),
				.max_viewport_dist = VIEWPORT_H,

			);
		}
		WAIT(delay);
	}
	
}

DEFINE_EXTERN_TASK(stage6_spell_newton) {
	Boss *boss = stage6_elly_init_scythe_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	//INVOKE_SUBTASK(scythe_newton, ARGS.scythe);

	/*int start_delay = difficulty_value(210, 150, 90, 30);
	WAIT(start_delay);

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
	}*/

	int rows = 5;
	real width = VIEWPORT_H / (real)rows;

	for(int i = 0;; i++) {
		real y = width * (0.5 + i);
		
		for(int s = -1; s <= 1; s += 2) {
			cmplx dir = cdir(i) * I;
			cmplx pos = boss->pos + 0* dir;
			INVOKE_SUBTASK(newton_spawn_square, pos, -s*dir,
				       .width = width,
				       .count = 6,
				       .speed = 3
			);
		}

		WAIT(20);
	}
}


