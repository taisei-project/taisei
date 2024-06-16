/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "nonspells.h"

TASK(spawn_bugs, { BoxedBoss boss; BoxedProjectileArray *bugs; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	real speed = difficulty_value(2, 2, 3, 2.25);
	int delay = difficulty_value(3, 2, 1, 1);

	for(int i = 0;; ++i) {
		play_sfx_loop("shot1_loop");

		for(int j = 0; j < 2; ++j) {
			cmplx dir = cdir(i * M_TAU / 20 + M_PI * j);

			ENT_ARRAY_ADD(ARGS.bugs, PROJECTILE(
				.proto = pp_rice,
				.pos = boss->pos + dir * 60,
				.color = RGB(1.0, 0.5, 0.2),
				.move = move_linear(speed * dir),
				.max_viewport_dist = 120,
			));
		}

		WAIT(delay);
	}
}

TASK(scatter_bugs, { BoxedProjectileArray *bugs; }) {
	DECLARE_ENT_ARRAY(Projectile, bugs, ARGS.bugs->size);
	ENT_ARRAY_FOREACH(ARGS.bugs, Projectile *bug, {
		ENT_ARRAY_ADD(&bugs, bug);
	});

	int p = difficulty_value(3, 4, 6, 8);
	cmplx rot = cdir(M_TAU / difficulty_value(20, 20, 15, 10));

	for(int i = bugs.size - 1; i >= 0; --i) {
		Projectile *bug = ENT_UNBOX(bugs.array[i]);

		if(bug) {
			cmplx v = 1.1 * bug->move.velocity;

			PROJECTILE(
				.proto = pp_thickrice,
				.pos = bug->pos,
				.color = RGB(0.2, 1.0, 0.2),
				.move = move_asymptotic_simple(v * rot, -4)
			);

			PROJECTILE(
				.proto = pp_thickrice,
				.pos = bug->pos,
				.color = RGB(0.2, 1.0, 0.2),
				.move = move_asymptotic_simple(v / rot, -4)
			);

			kill_projectile(bug);
		}

		if(i % p == 0) {
			play_sfx("redirect");
			YIELD;
		}
	}
}

static void wriggle_anim_begin_charge(Boss *boss) {
	aniplayer_soft_switch(&boss->ani, "specialshot_charge", 1);
	aniplayer_queue(&boss->ani, "specialshot_hold", 0);
}

static void wriggle_anim_end_charge(Boss *boss) {
	aniplayer_soft_switch(&boss->ani, "specialshot_release", 1);
	aniplayer_queue(&boss->ani, "main", 0);
}

DEFINE_EXTERN_TASK(stage2_midboss_nonspell_1) {
	STAGE_BOOKMARK(non1);

	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 100.0*I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	DECLARE_ENT_ARRAY(Projectile, bugs, 512);

	INVOKE_SUBTASK(spawn_bugs, ENT_BOX(boss), &bugs);

	Rect wander_bounds = viewport_bounds(80);
	wander_bounds.bottom = 180;

	for(;;) {
		// boss->move.attraction_point = common_wander(boss->pos, 20, wander_bounds);
		WAIT(60);
		boss->move.attraction_point = common_wander(boss->pos, 20, wander_bounds);

		int delay = 110;
		int charge_time = 60;

		WAIT(delay - charge_time);
		wriggle_anim_begin_charge(boss);
		INVOKE_SUBTASK(common_charge,
			.anchor = &boss->pos,
			.color = RGBA(0.2, 1.0, 0.2, 0),
			.time = charge_time,
			.sound = COMMON_CHARGE_SOUNDS
		);
		WAIT(charge_time);
		wriggle_anim_end_charge(boss);

		INVOKE_TASK(scatter_bugs, &bugs);
		ENT_ARRAY_CLEAR(&bugs);
		WAIT(30);

		play_sfx("shot_special1");

		int n = 20;
		int m = difficulty_value(1, 3, 4, 5);
		real a = 0.05;

		for(int i = 0; i < n; i++) {
			for(int j = 0; j < m; ++j) {
				PROJECTILE(
					.proto = pp_bigball,
					.pos = boss->pos,
					.color = RGB(0.1, 0.3, 0.0),
					.move = move_accelerated(0, a * (1 + j) * cdir((i + 0.5) * M_TAU/n - M_PI/2)),
				);
			}
		}

		boss->move.attraction_point = common_wander(boss->pos, 120, wander_bounds);
	}
}
