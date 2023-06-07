/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "nonspells.h"
#include "../scuttle.h"

#include "common_tasks.h"
#include "global.h"

TASK(bite_bullet, { cmplx pos; cmplx vel; }) {
	Color phase_colors[] = {
		{ 2.0, 0.0, 0.1, 1.0 },
		{ 1.2, 0.3, 0.1, 1.0 },
		{ 0.3, 0.7, 0.1, 1.0 },
	};

	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_wave,
		.pos = ARGS.pos,
		.color = phase_colors,
		.max_viewport_dist = 50,
	));

	int aimed_speed = difficulty_value(3.0, 3.5, 4.5, 5.0);
	int aimed_boost = difficulty_value(2, 2, 3, 3);

	p->move = move_asymptotic_simple(ARGS.vel, 2.0);
	WAIT(120);

	for(int phase = 1; phase < ARRAY_SIZE(phase_colors); ++phase) {
		p->move.acceleration = 0;
		p->flags |= PFLAG_MANUALANGLE;

		for(int i = 0; i < 20; ++i, YIELD) {
			fapproach_asymptotic_p(&p->angle, carg(global.plr.pos - p->pos), 0.2, 1e-3);
		}

		play_sfx("redirect");
		play_sfx("shot1");

		cmplx aim = cnormalize(global.plr.pos - p->pos);
		p->angle = carg(aim);
		p->color = phase_colors[phase];
		spawn_projectile_highlight_effect(p);
		p->move = move_asymptotic_simple(aimed_speed * cnormalize(aim), aimed_boost);

		PROJECTILE(
			.proto = pp_thickrice,
			.pos = p->pos,
			.color = RGB(0.4, 0.3, 1.0),
			.move = move_linear(-aim * 0.5),
		);

		for(int i = 0; i < 3; ++i) {
			cmplx v = rng_dir();
			v *= rng_range(1, 1.5);
			v += aim * aimed_speed;

			PARTICLE(
				.pos = p->pos,
				.sprite = "smoothdot",
				.color = RGBA(0.8, 0.6, 0.6, 0),
				.draw_rule = pdraw_timeout_scale(1, 0.01),
				.timeout = rng_irange(20, 40),
				.move = move_asymptotic_simple(v, 2),
			);
		}

		if(global.diff < D_Normal) {
			break;
		}

		WAIT(60);
	}
}

DEFINE_EXTERN_TASK(stage3_midboss_nonspell_1) {
	STAGE_BOOKMARK(non1);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 100.0*I, 0.03);
	BEGIN_BOSS_ATTACK(&ARGS);

	Rect bounds = viewport_bounds(80);
	bounds.bottom = 210;

	int time = 0;
	int cnt = difficulty_value(18, 19, 20, 21);
	int shape = difficulty_value(3, 3, 4, 5);
	int delay = 120;

	cmplx origin;
	Color charge_color = *RGBA(1, 0, 0, 0);

	for(;;) {
		origin = boss->pos;
		boss->move.attraction_point = common_wander(boss->pos, 200, bounds);
		time += common_charge_static(delay, origin, &charge_color);

		for(int i = 0; i < cnt; ++i) {
			play_sfx("shot_special1");

			cmplx v = (2 - psin((shape*M_TAU*i/(real)cnt) + time)) * cdir(M_TAU/cnt*i);
			INVOKE_TASK(bite_bullet,
				.pos = origin - v * 25,
				.vel = v,
			);
		}

		delay = imax(60, delay - 10);
	}
}
