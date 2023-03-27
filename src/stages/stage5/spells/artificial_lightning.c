/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#define CHARGE_TIME 90

TASK(rain_lane, { cmplx origin; real spread; }) {
	real accel = difficulty_value(0.005, 0.01, 0.01, 0.01);
	int delay = 16;

	for(;;WAIT(delay)) {
		cmplx ofs = ARGS.spread * rng_sreal();
		cmplx p = ofs + ARGS.origin;

		PROJECTILE(
			.pos = p,
			.proto = pp_rice,
			.color = RGB(0.2, 0.3, 1.0),
			.move = move_next(p, move_accelerated(0, I * accel)),
			.max_viewport_dist = ARGS.spread * 2,
		);
	}
}

TASK(rain) {
	int cnt = difficulty_value(8, 9, 10, 11);
	real spacing = VIEWPORT_W / cnt;

	for(int i = 0; i < cnt; ++i) {
		INVOKE_SUBTASK_DELAYED((7 * i) % 13, rain_lane, {
			.origin = spacing * (i + 0.5),
			.spread = 0.25 * spacing,
		});
	}

	for(;;WAIT(5)) {
		play_sfx_loop("shot1_loop");
	}
}

TASK(lightning_segment, {
	cmplx a; cmplx b; real width; Color *color;
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
	INVOKE_TASK(lightning_segment, orig, mid, ARGS.width, &ARGS.color);

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

TASK(fork, { cmplx orig; cmplx dest; real maxwidth; real minwidth; }) {
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

	Color *color = RGBA(0.1, 0.5, 1, 0);

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
					b, b + arcdir * r * branch_len, branch_chance, width, *color
				);
			}
		}

		branch_chance = lerp(branch_chance, 0.5, 0.05);
		seglen = lerp(seglen, 32, 0.2);
		branch_len *= 0.98;

		color_lerp(color, RGBA(0.5, 0.1, 1, 0), 0.1);

		WAIT(1);
	}

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
	int delay = difficulty_value(180, 140, 120, 110);

	for(int x = 0;; ++x) {
		WAIT(delay);
		aniplayer_hard_switch(&boss->ani, (x & 1) ? "dashdown_left" : "dashdown_right", 1);
		aniplayer_queue(&boss->ani, "main", 0);
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W * 0.3, wander_bounds);
		INVOKE_SUBTASK(fork, creal(boss->pos), global.plr.pos, 64, 8);
	}
}
