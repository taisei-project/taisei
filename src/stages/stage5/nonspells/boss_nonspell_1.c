/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

void iku_bolts(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	FROM_TO(0, 400, 2) {
		iku_nonspell_spawn_cloud();
	}

	FROM_TO(60, 400, 50) {
		int i, c = 10+global.diff;

		for(i = 0; i < c; i++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = b->pos,
				.color = RGBA(0.4, 1.0, 1.0, 0),
				.rule = asymptotic,
				.args = {
					(i+2)*0.4*cexp(I*carg(global.plr.pos-b->pos))+0.2*(global.diff-1)*frand(),
					3
				},
			);
		}

		play_sound("shot2");
		play_sound("redirect");
	}

	FROM_TO(0, 70, 1)
		GO_TO(b, 100+300.0*I, 0.02);

	FROM_TO(100, 200, 1)
		GO_TO(b, VIEWPORT_W/2+100.0*I, 0.02);

	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+300.0*I, 0.02);

	FROM_TO(330, 400, 1)
		GO_TO(b, VIEWPORT_W/2+100.0*I, 0.02);

}
