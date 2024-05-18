/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

TASK(ring, {
	cmplx pos;
	real spawn_interval;
	real expand_time;
	real gap_factor;
	real direction;
}) {
	static const Color color0 = { 1, 0.5, 0, 0 };
	static const Color color1 = { 1, 0, 0.5, 0 };

	// "time base", i.e. how many laser-time units is a full circle revolution
	// This affects the quantization sample count (~2x this number of samples taken for the full circle)
	// In other words: bigger number = smoother arcs = slower
	const real T = 53;
	const real target_radius = hypot(VIEWPORT_W, VIEWPORT_H) * 0.5;
	const real width = 15;

	real radius = 0;
	real expand_time = ARGS.expand_time;
	real spawn_interval = ARGS.spawn_interval;
	real wlen = spawn_interval * target_radius / expand_time;

	auto louter = create_laser(ARGS.pos, T, expand_time, RGB(0.5, 0.25, 0),
		laser_rule_arc(0, M_TAU/T, 0));
	louter->width = 2;
	louter->width_exponent = 0;
	louter->collision_active = false;
	laser_make_static(louter);
	auto b_louter = ENT_BOX(louter);

	int num_segs = rng_i32_range(2, 6);
	DECLARE_ENT_ARRAY(Laser, segs, num_segs);
	DECLARE_ENT_ARRAY(Laser, walls, num_segs);

	real f = T/(real)num_segs;
	real s = ARGS.direction;
	real o = T * rng_real() * s;

	for(int i = 0; i < num_segs; ++i) {
		auto l = create_laser(ARGS.pos, f * ARGS.gap_factor, expand_time + spawn_interval, &color0,
			laser_rule_arc(0, s*M_TAU/T, o+i*f));
		l->width = width;
		l->width_exponent = 0;
		laser_make_static(l);
		ENT_ARRAY_ADD(&segs, l);

		auto lwall = create_laser(0, 4, expand_time + spawn_interval, &color0,
			laser_rule_linear(0));
		lwall->width = width;
		lwall->width_exponent = 0;
		laser_make_static(lwall);
		ENT_ARRAY_ADD(&walls, lwall);
	}

	for(int t = 0; t < expand_time + spawn_interval; ++t, YIELD) {
		radius = approach(radius, target_radius + wlen, target_radius / expand_time);

		if((louter = ENT_UNBOX(b_louter))) {
			auto rd = NOT_NULL(laser_get_ruledata_arc(louter));
			rd->radius = radius;
		}

		ENT_ARRAY_FOREACH(&segs, Laser *l, {
			auto rd = NOT_NULL(laser_get_ruledata_arc(l));
			rd->radius = radius;
			rd->time_ofs -= 0.5 * T/radius;
			color_lerp(&l->color, &color1, 0.0025);
		});

		ENT_ARRAY_FOREACH_COUNTER(&walls, int i, Laser *lwall, {
			auto ref = ENT_ARRAY_GET(&segs, i);

			if(!ref || global.frames - ref->birthtime >= ref->deathtime) {
				lwall->deathtime = 0;
				continue;
			}

			lwall->color = ref->color;

			cmplx a = laser_pos_at(ref, 0);
			cmplx v = cnormalize(ARGS.pos - a);
			cmplx ofs = v * width * 0.25;
			laserline_set_ab(lwall, a + ofs, a - ofs + min(wlen, radius) * v);
		});
	}
}

DEFINE_EXTERN_TASK(stagex_spell_rings) {
	auto boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, (VIEWPORT_W+VIEWPORT_H*I)*0.5, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);

	const int spawn_interval = 120;
	const int expand_time = 60 * 12;
	const real gap_factor = 0.8;

	real dir = rng_sign();

	for(;;) {
		common_charge(spawn_interval, &boss->pos, 0, RGBA(1, 0.5, 0, 0));
		play_sfx("redirect");
		INVOKE_TASK(ring,
			.pos = boss->pos,
			.direction = dir,
			.spawn_interval = spawn_interval,
			.expand_time = expand_time,
			.gap_factor = gap_factor,
		);
		// NOTE: keeping or randomizing the direction may create unwinnable scenarios
		dir = -dir;
	}
}
