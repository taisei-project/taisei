/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "global.h"

DEFINE_EXTERN_TASK(stage2_spell_amulet_of_harm) {
	Boss *boss = INIT_BOSS_ATTACK();
	boss->move = move_towards(VIEWPORT_W/2 + 200.0*I, 0.02);
	BEGIN_BOSS_ATTACK();

	ProjPrototype *t0 = pp_ball;
	ProjPrototype *t1 = global.diff == D_Easy ? t0 : pp_crystal;

	int cycleduration = 200;
	int loopduration = cycleduration * (global.diff + 0.5) / (D_Lunatic + 0.5);

	real sf = 1.0 / sqrt(0.5 + global.diff);
	real speed = sf * 2.00 * (1.0 + 0.75 * imax(0, global.diff - D_Normal));
	real accel = sf * 0.01 * (1.0 + 1.20 * imax(0, global.diff - D_Normal));

	for(int t = 0;;) {
		int i = 0;

		aniplayer_soft_switch(&boss->ani, "guruguru", 0);

		while(i < loopduration) {
			play_sfx_loop("shot1_loop");

			real f = i / 30.0;
			cmplx n = cdir(M_TAU * f + 0.7 * t / 200);
			n *= cnormalize(global.plr.pos - boss->pos);

			cmplx p = boss->pos + 30 * log(1 + i/2.0) * n;

			PROJECTILE(
				.proto = t0,
				.pos = p,
				.color = RGB(0.8, 0.0, 0.0),
				.move = move_accelerated(speed * n * I, accel * -n),
			);

			PROJECTILE(
				.proto = t1,
				.pos = p,
				.color = RGB(0.8, 0.0, 0.5),
				.move = move_accelerated(speed * -n * I, accel * -n),
			);

			++t;
			++i;
			WAIT(1);
		}

		aniplayer_soft_switch(&boss->ani, "main", 0);

		WAIT(cycleduration - i);
	}
}
