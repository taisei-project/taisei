/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../cirno.h"

#include "global.h"

TASK(cirno_icicle, { cmplx pos; cmplx vel; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_crystal,
		.pos = ARGS.pos,
		.color = RGB(0.3, 0.3, 0.9),
		.move = move_asymptotic(ARGS.vel, 0, 0.9),
		.max_viewport_dist = 64,
		.flags = PFLAG_MANUALANGLE,
	));

	cmplx v = p->move.velocity;
	p->angle = carg(v);

	WAIT(80);

	v = 2.5 * cdir(carg(v) - M_PI/2.0 + M_PI * (creal(v) > 0));
	p->move = move_asymptotic_simple(v, 2);
	p->angle = carg(p->move.velocity);
	p->color = *RGB(0.5, 0.5, 0.5);

	play_sfx("redirect");
	spawn_projectile_highlight_effect(p);
}

DEFINE_EXTERN_TASK(stage1_spell_icicle_cascade) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W / 2.0 + 120.0*I, 0.01);
	BEGIN_BOSS_ATTACK(&ARGS);

	int icicle_interval = difficulty_value(30, 22, 16, 8);
	int icicles = 4;
	real turn = difficulty_value(0.4, 0.4, 0.2, 0.1);

	WAIT(20);
	aniplayer_queue(&boss->ani, "(9)", 0);

	for(int round = 0;; ++round) {
		WAIT(icicle_interval);
		play_sfx("shot1");

		for(int i = 0; i < icicles; ++i) {
			real speed = 8 + 3 * i;
			cmplx o1 = 28 - 42 * I;
			cmplx o2 = -conj(o1);
			INVOKE_TASK(cirno_icicle, boss->pos + o1, speed * cdir(-0.1 + turn * (round - 1)));
			INVOKE_TASK(cirno_icicle, boss->pos + o2, speed * cdir(+0.1 + turn * (1 - round) + M_PI));
		}
	}
}
