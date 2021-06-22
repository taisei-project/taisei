/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../scuttle.h"

#include "common_tasks.h"
#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static int scuttle_poison(Projectile *p, int time) {
	int result = accelerated(p, time);

	if(time < 0)
		return result;

	if(!(time % (57 - global.diff * 3)) && p->type != PROJ_DEAD) {
		float a = p->args[2];
		float t = p->args[3] + time;

		PROJECTILE(
			.proto = (frand() > 0.5)? pp_thickrice : pp_rice,
			.pos = p->pos,
			.color = RGB(0.3, 0.7 + 0.3 * psin(a/3.0 + t/20.0), 0.3),
			.rule = accelerated,
			.args = {
				0,
				0.005*cexp(I*(M_PI*2 * sin(a/5.0 + t/20.0))),
			},
		);

		play_sound("redirect");
	}

	return result;
}

void scuttle_deadly_dance(Boss *boss, int time) {
	int i;
	TIMER(&time)

	if(time < 0) {
		return;
	}

	AT(0) {
		aniplayer_queue(&boss->ani, "dance", 0);
	}
	play_sfx_loop("shot1_loop");

	FROM_TO(0, 120, 1)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.03)

	if(time > 30) {
		float angle_ofs = frand() * M_PI * 2;
		double t = time * 1.5 * (0.4 + 0.3 * global.diff);
		double moverad = fmin(160, time/2.7);
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2 + sin(t/50.0) * moverad * cexp(I * M_PI_2 * t/100.0), 0.03)

		if(!(time % 70)) {
			for(i = 0; i < 15; ++i) {
				double a = M_PI/(5 + global.diff) * i * 2;
				PROJECTILE(
					.proto = pp_wave,
					.pos = boss->pos,
					.color = RGB(0.3, 0.3 + 0.7 * psin(a*3 + time/50.0), 0.3),
					.rule = scuttle_poison,
					.args = {
						0,
						0.02 * cexp(I*(angle_ofs+a+time/10.0)),
						a,
						time
					}
				);
			}

			play_sound("shot_special1");
		}

		if(global.diff > D_Easy && !(time % 35)) {
			int cnt = global.diff * 2;
			for(i = 0; i < cnt; ++i) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = boss->pos,
					.color = RGB(1.0, 1.0, 0.3),
					.rule = asymptotic,
					.args = {
						(0.5 + 3 * psin(time + M_PI/3*2*i)) * cexp(I*(angle_ofs + time / 20.0 + M_PI/cnt*i*2)),
						1.5
					}
				);
			}

			play_sound("shot1");
		}
	}

	if(!(time % 3)) {
		for(i = -1; i < 2; i += 2) {
			double c = psin(time/10.0);
			PROJECTILE(
				.proto = pp_crystal,
				.pos = boss->pos,
				.color = RGBA_MUL_ALPHA(0.3 + c * 0.7, 0.6 - c * 0.3, 0.3, 0.7),
				.rule = linear,
				.args = {
					10 * cexp(I*(carg(global.plr.pos - boss->pos) + (M_PI/4.0 * i * (1-time/2500.0)) * (1 - 0.5 * psin(time/15.0))))
				}
			);
		}
	}
}
