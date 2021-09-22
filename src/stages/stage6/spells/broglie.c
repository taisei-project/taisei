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

TASK(broglie_particle, { BoxedLaser laser; real laser_offset; Color color; bool fast; real angle_freq; }) {
	Laser *l = NOT_NULL(ENT_UNBOX(ARGS.laser));
	cmplx pos = l->prule(l, ARGS.laser_offset);

	
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = ARGS.fast ? pp_thickrice : pp_rice,
		.pos = pos,
		.color = &ARGS.color,
		.layer = LAYER_NODRAW,
		.flags = PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT | PFLAG_NOSPAWNEFFECTS | PFLAG_NOCOLLISION | PFLAG_MANUALANGLE,
	));
	
	int scatter_time = l->timespan + ARGS.laser_offset - 10;
	real speed = ARGS.fast ? 2.0 : 1.5;

	for(int t = 0; t < scatter_time; t++) {
		Laser *laser = ENT_UNBOX(ARGS.laser);
		if(laser) {
			cmplx oldpos = p->pos;
			p->pos = laser->prule(laser, fmin(t, ARGS.laser_offset));

			if(oldpos != p->pos) {
				p->angle = carg(p->pos - oldpos);
			}
		}

		YIELD;
	}

	
	projectile_set_layer(p, LAYER_BULLET);
	p->flags &= ~(PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT | PFLAG_NOCOLLISION | PFLAG_MANUALANGLE);

	double angle_ampl = difficulty_value(0.28, 0.48, 0.67, 0.86);

	p->angle += angle_ampl * sin(scatter_time * ARGS.angle_freq) * cos(2 * scatter_time * ARGS.angle_freq);
	p->move = move_linear(-speed * cdir(p->angle));
	play_sfx("redirect");
}


static void broglie_laser_logic(Laser *l, int t) {
	double hue = cimag(l->args[3]);

	if(t == EVENT_BIRTH) {
		l->color = *HSLA(hue, 1.0, 0.5, 0.0);
	}

	if(t < 0) {
		return;
	}

	
}

TASK(broglie_laser, { BoxedLaser laser; float hue; }) {
	Laser *l = TASK_BIND(ARGS.laser);

	real dt = l->timespan * l->speed;
	
	for(int t = 0;; t++) {
		real charge = fmin(1, powf(t / dt, 4));
		l->color = *HSLA(ARGS.hue, 1.0, 0.5 + 0.2 * charge, 0.0);
		l->width_exponent = 1.0 - 0.5 * charge;
		
		YIELD;
	}
}


TASK(broglie_charger_bullet, { cmplx pos; real angle; real aim_angle; int firetime; int attack_num; }) {
	play_sfx_ex("shot3", 10, false);

	float hue = ARGS.attack_num * 0.5 + ARGS.angle / M_TAU + 1.0 / 12.0;
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = ARGS.pos,
		.color = HSLA(hue, 1.0, 0.55, 0.0),
		.flags = PFLAG_NOCLEAR,
	));

	for(int t = 0; t < ARGS.firetime; t++) {
		real f = pow(clamp((140 - (ARGS.firetime - t)) / 90.0, 0, 1), 8);

		// TODO: make the baryon spin in tune
		ARGS.angle += M_PI * 0.2 * f;
		p->pos = ARGS.pos + 15 * cdir(ARGS.angle);

		if(f > 0.1) {
			play_sfx_loop("charge_generic");

			cmplx dir = rng_dir();
			real l = 50 * rng_real() + 25;
			real s = 4 + f;

			Color *clr = color_lerp(RGB(1, 1, 1), &p->color, clamp((1 - f * 0.5), 0.0, 1.0));
			clr->a = 0;

			PARTICLE(
				.sprite = "flare",
				.pos = p->pos + l * dir,
				.color = clr,
				.draw_rule = pdraw_timeout_fade(1, 0),
				.move = move_linear(-s * dir),
				.timeout = l / s,
			);
		}

		YIELD;
	}


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
		.draw_rule = pdraw_timeout_scalefade(1, 2.4, 1, 0),
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	cmplx aim = cexp(I * ARGS.aim_angle);

	real s_ampl = 30 + 2 * ARGS.attack_num;
	real s_freq = 0.10 + 0.01 * ARGS.attack_num;

	for(int lnum = 0; lnum < 2; ++lnum) {
		Laser *l = create_lasercurve4c(ARGS.pos, 75, 100, RGBA(1, 1, 1, 0), las_sine,
			5*aim, s_ampl, s_freq, lnum * M_PI +
			I*(hue + lnum / 6.0));

		l->width = 20;
		INVOKE_TASK(broglie_laser, ENT_BOX(l), hue + lnum / 6.0);

		int pnum = 0;
		real inc = difficulty_value(1.21, 1.0, 0.81, 0.64);

		for(real laser_offset = 0; laser_offset < l->deathtime; laser_offset += inc, ++pnum) {
			bool fast = global.diff == D_Easy || pnum & 1;

			INVOKE_TASK(broglie_particle,
				    .laser = ENT_BOX(l),
				    .laser_offset = laser_offset,
				    .angle_freq = s_freq * 10,
				    .color = *HSLA(hue + lnum / 6.0, 1.0, 0.5, 0.0),
				    .fast = fast
			);
		}
	}	

	kill_projectile(p);
}

