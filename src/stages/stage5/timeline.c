/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage5.h"
#include "background_anim.h"
#include "nonspells/nonspells.h"
#include "spells/spells.h"

#include "coroutine.h"
#include "enemy_classes.h"

static void stage5_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage5PostBossDialog, pm->dialog->Stage5PostBoss);
}

static void stage5_dialog_post_midboss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage5PostMidBossDialog, pm->dialog->Stage5PostMidBoss);
}

TASK_WITH_INTERFACE(midboss_flee, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_linear(I);
}

TASK(spawn_midboss) {
	STAGE_BOOKMARK(midboss);
	Boss *boss = global.boss = create_boss("Bombs?", "iku_mid", VIEWPORT_W + 800.0 * I);
	boss->glowcolor = *RGB(0.2, 0.4, 0.5);
	boss->shadowcolor = *RGBA_MUL_ALPHA(0.65, 0.2, 0.75, 0.5);

	Attack *a = boss_add_attack_from_info(boss, &stage5_spells.mid.static_bomb, false);
	boss_set_attack_bonus(a, 5);

	// suppress the boss death effects (this triggers the "boss fleeing" case)
	// TODO: investigate why the death effect doesn't work (points/items still spawn off-screen)
	boss_add_attack_task(boss, AT_Move, "Flee", 2, 0, TASK_INDIRECT(BossAttack, midboss_flee), NULL);

	stage5_bg_raise_lightning();

	boss_engage(boss);
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 240.0 * I, 0.015);
}

TASK(spawn_boss) {
	STAGE_BOOKMARK_DELAYED(120, boss);
	stage_unlock_bgm("stage5");
	Boss *boss = global.boss = stage5_spawn_iku(VIEWPORT_W/2 - 200.0 * I);
	PlayerMode *pm = global.plr.mode;
	Stage5PreBossDialogEvents *e;

	INVOKE_TASK_INDIRECT(Stage5PreBossDialog, pm->dialog->Stage5PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage5boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task(boss, AT_Normal, "Bolts1", 40, 24000, TASK_INDIRECT(BossAttack, stage5_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage5_spells.boss.atmospheric_discharge, false);

	boss_add_attack_task(boss, AT_Normal, "Bolts2", 45, 27000, TASK_INDIRECT(BossAttack, stage5_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stage5_spells.boss.artificial_lightning, false);

	boss_add_attack_task(boss, AT_Normal, "Bolts3", 50, 30000, TASK_INDIRECT(BossAttack, stage5_boss_nonspell_3), NULL);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(boss, &stage5_spells.boss.induction_field, false);
	} else {
		boss_add_attack_from_info(boss, &stage5_spells.boss.inductive_resonance, false);
	}
	boss_add_attack_from_info(boss, &stage5_spells.boss.natural_cathode, false);

	boss_add_attack_from_info(boss, &stage5_spells.extra.overload, false);

	boss_engage(boss);
}

TASK(greeter_fairy, {
	cmplx pos;
	MoveParams move_enter;
	MoveParams move_exit;
	bool red;
}) {
	Enemy *e;

	if(ARGS.red) {
		e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.power = 1)));
	} else {
		e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(.points = 1)));
	}

	e->move = ARGS.move_enter;
	real speed = difficulty_value(3.0, 3.5, 4.0, 4.5);
	real count = difficulty_value(1, 2, 2, 3);
	int reps = difficulty_value(3,4,5,5);

	Color clr_charge = *(ARGS.red ? RGBA(0.25, 0.05, 0, 0) : RGBA(0, 0.05, 0.25, 0));
	Color clr_bullet = *(ARGS.red ? RGB(1.0, 0.0, 0.0) : RGB(0.0, 0.0, 1.0));

	common_charge(80, &e->pos, 0, &clr_charge);

	for(int x = 0; x < reps; x++) {
		cmplx dir = cnormalize(global.plr.pos - e->pos);
		for(int i = -count; i <= count; i++) {
			real boost = 5 - glm_ease_quad_in(fabs(i / (real)count));
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = &clr_bullet,
				.move = move_asymptotic_simple(speed * dir * cdir(0.06 * i), boost),
			);
			play_sfx("shot1");
		}
		WAIT(20);
	}

	WAIT(20);

	e->move = ARGS.move_exit;
}

TASK(greeter_fairies_1, {
	int num;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		cmplx pos = VIEWPORT_W * (i % 2) + 70.0 * I + 50 * i * I;
		real xdir = 1 - 2 * (i % 2);
		INVOKE_TASK(greeter_fairy,
			.pos = pos,
			.move_enter = move_from_towards(pos, pos + (150 - 300 * (i % 2)), 0.05),
			.move_exit = move_linear(3 * xdir)
		);
		WAIT(15);
	}
}

