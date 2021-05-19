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

#include "stage.h"
#include "common_tasks.h"
#include "global.h"

TASK(crystal_rain_drops, NO_ARGS) {
	const int nshots = difficulty_value(1, 2, 4, 5);
	const real accel_rate = difficulty_value(0.001, 0.001, 0.01, 0.01);

	for(;;) {
		play_sfx("shot2");

		for(int i = 0; i < nshots; ++i) {
			RNG_ARRAY(rng, 2);
			cmplx org = vrng_range(rng[0], 0, VIEWPORT_W);
			cmplx aim = cnormalize(global.plr.pos - org);
			PROJECTILE(
				.proto = pp_crystal,
				.pos = org,
				.color = RGB(0.2, 0.2, 0.4),
				.move = move_accelerated(I, accel_rate * aim),
			);
		}

		WAIT(10);
	}
}

TASK(crystal_rain_cirno_shoot, { BoxedBoss boss; int charge_time; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	cmplx shot_ofs = -60*I;

	aniplayer_queue(&boss->ani, "(9)", 0);
	WAIT(10);

	INVOKE_SUBTASK(common_charge, shot_ofs, RGBA(0.3, 0.5, 1, 0), ARGS.charge_time,
		.anchor = &boss->pos,
		.sound = COMMON_CHARGE_SOUNDS
	);
	WAIT(ARGS.charge_time);

	int interval = difficulty_value(100, 80, 50, 20);

	for(int t = 0, round = 0; t < 400; ++round) {
		bool odd = (global.diff > D_Normal ? (round & 1) : 0);
		real n = (difficulty_value(1, 2, 6, 11) + odd)/2.0;
		cmplx org = boss->pos + shot_ofs;

		play_sfx("shot_special1");
		for(real i = -n; i <= n; i++) {
			PROJECTILE(
				.proto = odd? pp_plainball : pp_bigball,
				.pos = org,
				.color = RGB(0.2, 0.2, 0.9),
				.move = move_asymptotic_simple(2 * cdir(carg(global.plr.pos - org) + 0.3 * i), 2.3),
			);
		}

		t += WAIT(interval);
	}

	aniplayer_queue(&boss->ani, "main", 0);
}

DEFINE_EXTERN_TASK(stage1_spell_crystal_rain) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	INVOKE_SUBTASK(crystal_rain_drops);
	boss->move = move_towards_power(boss->pos, 0.1, 0.5);

	for(;;) {
		WAIT(20);
		stage1_cirno_wander(boss, 80, 230);
		INVOKE_SUBTASK(crystal_rain_cirno_shoot, ENT_BOX(boss), 80);
		WAIT(180);
		stage1_cirno_wander(boss, 40, 230);
		WAIT(120);
		stage1_cirno_wander(boss, 40, 230);
		WAIT(180);
	}
}
