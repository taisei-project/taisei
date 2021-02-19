/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "common_tasks.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static void lhc_laser_logic(Laser *l, int t) {
	Enemy *e;

	static_laser(l, t);

	TIMER(&t);
	AT(EVENT_DEATH) {
		free_ref(l->args[2]);
		return;
	}

	e = REF(l->args[2]);

	if(e)
		l->pos = e->pos;
}

static int baryon_lhc(Enemy *e, int t) {
	int t1 = t % 400;
	int g = (int)creal(e->args[2]);
	if(g == 0 || g == 3)
		return 1;
	TIMER(&t1);

	AT(1) {
		e->args[3] = 100.0*I+400.0*I*((t/400)&1);

		if(g == 2 || g == 5) {
			play_sfx_delayed("laser1",10,true,200);

			Laser *l = create_laser(e->pos, 200, 300, RGBA(0.1+0.9*(g>3), 0, 1-0.9*(g>3), 0), las_linear, lhc_laser_logic, (1-2*(g>3))*VIEWPORT_W*0.005, 200+30.0*I, add_ref(e), 0);
			l->unclearable = true;
		}
	}

	GO_TO(e, VIEWPORT_W*(creal(e->pos0) > VIEWPORT_W/2)+I*cimag(e->args[3]) + (100-0.4*t1)*I*(1-2*(g > 3)), 0.02);

	return 1;
}

void elly_lhc(Boss *b, int t) {
	TIMER(&t);

	AT(0)
		set_baryon_rule(baryon_lhc);
	AT(EVENT_DEATH) {
		set_baryon_rule(baryon_reset);
	}

	FROM_TO(260, 10000, 400) {
		elly_clap(b,100);
	}

	FROM_TO(280, 10000, 400) {
		int i;
		int c = 30+10*global.diff;
		cmplx pos = VIEWPORT_W/2 + 100.0*I+400.0*I*((t/400)&1);

		stage_shake_view(160);
		play_sfx("boom");

		for(i = 0; i < c; i++) {
			cmplx v = 3*cexp(2.0*I*M_PI*frand());
			tsrand_fill(4);
			create_lasercurve2c(pos, 70+20*global.diff, 300, RGBA(0.5, 0.3, 0.9, 0), las_accel, v, 0.02*frand()*copysign(1,creal(v)))->width=15;

			PROJECTILE(
				.proto = pp_soul,
				.pos = pos,
				.color = RGBA(0.4, 0.0, 1.0, 0.0),
				.rule = linear,
				.args = { (1+2.5*afrand(0))*cexp(2.0*I*M_PI*afrand(1)) },
				.flags = PFLAG_NOSPAWNFLARE,
			);

			PROJECTILE(
				.proto = pp_bigball,
				.pos = pos,
				.color = RGBA(1.0, 0.0, 0.4, 0.0),
				.rule = linear,
				.args = { (1+2.5*afrand(2))*cexp(2.0*I*M_PI*afrand(3)) },
				.flags = PFLAG_NOSPAWNFLARE,
			);

			if(i < 5) {
				PARTICLE(
					.sprite = "stain",
					.pos = pos,
					.color = RGBA(0.3, 0.3, 1.0, 0.0),
					.timeout = 60,
					.rule = linear,
					.draw_rule = ScaleFade,
					.args = { (10+2*frand())*cexp(2*M_PI*I*i/5), 0, 2+5*I },
					.flags = PFLAG_REQUIREDPARTICLE,
				);
			}
		}
	}

	FROM_TO(0, 100000,7-global.diff) {
		play_sfx_ex("shot2",10,false);
		PROJECTILE(
			.proto = pp_ball,
			.pos = b->pos,
			.color = RGBA(0.0, 0.4, 1.0, 0.0),
			.rule = asymptotic,
			.args = { cexp(2.0*I*_i), 3 },
		);
	}
}

