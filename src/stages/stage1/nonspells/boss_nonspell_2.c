/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "nonspells.h"

TASK(snowburst, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	play_sfx("shot_special1");
	aniplayer_queue(&boss->ani, "(9)", 0);

	int rounds = difficulty_value(3, 4, 5, 6);
	int shots_per_round = difficulty_value(4, 4, 6, 7);
	real spread = difficulty_value(0.13, 0.16, 0.19, 0.22);

	real angle_ofs = carg(global.plr.pos - boss->pos);
	for(int round = 0; round < rounds; ++round) {
		real speed = (3 + round / 3.0);
		real boost = round * 0.7;

		for(int k = 0; k < 8; ++k) {
			real base_angle = k * M_TAU / 8 + angle_ofs;
			int cnt_mod = round & 1;
			int effective_count = shots_per_round - cnt_mod;
			real effective_spread = spread * ((real)effective_count / shots_per_round);

			for(int i = 0; i < effective_count; ++i) {
				real spread_factor = rng_real();
				spread_factor = 2.0 * (i / (effective_count - 1.0) - 0.5);
				cmplx dir = cdir(base_angle + effective_spread * spread_factor);

				PROJECTILE(
					.proto = pp_plainball,
					.pos = boss->pos,
					.color = RGB(0, 0, 0.5),
					.move = move_asymptotic_simple(speed * dir, boost),
				);
			}
		}

		WAIT(2);
	}

	aniplayer_queue(&boss->ani, "main", 0);
}

TASK(spiralshot, {
	cmplx pos;
	int count;
	real spread;
	real winding;
	int interval;
	real angle_ofs;
	real bullet_speed;
} ) {
	int count = ARGS.count;
	real winding = ARGS.winding;
	real angle_ofs = ARGS.angle_ofs;
	real dist_per_bullet = ARGS.spread;
	real angle_per_bullet = winding / (count - 1.0);
	int interval = ARGS.interval;
	cmplx pos = ARGS.pos;

	DECLARE_ENT_ARRAY(Projectile, projs, count);

	int fire_delay = 20;
	int charge_time = 60;
	int charge_delay = count * interval - charge_time + fire_delay;

	INVOKE_SUBTASK_DELAYED(charge_delay, common_charge,
		.pos = pos,
		.color = RGBA(0.3, 0.3, 0.8, 0),
		.time = charge_time,
		.sound = COMMON_CHARGE_SOUNDS
	);

	for(int b = 0; b < count; ++b) {
		play_sfx_loop("shot1_loop");

		real dist = b * dist_per_bullet;
		real angle = angle_ofs + b * angle_per_bullet;

		ENT_ARRAY_ADD(&projs, PROJECTILE(
			.proto = pp_crystal,
			.pos = pos + dist * cdir(angle),
			.color = RGB(0.3, 0.3, 0.8),
			.angle = angle + M_PI,
			.flags = PFLAG_NOMOVE | PFLAG_MANUALANGLE,
		));

		WAIT(interval);
	}

	WAIT(fire_delay);
	play_sfx("shot_special1");
	play_sfx("redirect");
	ENT_ARRAY_FOREACH(&projs, Projectile *p, {
		spawn_projectile_highlight_effect(p);
		p->flags &= ~PFLAG_NOMOVE;
		p->move = move_linear(cdir(p->angle) * ARGS.bullet_speed);
	});
	ENT_ARRAY_CLEAR(&projs);
}

DEFINE_EXTERN_TASK(stage1_boss_nonspell_2) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2.0 + 100.0*I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	for(;;) {
		WAIT(20);
		INVOKE_SUBTASK(snowburst, ENT_BOX(boss));
		WAIT(20);

		int spiral_bullets = difficulty_value(40, 60, 100, 100);
		int turns = difficulty_value(8, 6, 4, 5);
		int interval = difficulty_value(2, 1, 1, 1);
		real bullet_speed = difficulty_value(2, 3, 3, 4.5);

		INVOKE_SUBTASK(spiralshot,
			.pos = boss->pos + 100,
			.count = spiral_bullets,
			.spread = 1,
			.winding = turns * M_TAU,
			.interval = interval,
			.angle_ofs = M_PI,
			.bullet_speed = bullet_speed
		);

		INVOKE_SUBTASK(spiralshot,
			.pos = boss->pos - 100,
			.count = spiral_bullets,
			.spread = 1,
			.winding = -turns * M_TAU,
			.interval = interval,
			.angle_ofs = 0,
			.bullet_speed = bullet_speed
		);

		WAIT(140);

		interval = difficulty_value(24, 18, 12, 6);
		for(int t = 0, i = 0; t < 150;) {
			float dif = rng_angle();

			play_sfx("shot1");
			for(int j = 0; j < 20; ++j) {
				PROJECTILE(
					.proto = pp_plainball,
					.pos = boss->pos,
					.color = RGB(0.04*i, 0.04*i, 0.4+0.04*i),
					.move = move_asymptotic_simple((3+i/3.0)*cdir(M_TAU/8.0*j + dif), 2.5),
				);
			}

			t += WAIT(interval);

			if(i < 15) {
				++i;
			}
		}
	}
}
