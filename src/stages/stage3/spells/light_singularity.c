/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../wriggle.h"

#include "common_tasks.h"
#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static void wriggle_singularity_laser_logic(Laser *l, int time) {
	if(time == EVENT_BIRTH) {
		l->width = 0;
		l->speed = 0;
		l->timeshift = l->timespan;
		l->unclearable = true;
		return;
	}

	if(time == 140) {
		play_sound("laser1");
	}

	laser_charge(l, time, 150, 10 + 10 * psin(l->args[0] + time / 60.0));
	l->args[3] = time / 10.0;
	l->args[0] *= cexp(I*(M_PI/500.0) * (0.7 + 0.35 * global.diff));

	l->color = *HSLA((carg(l->args[0]) + M_PI) / (M_PI * 2), 1.0, 0.5, 0.0);
}

void wriggle_light_singularity(Boss *boss, int time) {
	TIMER(&time)

	AT(EVENT_DEATH) {
		return;
	}

	time -= 120;

	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.05)
		return;
	}

	AT(0) {
		int cnt = 2 + global.diff;
		for(int i = 0; i < cnt; ++i) {
			double aofs = 0;

			if(global.diff == D_Hard || global.diff == D_Easy) {
				aofs = 0.7;
			}

			cmplx vel = 2 * cexp(I*(aofs + M_PI / 4 + M_PI * 2 * i / (double)cnt));
			double amp = (4.0/cnt) * (M_PI/5.0);
			double freq = 0.05;

			create_laser(boss->pos, 200, 10000, RGBA(0.0, 0.2, 1.0, 0.0), las_sine_expanding,
				wriggle_singularity_laser_logic, vel, amp, freq, 0);
		}

		play_sound("charge_generic");
		aniplayer_queue(&boss->ani, "main", 0);
	}

	if(time > 120) {
		play_sfx_loop("shot1_loop");
	}

	if(time == 0) {
		return;
	}

	if(!((time+30) % 300)) {
		aniplayer_queue(&boss->ani, "specialshot_charge", 1);
		aniplayer_queue(&boss->ani, "specialshot_release", 1);
		aniplayer_queue(&boss->ani, "main", 0);
	}

	if(!(time % 300)) {
		ProjPrototype *ptype = NULL;

		switch(time / 300 - 1) {
			case 0:  ptype = pp_thickrice;   break;
			case 1:  ptype = pp_rice;        break;
			case 2:  ptype = pp_bullet;      break;
			case 3:  ptype = pp_wave;        break;
			case 4:  ptype = pp_ball;        break;
			case 5:  ptype = pp_plainball;   break;
			case 6:  ptype = pp_bigball;     break;
			default: ptype = pp_soul;        break;
		}

		int cnt = 6 + 2 * global.diff;
		float colorofs = frand();

		for(int i = 0; i < cnt; ++i) {
			double a = ((M_PI*2.0*i)/cnt);
			cmplx dir = cexp(I*a);

			PROJECTILE(
				.proto = ptype,
				.pos = boss->pos,
				.color = HSLA(a/(M_PI*2) + colorofs, 1.0, 0.5, 0),
				.rule = asymptotic,
				.args = {
					dir * (1.2 - 0.2 * global.diff),
					20
				},
			);
		}

		play_sound("shot_special1");
	}

}
