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

static int wriggle_bug(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->pos += p->args[0];
	p->angle = carg(p->args[0]);

	if(global.boss && global.boss->current && !((global.frames - global.boss->current->starttime - 30) % 200)) {
		play_sound("redirect");
		p->args[0] *= cexp(I*(M_PI/3)*nfrand());
		spawn_projectile_highlight_effect(p);
	}

	return ACTION_NONE;
}

void wriggle_small_storm(Boss *w, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time < 0)
		return;

	FROM_TO_SND("shot1_loop", 0,400,5-global.diff) {
		PROJECTILE(.proto = pp_rice, .pos = w->pos, .color = RGB(1,0.5,0.2), .rule = wriggle_bug, .args = { 2*cexp(I*_i*2*M_PI/20) });
		PROJECTILE(.proto = pp_rice, .pos = w->pos, .color = RGB(1,0.5,0.2), .rule = wriggle_bug, .args = { 2*cexp(I*_i*2*M_PI/20+I*M_PI) });
	}

	GO_AT(w, 60, 120, 1)
	GO_AT(w, 180, 240, -1)

	if(!((t+200-15)%200)) {
		aniplayer_queue(&w->ani,"specialshot_charge",1);
		aniplayer_queue(&w->ani,"specialshot_hold",20);
		aniplayer_queue(&w->ani,"specialshot_release",1);
		aniplayer_queue(&w->ani,"main",0);
	}

	if(!(t%200)) {
		int i;
		play_sound("shot_special1");

		for(i = 0; i < 10+global.diff; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = w->pos,
				.color = RGB(0.1,0.3,0.0),
				.rule = asymptotic,
				.args = {
					2*cexp(I*i*2*M_PI/(10+global.diff)),
					2
				}
			);
		}
	}
}
