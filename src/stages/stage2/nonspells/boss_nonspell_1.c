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

void hina_cards1(Boss *h, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	AT(0) {
		aniplayer_queue(&h->ani, "guruguru", 0);
	}
	FROM_TO(0, 500, 2-(global.diff > D_Normal)) {
		play_sound_ex("shot1", 4, false);
		PROJECTILE(.proto = pp_card, .pos = h->pos+50*cexp(I*t/10), .color = RGB(0.8,0.0,0.0), .rule = asymptotic, .args = { (1.6+0.4*global.diff)*cexp(I*t/5.0), 3 });
		PROJECTILE(.proto = pp_card, .pos = h->pos-50*cexp(I*t/10), .color = RGB(0.0,0.0,0.8), .rule = asymptotic, .args = {-(1.6+0.4*global.diff)*cexp(I*t/5.0), 3 });
	}
}
