/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

#define NUM_SLOTS 3
#define SLOT_WIDTH (VIEWPORT_W / (real)NUM_SLOTS)

static void goat_bullets(
	int slot,
	int t,
	int cnt,
	bool top,
	ProjFlags flags,
	BoxedProjectileArray *array
) {
	for(int i = 0; i < cnt; i++) {
		cmplx o = !top*VIEWPORT_H*I + SLOT_WIDTH * (slot + i/(cnt - 1.0));
		cmplx a = 0.007 * (sin((M_PI * 4 * i / (cnt - 1))) * 0.1 * global.diff - I * (1 + psin(i + t)));

		if(top) {
			a *= -0.5;
		}

		Projectile *p = PROJECTILE(
			.proto = pp_ball,
			.pos = o,
			.color = top ? RGBA(0, 0, 0.7, 0) : RGBA(0.7, 0, 0, 0),
			.move = move_accelerated(a, a),
			.flags = flags,
		);

		if(array) {
			ENT_ARRAY_ADD(array, p);
		}
	}
}

TASK(goat, { int slot; CoEvent *activation_event; }) {
	int cnt = difficulty_value(15, 20, 25, 30);
	int slot = ARGS.slot;
	int charge_time = 60;
	int period = 60;
	int timeout = 300;

	bool top_enabled = global.diff > D_Hard;

	INVOKE_SUBTASK(common_charge,
		.pos = SLOT_WIDTH * (slot + 0.5) + VIEWPORT_H*I,
		.color = RGBA(1.0, 0.2, 0.2, 0),
		.time = charge_time,
		.sound = COMMON_CHARGE_SOUNDS
	);

	if(top_enabled) {
		INVOKE_SUBTASK(common_charge,
			.pos = SLOT_WIDTH * (slot + 0.5),
			.color = RGBA(0.2, 0.2, 1.0, 0),
			.time = charge_time,
			.sound = COMMON_CHARGE_SOUNDS
		);
	}

	WAIT(charge_time);
	play_sfx("shot_special1");

	DECLARE_ENT_ARRAY(Projectile, initial_bullets_bottom, cnt);
	DECLARE_ENT_ARRAY(Projectile, initial_bullets_top, cnt);

	goat_bullets(slot, 0, cnt, false, PFLAG_NOMOVE, &initial_bullets_bottom);

	if(top_enabled) {
		goat_bullets(slot, period, cnt, true, PFLAG_NOMOVE, &initial_bullets_top);
	}

	WAIT_EVENT(ARGS.activation_event);

	int t = 0;

	play_sfx("redirect");
	ENT_ARRAY_FOREACH(&initial_bullets_bottom, Projectile *p, {
		p->flags &= ~PFLAG_NOMOVE;
	});

	t += WAIT(period);

	if(top_enabled) {
		play_sfx("redirect");
		ENT_ARRAY_FOREACH(&initial_bullets_top, Projectile *p, {
			p->flags &= ~PFLAG_NOMOVE;
		});

		t += WAIT(period);
	}

	for(bool top = false; t < timeout; t += WAIT(period), top = !top) {
		play_sfx("shot_special1");
		goat_bullets(slot, t, cnt, top && top_enabled, 0, NULL);
	}
}

TASK(cards, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	int cnt = 10;
	int period = 30;
	int period_reduction = global.diff;
	real side = rng_sign();
	real w = SLOT_WIDTH * 0.5 * difficulty_value(1.1, 1.25, 1.5, 1.5);
	real ofs = (SLOT_WIDTH * 0.5 - w);

	for(int t = 0; t < 360;) {
		for(int i = 0; i < cnt; ++i) {
			play_sfx("shot3");
			cmplx o = re(boss->pos) + side * (ofs + (w*i)/cnt) + I*im(boss->pos);
			PROJECTILE(
				.proto = pp_card,
				.pos = o,
				.color = RGB(1.0, 0.0, 1.0),
				.move = move_accelerated(-3*I, 0.06*I),
			);
			t += WAIT(1);
		}

		side = -side;
		t += WAIT(period);
		period = max(1, period - period_reduction);
	}
}

DEFINE_EXTERN_TASK(stage2_spell_monty_hall_danmaku) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);

	COEVENTS_ARRAY(goat_trigger) events;
	TASK_HOST_EVENTS(events);

	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2.0 + VIEWPORT_H/2.0 * I, 0.06);
	BEGIN_BOSS_ATTACK(&ARGS);

	int plr_slot;
	int goat1_slot;
	int goat2_slot;
	int win_slot;

	for(;;) {
		boss->move.attraction_point = VIEWPORT_W/2.0 + VIEWPORT_H/2.0 * I;

		goat1_slot = rng_irange(0, 3);
		do {
			win_slot = rng_irange(0, 3);
		} while(win_slot == goat1_slot);

		goat2_slot = 0;
		while(goat2_slot == win_slot || goat2_slot == goat1_slot) {
			goat2_slot++;
		}
		assert(goat2_slot < 3);

		for(int i = 0; i < 2; ++i) {
			real x = SLOT_WIDTH * (1 + i);
// 			play_sfx("laser1");
// 			create_laserline_ab(x, x + VIEWPORT_H*I, 15, 30, 60, RGBA(0.3, 1.0, 1.0, 0.0))->unclearable = true;
			create_laserline_ab(x, x + VIEWPORT_H*I, 20, 250, 600, RGBA(1.0, 0.3, 1.0, 0.0))->unclearable = true;
		}

		WAIT(90);
		plr_slot = re(global.plr.pos) / SLOT_WIDTH;

		// goat1_slot is the one that we will reveal
		if(goat1_slot == plr_slot) {
			goat1_slot = goat2_slot;
			goat2_slot = plr_slot;
		}

		aniplayer_queue(&boss->ani, "guruguru", 1);
		aniplayer_queue(&boss->ani, "main", 0);
		INVOKE_SUBTASK(goat, goat1_slot, &events.goat_trigger);

		WAIT(140);
		INVOKE_SUBTASK(goat, goat2_slot, &events.goat_trigger);
		boss->move.attraction_point = SLOT_WIDTH * (0.5 + win_slot) + VIEWPORT_H/2.0*I - 200.0*I;

		play_sfx("laser1");

		WAIT(61);
		coevent_signal(&events.goat_trigger);

		INVOKE_SUBTASK(common_drop_items, &boss->pos, {
			.power = 10,
			.points = 10,
		});

		INVOKE_SUBTASK(cards, ENT_BOX(boss));

		WAIT(400);
	}
}