// true = red, false = blue
typedef bool (*ArcColorRule)(int wave, int nwaves, int idx, int wavesize);

TASK(arc_fairies, {
	cmplx origin;
	real arc;
	cmplx dir;
	real dist;
	int num;
	int waves;
	int wave_delay;
	ArcColorRule color_rule;
}) {
	cmplx d0 = ARGS.dir / cdir(ARGS.arc * 0.5);
	cmplx r = cdir(ARGS.arc / (ARGS.num - 1));
	cmplx o = ARGS.origin;

	for(int w = 0; w < ARGS.waves; ++w, WAIT(ARGS.wave_delay)) {
		cmplx d = d0;

		for(int i = 0; i < ARGS.num; ++i) {
			INVOKE_TASK(greeter_fairy,
				.pos = o,
				.move_enter = move_from_towards(o, o + d * ARGS.dist, 0.04),
				.move_exit = move_asymptotic_halflife(d, 4 * d, 120),
				.red = ARGS.color_rule
						? ARGS.color_rule(w, ARGS.waves, i, ARGS.num)
						: false,
			);

			d *= r;
		}
	}
}

attr_unused
static bool arcspawn_red(int wave, int nwaves, int idx, int wavesize) {
	return true;
}

attr_unused
static bool arcspawn_blue(int wave, int nwaves, int idx, int wavesize) {
	return false;
}

attr_unused
static bool arcspawn_fairy_alternate(int wave, int nwaves, int idx, int wavesize) {
	return (wave * wavesize + idx) & 1;
}

attr_unused
static bool arcspawn_wave_alternate(int wave, int nwaves, int idx, int wavesize) {
	return wave & 1;
}

attr_unused
static bool arcspawn_split(int wave, int nwaves, int idx, int wavesize) {
	return (idx < wavesize/2) ^ (wave & 1);
}

TASK(lightburst_fairy_move, {
	BoxedEnemy e;
	MoveParams move;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
	e->move = ARGS.move;
}

TASK(lightburst_fairy_2, {
	cmplx pos;
	MoveParams move_enter;
	MoveParams move_exit;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 4, .power = 2)));

	e->move = ARGS.move_enter;
	INVOKE_SUBTASK_DELAYED(200, lightburst_fairy_move, {
		.e = ENT_BOX(e),
		.move = ARGS.move_exit
	});

	real count = difficulty_value(6, 7, 8, 9);
	int difficulty = difficulty_value(1, 2, 3, 4);
	WAIT(70);
	for(int x = 0; x < 30; x++, WAIT(5)) {
		play_sfx_loop("shot1_loop");
		for(int i = 0; i < count; i++) {
			cmplx n = cnormalize(global.plr.pos - e->pos) * cdir(M_TAU / count * i);
			RNG_ARRAY(rand, 2);
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos + 50 * n * cdir(-1.0 * x * difficulty),
				.color = RGB(0.3, 0, 0.7 + 0.3 * (i % 2)),
				.move = move_asymptotic_simple(2.5 * n + 0.25 * difficulty * vrng_sreal(rand[0]) * vrng_dir(rand[1]), 3),
			);
		}
		play_sfx("shot2");
	}
}

TASK(lightburst_fairy_1, {
	cmplx pos;
	MoveParams move_enter;
	MoveParams move_exit;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 4, .power = 2)));

	e->move = ARGS.move_enter;
	INVOKE_SUBTASK_DELAYED(200, lightburst_fairy_move, {
		.e = ENT_BOX(e),
		.move = ARGS.move_exit
	});

	real count = difficulty_value(1, 4, 7, 9);
	real difficulty = difficulty_value(1, 2, 3, 4);

	real speed = difficulty_value(2, 2.33, 2.66, 3.0);
	int length = difficulty_value(35, 40, 45, 50);
	WAIT(20);
	for(int x = 0; x < length; x++, WAIT(5)) {
		play_sfx_loop("shot1_loop");
		for(int i = 0; i < count; i++) {
			cmplx n = cnormalize(global.plr.pos - e->pos) * cdir(M_TAU / count * i);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos + 50 * n * cdir(-0.4 * x * difficulty),
				.color = RGB(0.3, 0, 0.7),
				.move = move_asymptotic_simple(speed * n, 3),
			);
		}
		play_sfx("shot2");
	}
}

TASK(lightburst_fairies_1, {
	int num;
	cmplx pos;
	cmplx offset;
	cmplx exit;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos + ARGS.offset * i;
		INVOKE_TASK(lightburst_fairy_1,
			.pos = pos,
			.move_enter = move_from_towards(pos, pos + ARGS.exit * 70, 0.05),
			.move_exit = move_linear(ARGS.exit)
		);
		WAIT(40);
	}
}

