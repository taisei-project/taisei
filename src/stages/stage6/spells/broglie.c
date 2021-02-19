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

#define BROGLIE_PERIOD (420 - 30 * global.diff)

static int broglie_particle(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[0]);
		return ACTION_ACK;
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	int scattertime = creal(p->args[1]);

	if(t < scattertime) {
		Laser *laser = (Laser*)REF(p->args[0]);

		if(laser) {
			cmplx oldpos = p->pos;
			p->pos = laser->prule(laser, fmin(t, cimag(p->args[1])));

			if(oldpos != p->pos) {
				p->angle = carg(p->pos - oldpos);
			}
		}
	} else {
		if(t == scattertime && p->type != PROJ_DEAD) {
			projectile_set_layer(p, LAYER_BULLET);
			p->flags &= ~(PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT | PFLAG_NOCOLLISION);

			double angle_ampl = creal(p->args[3]);
			double angle_freq = cimag(p->args[3]);

			p->angle += angle_ampl * sin(t * angle_freq) * cos(2 * t * angle_freq);
			p->args[2] = -cabs(p->args[2]) * cexp(I*p->angle);

			play_sfx("redirect");
		}

		p->angle = carg(p->args[2]);
		p->pos = p->pos + p->args[2];
	}

	return 1;
}

static void broglie_laser_logic(Laser *l, int t) {
	double hue = cimag(l->args[3]);

	if(t == EVENT_BIRTH) {
		l->color = *HSLA(hue, 1.0, 0.5, 0.0);
	}

	if(t < 0) {
		return;
	}

	int dt = l->timespan * l->speed;
	float charge = fmin(1, pow((double)t / dt, 4));
	l->color = *HSLA(hue, 1.0, 0.5 + 0.2 * charge, 0.0);
	l->width_exponent = 1.0 - 0.5 * charge;
}


static int broglie_charge(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		play_sfx_ex("shot3", 10, false);
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	int firetime = creal(p->args[1]);

	if(t == firetime) {
		play_sfx_ex("laser1", 10, true);
		play_sfx("boom");

		stage_shake_view(20);

		Color clr = p->color;
		clr.a = 0;

		PARTICLE(
			.sprite = "blast",
			.pos = p->pos,
			.color = &clr,
			.timeout = 35,
			.draw_rule = GrowFade,
			.args = { 0, 2.4 },
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		int attack_num = creal(p->args[2]);
		double hue = creal(p->args[3]);

		p->pos -= p->args[0] * 15;
		cmplx aim = cexp(I*p->angle);

		double s_ampl = 30 + 2 * attack_num;
		double s_freq = 0.10 + 0.01 * attack_num;

		for(int lnum = 0; lnum < 2; ++lnum) {
			Laser *l = create_lasercurve4c(p->pos, 75, 100, RGBA(1, 1, 1, 0), las_sine,
				5*aim, s_ampl, s_freq, lnum * M_PI +
				I*(hue + lnum * (M_PI/12)/(M_PI/2)));

			l->width = 20;
			l->lrule = broglie_laser_logic;

			int pnum = 0;
			double inc = pow(1.10 - 0.10 * (global.diff - D_Easy), 2);

			for(double ofs = 0; ofs < l->deathtime; ofs += inc, ++pnum) {
				bool fast = global.diff == D_Easy || pnum & 1;

				PROJECTILE(
					.proto = fast ? pp_thickrice : pp_rice,
					.pos = l->prule(l, ofs),
					.color = HSLA(hue + lnum * (M_PI/12)/(M_PI/2), 1.0, 0.5, 0.0),
					.rule = broglie_particle,
					.args = {
						add_ref(l),
						I*ofs + l->timespan + ofs - 10,
						fast ? 2.0 : 1.5,
						(1 + 2 * ((global.diff - 1) / (double)(D_Lunatic - 1))) * M_PI/11 + s_freq*10*I
					},
					.layer = LAYER_NODRAW,
					.flags = PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT | PFLAG_NOSPAWNEFFECTS | PFLAG_NOCOLLISION,
				);
			}
		}

		return ACTION_DESTROY;
	} else {
		float f = pow(clamp((140 - (firetime - t)) / 90.0, 0, 1), 8);
		cmplx o = p->pos - p->args[0] * 15;
		p->args[0] *= cexp(I*M_PI*0.2*f);
		p->pos = o + p->args[0] * 15;

		if(f > 0.1) {
			play_sfx_loop("charge_generic");

			cmplx n = cexp(2.0*I*M_PI*frand());
			float l = 50*frand()+25;
			float s = 4+f;

			Color *clr = color_lerp(RGB(1, 1, 1), &p->color, clamp((1 - f * 0.5), 0.0, 1.0));
			clr->a = 0;

			PARTICLE(
				.sprite = "flare",
				.pos = p->pos+l*n,
				.color = clr,
				.draw_rule = Fade,
				.rule = linear,
				.timeout = l/s,
				.args = { -s*n },
			);
		}
	}

	return ACTION_NONE;
}

static int baryon_broglie(Enemy *e, int t) {
	if(t < 0) {
		return 1;
	}

	if(!global.boss) {
		return ACTION_DESTROY;
	}

	int period = BROGLIE_PERIOD;
	int t_real = t;
	int attack_num = t_real / period;
	t %= period;

	TIMER(&t);
	Enemy *master = NULL;

	for(Enemy *en = global.enemies.first; en; en = en->next) {
		if(en->visual_rule == baryon) {
			master = en;
			break;
		}
	}

	assert(master != NULL);

	if(master != e) {
		if(t_real < period) {
			GO_TO(e, global.boss->pos, 0.03);
		} else {
			e->pos = global.boss->pos;
		}

		return 1;
	}

	int delay = 140;
	int step = 15;
	int cnt = 3;
	int fire_delay = 120;

	static double aim_angle;

	AT(delay) {
		elly_clap(global.boss,fire_delay);
		aim_angle = carg(e->pos - global.boss->pos);
	}

	FROM_TO(delay, delay + step * cnt - 1, step) {
		double a = 2*M_PI * (0.25 + 1.0/cnt*_i);
		cmplx n = cexp(I*a);
		double hue = (attack_num * M_PI + a + M_PI/6) / (M_PI*2);

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos + 15*n,
			.color = HSLA(hue, 1.0, 0.55, 0.0),
			.rule = broglie_charge,
			.args = {
				n,
				(fire_delay - step * _i),
				attack_num,
				hue
			},
			.flags = PFLAG_NOCLEAR,
			.angle = (2*M_PI*_i)/cnt + aim_angle,
		);
	}

	if(t < delay /*|| t > delay + fire_delay*/) {
		cmplx target_pos = global.boss->pos + 100 * cexp(I*carg(global.plr.pos - global.boss->pos));
		GO_TO(e, target_pos, 0.03);
	}

	return 1;
}

void elly_broglie(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		set_baryon_rule(baryon_broglie);
	}

	AT(EVENT_DEATH) {
		set_baryon_rule(baryon_reset);
	}

	if(t < 0) {
		return;
	}

	int period = BROGLIE_PERIOD;
	double ofs = 100;

	cmplx positions[] = {
		VIEWPORT_W-ofs + ofs*I,
		VIEWPORT_W-ofs + ofs*I,
		ofs + (VIEWPORT_H-ofs)*I,
		ofs + ofs*I,
		ofs + ofs*I,
		VIEWPORT_W-ofs + (VIEWPORT_H-ofs)*I,
	};

	if(t/period > 0) {
		GO_TO(b, positions[(t/period) % (sizeof(positions)/sizeof(cmplx))], 0.02);
	}
}
