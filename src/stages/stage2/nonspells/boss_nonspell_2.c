/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

#include "global.h"

void hina_cards2(Boss *h, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	hina_cards1(h, time);

	GO_AT(h, 100, 200, 2);
	GO_AT(h, 260, 460, -2);
	GO_AT(h, 460, 500, 5);

	AT(100) {
		int i;
		for(i = 0; i < 30; i++) {
			play_sound("shot_special1");
			PROJECTILE(
				.proto = pp_bigball,
				.pos = h->pos,
				.color = RGB(0.7, 0, 0.7),
				.rule = asymptotic,
				.args = {
					2*cexp(I*2*M_PI*i/20.0),
					3
				}
			);
		}
	}
}
