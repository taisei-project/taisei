/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

TASK(kurumi_blowwall_exploder, { cmplx pos; cmplx acceleration; }) {

	cmplx pos = ARGS.pos;
	for(int t = 0; pos == cwclamp(pos, 0, VIEWPORT_W + I * VIEWPORT_H); t++, YIELD)  {
		pos += ARGS.acceleration * t;
	}

	float f;
	ProjPrototype *type;

	int count = difficulty_value(60, 100, 140, 180);

	for(int i = 0; i < count; i++) {
		f = rng_real();

		if(f < 0.3) {
			type = pp_soul;
		} else if(f < 0.6) {
			type = pp_bigball;
		} else {
			type = pp_plainball;
		}

		cmplx v = (1 + 3 * f) * rng_dir();
		real baseangle = rng_angle();
		real spin = rng_range(M_PI/49, M_PI/53);
		spin *= rng_sign();

		PROJECTILE(
			.proto = type,
			.pos = pos,
			.color = RGBA(1.0, 0.1, 0.1, 0.0),
			.move = move_asymptotic_simple(v, 4),
			.flags = PFLAG_MANUALANGLE,
			.angle = baseangle,
			.angle_delta = spin,
		);
	}

	play_sfx("shot_special1");
}

static void kurumi_blowwall_laser(Boss *b, cmplx direction, bool explode) {
	cmplx acceleration = 0.1 * (1 + explode) * direction;
	create_lasercurve2c(b->pos, 50, 100, RGBA(1.0, 0.3, 0.3, 0.0), las_accel, 0, acceleration);

	if(explode) {
		play_sfx("laser1");

		INVOKE_SUBTASK(kurumi_blowwall_exploder, b->pos, acceleration);
	} else {
		// FIXME: needs a better sound
		play_sfx("shot_special1");
		play_sfx("redirect");
	}
}

DEFINE_EXTERN_TASK(kurumi_blowwall) {
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	b->move = move_from_towards(b->pos, BOSS_DEFAULT_GO_POS, 0.04);

	INVOKE_SUBTASK(common_charge, b->pos, RGBA(1, 0.3, 0.2, 0), 50, .sound = COMMON_CHARGE_SOUNDS);
	for(;;) {
		aniplayer_queue(&b->ani,"muda",0);
		WAIT(50);
		kurumi_blowwall_laser(b, cdir(0.4), true);

		WAIT(50);
		kurumi_blowwall_laser(b, cdir(M_PI-0.4), true);
		WAIT(100);
		for(int i = 0; i < 2; i++) {
			kurumi_blowwall_laser(b, cdir(-M_PI * rng_real()), true);
			WAIT(50);
		}
		play_sfx("laser1");
		for(int i = 0; i < 20; i++) {
			kurumi_blowwall_laser(b, cdir(M_PI / 10 * i), false);
			WAIT(10);
		}
		INVOKE_SUBTASK(common_charge, b->pos, RGBA(1, 0.3, 0.2, 0), 100, .sound = COMMON_CHARGE_SOUNDS);
		WAIT(50);
	}
}
