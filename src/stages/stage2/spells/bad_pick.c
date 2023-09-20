/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "global.h"
#include "common_tasks.h"

#define SLOTS 5

static int slot_of_position(cmplx pos) {
	return (int)(re(pos) / (VIEWPORT_W / SLOTS) + 1) - 1;  // correct rounding for slot == -1
}

TASK(bad_pick_bullet, { cmplx pos; cmplx vel; cmplx accel; int target_slot; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = ARGS.pos,
		.color = RGBA(0.7, 0.0, 0.0, 0.0),
		.move = move_accelerated(ARGS.vel, ARGS.accel),
	));

	for(;;YIELD) {
		// reflection
		int slot = slot_of_position(p->pos);

		if(slot != ARGS.target_slot) {
			p->move.velocity = copysign(re(p->move.velocity), ARGS.target_slot - slot) + I*im(p->move.velocity);
		}
	}
}

static cmplx pick_boss_position(void) {
	return VIEWPORT_W/SLOTS * (rng_irange(0, SLOTS) + 0.5) + 100 * I;
}

TASK(walls, { int duration; }) {
	for(int t = 0; t < ARGS.duration; t += WAIT(5)) {
		play_sfx_loop("shot1_loop");

		for(int i = 1; i < SLOTS; i++) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = VIEWPORT_W / SLOTS * i,
				.color = RGB(0.5, 0, 0.6),
				.move = move_linear(7*I),
			);
		}

		if(global.diff >= D_Hard) {
			real shift = 0;

			if(global.diff == D_Lunatic) {
				shift = 0.3 * fmax(0, t - 100);
			}

			for(int i = 1; i < SLOTS; i++) {
				real height = VIEWPORT_H / SLOTS * i + shift;

				if(height > VIEWPORT_H - 40) {
					height -= VIEWPORT_H -40;
				}

				PROJECTILE(
					.proto = pp_crystal,
					.pos = (i & 1) * VIEWPORT_W + I * height,
					.color = RGB(0.5, 0, 0.6),
					.move = move_linear(5.0 * (1 - 2 * (i & 1))),
				);
			}
		}
	}
}

DEFINE_EXTERN_TASK(stage2_spell_bad_pick) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, pick_boss_position(), 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	int balls_per_slot = difficulty_value(15, 15, 20, 25);

	for(int t = 0;;) {
		t += WAIT(100);
		INVOKE_SUBTASK(walls, 400);
		t += WAIT(30);

		int win;
		int plr_slot = slot_of_position(global.plr.pos);

		do {
			win = rng_irange(0, SLOTS);
		} while(win == plr_slot && global.diff > D_Easy);

		for(int i = 0; i < SLOTS; i++) {
			if(i == win) {
				continue;
			}

			INVOKE_SUBTASK(common_charge,
				.pos = VIEWPORT_W / (real)SLOTS * (i + 0.5),
				.color = RGBA(1.0, 0.1, 0.1, 0),
				.time = 65,
				.sound = COMMON_CHARGE_SOUNDS
			);

			if(i == plr_slot && global.diff > D_Normal) {
				INVOKE_SUBTASK(common_charge,
					.pos = VIEWPORT_W / (real)SLOTS * (i + 0.5) + VIEWPORT_H*I,
					.color = RGBA(0.2, 0.2, 1.0, 0),
					.time = 65,
					.sound = COMMON_CHARGE_SOUNDS
				);
			}
		}

		t += WAIT(60);
		aniplayer_queue(&boss->ani, "guruguru", 2);
		aniplayer_queue(&boss->ani, "main", 0);
		t += WAIT(10);

		play_sfx("shot_special1");

		for(int i = 0; i < SLOTS; i++) {
			if(i == win) {
				continue;
			}

			for(int j = 0; j < balls_per_slot; j++) {
				INVOKE_TASK(bad_pick_bullet,
					.pos = VIEWPORT_W / (real)SLOTS * (i + 0.1 + 0.8 * j / (balls_per_slot - 1.0)),
					.vel = 0,
					.accel = 0.005 * (rng_sreal() + I * (1 + psin(i + j + t))),
					.target_slot = i
				);
			}

			if(i == plr_slot && global.diff > D_Normal) {
				for(int j = 0; j < balls_per_slot; j++) {
					Projectile *p = PROJECTILE(
						.proto = pp_ball,
						.pos = VIEWPORT_W / (real)SLOTS * (i + 0.1 + 0.8 * j / (balls_per_slot - 1.0)) + VIEWPORT_H*I,
						.color = RGBA(0.2, 0.2, 0.7, 0.0),
						.move = move_accelerated(0, 0),
					);

					INVOKE_TASK(common_move,
						.ent = ENT_BOX(p).as_generic,
						.pos = &p->move.acceleration,
						.move_params = move_linear(0.000001 * (rng_sreal() - I * (1 + psin(i + j + t))))
					);
				}
			}
		}

		t += WAIT(300);
		boss->move.attraction_point = pick_boss_position();
	}
}
