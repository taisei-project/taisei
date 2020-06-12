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

TASK(move_frozen, { BoxedProjectileArray *parray; }) {
	DECLARE_ENT_ARRAY(Projectile, projs, ARGS.parray->size);
	ENT_ARRAY_FOREACH(ARGS.parray, Projectile *p, {
		ENT_ARRAY_ADD(&projs, p);
	});

	ENT_ARRAY_FOREACH(&projs, Projectile *p, {
		cmplx dir = rng_dir();
		p->move = move_asymptotic_halflife(0, 4 * dir, rng_range(60, 80));
		p->color = *RGB(0.9, 0.9, 0.9);

		stage1_spawn_stain(p->pos, p->angle, 30);
		spawn_projectile_highlight_effect(p);
		play_sound_ex("shot2", 0, false);

		if(rng_chance(0.4)) {
			YIELD;
		}
	});
}

DEFINE_EXTERN_TASK(stage1_spell_perfect_freeze) {
	Boss *boss = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	for(int run = 1;;run++) {
		boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.04);

		INVOKE_SUBTASK(common_charge, 0, RGBA(1.0, 0.5, 0.0, 0), 40, .anchor = &boss->pos, .sound = COMMON_CHARGE_SOUNDS);
		WAIT(40);

		int n = global.diff;
		int nfrog = n*60;

		DECLARE_ENT_ARRAY(Projectile, projs, nfrog);

		for(int i = 0; i < nfrog/n; i++) {
			play_sfx_loop("shot1_loop");

			float r = rng_f32();
			float g = rng_f32();
			float b = rng_f32();

			for(int j = 0; j < n; j++) {
				float speed = rng_range(1.0f, 5.0f + 0.5f * global.diff);

				ENT_ARRAY_ADD(&projs, PROJECTILE(
					.proto = pp_ball,
					.pos = boss->pos,
					.color = RGB(r, g, b),
					.move = move_linear(speed * rng_dir()),
				));
			}
			YIELD;
		}
		WAIT(20);

		ENT_ARRAY_FOREACH(&projs, Projectile *p, {
			stage1_spawn_stain(p->pos, p->angle, 30);
			stage1_spawn_stain(p->pos, p->angle, 30);
			spawn_projectile_highlight_effect(p);
			play_sound("shot_special1");

			p->color = *RGB(0.9, 0.9, 0.9);
			p->move.retention = 0.8;

			if(rng_chance(0.2)) {
				YIELD;
			}
		});

		WAIT(60);

		double dir = rng_sign();
		boss->move = (MoveParams){ .velocity = dir*2.7+I, .retention = 0.99, .acceleration = -dir*0.017 };

		int charge_time = difficulty_value(85, 80, 75, 70);
		aniplayer_queue(&boss->ani, "(9)", 0);

		INVOKE_SUBTASK(common_charge, +60, RGBA(0.3, 0.4, 0.9, 0), charge_time, .anchor = &boss->pos, .sound = COMMON_CHARGE_SOUNDS);
		INVOKE_SUBTASK(common_charge, -60, RGBA(0.3, 0.4, 0.9, 0), charge_time, .anchor = &boss->pos);
		WAIT(charge_time);

		INVOKE_SUBTASK_DELAYED(120, move_frozen, &projs);

		int d = imax(0, global.diff - D_Normal);
		for(int i = 0; i < 30+10*d; i++) {
			play_sfx_loop("shot1_loop");
			float r1, r2;

			if(global.diff > D_Normal) {
				r1 = sin(i/M_PI*5.3) * cos(2*i/M_PI*5.3);
				r2 = cos(i/M_PI*5.3) * sin(2*i/M_PI*5.3);
			} else {
				r1 = rng_f32();
				r2 = rng_f32();
			}

			cmplx aim = cnormalize(global.plr.pos - boss->pos);
			float speed = 2+0.2*global.diff;

			for(int sign = -1; sign <= 1; sign += 2) {
				PROJECTILE(
					.proto = pp_rice,
					.pos = boss->pos + sign*60,
					.color = RGB(0.3, 0.4, 0.9),
					.move = move_asymptotic_simple(speed*aim*cdir(0.5*(sign > 0 ? r1 : r2)), 2.5+(global.diff>D_Normal)*0.1*sign*I),
				);
			}
			WAIT(6-global.diff/2);
		}
		aniplayer_queue(&boss->ani,"main",0);

		WAIT(20-5*global.diff);
	}
}
