/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

#define CHARGE_TIME 120

TASK(rain_lane, { cmplx origin; real spread; cmplx *wind; real max_viewport_dist; }) {
	real accel = difficulty_value(0.005, 0.01, 0.01, 0.015);
	int delay = 16;

	for(;;WAIT(delay)) {
		cmplx wind = *ARGS.wind;

		if( (re(wind) <= 0 && re(ARGS.origin) < 0) ||
			(re(wind) >= 0 && re(ARGS.origin) > VIEWPORT_H)
		) {
			// This bullet will never enter the viewport, so don't spawn it.
			continue;
		}

		cmplx ofs = ARGS.spread * rng_sreal();
		cmplx p = ofs + ARGS.origin;

		PROJECTILE(
			.pos = p,
			.proto = (ProjPrototype*[]) { pp_droplet, pp_rice, pp_thickrice }[rng_irange(0, 3)],
			.color = RGB(0.3, 0.6, 1.0),
			.move = move_next(p, move_accelerated(wind, I * accel)),
			.max_viewport_dist = ARGS.max_viewport_dist,
		);
	}
}

TASK(rain) {
	int cnt = difficulty_value(8, 9, 9, 10);
	real spacing = VIEWPORT_W / cnt;
	cmplx wind = 0;

	int extra = 3;
	real spread = 0.25 * spacing;
	real max_viewport_dist = spread + spacing * extra;

	for(int i = -extra; i < cnt + extra; ++i) {
		INVOKE_SUBTASK_DELAYED((7 * (i + extra)) % 13, rain_lane, {
			.origin = spacing * (i + 0.5),
			.spread = spread,
			.wind = &wind,
			.max_viewport_dist = max_viewport_dist,
		});
	}

	for(int t = 0;; ++t, YIELD) {
		play_sfx_loop("shot1_loop");
		re(wind) = 0.5 * sin(t * 0.0076) * smoothstep(0, 600, t);
	}
}

TASK(lightning_segment, {
	cmplx a; cmplx b; real width; Color color;
}) {
	real lifetime = 10;
	real chargetime = CHARGE_TIME;

	auto l = TASK_BIND(create_laserline_ab(
		ARGS.a, ARGS.b, ARGS.width, chargetime, chargetime + lifetime, ARGS.color
	));
	l->width_exponent = 0.25;

	WAIT(chargetime);

	play_sfx("shot_special1");
	play_sfx("shot1");
}

TASK(fork_branch, {
	cmplx orig; cmplx dest; real chance; real width; Color color; int *branch_count;
}) {
	cmplx orig = ARGS.orig;
	cmplx dest = ARGS.dest;

	if(ARGS.width < 8 || cabs(orig - dest) < 64) {
		if(ARGS.branch_count) {
			--*ARGS.branch_count;
		}
		return;
	}

	cmplx mid = clerp(orig, dest, rng_range(0.15, 0.25));
	INVOKE_TASK(lightning_segment, orig, mid, ARGS.width, ARGS.color);

	int delay = 1;
	WAIT(delay);

	real s = rng_sign();

	int bc = 0;
	for(real c = ARGS.chance; rng_chance(c); c *= 0.4) {
		cmplx r = cdir(s * rng_range(M_TAU/24, M_TAU/16));
		cmplx o = (dest - mid) * r;
		real m = cabs(o);
		o = m * cnormalize(clerp(o, global.plr.pos - mid, 0.125));

		++bc;
		INVOKE_TASK(fork_branch, mid, mid + o, c, ARGS.width * 0.8, ARGS.color, &bc);
		s *= -1;
	}
}