TASK(lightburst_fairies_2, {
	int num;
	cmplx pos;
	cmplx offset;
	cmplx exit;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos + ARGS.offset * i;
		INVOKE_TASK(lightburst_fairy_2,
			.pos = pos,
			.move_enter = move_from_towards(pos, pos + ARGS.exit * 70, 0.05),
			.move_exit = move_linear(ARGS.exit)
		);
		WAIT(40);
	}
}

TASK(laser_fairy, {
	cmplx pos;
	MoveParams move_enter;
	MoveParams move_exit;
	int time;
}) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(.points = 4, .power = 2)));

	e->move = ARGS.move_enter;
	common_charge(60, &e->pos, 0, RGBA(0.7, 0.3, 1, 0));

	int delay = difficulty_value(9, 8, 7, 6);
	int amount = ARGS.time / delay;

	int angle_mod = difficulty_value(1, 2, 3, 4);
	int dir_mod = difficulty_value(9, 8, 7, 6);

	for(int x = 0; x < amount; x++) {
		cmplx n = cnormalize(global.plr.pos - e->pos) * cdir((0.02 * dir_mod) * x);
		real fac = (0.5 + 0.2 * angle_mod);

		create_lasercurve2c(e->pos, 100, 300, RGBA(0.7, 0.3, 1, 0), las_accel, fac * 4 * n, fac * 0.05 * n);

		PROJECTILE(
			.proto = pp_plainball,
			.pos = e->pos,
			.color = RGBA(0.7, 0.3, 1, 0),
			.move = move_accelerated(fac * 4 * n, fac * 0.05 * n),
		);
		play_sfx_ex("shot_special1", 0, true);
		WAIT(delay);
	}

	WAIT(30);
	e->move = ARGS.move_exit;
}

TASK(sine_swirl_sway, { BoxedEnemy e; }) {
	auto e = TASK_BIND(ARGS.e);

	for(int t = 0;; ++t, YIELD) {
		e->pos += sin(t / 5.0) * 2 * cnormalize(e->move.velocity * I);
	}
}

TASK(sine_swirl, { cmplx pos; cmplx velocity; int fire_delay; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.points = 2)));

	e->move = move_linear(ARGS.velocity);

	INVOKE_SUBTASK(sine_swirl_sway, ENT_BOX(e));

	int delay = ARGS.fire_delay;
	int nshots = 1;

	cmplx aim = I;

	real shot_chance = difficulty_value(0.0,0.33, 0.66, 1.0);
	for(;;WAIT(delay)) {
		for(int i = 0; i < nshots; ++i) {
			if(rng_chance(shot_chance)) {
				PROJECTILE(
					.proto = pp_thickrice,
					.pos = e->pos,
					.color = RGB(0.3, 0.4, 0.5),
					.move = move_asymptotic_simple(
						3 * aim, 1 + i),
				);
				play_sfx("shot2");
			}
		}
	}
}

TASK(sine_swirls, { int num; bool dense; }) {
	int fire_delay  = ARGS.dense ?  7 : 15;
	int spawn_delay = ARGS.dense ? 15 : 30;
	real move_speed = ARGS.dense ?  4 :  2;

	for(int i = 0; i < ARGS.num; ++i, WAIT(spawn_delay)) {
		INVOKE_TASK(sine_swirl,
			.pos = 40i,
			.velocity = move_speed,
			.fire_delay = fire_delay,
		);
		INVOKE_TASK_DELAYED(10, sine_swirl,
			.pos = VIEWPORT_W + 40i,
			.velocity = -move_speed,
			.fire_delay = fire_delay,
		);
	}
}

TASK(loop_swirl, {
	cmplx start;
	cmplx velocity;
	real turn_angle;
	int turn_duration;
	int turn_delay;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.start, ITEMS(.points = 2)));

	e->move = move_linear(ARGS.velocity);
	INVOKE_SUBTASK_DELAYED(ARGS.turn_delay, common_rotate_velocity,
		.move = &e->move,
		.angle = ARGS.turn_angle,
		.duration = ARGS.turn_duration,
    );

	int delay = 20;
	int nshots = difficulty_value(3, 5, 7, 10);

	for(;;WAIT(delay)) {
		for(int s = 1; s >= -1; s -= 2) {
			for(int i = 0; i < nshots; ++i) {
				PROJECTILE(
					.proto = pp_wave,
					.pos = e->pos,
					.color = RGB(0.3, 0.4, 0.5),
					.move = move_asymptotic_simple(
						s * 3 * I / cnormalize(e->move.velocity), 1 + i),
				);
			}
		}

		play_sfx("shot2");
	}
}

