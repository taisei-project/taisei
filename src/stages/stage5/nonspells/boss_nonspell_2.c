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

static cmplx bolts2_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("lasers/iku_lightning");
		return 0;
	}

	double diff = creal(l->args[2]);
	return creal(l->args[0])+I*cimag(l->pos) + sign(cimag(l->args[0]-l->pos))*0.06*I*t*t + (20+4*diff)*sin(t*0.025*diff+creal(l->args[0]))*l->args[1];
}

void iku_bolts2(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	// FIXME: ANOTHER one of these... get rid of this hack when attacks have proper state
	static bool flip_laser;

	if(time == EVENT_BIRTH) {
		flip_laser = true;
	}

	FROM_TO(0, 400, 2) {
		iku_nonspell_spawn_cloud();
	}

	FROM_TO(0, 400, 60) {
		flip_laser = !flip_laser;
		aniplayer_queue(&b->ani, flip_laser ? "dashdown_left" : "dashdown_right", 1);
		aniplayer_queue(&b->ani, "main", 0);
		create_lasercurve3c(creal(global.plr.pos), 100, 200, RGBA(0.3, 1, 1, 0), bolts2_laser, global.plr.pos, flip_laser*2-1, global.diff);
		play_sfx_ex("laser1", 0, false);
	}

	FROM_TO_SND("shot1_loop", 0, 400, 5-global.diff)
		if(frand() < 0.9) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = b->pos,
				.color = RGB(0.2,0,0.8),
				.rule = linear,
				.args = { cexp(0.1*I*_i) }
			);
		}

	FROM_TO(0, 70, 1)
		GO_TO(b, 100+200.0*I, 0.02);

	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+200.0*I, 0.02);

}