TASK(broglie_baryons_centralize, { BoxedBoss boss; BoxedEllyBaryons baryons; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);
	for(int t = 0;; t++) {
		Boss *boss = NOT_NULL(ENT_UNBOX(ARGS.boss));
		baryons->center_pos = boss->pos; 

		for(int i = 1; i < NUM_BARYONS; i++) {
			cmplx dir = cnormalize(baryons->poss[0] - baryons->center_pos);
			baryons->target_poss[i] = baryons->center_pos + 50 * dir * cdir(M_TAU/5*(i-1)) * sin(0.05 * t);
		}
		YIELD;
	}
}

TASK(broglie_baryons, { BoxedBoss boss; BoxedEllyBaryons baryons; int period; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);
	INVOKE_SUBTASK(broglie_baryons_centralize, ARGS.boss, ARGS.baryons);

	real aim_angle;
	int delay = 140;
	int step = 15;
	int cnt = 3;
	int fire_delay = 120;

	for(int attack_num = 0;; attack_num++) {
		for(int t = 0; t < delay; t++) {
			cmplx boss_pos = NOT_NULL(ENT_UNBOX(ARGS.boss))->pos;
			baryons->target_poss[0] = boss_pos + 100 * cnormalize(global.plr.pos - boss_pos);

			YIELD;
		}

		elly_clap(global.boss,fire_delay);
		aim_angle = carg(baryons->poss[0] - NOT_NULL(ENT_UNBOX(ARGS.boss))->pos);

		for(int i = 0; i < cnt; i++) {
			real angle = M_TAU * (0.25 + 1.0 / cnt * i);

			INVOKE_TASK(broglie_charger_bullet,
				    .pos = baryons->poss[0],
				    .angle = angle,
				    .aim_angle = M_TAU / cnt * i + aim_angle,
				    .firetime = fire_delay - step * i,
				    .attack_num = attack_num
			);

			WAIT(step);
		}

		WAIT(ARGS.period - delay - cnt * step);
	}
}

DEFINE_EXTERN_TASK(stage6_spell_broglie) {
	Boss *boss = stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	int period = difficulty_value(390, 360, 330, 300);
	INVOKE_SUBTASK(broglie_baryons, ENT_BOX(boss), ARGS.baryons, period);

	double ofs = 100;

	cmplx positions[] = {
		VIEWPORT_W-ofs + ofs*I,
		VIEWPORT_W-ofs + ofs*I,
		ofs + (VIEWPORT_H-ofs)*I,
		ofs + ofs*I,
		ofs + ofs*I,
		VIEWPORT_W-ofs + (VIEWPORT_H-ofs)*I,
	};
	
	for(int i = 0;; i++) {
		WAIT(period);
		boss->move = move_towards(positions[i % ARRAY_SIZE(positions)], 0.02);
	}
}