TASK(loop_swirls, {
	int num;
	int duration;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		real f = rng_real();
		real xdir = 1 - 2 * (i % 2);
		INVOKE_TASK(loop_swirl, {
			.start = VIEWPORT_W/2 + 300 * sin(i * 20) * cos (2 * i * 20),
			.velocity = 2 * cdir(M_PI * f) + I,
			.turn_angle = M_PI/2 * xdir,
			.turn_duration = 50,
			.turn_delay = 100,
		});
		WAIT(ARGS.duration / ARGS.num);
	}
}

TASK(limiter_fairy, {
	cmplx pos;
	MoveParams exit;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(.points = 2, .power = 1)));
	e->move = ARGS.exit;

	real baseangle = difficulty_value(0.2, 0.15, 0.125, 0.1);

	for(int x = 0; x < 400; x++) {
		for(int i = 1; i >= -1; i -= 2) {
			real a = i * (baseangle + 3.0 / (x + 1));
			cmplx plraim = cnormalize(global.plr.pos - e->pos);
			cmplx aim = plraim * cdir(a);

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.5, 0.1, 0.2),
				.move = move_asymptotic_simple(10 * aim, 2),
				.flags = PFLAG_NOSPAWNFADE,
			);
		}

		play_sfx_loop("shot1_loop");
		WAIT(3);
	}
}

TASK(limiter_fairies, {
	int num;
	cmplx pos;
	cmplx offset;
	cmplx exit;
}) {
	for(int i = 0; i < ARGS.num; i++) {
		cmplx pos = ARGS.pos + ARGS.offset * (i % 2);
		INVOKE_TASK(limiter_fairy,
			.pos = pos,
			.exit = move_linear(ARGS.exit)
		);
		WAIT(60);
	}
}

TASK(miner_swirl, {
	cmplx start;
	cmplx velocity;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.start, ITEMS(.points = 2)));

	e->move = move_linear(ARGS.velocity);

	int delay = difficulty_value(8, 6, 4, 3);

	cmplx shotdir = rng_dir();
	cmplx r = cdir(M_TAU/17);

	for(;;WAIT(delay)) {
		for(int s = -1; s < 2; s += 2) {
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(s > 0, 0, s < 0),
				// .move = move_asymptotic_simple(s * shotdir, 10),
				.move = move_asymptotic_simple(s * shotdir + e->move.velocity, 5),
			);
		}
		shotdir *= r;
		play_sfx_ex("shot3", 0, false);
	}
}

TASK(miner_swirls_3, {
	int num;
	cmplx start;
	cmplx velocity;
}) {
	for(int x = 0; x < ARGS.num; x++) {
		INVOKE_TASK(miner_swirl, {
			.start = 32 + (VIEWPORT_W - 2 * 32) * (x % 2) + VIEWPORT_H * I,
			.velocity = ARGS.velocity,
		});

		WAIT(60);
	}
}

TASK(miner_swirls_2, {
	int num;
	int delay;
	cmplx start;
	cmplx velocity;
}) {
	for(int x = 0; x < ARGS.num; x++) {
		INVOKE_TASK(miner_swirl, {
			.start = ARGS.start * x,
			.velocity = ARGS.velocity,
		});

		WAIT(ARGS.delay);
	}
}

TASK(miner_swirls_1, {
	int num;
	int delay;
	cmplx start;
	cmplx velocity;
	int time;
}) {
	if(ARGS.time > 0) {
		assert(ARGS.num == 0);
		ARGS.num = ARGS.time / ARGS.delay;
	}

	for(int x = 0; x < ARGS.num; x++) {
		INVOKE_TASK(miner_swirl, {
			.start = VIEWPORT_W + 200.0 * I * ((x % 5) / 4.0),
			.velocity = ARGS.velocity,
		});
		WAIT(ARGS.delay);
	}
}

TASK(superbullet_move, { BoxedProjectile p; cmplx dir; }) {
	auto p = TASK_BIND(ARGS.p);
	p->move = move_asymptotic(5 * ARGS.dir, 0, 0.9 * cdir(0.2));
	WAIT(30);
	play_sfx("warp");
	p->move = move_asymptotic_simple(10 * p->move.velocity, 8);
}

