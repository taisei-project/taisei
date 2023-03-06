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
	Laser *l = TASK_BIND(ARGS.laser);

	real angle_ampl = difficulty_value(0.28, 0.48, 0.67, 0.86);
	int scatter_time = l->timespan + ARGS.laser_offset - 10;

	WAIT(scatter_time);

	real speed = ARGS.fast ? 2.0 : 1.5;

	cmplx pos = l->prule(l, ARGS.laser_offset);
	cmplx dir = cnormalize(pos - l->prule(l, ARGS.laser_offset-0.1));
	dir *= cdir(angle_ampl * sin(scatter_time * ARGS.angle_freq) * cos(2 * scatter_time * ARGS.angle_freq));

	PROJECTILE(
		.proto = ARGS.fast ? pp_thickrice : pp_rice,
		.pos = pos,
		.color = &ARGS.color,
		.move = move_linear(-speed * dir)
	);

	play_sfx("redirect");
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

TASK(broglie_spin_baryon, { BoxedEllyBaryons baryons; int peaktime; }) {
	auto baryons = TASK_BIND(ARGS.baryons);
	int peaktime = ARGS.peaktime;

	real maxspin = M_PI * 0.2;
	real spin = 0;

	INVOKE_SUBTASK(common_charge,
		.anchor = &baryons->poss[0],
		.color = RGBA(0.05, 1, 0.5, 0),
		.sound = COMMON_CHARGE_SOUNDS,
		.time = peaktime,
	);

	for(int t = 0; t < peaktime; ++t, YIELD) {
		approach_asymptotic_p(&spin, maxspin, 0.02, 1e-4);
		baryons->angles[0] += spin;
	}

	while(spin > 0) {
		approach_asymptotic_p(&spin, 0, 0.02, 1e-4);
		baryons->angles[0] += spin;
		YIELD;
	}
}

TASK(broglie_charger_bullet, {
	BoxedEllyBaryons baryons;
	real angle;
	cmplx aim;
	int predelay;
	int firetime;
	int attack_num;
}) {
	play_sfx_ex("shot3", 10, false);

	cmplx center = NOT_NULL(ENT_UNBOX(ARGS.baryons))->poss[0];
	cmplx ofs = 14 * cdir(ARGS.angle);

	float hue = ARGS.attack_num * 0.5 + ARGS.angle / M_TAU + 1.0 / 12.0;
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = center + ofs * cdir(ARGS.angle),
		.color = HSLA(hue, 1.0, 0.55, 0.0),
		.flags = PFLAG_NOCLEAR,
	));

	cmplx spin = 0;

	for(int t = -ARGS.predelay; t < ARGS.firetime; t++, YIELD) {
		// NOTE: We can't use a static position here because we want it to look like it's attached
		// to the baryon, which may be still decelerating to its target position.

		EllyBaryons *baryons = ENT_UNBOX(ARGS.baryons);

		if(baryons) {
			center = baryons->poss[0];
			spin = cdir(baryons->angles[0]);
		}

		p->pos = center + ofs * spin;

		if(t < 0) {
			continue;
		}

		real f = pow(clamp((140 - (ARGS.firetime - t)) / 90.0, 0, 1), 8);

		if(f > 0.1) {
			Color *clr = COLOR_COPY(&p->color);
			color_mul_scalar(clr, 2);
			clr->a = 0;

			real l = rng_range(70, 100) * (0.5 + 0.5 * f);
			real s = 4 + f;
			cmplx dir = rng_dir();
			cmplx o = l * dir;

			PARTICLE(
				.sprite = "flare",
				.pos = p->pos + o,
				.color = clr,
				.draw_rule = pdraw_timeout_scalefade(1+I, 2*I, 0, 1),
				.move = move_linear(s * -dir),
				.timeout = l / s,
			);
		}
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

	real s_ampl = 30 + 2 * ARGS.attack_num;
	real s_freq = 0.10 + 0.01 * ARGS.attack_num;

	for(int lnum = 0; lnum < 2; ++lnum) {
		Laser *l = create_lasercurve4c(center, 75, 100, RGBA(1, 1, 1, 0), las_sine,
			5 * ARGS.aim, s_ampl, s_freq, lnum * M_PI);

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

		elly_clap(global.boss, fire_delay);
		cmplx center = baryons->poss[0];
		cmplx aim = cnormalize(center - NOT_NULL(ENT_UNBOX(ARGS.boss))->pos);

		INVOKE_SUBTASK_DELAYED(step * cnt,
			broglie_spin_baryon, ENT_BOX(baryons), fire_delay - step * (cnt - 1));

		for(int i = 0; i < cnt; i++) {
			real angle = M_TAU * (0.25 + 1.0 / cnt * i);

			INVOKE_TASK(broglie_charger_bullet,
				.baryons = ENT_BOX(baryons),
				.angle = angle,
				.aim = cdir(M_TAU / cnt * i) * aim,
				.predelay = step * (cnt - i),
				.firetime = fire_delay - step * (cnt - 1),
				.attack_num = attack_num
			);

			WAIT(step);
		}

		WAIT(ARGS.period - delay - cnt * step);
	}
}

TASK(reset_baryon_angle, { BoxedEllyBaryons baryons; }) {
	auto baryons = TASK_BIND(ARGS.baryons);
	// FIXME better way to do this?
	baryons->angles[0] = fmod(baryons->angles[0], M_TAU);
	for(;baryons->angles[0] != 0; YIELD) {
		approach_asymptotic_p(&baryons->angles[0], 0, 0.05, 1e-4);
	}
}

DEFINE_EXTERN_TASK(stage6_spell_broglie) {
	Boss *boss = stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	int period = difficulty_value(390, 360, 330, 300);
	INVOKE_SUBTASK(broglie_baryons, ENT_BOX(boss), ARGS.baryons, period);
	INVOKE_TASK_AFTER(&ARGS.base.attack->events.finished, reset_baryon_angle, ARGS.baryons);

	real ofs = 100;

	cmplx positions[] = {
		VIEWPORT_W-ofs + ofs*I,
		VIEWPORT_W-ofs + ofs*I,
		ofs + (VIEWPORT_H-ofs)*I,
		ofs + ofs*I,
		ofs + ofs*I,
		VIEWPORT_W-ofs + (VIEWPORT_H-ofs)*I,
	};

	for(int i = 1;; i++) {
		WAIT(period);
		boss->move = move_from_towards(boss->pos, positions[i % ARRAY_SIZE(positions)], 0.02);
	}
}
