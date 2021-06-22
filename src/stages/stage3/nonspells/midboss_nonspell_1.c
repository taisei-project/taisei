/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "nonspells.h"
#include "../scuttle.h"

#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static int scuttle_lethbite_proj(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	#define A0_PROJ_START 120
	#define A0_PROJ_CHARGE 20
	TIMER(&time)

	FROM_TO(A0_PROJ_START, A0_PROJ_START + A0_PROJ_CHARGE, 1)
		return 1;

	AT(A0_PROJ_START + A0_PROJ_CHARGE + 1) if(p->type != PROJ_DEAD) {
		p->args[1] = 3;
		p->args[0] = (3 + 2 * global.diff / (float)D_Lunatic) * cexp(I*carg(global.plr.pos - p->pos));

		int cnt = 3, i;
		for(i = 0; i < cnt; ++i) {
			tsrand_fill(2);

			PARTICLE(
				.sprite = "smoothdot",
				.color = RGBA(0.8, 0.6, 0.6, 0),
				.draw_rule = Shrink,
				.rule = enemy_flare,
				.timeout = 100,
				.args = {
					cexp(I*(M_PI*anfrand(0))) * (1 + afrand(1)),
					add_ref(p)
				},
			);

			float offset = global.frames/15.0;
			if(global.diff > D_Hard && global.boss) {
				offset = M_PI+carg(global.plr.pos-global.boss->pos);
			}

			PROJECTILE(
				.proto = pp_thickrice,
				.pos = p->pos,
				.color = RGB(0.4, 0.3, 1.0),
				.rule = linear,
				.args = {
					-cexp(I*(i*2*M_PI/cnt + offset)) * (1.0 + (global.diff > D_Normal))
				},
			);
		}

		play_sound("redirect");
		play_sound("shot1");
		spawn_projectile_highlight_effect(p);
	}

	return asymptotic(p, time);
	#undef A0_PROJ_START
	#undef A0_PROJ_CHARGE
}

void stage3_midboss_nonspell1(Boss *boss, int time) {
	int i;
	TIMER(&time)

	GO_TO(boss, VIEWPORT_W/2+VIEWPORT_W/3*sin(time/300) + I*cimag(boss->pos), 0.01)

	FROM_TO_INT(0, 90000, 72 + 6 * (D_Lunatic - global.diff), 0, 1) {
		int cnt = 21 - 1 * (D_Lunatic - global.diff);

		for(i = 0; i < cnt; ++i) {
			cmplx v = (2 - psin((fmax(3, global.diff+1)*2*M_PI*i/(float)cnt) + time)) * cexp(I*2*M_PI/cnt*i);
			PROJECTILE(
				.proto = pp_wave,
				.pos = boss->pos - v * 50,
				.color = _i % 2? RGB(0.7, 0.3, 0.0) : RGB(0.3, .7, 0.0),
				.rule = scuttle_lethbite_proj,
				.args = { v, 2.0 },
			);
		}

		// FIXME: better sound
		play_sound("shot_special1");
	}
}
