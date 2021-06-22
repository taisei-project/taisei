/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../wriggle.h"

#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static int wriggle_nonspell_slave(Enemy *e, int time) {
	TIMER(&time)

	int level = e->args[3];
	float angle = e->args[2] * (time / 70.0 + e->args[1]);
	cmplx dir = cexp(I*angle);
	Boss *boss = (Boss*)REF(e->args[0]);

	if(!boss)
		return ACTION_DESTROY;

	AT(EVENT_DEATH) {
		free_ref(e->args[0]);
		return 1;
	}

	if(time < 0)
		return 1;

	GO_TO(e, boss->pos + (100 + 20 * e->args[2] * sin(time / 100.0)) * dir, 0.03)

	int d = 10 - global.diff;
	if(level > 2)
		d += 4;

	if(!(time % d)) {
		play_sound("shot1");

		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.color = RGB(0.7, 0.2, 0.1),
			.rule = linear,
			.args = { 3 * cexp(I*carg(boss->pos - e->pos)) },
		);

		if(!(time % (d*2)) || level > 1) {
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = e->pos,
				.color = RGB(0.7, 0.7, 0.1),
				.rule = linear,
				.args = { 2.5 * cexp(I*carg(boss->pos - e->pos)) },
			);
		}

		if(level > 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = RGB(0.3, 0.1 + 0.6 * psin(time / 25.0), 0.7),
				.rule = linear,
				.args = { 2 * cexp(I*carg(boss->pos - e->pos)) },
			);
		}
	}

	return 1;
}

static void wriggle_nonspell_common(Boss *boss, int time, int level) {
	TIMER(&time)
	int i, j, cnt = 3 + global.diff;

	AT(0) for(j = -1; j < 2; j += 2) for(i = 0; i < cnt; ++i)
		create_enemy4c(boss->pos, ENEMY_IMMUNE, wriggle_slave_visual, wriggle_nonspell_slave, add_ref(boss), i*2*M_PI/cnt, j, level);

	AT(EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.05)
		return;
	}

	FROM_TO(120, 240, 1)
		GO_TO(boss, VIEWPORT_W/3 + VIEWPORT_H*I/3, 0.03)

	FROM_TO(360, 480, 1)
		GO_TO(boss, VIEWPORT_W - VIEWPORT_W/3 + VIEWPORT_H*I/3, 0.03)

	FROM_TO(600, 720, 1)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.03)
}

void stage3_boss_nonspell1(Boss *boss, int time) {
	wriggle_nonspell_common(boss, time, 1);
}

void stage3_boss_nonspell2(Boss *boss, int time) {
	wriggle_nonspell_common(boss, time, 2);
}

void stage3_boss_nonspell3(Boss *boss, int time) {
	wriggle_nonspell_common(boss, time, 3);
}
