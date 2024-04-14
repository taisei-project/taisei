/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "../stagex.h"
#include "common_tasks.h"
#include "stage.h"

#define GRID_W 32
#define GRID_H (int)(((real)VIEWPORT_H/VIEWPORT_W) * (GRID_W + 0.5))

static Laser *make_grid_laser(cmplx a, cmplx b) {
	auto l = create_laserline_ab(a, b, 10, 140, 200, RGBA(1, 0.0, 0, 0));
	l->width_exponent = 0;
	return l;
}

TASK(grid_lasers_axis, { BoxedLaserArray *lasers; int count; cmplx a; cmplx b; }) {
	for(int x = 0; x < ARGS.count; ++x) {
		cmplx a = ARGS.a * (x + 0.5) / GRID_W;
		cmplx b = a + ARGS.b;
		ENT_ARRAY_ADD(ARGS.lasers, make_grid_laser(a, b));
		// WAIT(1);
	}
}

TASK(grid_lasers) {
	DECLARE_ENT_ARRAY(Laser, lasers, GRID_W + GRID_H);
	INVOKE_SUBTASK(grid_lasers_axis, &lasers, GRID_W, VIEWPORT_W, I*VIEWPORT_H);
	INVOKE_SUBTASK(grid_lasers_axis, &lasers, GRID_H, VIEWPORT_H*I, VIEWPORT_W);

	int alive = 0;

	for(;;YIELD) {
		ENT_ARRAY_FOREACH(&lasers, Laser *l, {
			++alive;

			l->color = *RGBA(0.7, 0, 0, 0);
			color_lerp(&l->color, RGBA(1, 0.7, 0, 0), (l->width - 3) / 7);

			if(l->collision_active) {
				cmplx q = l->args[0] * l->timespan;
				real w = 0.5*VIEWPORT_W/GRID_W;
				cmplx expand = -conj(I * w * cnormalize(q));

				Rect hurtbox = {
					.top_left = l->pos - expand,
					.bottom_right = l->pos + q + expand,
				};

				for(Projectile *p = global.projs.first, *next; p; p = next) {
					next = p->next;

					if(p->type != PROJ_ENEMY || !point_in_rect(p->pos, hurtbox)) {
						continue;
					}

					cmplx v = 0;
					real g;

					if(re(expand)) {  // vertical
						g = 2 * ((re(p->pos) - hurtbox.left) / (hurtbox.right - hurtbox.left) - 0.5);
					} else {  // horizontal
						g = 2 * ((im(p->pos) - hurtbox.top) / (hurtbox.bottom - hurtbox.top) - 0.5);
					}

					g = copysign(1 - fabs(g), g);

					if(g) {
						p->pos += g * expand;

						cmplx d = global.plr.pos - p->pos;
						real xdist = fabs(re(d));
						real ydist = fabs(im(d));
						if(xdist < ydist) {
							v = im(d)*I;
						} else {
							v = re(d);
						}

						p->move.velocity = cabs(p->move.velocity) * cnormalize(v);
						p->angle = carg(p->move.velocity);
						p->flags |= PFLAG_MANUALANGLE;
					}
				}
			}
		});

		if(alive == 0) {
			return;
		}
	}

	AWAIT_SUBTASKS;
}

TASK(spam, { BoxedBoss boss; }) {
	auto boss = TASK_BIND(ARGS.boss);

	cmplx dir = -I;
	cmplx r = cdir(M_TAU/43);

	real p = 0;

	for(;;) {
		PROJECTILE(
			.pos = boss->pos + dir * 64 * sin(p),
			.proto = pp_wave,
			.color = RGB(0.2, 0.4, 1),
			.move = move_linear(dir * 1),
		);
		PROJECTILE(
			.pos = boss->pos + dir * -64 * sin(p),
			.proto = pp_wave,
			.color = RGB(0.2, 0.4, 1),
			.move = move_linear(dir * -1),
		);

		dir *= r;
		p += 0.14315;

		WAIT(4);
	}

	for(;;) {
		RADIAL_LOOP(l, 128, rng_dir()) {
			PROJECTILE(
				.pos = boss->pos,
				.proto = pp_plainball,
				.color = RGB(0.2, 0.4, 1),
				.move = move_accelerated(0, l.dir * 0.05),
			);
		}

		WAIT(60);
	}
}

DEFINE_EXTERN_TASK(stagex_spell_alignment) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(boss->move.velocity, BOSS_DEFAULT_GO_POS, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	INVOKE_SUBTASK_DELAYED(60, spam, ENT_BOX(boss));

	for(;;) {
		INVOKE_SUBTASK(grid_lasers);
		WAIT(360);
	}
}
