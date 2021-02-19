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

/* !!! You are entering Akari danmaku code !!! */
#define SAFE_RADIUS_DELAY 300
#define SAFE_RADIUS_BASE 100
#define SAFE_RADIUS_STRETCH 100
#define SAFE_RADIUS_MIN 60
#define SAFE_RADIUS_MAX 150
#define SAFE_RADIUS_SPEED 0.015
#define SAFE_RADIUS_PHASE 3*M_PI/2
#define SAFE_RADIUS_PHASE_FUNC(o) ((int)(creal(e->args[2])+0.5) * M_PI/3 + SAFE_RADIUS_PHASE + fmax(0, time - SAFE_RADIUS_DELAY) * SAFE_RADIUS_SPEED)
#define SAFE_RADIUS_PHASE_NORMALIZED(o) (fmod(SAFE_RADIUS_PHASE_FUNC(o) - SAFE_RADIUS_PHASE, 2*M_PI) / (2*M_PI))
#define SAFE_RADIUS_PHASE_NUM(o) ((int)((SAFE_RADIUS_PHASE_FUNC(o) - SAFE_RADIUS_PHASE) / (2*M_PI)))
#define SAFE_RADIUS(o) smoothreclamp(SAFE_RADIUS_BASE + SAFE_RADIUS_STRETCH * sin(SAFE_RADIUS_PHASE_FUNC(o)), SAFE_RADIUS_BASE - SAFE_RADIUS_STRETCH, SAFE_RADIUS_BASE + SAFE_RADIUS_STRETCH, SAFE_RADIUS_MIN, SAFE_RADIUS_MAX)

static void ricci_laser_logic(Laser *l, int t) {
	if(t == EVENT_BIRTH) {
		// remember maximum radius, start at 0 actual radius
		l->args[1] = 0 + creal(l->args[1]) * I;
		l->width = 0;
		return;
	}

	if(t == EVENT_DEATH) {
		free_ref(l->args[2]);
		return;
	}

	Enemy *e = (Enemy*)REF(l->args[2]);

	if(e) {
		// attach to baryon
		l->pos = e->pos + l->args[3];
	}

	if(t > 0) {
		// expand then shrink radius
		l->args[1] = cimag(l->args[1]) * (I + sin(M_PI * t / (l->deathtime + l->timespan)));
		l->width = clamp(t / 4.0, 0, 10);
	}

	return;
}

