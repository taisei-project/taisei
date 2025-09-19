/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "nonspells.h"

#define SNOWFLAKE_ARMS 6

static int snowflake_bullet_limit(int size) {
	// >= number of bullets spawned per snowflake of this size
	return SNOWFLAKE_ARMS * 4 * size;
}

TASK(make_snowflake, {
	cmplx pos;
	MoveParams move;
	int size;
	real rot_angle;
	BoxedProjectileArray *array;
}) {
	const real spacing = 12;
	const int split = 3;
	int t = 0;

	for(int j = 0; j < ARGS.size; j++) {
		play_sfx_loop("shot1_loop");

		for(int i = 0; i < SNOWFLAKE_ARMS; i++) {
			real ang = M_TAU / SNOWFLAKE_ARMS * i + ARGS.rot_angle;
			cmplx phase = cdir(ang);
			cmplx pos0 = ARGS.pos + spacing * j * phase;

			Projectile *p;

			for(int side = -1; side <= 1; side += 2) {
				p = PROJECTILE(
					.proto = pp_crystal,
					.pos = pos0 + side * 5 * I * phase,
					.color = RGB(0.0 + 0.05 * j, 0.1 + 0.1 * j, 0.9),
					.move = ARGS.move,
					.angle = ang + side * M_PI / 4,
					.max_viewport_dist = 128,
					.flags = PFLAG_MANUALANGLE,
				);
				move_update_multiple(t, &p->pos, &p->move);
				p->pos0 = p->prevpos = p->pos;
				ENT_ARRAY_ADD(ARGS.array, p);
			}

			WAIT(1);
			++t;

			if(j > split) {
				cmplx pos1 = ARGS.pos + spacing * split * phase;

				for(int side = -1; side <= 1; side += 2) {
					cmplx phase2 = cdir(M_PI / 4 * side) * phase;
					cmplx pos2 = pos1 + (spacing * (j - split)) * phase2;

					p = PROJECTILE(
						.proto = pp_crystal,
						.pos = pos2,
						.color = RGB(0.0, 0.3 * ARGS.size / 5, 1),
						.move = ARGS.move,
						.angle = ang + side * M_PI / 4,
						.max_viewport_dist = 128,
						.flags = PFLAG_MANUALANGLE,
					);
					move_update_multiple(t, &p->pos, &p->move);
					p->pos0 = p->prevpos = p->pos;
					ENT_ARRAY_ADD(ARGS.array, p);
				}

				WAIT(1);
				++t;
			}
		}
	}
}

DEFINE_EXTERN_TASK(stage1_midboss_nonspell_1) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, CMPLX(VIEWPORT_W/2, 200), 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_dampen(boss->move.velocity, 0.8);

	int flake_spawn_interval = difficulty_value(11, 10, 9, 8);
	int flakes_per_burst = difficulty_value(3, 5, 7, 9);
	real launch_speed = difficulty_value(5, 6.25, 6.875, 8.75);
	int size_base = 5;
	int size_oscillation = 3;
	int size_max = size_base + size_oscillation;
	int burst_interval = difficulty_value(200, 120, 80, 80);
	int charge_time = difficulty_value(80, 60, 60, 60);
	int split_delay = difficulty_value(85, 75, 65, 65);
	burst_interval -= charge_time;

	int flakes_limit = flakes_per_burst * snowflake_bullet_limit(size_max);
	DECLARE_ENT_ARRAY(Projectile, snowflake_projs, flakes_limit);

	for(int burst = 0;; ++burst) {
		aniplayer_queue(&boss->ani, "(9)", 0);
		common_charge_static(charge_time, boss->pos, RGBA(0, 0.5, 1.0, 0.0));
		aniplayer_queue(&boss->ani, "main", 0);

		real angle_ofs = carg(global.plr.pos - boss->pos);
		int size = size_base + size_oscillation * sin(burst * 2.21);

		for(int flake = 0; flake < flakes_per_burst; ++flake) {
			real angle = circle_angle(flake + flakes_per_burst / 2, flakes_per_burst) + angle_ofs;
			MoveParams move = move_asymptotic(launch_speed * cdir(angle), 0, 0.95);
			INVOKE_SUBTASK(make_snowflake, boss->pos, move, size, angle, &snowflake_projs);
			WAIT(flake_spawn_interval);
		}

		WAIT(split_delay - 4 * (size_base + size_oscillation - size));

		play_sfx("redirect");
		// play_sfx("shot_special1");

		ENT_ARRAY_FOREACH(&snowflake_projs, Projectile *p, {
			spawn_projectile_highlight_effect(p)->opacity = 0.25;
			color_lerp(&p->color, RGB(0.5, 0.5, 0.5), 0.5);
			p->move.velocity = 2 * cdir(p->angle);
			p->move.acceleration = -cdir(p->angle) * difficulty_value(0.1, 0.15, 0.2, 0.2);
		});

		ENT_ARRAY_CLEAR(&snowflake_projs);
		WAIT(burst_interval);
	}
}
