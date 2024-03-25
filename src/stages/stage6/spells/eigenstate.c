/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "common_tasks.h"

TASK(eigenstate_baryons, { BoxedBoss boss; BoxedEllyBaryons baryons; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);

	cmplx boss_pos = NOT_NULL(ENT_UNBOX(ARGS.boss))->pos;
	for(int t = 0;; t++) {
		for(int i = 0; i < NUM_BARYONS; i++) {
			cmplx pos0 = 150 * cdir(M_TAU / NUM_BARYONS * i);

			cmplx offset = 40 * sin(0.03 * t + M_PI * (re(pos0) > 0)) + 45 * im(cnormalize(pos0)) * I * sin(0.06*t);

			baryons->target_poss[i] = boss_pos + pos0 + offset;
		}
		YIELD;
	}
}	

TASK(eigenstate_bullet, { cmplx pos; cmplx v0; cmplx v1; Color color; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_plainball,
		.pos = ARGS.pos,
		.color = &ARGS.color,
		.move = move_asymptotic_simple(ARGS.v0, 1)
	));

	WAIT(60);

	play_sfx("redirect");
	p->move = move_linear(ARGS.v1);
}

TASK(eigenstate_spawn_bullets, { BoxedEllyBaryons baryons; int baryon_idx; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);
	
	int interval = difficulty_value(138, 126, 114, 102);
	real deflection = difficulty_value(0.3, 0.2, 0.1, 0);

	for(int t = 0;; t++) {
		play_sfx("shot_special1");

		int count = 9;
		for(int i = 0; i < count; i++) {
			cmplx v0 = cdir(2 * t + ARGS.baryon_idx) * I;

			for(int j = 0; j < 3; j++) {
				cmplx v1 = v0 * (1 + 0.6 * I * (j-1) * cdir(deflection));
				
				INVOKE_TASK(eigenstate_bullet,
					    .pos = baryons->poss[ARGS.baryon_idx] + 60 * cdir(M_TAU/count*i),
					    .v0 = v0,
					    .v1 = v1,
					    .color = *RGBA(j == 0, j == 1, j == 2, 0.0)
				);
			}
		}
		WAIT(interval);
	}
}

DEFINE_EXTERN_TASK(stage6_spell_eigenstate) {
	Boss *boss = stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK(eigenstate_baryons, ENT_BOX(boss), ARGS.baryons);
	aniplayer_queue(&boss->ani, "snipsnip", 0);

	WAIT(100);
	for(int baryon_idx = 0; baryon_idx < NUM_BARYONS; baryon_idx++) {
		INVOKE_SUBTASK(eigenstate_spawn_bullets, ARGS.baryons, baryon_idx);
		WAIT(20);
	}

	STALL;
}