TASK(fork, {
	cmplx orig; cmplx dest; real maxwidth; real minwidth;
	Color color0; Color color1;
}) {
	cmplx orig = ARGS.orig;
	cmplx dest = ARGS.dest;
	real width = ARGS.maxwidth;

	real seglen = 64;

	cmplx delta = dest - orig;
	real maxdist = cabs(delta);

	cmplx a = orig, b;

	real branch_chance = 1;
	real branch_len = seglen * 6;
	real next_len = seglen;

	Color color = ARGS.color0;

	INVOKE_SUBTASK(common_charge, {
		.time = CHARGE_TIME,
		.sound = {
			.charge = COMMON_CHARGE_SOUND_CHARGE,
			.discharge = "boom",
		},
		.pos = orig,
		.color = RGBA(1.5, 1, 2, 0),
	});

	for(real d = 0; d < maxdist; d += next_len) {
		next_len = seglen * rng_range(0.5, 1.2);
		cmplx arcdir;

		arcdir = global.plr.pos - a;

		if(cabs2(arcdir) > next_len * next_len) {
			arcdir = cnormalize(arcdir) * next_len;
		}

		arcdir *= cdir(rng_sreal() * M_TAU/12);
		b = a + arcdir;

		INVOKE_TASK(lightning_segment, a, b, width, color);

		a = b;
		width = lerp(width, ARGS.minwidth, 0.1);
		arcdir = cnormalize(arcdir);

		for(real s = -1; s < 2; s += 2) {
			if(rng_chance(branch_chance)) {
				real angle = rng_range(M_TAU/24, M_TAU/12);
				cmplx r = cdir(s * angle);
				INVOKE_TASK(fork_branch,
					b, b + arcdir * r * branch_len, branch_chance, width, color
				);
			}
		}

		branch_chance = lerp(branch_chance, 0.5, 0.05);
		seglen = lerp(seglen, 32, 0.2);
		branch_len *= 0.98;

		color = color_lerp(color, ARGS.color1, 0.1);

		WAIT(1);
	}

	AWAIT_SUBTASKS;
}

TASK(double_strike, { cmplx origin; real maxwidth; real minwidth; int delay; }) {
	INVOKE_SUBTASK(fork,
		.orig = ARGS.origin,
		.dest = global.plr.pos,
		.maxwidth = ARGS.maxwidth,
		.minwidth = ARGS.minwidth,
		.color0 = RGBA(0.1, 0.5, 1, 0),
		.color1 = RGBA(0.5, 0.1, 1, 0),
	);

	WAIT(ARGS.delay);

	INVOKE_SUBTASK(fork,
		.orig = ARGS.origin,
		.dest = global.plr.pos,
		.maxwidth = ARGS.maxwidth,
		.minwidth = ARGS.minwidth,
		.color0 = RGBA(1.0, 1.0, 0.5, 0),
		.color1 = RGBA(1.0, 0.5, 0.5, 0),
	);

	AWAIT_SUBTASKS;
}

DEFINE_EXTERN_TASK(stage5_spell_artificial_lightning) {
	STAGE_BOOKMARK(artificial-lightning);
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 200.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	Rect wander_bounds = viewport_bounds(64);
	wander_bounds.top += 64;
	wander_bounds.bottom = VIEWPORT_H * 0.4;

	INVOKE_SUBTASK(rain);

	boss->move.attraction = 0.03;

	int delay_reduction = 20;
	int delay_reduction_steps = 5;
	int delay_final = difficulty_value(180, 140, 140, 140);
	int delay = delay_final + delay_reduction * delay_reduction_steps;
	int movement_delay = 60;

	assert(delay_final > movement_delay);

	for(int x = 0;; ++x) {
		WAIT(movement_delay);
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W * 0.25, wander_bounds);
		WAIT(delay - movement_delay);
		aniplayer_hard_switch(&boss->ani, (x & 1) ? "dashdown_left" : "dashdown_right", 1);
		aniplayer_queue(&boss->ani, "main", 0);

		if(global.diff > D_Normal) {
			INVOKE_SUBTASK(double_strike,
				.origin = re(boss->pos),
				.minwidth = 8,
				.maxwidth = 64,
				.delay = 30,
			);
		} else {
			INVOKE_SUBTASK(fork,
				.orig = re(boss->pos),
				.dest = global.plr.pos,
				.minwidth = 8,
				.maxwidth = 64,
				.color0 = RGBA(0.1, 0.5, 1, 0),
				.color1 = RGBA(0.5, 0.1, 1, 0),
			);
		}

		if(x > 23) {  // rage phase!
			delay = 30;
			movement_delay = 0;
		} else if(delay > delay_final) {
			delay -= delay_reduction;
		} else {
			assert(delay == delay_final);
		}
	}
}