static int ricci_proj2(Projectile *p, int t) {
	TIMER(&t);

	AT(EVENT_BIRTH) {
		play_sfx("shot3");
	}

	AT(EVENT_DEATH) {
		Enemy *e = (Enemy*)REF(p->args[1]);

		if(!e) {
			return ACTION_ACK;
		}

		// play_sound_ex("shot3",8, false);
		play_sfx("shot_special1");

		double rad = SAFE_RADIUS_MAX * (0.6 - 0.2 * (double)(D_Lunatic - global.diff) / 3);

		create_laser(p->pos, 12, 60, RGBA(0.2, 1, 0.5, 0), las_circle, ricci_laser_logic,  6*M_PI +  0*I, rad, add_ref(e), p->pos - e->pos);
		create_laser(p->pos, 12, 60, RGBA(0.2, 0.4, 1, 0), las_circle, ricci_laser_logic,  6*M_PI + 30*I, rad, add_ref(e), p->pos - e->pos);

		create_laser(p->pos, 1,  60, RGBA(1.0, 0.0, 0, 0), las_circle, ricci_laser_logic, -6*M_PI +  0*I, rad, add_ref(e), p->pos - e->pos);
		create_laser(p->pos, 1,  60, RGBA(1.0, 0.0, 0, 0), las_circle, ricci_laser_logic, -6*M_PI + 30*I, rad, add_ref(e), p->pos - e->pos);

		free_ref(p->args[1]);
		return ACTION_ACK;
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	Enemy *e = (Enemy*)REF(p->args[1]);

	if(!e) {
		return ACTION_DESTROY;
	}

	p->pos = e->pos + p->pos0;

	if(t > 30)
		return ACTION_DESTROY;

	return ACTION_NONE;
}

static int baryon_ricci(Enemy *e, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	int num = creal(e->args[2])+0.5;

	if(num % 2 == 1) {
		GO_TO(e, global.boss->pos,0.1);
	} else if(num == 0) {
		if(t < 150) {
			GO_TO(e, global.plr.pos, 0.1);
		} else {
			float s = 1.00 + 0.25 * (global.diff - D_Easy);
			cmplx d = e->pos - VIEWPORT_W/2-VIEWPORT_H*I*2/3 + 100*sin(s*t/200.)+25*I*cos(s*t*3./500.);
			e->pos += -0.5*d/cabs(d);
		}
	} else {
		for(Enemy *reference = global.enemies.first; reference; reference = reference->next) {
			if(reference->logic_rule == baryon_ricci && (int)(creal(reference->args[2])+0.5) == 0) {
				e->pos = global.boss->pos+(reference->pos-global.boss->pos)*cexp(I*2*M_PI*(1./6*creal(e->args[2])));
			}
		}
	}

	int time = global.frames - global.boss->current->starttime;

	if(num % 2 == 0 && SAFE_RADIUS_PHASE_NUM(e) > 0) {
		TIMER(&time);
		float phase = SAFE_RADIUS_PHASE_NORMALIZED(e);

		if(phase < 0.55 && phase > 0.15) {
			FROM_TO(150,100000,10) {
				int c = 3;
				cmplx n = cexp(2*M_PI*I * (0.25 + 1.0/c*_i));
				PROJECTILE(
					.proto = pp_ball,
					.pos = 15*n,
					.color = RGBA(0.0, 0.5, 0.1, 0.0),
					.rule = ricci_proj2,
					.args = {
						-n,
						add_ref(e),
					},
					.flags = PFLAG_NOCOLLISION,
				);
			}
		}
	}

	return 1;
}

static int ricci_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(!global.boss) {
		p->prevpos = p->pos;
		return ACTION_DESTROY;
	}

	if(p->type == PROJ_DEAD) {
		// p->pos += p->args[0];
		p->prevpos = p->pos0 = p->pos;
		p->birthtime = global.frames;
		p->rule = asymptotic;
		p->args[0] = cexp(I*M_PI*2*frand());
		p->args[1] = 9;
		return ACTION_NONE;
	}

	int time = global.frames-global.boss->current->starttime;

	cmplx shift = 0;
	p->pos = p->pos0 + p->args[0]*t;

	double influence = 0;

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->logic_rule != baryon_ricci) {
			continue;
		}

		int num = creal(e->args[2])+0.5;

		if(num % 2 == 0) {
			double radius = SAFE_RADIUS(e);
			cmplx d = e->pos-p->pos;
			float s = 1.00 + 0.25 * (global.diff - D_Easy);
			int gaps = SAFE_RADIUS_PHASE_NUM(e) + 5;
			double r = cabs(d)/(1.0-0.15*sin(gaps*carg(d)+0.01*s*time));
			double range = 1/(exp((r-radius)/50)+1);
			shift += -1.1*(radius-r)/r*d*range;
			influence += range;
		}
	}

	p->pos = p->pos0 + p->args[0]*t+shift;
	p->angle = carg(p->args[0]);
	p->prevpos = p->pos;

	float a = 0.5 + 0.5 * fmax(0,tanh((time-80)/100.))*clamp(influence,0.2,1);
	a *= fmin(1, t / 20.0f);

	p->color.r = 0.5;
	p->color.g = 0;
	p->color.b = cabs(shift)/20.0;
	p->color.a = 0;
	p->opacity = a;

	return ACTION_NONE;
}

void elly_ricci(Boss *b, int t) {
	TIMER(&t);
	AT(0)
		set_baryon_rule(baryon_ricci);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);

	int c = 15;
	float v = 3;
	int interval = 5;

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);

	FROM_TO(30, 100000, interval) {
		float w = VIEWPORT_W;
		float ofs = -1 * w/(float)c;
		w *= (1 + 2.0 / c);

		for(int i = 0; i < c; i++) {
			cmplx pos = ofs + fmod(w/(float)c*(i+0.5*_i),w) + (VIEWPORT_H+10)*I;

			PROJECTILE(
				.proto = pp_ball,
				.pos = pos,
				.color = RGBA(0, 0, 0, 0),
				.rule = ricci_proj,
				.args = { -v*I },
				.flags = PFLAG_NOSPAWNEFFECTS | PFLAG_NOCLEAR,
				.max_viewport_dist = SAFE_RADIUS_MAX,
			);
		}
	}
}

/* Thank you for visiting Akari danmaku code (tm) */

