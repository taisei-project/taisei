/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "../stagex.h"
#include "global.h"
#include "common_tasks.h"

TASK(pipe_conversions, { BoxedBoss bbox; BoxedProjectileArray *projs; }) {

}

DEFINE_EXTERN_TASK(stagex_spell_pipe_dream) {
	Boss *boss = INIT_BOSS_ATTACK();
	boss->move = move_towards(CMPLX(VIEWPORT_W/2, VIEWPORT_H/2), 0.02);
	BEGIN_BOSS_ATTACK();

	int enough = 1000;
	DECLARE_ENT_ARRAY(Projectile, projs, enough);

	INVOKE_SUBTASK(pipe_conversions, ENT_BOX(boss), &projs);

	real golden = (1+sqrt(5))/2;
	for(int i = 0; ; i++) {
		ENT_ARRAY_ADD(&projs, PROJECTILE(
			.proto = pp_wave,
			.pos = boss->pos,
			.color = RGB(1, 0, 0),
			.move = move_linear(5*cdir(M_TAU/golden/golden*i)),
		));
		YIELD;
		if(i % 100 == 0) {
			ENT_ARRAY_COMPACT(&projs);
		}
	}
}
