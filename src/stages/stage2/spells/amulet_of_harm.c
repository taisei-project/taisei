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

TASK(spinner_bullet_redirect, { BoxedProjectile p; MoveParams move; }) {
	Projectile *p = TASK_BIND(ARGS.p);
	cmplx ov = p->move.velocity;
	p->move = ARGS.move;
	p->move.velocity += ov;
}

static void amulet_visual(Enemy *e, int t, bool render) {
	if(render) {
		r_draw_sprite(&(SpriteParams) {
			.color = RGBA(2, 1, 1, 0),
			.sprite = "fairy_circle_big",
			.pos.as_cmplx = e->pos,
			.rotation.angle = t * 5 * DEG2RAD,
		});

		r_draw_sprite(&(SpriteParams) {
			.sprite = "enemy/swirl",
			.pos.as_cmplx = e->pos,
			.rotation.angle = t * -10 * DEG2RAD,
		});
	}
}

TASK(amulet_fire_spinners, { BoxedEnemy core; BoxedProjectileArray *spinners; }) {
	Enemy *core = TASK_BIND(ARGS.core);

	int nshots = difficulty_value(1, 1, 3, 69);
	int charge_time = difficulty_value(90, 75, 60, 60);
	real accel = difficulty_value(0.05, 0.06, 0.085, 0.085);

	for(int i = 0; i < nshots; ++i) {
		WAIT(60);
		common_charge(charge_time, &core->pos, 0, RGBA(0.6, 0.2, 0.5, 0));

		ENT_ARRAY_FOREACH(ARGS.spinners, Projectile *p, {
			int cnt = difficulty_value(12, 16, 22, 24);
			for(int x = 0; x < cnt; ++x) {
				cmplx ca = circle_dir(x, cnt);
				cmplx o = p->pos + 42 * ca;
				cmplx aim = cnormalize(o - core->pos);

				Projectile *c = PROJECTILE(
					.proto = pp_crystal,
					.pos = p->pos,
					.color = RGB(0.2 + 0.8 * tanh(cabs2(o - core->pos) / 4200), 0.2, 1.0),
					.max_viewport_dist = p->max_viewport_dist,
					.move = move_towards(o, 0.02),
				);

				INVOKE_TASK_DELAYED(42, spinner_bullet_redirect, ENT_BOX(c), move_accelerated(0, aim * accel));
			}
		});
	}

	WAIT(60);
	enemy_kill(core);
}

TASK(amulet, {
	cmplx pos;
	MoveParams move;
	CoEvent *death_event;
}) {
	Enemy *core = create_enemy_p(&global.enemies, ARGS.pos, 2000, amulet_visual, NULL, 0, 0, 0, 0);
	core->hurt_radius = 18;
	core->hit_radius = 36;
	core->flags |= EFLAG_NO_VISUAL_CORRECTION;
	core->move = ARGS.move;

	INVOKE_TASK_AFTER(NOT_NULL(ARGS.death_event), common_kill_enemy, ENT_BOX(core));

	int num_spinners = difficulty_value(2, 3, 4, 4);
	real spinner_ofs = 32;
	cmplx spin = cdir(M_PI/8);

	DECLARE_ENT_ARRAY(Projectile, spinners, num_spinners);
	cmplx spinner_offsets[num_spinners];

	cmplx init_orientation = cnormalize(core->move.velocity);

	for(int i = 0; i < num_spinners; ++i) {
		spinner_offsets[i] = spinner_ofs * cdir(i * M_TAU / num_spinners) * init_orientation;

		Projectile *p = PROJECTILE(
			.proto = pp_ball,
			.pos = core->pos,
			.color = RGB(0.8, 0.2, 1.0),
			.max_viewport_dist = spinner_ofs * 2,
			.flags = PFLAG_NOCLEAR,
		);

		INVOKE_TASK_AFTER(&core->events.killed, common_kill_projectile, ENT_BOX(p));
		ENT_ARRAY_ADD(&spinners, p);
	}

	INVOKE_SUBTASK(amulet_fire_spinners, ENT_BOX(core), &spinners);

	for(;;YIELD) {
		int live = 0;

		ENT_ARRAY_FOREACH_COUNTER(&spinners, int i, Projectile *p, {
			p->pos = core->pos + spinner_offsets[i];
			spinner_offsets[i] *= spin;
			++live;
		});

		if(!live) {
			enemy_kill(core);
			return;
		}

		real s = approach_asymptotic(carg(spin), -M_PI/32, 0.02, 1e-5);
		spin = cabs(spin) * cdir(s);
	}
}

static void fan_burst_side(cmplx o, cmplx ofs, real x) {
	int cnt = difficulty_value(2, 3, 3, 3);
	real speed = difficulty_value(2, 2.5, 3, 3);
	real sat = 0.75;

	for(int i = 0; i < cnt; ++i) {
		real s = speed * (1 + 0.1 * i);

		PROJECTILE(
			.pos = o + ofs,
			.color = RGB(sat * 1, sat * 0.25 * i / (cnt - 1.0), 0),
			.proto = pp_card,
			.move = move_asymptotic_simple(s * I*cnormalize(ofs) * cdir(x * (i+1) * -0.3), 3 - 0.1 * i),
		);

		PROJECTILE(
			.pos = o + ofs,
			.color = RGB(0, sat * 0.25 * i / (cnt - 1.0), sat * 1),
			.proto = pp_card,
			.move = move_asymptotic_simple(s * cnormalize(ofs)/I * cdir(x * (i+1) * 0.3), 3 - 0.1 * i),
		);
	}
}

TASK(fan_burst, { Boss *boss; }) {
	int nbursts = difficulty_value(1, 1, 2, 2);

	for(int burst = 0; burst < nbursts; ++burst) {
		int cnt = 30;
		for(int i = 0; i < cnt; ++i, WAIT(3)) {
			play_sfx_loop("shot1_loop");

			cmplx o = 32;
			real u = 32;

			real x = i / (cnt - 1.0);

			o += sin(4 * -x * M_PI) * u;
			o += cos(4 * -x * 1.23 * M_PI) * I * u;

			fan_burst_side(ARGS.boss->pos, o, x);
			fan_burst_side(ARGS.boss->pos, -conj(o), x);
		}

		WAIT(30);
	}
}

DEFINE_EXTERN_TASK(stage2_spell_amulet_of_harm) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(VIEWPORT_W/2 + 200.0*I, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS);

	Rect wander_bounds = viewport_bounds(64);
	wander_bounds.top += 64;
	wander_bounds.bottom = VIEWPORT_H * 0.4;

	for(;;) {
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W * 0.3, wander_bounds);

		WAIT(20);

		int cnt = 3;
		real arange = M_PI/3;

		for(int i = 0; i < cnt; ++i) {
			real s = 4 * fmax(2, log1p(cabs(boss->move.velocity)));
			cmplx dir = cnormalize(-boss->move.velocity) * cdir(arange * (i / (cnt - 1.0) - 0.5));

			play_sfx("redirect");
			INVOKE_TASK(amulet, boss->pos, move_asymptotic_halflife(s * dir, 0, 12), &ARGS.attack->events.finished);
			WAIT(15);
		}

		WAIT(40);
		aniplayer_soft_switch(&boss->ani, "guruguru", 0);
		WAIT(20);
		boss->move.attraction_point = common_wander(boss->pos, VIEWPORT_W * 0.25, wander_bounds);
		play_sfx("shot_special1");
		INVOKE_SUBTASK(fan_burst, boss);
		WAIT(120);
		aniplayer_soft_switch(&boss->ani, "main", 0);
	}
}
