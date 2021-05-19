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
#include "../misc.h"

#include "common_tasks.h"
#include "global.h"

TASK(crystal_wall, NO_ARGS) {
	int num_crystals = difficulty_value(18, 21, 24, 27);
	real spacing = VIEWPORT_W / (real)(num_crystals - 1);
	real ofs = rng_real() - 1;

	for(int i = 0; i < 30; ++i) {
		play_sfx("shot1");

		for(int c = 0; c < num_crystals; ++c) {
			cmplx accel = 0.02*I + 0.01*I * ((c % 2) ? 1 : -1) * sin((c * 3 + global.frames) / 30.0);
			PROJECTILE(
				.proto = pp_crystal,
				.pos = (ofs + c) * spacing + 20,
				.color = (c % 2) ? RGB(0.2, 0.2, 0.4) : RGB(0.5, 0.5, 0.5),
				.move = move_accelerated(0, accel),
				.max_viewport_dist = 16,
			);
		}

		WAIT(10);
	}
}

TASK(cirno_frostbolt_trail, { BoxedProjectile proj; }) {
	Projectile *p = TASK_BIND(ARGS.proj);
	int period = 12;

	WAIT(rng_irange(0, period + 1));

	for(;;) {
		stage1_spawn_stain(p->pos, rng_f32_angle(), 20);
		WAIT(period);
	}
}

TASK(cirno_frostbolt, { cmplx pos; cmplx vel; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_wave,
		.pos = ARGS.pos,
		.color = RGBA(0.2, 0.2, 0.4, 0.0),
		.move = move_asymptotic_simple(ARGS.vel, 5),
	));

	INVOKE_TASK(cirno_frostbolt_trail, ENT_BOX(p));
	WAIT(difficulty_value(200, 300, 400, 500));
	p->move.retention = 1.03;
}

DEFINE_EXTERN_TASK(stage1_spell_crystal_blizzard) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W / 2.0 + 300 * I, 0.1);
	BEGIN_BOSS_ATTACK(&ARGS);

	int frostbolt_period = difficulty_value(4, 3, 2, 1);

	for(;;) {
		INVOKE_SUBTASK_DELAYED(60, crystal_wall);

		int charge_time = 120;

		WAIT(330 - charge_time);
		aniplayer_queue(&boss->ani, "(9)" ,0);
		INVOKE_SUBTASK(common_charge, boss->pos, RGBA(0.5, 0.6, 2.0, 0.0), charge_time, .sound = COMMON_CHARGE_SOUNDS);
		WAIT(charge_time);

		boss->move = move_towards_power(global.plr.pos, 1, 0.1);

		for(int t = 0; t < 370; ++t) {
			play_sfx_loop("shot1_loop");
			boss->move.attraction_point = global.plr.pos;

			if(!(t % frostbolt_period)) {
				cmplx aim = cnormalize(global.plr.pos - boss->pos) * cdir(rng_sreal() * 0.2);
				cmplx vel = rng_range(0.01, 4) * aim;
				INVOKE_TASK(cirno_frostbolt, boss->pos, vel);
			}

			if(!(t % 7)) {
				play_sfx("shot1");
				int cnt = global.diff - 1;
				for(int i = 0; i < cnt; ++i) {
					PROJECTILE(
						.proto = pp_ball,
						.pos = boss->pos,
						.color = RGBA(0.1, 0.1, 0.5, 0.0),
						.move = move_accelerated(0, 0.01 * cdir(global.frames/20.0 + i*M_TAU/cnt)),
					);
				}
			}

			WAIT(1);
		}

		boss->move.attraction_point = 0;
		boss->move.attraction = 0;
		boss->move.retention = 0.8;
		aniplayer_queue(&boss->ani, "main" ,0);
	}
}