TASK(superbullet_fairy, {
	cmplx pos;
	cmplx acceleration;
	real offset;
}) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(.points = 4, .power = 1)));

	e->move = move_from_towards(e->pos, e->pos + ARGS.acceleration * 70 + ARGS.offset, 0.05);
	common_charge(60, &e->pos, 0, RGBA(1.0, 0.5, 0, 0));

	real difficulty = difficulty_value(5.0, 8.0, 11.0, 12.0);
	cmplx r = cnormalize(global.plr.pos - e->pos);

	for(int x = 0; x < 140; x++, YIELD) {
		real s = sin(x / difficulty);
		cmplx n = cdir(M_PI * s) * r;
		auto p = PROJECTILE(
			.proto = pp_bullet,
			.pos = e->pos + 50 * n,
			.color = RGB(1.0, 0.5, 0.0),
		);
		INVOKE_TASK(superbullet_move, ENT_BOX(p), n);
		play_sfx("shot1");
	}

	WAIT(60);
	e->move = move_linear(-ARGS.acceleration);
}

TASK(superbullet_fairies_1, {
	int num;
	int time;
}) {
	int delay = ARGS.time / ARGS.num;
	for(int i = 0; i < ARGS.num; i++) {
		real offset = -15 * i * (1 - 2 * (i % 2));



		INVOKE_TASK(superbullet_fairy, {
			.pos = VIEWPORT_W * (i % 2) + 100.0 * I + 15 * i * I,
			.acceleration = 3 - 6 * (i % 2),
			.offset = offset,
		});
		WAIT(delay);
	}
}

TASK(lasertrap_warning, { cmplx pos; int time; real radius; }) {
	auto spr = res_sprite("fairy_circle_big");  // TODO placeholder
	float maxscale = 2.0f * (float)ARGS.radius / (spr->w - 10);

	auto p = TASK_BIND(PARTICLE(
		.pos = ARGS.pos,
		.sprite_ptr = spr,
		.opacity = 0,
		.angle_delta = M_TAU/96,
		.flags = PFLAG_MANUALANGLE,
		.color = RGBA(1, 0.8, 1, 0),
	));

	int fadeintime = 60;
	int fadeouttime = 15;

	for(int i = 0; i < fadeintime; ++i, YIELD) {
		float f = (i + 1.0f) / fadeintime;
		p->opacity = f;
		p->scale = (1 + I) * lerpf(0, maxscale, glm_ease_quad_out(f));
	}

	WAIT(ARGS.time - fadeintime);

	for(int i = 0; i < fadeouttime; ++i, YIELD) {
		float f = 1.0f - (i + 1.0f) / fadeouttime;
		p->opacity = f * f;
		p->scale = (1 + I) * lerpf(maxscale * 1.5f, maxscale, glm_ease_quad_in(f));
	}

	kill_projectile(p);
}

static Laser *laser_arc(cmplx center, real lifetime, real radius, real arcangle, real arcshift) {
	real a = arcangle/lifetime;

	Laser *l = create_lasercurve2c(
		center, lifetime, lifetime, RGBA(0.5, 0.1, 1.0, 0), las_circle,
		a + I*arcshift/a, radius
	);

	laser_make_static(l);
	return l;
}

TASK(animate_arc, { BoxedLaser arc; real radius_delta; }) {
	auto arc = TASK_BIND(ARGS.arc);

	for(;;YIELD) {
		arc->args[1] += ARGS.radius_delta;
		real a = re(arc->args[0]);
		arc->args[0] = a + I * (im(arc->args[0]) * a + M_TAU/194) / a;
	}
}

TASK(lasertrap_arc_bullet, { BoxedLaser l; int timeout; }) {
	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_soul,
		.angle = rng_angle(),
		.angle_delta = M_PI/49,
		.flags = PFLAG_MANUALANGLE | PFLAG_NOAUTOREMOVE,
		.color = RGBA(0.25, 0.1, 1.0, 0.0),
		.timeout = ARGS.timeout,
	));

	Laser *l = NOT_NULL(ENT_UNBOX(ARGS.l));
	p->pos = laser_pos_at(l, 0);

	for(;(l = ENT_UNBOX(ARGS.l)); YIELD) {
		p->prevpos = p->pos;
		p->pos = laser_pos_at(l, 0);
		iku_lightning_particle(p->pos);
	}

	kill_projectile(p);
}

