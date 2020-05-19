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

#define SLOTS 5

static int bad_pick_bullet(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->pos += p->args[0];
	p->args[0] += p->args[1];

	// reflection
	int slot = (int)(creal(p->pos)/(VIEWPORT_W/SLOTS)+1)-1; // correct rounding for slot == -1
	int targetslot = creal(p->args[2])+0.5;
	if(slot != targetslot)
		p->args[0] = copysign(creal(p->args[0]),targetslot-slot)+I*cimag(p->args[0]);

	return ACTION_NONE;
}

void hina_bad_pick(Boss *h, int time) {
	int t = time % 500;
	int i, j;

	TIMER(&t);

	if(time < 0)
		return;

	GO_TO(h, VIEWPORT_W/SLOTS*((113*(time/500))%SLOTS+0.5)+ 100.0*I, 0.02);

	FROM_TO(100, 500, 5) {
		play_sound_ex("shot1", 4, false);

		for(i = 1; i < SLOTS; i++) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = VIEWPORT_W/SLOTS*i,
				.color = RGB(0.5,0,0.6),
				.rule = linear,
				.args = { 7.0*I }
			);
		}

		if(global.diff >= D_Hard) {
			double shift = 0;
			if(global.diff == D_Lunatic)
				shift = 0.3*fmax(0,t-200);
			for(i = 1; i < SLOTS; i++) {
				double height = VIEWPORT_H/SLOTS*i+shift;
				if(height > VIEWPORT_H-40)
					height -= VIEWPORT_H-40;

				PROJECTILE(
					.proto = pp_crystal,
					.pos = (i&1)*VIEWPORT_W+I*height,
					.color = RGB(0.5,0,0.6),
					.rule = linear,
					.args = { 5.0*(1-2*(i&1)) }
				);
			}
		}
	}

	AT(190) {
		aniplayer_queue(&h->ani,"guruguru",2);
		aniplayer_queue(&h->ani,"main",0);
	}

	AT(200) {
		play_sound("shot_special1");

		int win = tsrand()%SLOTS;
		for(i = 0; i < SLOTS; i++) {
			if(i == win)
				continue;

			float cnt = (1+imin(D_Normal,global.diff)) * 5;
			for(j = 0; j < cnt; j++) {
				cmplx o = VIEWPORT_W/SLOTS*(i + j/(cnt-1));

				PROJECTILE(
					.proto = pp_ball,
					.pos = o,
					.color = RGBA(0.7, 0.0, 0.0, 0.0),
					.rule = bad_pick_bullet,
					.args = {
						0,
						0.005*nfrand() + 0.005*I * (1 + psin(i + j + global.frames)),
						i
					},
				);
			}
		}
	}
}
