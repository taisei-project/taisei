/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "spells.h"
#include "../kurumi.h"

#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static Projectile *vapor_particle(cmplx pos, const Color *clr) {
	return PARTICLE(
		.sprite = "stain",
		.color = clr,
		.timeout = 60,
		.draw_rule = ScaleFade,
		.args = { 0, 0, 0.2 + 2.0*I },
		.pos = pos,
		.angle = M_PI*2*frand(),
	);
}

static int kdanmaku_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	int time = creal(p->args[0]);

	if(t == time) {
		p->color = *RGBA(0.6, 0.3, 1.0, 0.0);
		projectile_set_prototype(p, pp_bullet);
		p->args[1] = (global.plr.pos - p->pos) * 0.001;

		if(frand() < 0.5) {
			Projectile *v = vapor_particle(p->pos, color_mul_scalar(COLOR_COPY(&p->color), 0.5));

			if(frand() < 0.5) {
				v->flags |= PFLAG_REQUIREDPARTICLE;
			}
		}

		PARTICLE(
			.sprite = "flare",
			.color = RGB(1, 1, 1),
			.timeout = 30,
			.draw_rule = ScaleFade,
			.args = { 0, 0, 3.0 },
			.pos = p->pos,
		);

		play_sound("shot3");
	}

	if(t > time && cabs(p->args[1]) < 2) {
		p->args[1] *= 1.02;
		fapproach_p(&p->color.a, 1, 0.025);
	}

	p->pos += p->args[1];
	p->angle = carg(p->args[1]);

	return ACTION_NONE;
}

static int kdanmaku_slave(Enemy *e, int t) {
	float re;

	if(t < 0)
		return 1;

	if(!e->args[1])
		e->pos += e->args[0]*t;
	else
		e->pos += 5.0*I;

	if(creal(e->pos) <= 0)
		e->pos = I*cimag(e->pos);
	if(creal(e->pos) >= VIEWPORT_W)
		e->pos = VIEWPORT_W + I*cimag(e->pos);

	re = creal(e->pos);

	if(re <= 0 || re >= VIEWPORT_W)
		e->args[1] = 1;

	if(cimag(e->pos) >= VIEWPORT_H)
		return ACTION_DESTROY;

	if(e->args[2] && e->args[1]) {
		int i, n = 3+imax(D_Normal,global.diff);
		float speed = 1.5+0.1*global.diff;

		for(i = 0; i < n; i++) {
			cmplx p = VIEWPORT_W/(float)n*(i+psin(t*t*i*i+t*t)) + I*cimag(e->pos);
			if(cabs(p-global.plr.pos) > 60) {
				PROJECTILE(
					.proto = pp_thickrice,
					.pos = p,
					.color = RGBA(1.0, 0.5, 0.5, 0.0),
					.rule = kdanmaku_proj,
					.args = { 160, speed*0.5*cexp(2.0*I*M_PI*sin(245*t+i*i*3501)) },
					.flags = PFLAG_NOSPAWNFLARE,
				);

				if(frand() < 0.5) {
					vapor_particle(p, RGBA(0.5, 0.125 * frand(), 0.125 * frand(), 0.1));
				}
			}
		}
		play_sound_ex("redirect", 3, false);
	}

	return 1;
}

void kurumi_danmaku(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time == EVENT_DEATH)
		enemy_kill_all(&global.enemies);
	if(time < 0)
		return;

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.04)

	AT(260) {
		aniplayer_queue(&b->ani,"muda",4);
		aniplayer_queue(&b->ani,"main",0);
	}

	AT(50) {
		play_sound("laser1");
		create_lasercurve2c(b->pos, 50, 100, RGBA(1.0, 0.8, 0.8, 0.0), las_accel, 0, 0.2*cexp(I*carg(-b->pos)));
		create_lasercurve2c(b->pos, 50, 100, RGBA(1.0, 0.8, 0.8, 0.0), las_accel, 0, 0.2*cexp(I*carg(VIEWPORT_W-b->pos)));
		create_enemy3c(b->pos, ENEMY_IMMUNE, kurumi_slave_static_visual, kdanmaku_slave, 0.2*cexp(I*carg(-b->pos)), 0, 1);
		create_enemy3c(b->pos, ENEMY_IMMUNE, kurumi_slave_static_visual, kdanmaku_slave, 0.2*cexp(I*carg(VIEWPORT_W-b->pos)), 0, 0);
	}
}