TASK(lasertrap, { cmplx pos; }) {
	int traptime = difficulty_value(200, 160, 140, 120);
	int boomdelay = 30;
	int boomtime = traptime + boomdelay;
	real radius = 120;
	real shrinkspeed = radius / boomdelay;
	real preradius = radius + traptime * shrinkspeed;

	// log_debug("shrinkspeed: %f", shrinkspeed);

	int narcs = 5;
	real ofs = rng_angle();
	real a = M_TAU/narcs;

	for(int i = 0; i < narcs; ++i) {
		Laser *l = laser_arc(ARGS.pos, boomtime + 20, preradius, a, ofs);
		INVOKE_TASK(animate_arc, ENT_BOX(l), -shrinkspeed);
		INVOKE_TASK(lasertrap_arc_bullet, ENT_BOX(l), boomtime);
		INVOKE_TASK(laser_charge, ENT_BOX(l),
			.target_width = 18,
			.charge_delay = traptime,
		);
		INVOKE_TASK_DELAYED(traptime, common_play_sfx, "laser1");
		ofs += a;
	}

	INVOKE_SUBTASK(lasertrap_warning, ARGS.pos, boomtime, radius);
	common_charge(boomtime, &ARGS.pos, 0, RGBA(0.5, 0.1, 1.0, 0));
	play_sfx("boom");

	int cnt = difficulty_value(15, 28, 32, 40);
	int ringcnt = difficulty_value(3, 4, 5, 6);
	cmplx startaim = rng_dir();
	cmplx r = cdir(M_TAU/cnt);

	for(int j = 0; j < ringcnt * 2; ++j) {
		play_sfx("shot_special1");

		cmplx aim = startaim;
		real s = (j & 1) * 2 - 1;
		float jf = (j / (2 * ringcnt - 1.0));
		Color *c = RGBA(0.25 * (1 - jf * jf), 0.25 * jf, 1, 0);

		for(int i = 0; i < cnt; ++i) {
			auto move = move_asymptotic(6*aim, -2*aim * cdir(s*M_PI/2), exp2(-1.0 / 30));
			PROJECTILE(
				.proto = pp_ball,
				.color = c,
				.pos = ARGS.pos - aim * 16,
				.move = move,
				.flags = PFLAG_NOSPAWNFLARE * (j & 1),
			);
			aim *= r;
		}

		if(j & 1) {
			WAIT(7);
		}
	}

	AWAIT_SUBTASKS;
}

TASK(lasertraps, { int num; int interval; }) {
	cmplx vp = VIEWPORT_W + VIEWPORT_H*I;

	for(int i = 0; i < ARGS.num; ++i, WAIT(ARGS.interval)) {
		INVOKE_TASK(lasertrap, cwclamp(global.plr.pos, 0, vp));
	}
}

TASK(magnet_bullet, { cmplx pos; cmplx vel; int idx; BoxedProjectileArray *magnets; }) {
	MoveParams m = {
		.velocity = ARGS.vel,
		.retention = 0.992,
		.attraction = 0.1,
		.attraction_point = global.plr.pos,
	};

	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_bigball,
		.pos = ARGS.pos,
		.color = (ARGS.idx & 1) ? RGBA(0.2, 0.3, 1.0, 0.0) : RGBA(1.0, 0.3, 0.2, 0.0),
		.move = m,
	));

	ENT_ARRAY_ADD(ARGS.magnets, p);

	for(;;) {
		YIELD;

		p->move.attraction_point = global.plr.pos;

		ENT_ARRAY_FOREACH(ARGS.magnets, Projectile *m, {
			if(m == p) {
				continue;
			}

			cmplx delta = m->pos - p->pos;
			real r = 1.5 * re(m->collision_size);

			real norm2 = cabs2(delta);
			real norm = sqrt(norm2);
			real s = 1 / max(r, norm - r);
			cmplx force = (s / norm) * delta;

			if(norm < r || color_equals(&m->color, &p->color)) {
				force = -force;
			}

			p->move.velocity += force;
		});

		if(rng_chance(0.5)) {
			iku_lightning_particle(p->pos + p->move.velocity * 2);
		}
	}
}

TASK(magnet_fairy, {
	cmplx pos;
	MoveParams move_enter;
	MoveParams move_exit;
	BoxedProjectileArray *magnets;
}) {
	auto e = espawn_huge_fairy(ARGS.pos, ITEMS(.power = 3, .points = 5));
	auto box = ENT_BOX(e);
	e->move = ARGS.move_enter;

	int ncycles = difficulty_value(3, 5, 7, 7);
	int nshots = difficulty_value(1, 1, 2, 2);
	real arc = M_PI * 0.5;
	real initspeed = 5;
	cmplx initdir = -I;

	ncycles *= 2;
	for(int i = 0; i < ncycles; ++i) {
		WAIT(60 - 50 * (i & 1));

		if(!(e = ENT_UNBOX(box))) {
			AWAIT_SUBTASKS;
			return;
		}

		for(int s = 0; s < nshots; ++s) {
			cmplx d = initdir * cdir(arc * (s + 1.0)/(nshots + 1.0) - arc/2);
			INVOKE_SUBTASK(magnet_bullet, {
				.pos = e->pos + 20 * d,
				.vel = initspeed * d,
				.idx = (s < nshots) ^ (i & 1) ^ ((i/2) & 1),
				.magnets = ARGS.magnets
			});
		}

		play_sfx("warp");
		play_sfx("shot_special1");
	}

	e->move = ARGS.move_exit;
	AWAIT_SUBTASKS;
}

