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

#include "refs.h"
#include "common_tasks.h"
#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static int wriggle_ignite_laserbullet(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return ACTION_ACK;
	} else if(time < 0) {
		return ACTION_ACK;
	}

	Laser *laser = (Laser*)REF(p->args[0]);

	if(laser) {
		p->args[3] = laser->prule(laser, time - p->args[1]) - p->pos;
	}

	p->angle = carg(p->args[3]);
	p->pos = p->pos + p->args[3];

	return ACTION_NONE;
}

static void wriggle_ignite_warnlaser_logic(Laser *l, int time) {
	if(time == EVENT_BIRTH) {
		l->width = 0;
		return;
	}

	if(time < 0) {
		return;
	}

	if(time == 90) {
		play_sound_ex("laser1", 30, false);
	}

	laser_charge(l, time, 90, 10);
	l->color = *color_lerp(RGBA(0.2, 0.2, 1, 0), RGBA(1, 0.2, 0.2, 0), time / l->deathtime);
}

static void wriggle_ignite_warnlaser(Laser *l) {
	float f = 6;
	create_laser(l->pos, 90, 120, RGBA(1, 1, 1, 0), l->prule, wriggle_ignite_warnlaser_logic, f*l->args[0], l->args[1], f*l->args[2], l->args[3]);
}

void wriggle_night_ignite(Boss *boss, int time) {
	TIMER(&time)

	float dfactor = global.diff / (float)D_Lunatic;

	if(time == EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.05)
		return;
	}

	AT(0) for(int j = -1; j < 2; j += 2) for(int i = 0; i < 7; ++i) {
		create_enemy4c(boss->pos, ENEMY_IMMUNE, wriggle_slave_visual, wriggle_spell_slave, add_ref(boss), i*2*M_PI/7, j, 1);
	}

	FROM_TO_INT(0, 1000000, 180, 120, 10) {
		float dt = 200;
		float lt = 100 * dfactor;

		float a = _ni*M_PI/2.5 + _i + time;
		float b = 0.3;
		float c = 0.3;

		cmplx vel = 2 * cexp(I*a);
		double amp = M_PI/5;
		double freq = 0.05;

		Laser *l1 = create_lasercurve3c(boss->pos, lt, dt, RGBA(b, b, 1.0, 0.0), las_sine_expanding, vel, amp, freq);
		wriggle_ignite_warnlaser(l1);

		Laser *l2 = create_lasercurve3c(boss->pos, lt * 1.5, dt, RGBA(1.0, b, b, 0.0), las_sine_expanding, vel, amp, freq - 0.002 * fmin(global.diff, D_Hard));
		wriggle_ignite_warnlaser(l2);

		Laser *l3 = create_lasercurve3c(boss->pos, lt, dt, RGBA(b, b, 1.0, 0.0), las_sine_expanding, vel, amp, freq - 0.004 * fmin(global.diff, D_Hard));
		wriggle_ignite_warnlaser(l3);

		for(int i = 0; i < 5 + 15 * dfactor; ++i) {
			#define LASERBULLLET(pproto, clr, laser) \
				PROJECTILE(.proto = (pproto), .pos = boss->pos, .color = (clr), .rule = wriggle_ignite_laserbullet, .args = { add_ref(laser), i })

			LASERBULLLET(pp_plainball, RGBA(c, c, 1.0, 0), l1);
			LASERBULLLET(pp_bigball,   RGBA(1.0, c, c, 0), l2);
			LASERBULLLET(pp_plainball, RGBA(c, c, 1.0, 0), l3);

			#undef LASERBULLLET

			// FIXME: better sound
			play_sound("shot1");
		}

		// FIXME: better sound
		play_sound_ex("shot_special1", 1, false);
	}
}
