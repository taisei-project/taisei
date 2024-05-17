/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

#define SHOT_OFS1 CMPLX(20, 8)
#define SHOT_OFS2 CMPLX(-14, -42)

TASK(limiters, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real angle = 3*M_PI/2;
	real final_angle = M_PI/16 * difficulty_value(2, 1.5, 1, 1);

	for(int time = 0;; time += WAIT(3)) {
		play_sfx_loop("shot1_loop");

		for(int i = -1; i < 2; i += 2) {
			real c = psin(time/10.0);
			cmplx aim = cnormalize(global.plr.pos - boss->pos);
			cmplx v = 8 * aim * cdir(i * angle);
			PROJECTILE(
				.proto = pp_crystal,
				.pos = boss->pos + SHOT_OFS2,
				.color = RGBA_MUL_ALPHA(0.3 + c * 0.7, 0.6 - c * 0.3, 0.3, 0.7),
				.move = move_linear(v),
			);
		}

		angle = lerp(angle, final_angle, 0.02);
	}
}

TASK(recursive_ball, { cmplx origin; cmplx dir; real angle; ProjFlags flags; }) {
	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = ARGS.origin,
		.color = RGB(1.0, 1.0, 0.3),
		.move = move_asymptotic_halflife(3 * ARGS.dir, 0, 13),
		.flags = ARGS.flags,
	));

	WAIT(difficulty_value(60, 60, 60, 50));

	cmplx r = cdir(ARGS.angle);

	play_sfx("redirect");

	real a = ARGS.angle * 0.75;

	if(a > M_PI/32) {
		INVOKE_TASK(recursive_ball, p->pos, ARGS.dir * r, a);
		INVOKE_TASK(recursive_ball, p->pos, ARGS.dir / r, a, PFLAG_NOSPAWNFLARE);
	} else {
		spawn_projectile_highlight_effect(p);
	}

	if(global.diff > D_Easy) {
		p->color.r = 0;
		projectile_set_prototype(p, pp_thickrice);
		WAIT(difficulty_value(0, 120, 120, 100));
		spawn_projectile_highlight_effect(p);
	}

	kill_projectile(p);
}

TASK(recursive_balls, { BoxedBoss boss; }) {
	auto boss = TASK_BIND(ARGS.boss);

	int delay = difficulty_value(120, 120, 60, 50);

	for(;;WAIT(delay)) {
		cmplx o = boss->pos + boss_get_sprite_offset(boss) + SHOT_OFS2;
		cmplx d = cnormalize(global.plr.pos - o + 32 * rng_dir());
		play_sfx("shot_special1");
		INVOKE_TASK(recursive_ball, o, d, M_PI/2);
	}
}

DEFINE_EXTERN_TASK(stage3_spell_logic_bomb) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.08);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move.attraction = 0;

	aniplayer_queue(&boss->ani, "dance", 0);

	INVOKE_SUBTASK(limiters, ENT_BOX(boss));
	INVOKE_SUBTASK_DELAYED(20, recursive_balls, ENT_BOX(boss));

	for(int time = 0;; ++time, YIELD) {
		real t = time * 1.5;
		real moverad = min(160, time/2.7);
		boss->pos = VIEWPORT_W/2 + VIEWPORT_H*I/2 + sin(t/50.0) * moverad * cdir(M_PI/2 * t/100.0);
	}
}