TASK(magnet_fairies_1, { int num; int delay; }) {
	struct { cmplx origin, target; } spawns[] = {
		{ 300*I,              VIEWPORT_W - 100 + 200*I },
		{ VIEWPORT_W + 300*I,              100 + 200*I },
	};

	DECLARE_ENT_ARRAY(Projectile, magnets, 256);

	for(int i = 0; i < ARGS.num; ++i, WAIT(ARGS.delay)) {
		int s = i % ARRAY_SIZE(spawns);
		INVOKE_SUBTASK(magnet_fairy, {
			.pos = spawns[s].origin,
			.move_enter = move_from_towards(spawns[s].origin, spawns[s].target, 0.02),
			.move_exit = move_accelerated(1 - 2 * !(i & 1), 0.1*I),
			.magnets = &magnets,
		});
	}

	AWAIT_SUBTASKS;
}

DEFINE_EXTERN_TASK(stage5_timeline) {
	INVOKE_TASK_DELAYED(60, greeter_fairies_1, {
		.num = 7,
	});

	INVOKE_TASK_DELAYED(210, lightburst_fairies_1, {
		.num = 2,
		.pos = VIEWPORT_W/4,
		.offset = VIEWPORT_W/2,
		.exit = 2.0 * I,
	});

	INVOKE_TASK_DELAYED(270, sine_swirls, 30);

	INVOKE_TASK_DELAYED(400, lightburst_fairies_1, {
		.num = 2,
		.pos = VIEWPORT_W/4 - 60,
		.offset = VIEWPORT_W/2 + 120,
		.exit = 2.5 * I,
	});

	INVOKE_TASK_DELAYED(600, arc_fairies,
		.origin = VIEWPORT_W/2 - 20i,
		.arc = M_PI * 0.85,
		.dir = I,
		.dist = 200,
		.num = 6,
		.waves = 1,
		.wave_delay = 300,
		.color_rule = arcspawn_fairy_alternate,
	);

	INVOKE_TASK_DELAYED(780, limiter_fairies, {
		.num = 6,
		.pos = VIEWPORT_W/4,
		.offset = VIEWPORT_W/2,
		.exit = I,
	});

	INVOKE_TASK_DELAYED(1000, laser_fairy, {
		.pos = VIEWPORT_W/2,
		.move_enter = move_from_towards(VIEWPORT_W/2, VIEWPORT_W/2 + 2.0 * I * 100, 0.05),
		.move_exit = move_linear(-(2.0 * I)),
		.time = 700,
	});

	INVOKE_TASK_DELAYED(1200, arc_fairies, {
		.origin = VIEWPORT_W/2 - 20i,
		.arc = M_PI * 0.85,
		.dir = I,
		.dist = 200,
		.num = 6,
		.waves = 2,
		.wave_delay = 300,
		.color_rule = arcspawn_split,
	});

	INVOKE_TASK_DELAYED(1400, miner_swirls_1, {
		.num = 30,
		.delay = 30,
		.start = 200.0 * I,
		.velocity = -3.0 + 2.0 * I,
	});

	INVOKE_TASK_DELAYED(2200, loop_swirls, {
		.num = difficulty_value(47, 53, 57, 60),
		.duration = 800,
	});

	INVOKE_TASK_DELAYED(1500, magnet_fairies_1, {
		.num = 4,
		.delay = 180,
	});

	INVOKE_TASK_DELAYED(2200, miner_swirls_2, {
		.num = 20,
		.delay = 60,
		.start = VIEWPORT_W / (6 + global.diff),
		.velocity = 3.0 * I,
	});

	INVOKE_TASK_DELAYED(2500, lightburst_fairies_1, {
		.num = 1,
		.pos = VIEWPORT_W/2,
		.offset = 0,
		.exit = 2.0 * I,
	});

	if(global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(2700, lightburst_fairies_1, {
			.num = 1,
			.pos = (VIEWPORT_W - 20) + (120 * I),
			.offset = 0,
			.exit = -2.0,
		});
	}

	STAGE_BOOKMARK_DELAYED(2900, pre-midboss);
	INVOKE_TASK_DELAYED(2900, spawn_midboss);
	while(!global.boss) YIELD;
	STAGE_BOOKMARK(post-midboss);
	WAIT(1100);
	stage5_dialog_post_midboss();

	INVOKE_TASK_DELAYED(120, sine_swirls, 30, .dense = global.diff > D_Normal);

	WAIT(200);

	INVOKE_TASK_DELAYED(0, lightburst_fairies_2, {
		.num = 3,
		.pos = VIEWPORT_W/4,
		.offset = VIEWPORT_W/4,
		.exit = 2.0 * I,
	});

	INVOKE_TASK_DELAYED(300, superbullet_fairies_1, {
		.num = difficulty_value(8, 10, 12, 14),
		.time = 700,
	});

	INVOKE_TASK_DELAYED(400, laser_fairy, {
		.pos = VIEWPORT_W/4,
		.move_enter = move_from_towards(VIEWPORT_W/4, VIEWPORT_W/4 + 2.0 * I * 100, 0.05),
		.move_exit = move_linear(-(2.0 * I)),
		.time = 600,
	});

	INVOKE_TASK_DELAYED(400, laser_fairy, {
		.pos = VIEWPORT_W/4 * 3,
		.move_enter = move_from_towards(VIEWPORT_W/4 * 3, VIEWPORT_W/4 * 3 + 2.0 * I * 100, 0.05),
		.move_exit = move_linear(-(2.0 * I)),
		.time = 600,
	});

	INVOKE_TASK_DELAYED(1160, lasertraps,
		.num = 3,
		.interval = 300,
	);

	INVOKE_TASK_DELAYED(1320, miner_swirls_3, {
		.num = 10,
		.velocity = -3*I,
	});

	INVOKE_TASK_DELAYED(2000, lightburst_fairies_1, {
		.num = 1,
		.pos = VIEWPORT_W/2,
		.offset = 0,
		.exit = 2.0 * I,
	});

	INVOKE_TASK_DELAYED(2100, magnet_fairies_1, {
		.num = 2,
		.delay = 60,
	});

	INVOKE_TASK_DELAYED(2030, superbullet_fairy, {
		.pos = 30.0 * I + VIEWPORT_W,
		.acceleration = -2 + I,
	});

	INVOKE_TASK_DELAYED(2060, superbullet_fairy, {
		.pos = 30.0 * I,
		.acceleration = 2 + I,
	});

	INVOKE_TASK_DELAYED(2060, miner_swirls_1, {
		.num = difficulty_value(5, 7, 9, 12),
		.delay = 60,
		.velocity = -3 + 2.0 * I,
	});

	INVOKE_TASK_DELAYED(2180, lightburst_fairies_2, {
		.num = 1,
		.pos = VIEWPORT_W/2,
		.offset = 100,
		.exit = 2.0 * I - 0.25,
	});

	INVOKE_TASK_DELAYED(2360, superbullet_fairy, {
		.pos = 30.0 * I + VIEWPORT_W,
		.acceleration = -2 + I,
	});

	INVOKE_TASK_DELAYED(2390, superbullet_fairy, {
		.pos = 30.0 * I,
		.acceleration = 2 + I,
	});

	INVOKE_TASK_DELAYED(2500, lightburst_fairies_1, {
		.num = 1,
		.pos = VIEWPORT_W+20 + VIEWPORT_H * 0.6 * I,
		.offset = 0,
		.exit = -2 * I - 2,
	});

	INVOKE_TASK_DELAYED(2500, lightburst_fairies_1, {
		.num = 1,
		.pos = -20 + VIEWPORT_H * 0.6 * I,
		.offset = 0,
		.exit = -2 * I + 2,
	});

	INVOKE_TASK_DELAYED(2620, loop_swirls, {
		.num = difficulty_value(47, 53, 57, 60),
		.duration = 800,
	});

	INVOKE_TASK_DELAYED(2800, arc_fairies,
		.origin = VIEWPORT_W,
		.arc = M_PI * 0.45,
		.dir = cpow(I, 1.5),
		.dist = 200,
		.num = 5,
		.waves = 3,
		.wave_delay = 150,
		.color_rule = arcspawn_wave_alternate,
	);

	INVOKE_TASK_DELAYED(2850, arc_fairies,
		.origin = 0,
		.arc = M_PI * 0.45,
		.dir = csqrt(I),
		.dist = 200,
		.num = 5,
		.waves = 3,
		.wave_delay = 150,
		.color_rule = arcspawn_wave_alternate,
	);

	INVOKE_TASK_DELAYED(2950, miner_swirls_1, {
		.time = 600,
		// .num = 10,
		.delay = difficulty_value(80, 60, 40, 30),
		.velocity = -3 + 2.0 * I,
	});

	INVOKE_TASK_DELAYED(3300, limiter_fairies, {
		.num = 4,
		.pos = VIEWPORT_W/4,
		.offset = VIEWPORT_W/2,
		.exit = 2 * I,
	});

	INVOKE_TASK_DELAYED(3300, common_call_func, stage5_bg_lower_camera);

	STAGE_BOOKMARK_DELAYED(3400, pre-boss);

	WAIT(3830);
	INVOKE_TASK(spawn_boss);
	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	stage_unlock_bgm("stage5boss");

	WAIT(240);
	stage5_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
