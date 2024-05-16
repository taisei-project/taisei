/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "timeline.h"
#include "stage3.h"
#include "wriggle.h"
#include "scuttle.h"
#include "spells/spells.h"
#include "nonspells/nonspells.h"
#include "background_anim.h"
#include "draw.h"

#include "global.h"
#include "stagetext.h"
#include "common_tasks.h"
#include "enemy_classes.h"

static void stage3_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage3PostBossDialog, pm->dialog->Stage3PostBoss);
}

TASK(swarm_trail_proj_cleanup, { BoxedProjectile phead; }) {
	Projectile *p = TASK_BIND(ARGS.phead);
	clear_projectile(p, CLEAR_HAZARDS_FORCE | CLEAR_HAZARDS_NOW);
}

static cmplx trail_pos(
	int t, cmplx origin, cmplx vstart, cmplx vend, real x, real width, real turn_length
) {
	cmplx z = t/width + I * x/turn_length;
	return origin + width * z * (vstart + (vend - vstart) / (cexp(-z) + 1));
}

TASK(swarm_trail_proj, { cmplx pos; cmplx vstart; cmplx vend; real x; real width; }) {
	int t0 = -70;
	real turn_length = 1; // |x/turn_length| should be smaller than pi
	cmplx prevpos = trail_pos(
		t0 - 1, ARGS.pos, ARGS.vstart, ARGS.vend, ARGS.x, ARGS.width, turn_length);

	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_rice,
		.pos = prevpos,
		.color = RGB(0.4, 0, 0.8),
		.flags = PFLAG_NOAUTOREMOVE | PFLAG_NOMOVE | PFLAG_MANUALANGLE,
		.max_viewport_dist = ARGS.width,
	));

	BoxedProjectile phead = ENT_BOX(PROJECTILE(
		.proto = pp_flea,
		.pos = prevpos,
		.color = RGB(0.8, 0.1, 0.4),
		.flags = PFLAG_NOAUTOREMOVE | PFLAG_NOMOVE | PFLAG_MANUALANGLE,
		.max_viewport_dist = ARGS.width,
	));

	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, swarm_trail_proj_cleanup, phead);

	for(int t = t0;; t++) {
		Projectile *head = ENT_UNBOX(phead);

		if(t == 0) {
			p->flags &= ~PFLAG_NOAUTOREMOVE;
			if(head) {
				head->flags &= ~PFLAG_NOAUTOREMOVE;
			}
			play_sfx("redirect");
		}

		p->pos = trail_pos(t, ARGS.pos, ARGS.vstart, ARGS.vend, ARGS.x, ARGS.width, turn_length);
		cmplx dpos = p->pos - prevpos;
		p->angle = carg(dpos);

		if(head) {
			head->angle = p->angle;
			head->pos = p->pos + 8*cnormalize(dpos);
		}

		prevpos = p->pos;
		YIELD;
	}
}

TASK(swarm_trail_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(
		.points = 1,
		.power = 2,
	)));

	e->move = ARGS.move;

	int shooting_time = 200;
	int interval = 10;
	int nrow = difficulty_value(3, 5, 5, 7);
	real width = 30 * (nrow / 5.0);

	WAIT(30);
	e->move.retention = 0.9;
	WAIT(20);
	cmplx aim = cnormalize(global.plr.pos - e->pos);

	for(int t = 0; t < shooting_time/interval; t++) {
		play_sfx("shot1");

		for(int i = 0; i < nrow; i++) {
			real x = (i/(real)nrow - 0.5);
			INVOKE_TASK(swarm_trail_proj, e->pos, ARGS.move.velocity, 3*aim, x, .width = width);
		}

		WAIT(interval);
	}
	play_sfx("redirect");
	e->move = move_asymptotic_halflife(0, -ARGS.move.velocity, 120);
}

TASK(swarm_trail_fairy_spawn, { int count; }) {
	for(int i = 0; i < ARGS.count; i++) {
		cmplx pos = VIEWPORT_W / 2.0;
		cmplx vel = 5*I + 4 * cdir(-M_PI * i / (real)(ARGS.count-1));
		INVOKE_TASK(swarm_trail_fairy, pos, move_linear(vel));

		WAIT(60);
	}
}

TASK(flower_swirl_trail, { BoxedEnemy e; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	for(;;) {
		YIELD;
		PARTICLE(
			.sprite = "smoothdot",
			.color = RGBA(2, 1, 0.6, 0),
			.pos = e->pos,
			.timeout = 30,
			.draw_rule = pdraw_timeout_scalefade_exp(1, 2, 1, 0, 2),
		);
	}
}

static inline MoveParams flower_swirl_move(cmplx dir, real angular_velocity, real radius) {
	cmplx retention = 0.99 * cdir(angular_velocity);
	return (MoveParams) {
		.velocity = radius * cnormalize(dir) * I * cabs(1 - retention),
		.retention = retention,
		.acceleration = dir * (1 - retention),
	};
}

TASK(flower_swirl, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(
		.power = 1,
	)));

	e->move = ARGS.move;
	INVOKE_TASK(flower_swirl_trail, ENT_BOX(e));

	cmplx twist = cdir(M_TAU/64);

	int interval = 20;
	int petals = 5;
	Color pcolor = *RGBA(1, 0.5, 0, 1);

	int orange_cnt = difficulty_value(5, 5, 7, 7);
	int red_cnt = difficulty_value(5, 7, 7, 9);
	real speed = difficulty_value(1, 1, 1.5, 2);

	for(int t = 0;; t++) {
		if(t & 1) {
			pcolor.g = 0;
			petals = red_cnt;
		} else {
			pcolor.g = 0.5;
			petals = orange_cnt;
		}

		play_sfx("shot2");

		cmplx r = cnormalize(e->move.velocity);

		for(int i = 0; i < petals; i++) {
			cmplx dir = r * cdir(M_TAU / petals * i);
			PROJECTILE(
				.proto = pp_wave,
				.color = &pcolor,
				.pos = e->pos - 4 * dir,
				.move = move_asymptotic_halflife(speed * 0.7 * dir, speed * -4 * dir * twist, 200),
			);
		}

		WAIT(interval);
	}
}

TASK(flower_swirl_spawn, { cmplx pos; MoveParams move; int count; int interval; }) {
	for(int i = 0; i < ARGS.count; i++) {
		INVOKE_TASK(flower_swirl, ARGS.pos, ARGS.move);
		WAIT(ARGS.interval);
	}
}

TASK(horde_fairy_motion, { BoxedEnemy e; cmplx velocity; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	real ofs = rng_angle();
	cmplx v = ARGS.velocity;
	e->move = move_linear(v);
	for(;;YIELD) {
		e->move.velocity = v * cdir(M_PI/8 * sin(ofs + global.frames/15.0));
	}
}

TASK(horde_fairy, { cmplx pos; cmplx velocity; bool blue; }) {
	Enemy *e;
	if(ARGS.blue) {
		e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(
			.points = 1,
		)));
	} else {
		e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(
			.power = 1,
		)));
	}

	INVOKE_SUBTASK(horde_fairy_motion, ENT_BOX(e), ARGS.velocity);

	int interval = difficulty_value(70, 40, 30, 20);
	real speed = difficulty_value(2, 2, 2.5, 3);
	real lead = difficulty_value(5, 5, 3.25, 2.75);
	real minrange = difficulty_value(80, 60, 40, 40);
	int maxshots = difficulty_value(3, 5, 10, 20);

	for(int i = 0; i < maxshots; ++i, WAIT(interval)) {
		play_sfx("shot1");
		cmplx diff = global.plr.pos - e->pos;

		if(cabs2(diff) < minrange * minrange) {
			continue;
		}

		cmplx aim = cnormalize(diff);
		PROJECTILE(
			.proto = pp_plainball,
			.pos = e->pos,
			.color = RGB(0.4, 0, 1),
			.move = move_asymptotic_halflife(0, speed * aim, 25),
		);
		PROJECTILE(
			.proto = pp_flea,
			.pos = e->pos,
			.color = RGB(0.1, 0.4, 0.8),
			.move = move_asymptotic_halflife(0, speed * aim, 25 - lead),
		);
	}
}

TASK(horde_fairy_spawn, { int count; int interval; cmplx velocity; }) {
	real p = 3.0 / ARGS.count;
	real w = 0.9*VIEWPORT_W/2;
	bool blue = rng_bool();
	for(int t = 0; t < ARGS.count; t++) {
		INVOKE_TASK(horde_fairy,
			.pos = VIEWPORT_W/2 + w * triangle(t * p),
			.blue = blue,
			.velocity = ARGS.velocity,
		);
		if(((t+1) % 3) == 0) {
			blue = !blue;
		}
		WAIT(ARGS.interval);
	}
}

TASK(circle_twist_fairy_lances, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	common_charge(120, &e->pos, 0, RGBA(0.6, 0.8, 2, 0));
	cmplx offset = rng_dir();

	int lance_count = difficulty_value(50, 100, 200, 300);
	int lance_segs = difficulty_value(10, 15, 18, 20);
	int lance_dirs = difficulty_value(50, 70, 100, 100);
	real split = difficulty_value(0, 0.02, 0.03, 0.05);
	int fib1 = 1;
	int fib2 = 1;
	for(int i = 0; i < lance_count; i++) {
		play_sfx("shot3");
		int tmp = fib1;
		fib1 = (fib1 + fib2) % lance_dirs;
		fib2 = tmp;

		cmplx dir = offset * cdir(M_TAU / lance_dirs * fib1);
		for(int j = 0; j < lance_segs; j++) {
			int s = 1 - 2 * (j & 1);

			cmplx d = dir * cdir(split * s);

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos + 20 * d,
				.color = RGB(0.3, 0.4, 1),
				.move = move_asymptotic_halflife(
					(0.5 + (4.5 * j) / lance_segs) * d,
					5 * d,
					50
				),
			);
		}

		WAIT(2);
	}
}

TASK(circle_twist_fairy, { cmplx pos; cmplx target_pos; }) {
	Enemy *e = TASK_BIND(espawn_super_fairy(ARGS.pos, ITEMS(
		.power = 5,
		.points = 5,
		.bomb_fragment = 1,
	)));

	e->move = move_from_towards(e->pos, ARGS.target_pos, 0.01);
	WAIT(50);

	int circle_count = 10;
	int count = 50;
	int interval = difficulty_value(60, 40, 30, 20);
	real twistangle = -0.01 * difficulty_value(1, 1, 0.5, 0.5);

	INVOKE_SUBTASK_DELAYED(180, circle_twist_fairy_lances, ENT_BOX(e));

	for(int i = 0; i < circle_count; i++) {
		int s = 1-2*(i&1);
		play_sfx("shot_special1");
		for(int t = 0; t < count; t++) {
			play_sfx_loop("shot1_loop");
			cmplx dir = -I * cdir(s * M_TAU / count * t);
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = RGBA(1.5, -2.3, 0, 0),
				.move = {
					.velocity = (2 + 0.005 * t) * dir,
					.acceleration = -0.005 * dir,
					.retention = 1.0 * cdir(twistangle * s)
				}
			);
			YIELD;
		}
		WAIT(interval);
	}

	e->move = move_asymptotic_halflife(0, 2 * I, 20);

}

TASK(laserball, { cmplx origin; cmplx velocity; Color *color; real freq_factor; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_bigball,
		.pos = ARGS.origin,
		.move = move_asymptotic_halflife(ARGS.velocity, 0, 15),
		.color = ARGS.color,
	));

	WAIT(60);

	real speed = difficulty_value(2, 2.5, 3, 4);
	real amp = 0.01 * difficulty_value(4, 8, 8, 10);
	real freq = (speed / 4) * 0.25 * ARGS.freq_factor;

	cmplx lv = speed * -cnormalize(ARGS.velocity);

	real lt = 20;
	real dt = 300;

	int charges = 4;
	int delay = difficulty_value(30, 20, 20, 20);
	int refire = 20;

	real phase = 0;

	real scale = 1;
	real scale_per_charge = 1 / (real)charges;
	real scale_per_tick = scale_per_charge / delay;

	cmplx orig_collision = p->collision_size;
	cmplx orig_scale = p->scale;

	for(int i = 0;; ++i) {
		play_sfx("laser1");
		PROJECTILE(
			.pos = p->pos,
			.proto = pp_flea,
			.timeout = 1,
			.color = RGBA(1, 1, 1, 0),
		);

		create_lasercurve4c(p->pos, lt, dt, &p->color, las_sine_expanding, lv, amp, freq, i * phase);

		for(int t = 0; t < delay; ++t) {
			YIELD;
			scale -= scale_per_tick;
			p->collision_size = orig_collision * scale;
			p->scale = orig_scale * scale;
		}

		if(i == charges - 1) {
			break;
		}

		WAIT(refire);
		// phase += 3.2;
		freq *= -1;
	}

	kill_projectile(p);
}

TASK(laserball_fairy, { cmplx pos; real freq_factor; }) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(
		.power = 5,
		.points = 5,
	)));

	ecls_anyfairy_summon(e, 120);
	WAIT(30);
	common_charge(90, &e->pos, 0, RGBA(0.5, 1, 0.25, 0));

	int balls = 6;
	int cycles = difficulty_value(1, 1, 2, 2);

	for(int c = 0; c < cycles; ++c) {
		play_sfx("shot1");
		play_sfx("warp");

		for(int i = 0; i < balls; ++i) {
			cmplx v = 3 * cdir(i * (M_TAU / balls));
			INVOKE_TASK(laserball, e->pos, v, RGBA(0.2, 2, 0.4, 0), ARGS.freq_factor);
		}

		WAIT(difficulty_value(240, 240, 120, 120));
		play_sfx("shot1");
		play_sfx("warp");

		for(int i = 0; i < balls; ++i) {
			cmplx v = 3 * cdir((i + 0.5) * (M_TAU / balls));
			INVOKE_TASK(laserball, e->pos, v, RGBA(2, 1, 0.4, 0), -ARGS.freq_factor);
		}

		WAIT(180);
	}

	e->move = move_asymptotic_halflife(e->move.velocity, 3*I, 60);
}

TASK(bulletring, { BoxedEnemy owner; int num; real radius; real speed; }) {
	int num_bullets = ARGS.num;
	DECLARE_ENT_ARRAY(Projectile, bullets, num_bullets);

	play_sfx("shot_special1");
	play_sfx("warp");

	real angular_velocity = ARGS.speed / ARGS.radius;

	Enemy *e = NOT_NULL(ENT_UNBOX(ARGS.owner));
	e->max_viewport_dist = max(e->max_viewport_dist, ARGS.radius);
	real r = 1;
	real a = 0;

	for(int i = 0; i < num_bullets; ++i) {
		ENT_ARRAY_ADD(&bullets, PROJECTILE(
			.proto = pp_rice,
			.color = RGB(0.4, 0, 0.8),
			.flags = PFLAG_MANUALANGLE,
			.move = move_linear(0),
			.pos = e->pos,
			.max_viewport_dist = ARGS.radius,
		));
	}

	cmplx opos = e->pos;

	for(;;YIELD) {
		if(!(e = ENT_UNBOX(ARGS.owner))) {
			break;
		}

		ENT_ARRAY_FOREACH_COUNTER(&bullets, int i, Projectile *p, {
			real angle = a + (M_TAU*i)/num_bullets;
			cmplx npos = e->pos + (r * (1 + 0.03 * sin(3.6 * a + 6 * angle))) * cdir(angle);
			p->move.velocity = npos - p->pos;
			p->angle = carg(p->move.velocity + opos - e->pos);
		});

		opos = e->pos;
		a += angular_velocity;
		approach_asymptotic_p(&r, ARGS.radius, 0.05, 1e-4);
	}

	ENT_ARRAY_FOREACH(&bullets, Projectile *p, {
		// p->move.velocity = ARGS.speed * cdir(p->angle);
		p->angle = carg(p->move.velocity);
		p->move = move_asymptotic_halflife(p->move.velocity, cnormalize(p->move.velocity) * fabs(ARGS.speed), 60);
	});
}

TASK(bulletring_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_huge_fairy(ARGS.pos, ITEMS(
		.power = 3,
		.points = 5,
	)));

	e->move = ARGS.move;

	int bullet_rings = difficulty_value(2, 3, 4, 4);

	for(int i = 0; i < bullet_rings; ++i) {
		WAIT(60);

		real rad = 64 + 16 * i;
		real spacing = 2;
		int bullets = round(rad / spacing);

		INVOKE_TASK(bulletring, ENT_BOX(e), bullets, rad, ((i&1) ? 1 : -1) * 2);
	}
}

TASK(bulletring_fairy_spawn, { int count; }) {
	for(int i = 0; i < ARGS.count; ++i) {
		real d = 120;
		cmplx o[] = { d, VIEWPORT_W - d };
		cmplx p = o[i % ARRAY_SIZE(o)];
		INVOKE_TASK(bulletring_fairy, p, move_linear(I));
		WAIT(120);
	}
}

// bosses

TASK_WITH_INTERFACE(midboss_intro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 100.0*I, 0.04);
}

TASK_WITH_INTERFACE(midboss_outro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	for(int t = 0;; t++) {
		boss->pos += t*t/900.0 * cdir(3*M_PI/2 + 0.5 * sin(t / 20.0));
		YIELD;
	}
}

TASK(spawn_midboss) {
	STAGE_BOOKMARK_DELAYED(120, midboss);

	Boss *boss = global.boss = stage3_spawn_scuttle(VIEWPORT_W/2 - 200.0*I);

	glm_vec3_copy((vec3) { -10, -20, -30 }, stage3_get_draw_data()->boss_light.radiance);

	boss_add_attack_task(boss, AT_Move, "Introduction", 1, 0, TASK_INDIRECT(BossAttack, midboss_intro), NULL);
	boss_add_attack_task(boss, AT_Normal, "Lethal Bite", 11, 20000, TASK_INDIRECT(BossAttack, stage3_midboss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.mid.deadly_dance, false);
	boss_add_attack_task(boss, AT_Move, "Runaway", 2, 1, TASK_INDIRECT(BossAttack, midboss_outro), NULL);

	boss_engage(boss);
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2.0 + 100.0*I, 0.05);
}

TASK(spawn_boss) {
	STAGE_BOOKMARK_DELAYED(120, boss);
	stage_unlock_bgm("stage3");

	Boss *boss = global.boss = stage3_spawn_wriggle(VIEWPORT_W/2 - 200.0*I);

	glm_vec3_copy((vec3) { 50, 30, 80 }, stage3_get_draw_data()->boss_light.radiance);

	PlayerMode *pm = global.plr.mode;
	Stage3PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage3PreBossDialog, pm->dialog->Stage3PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage3boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task(boss, AT_Normal, "", 11, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.moonlight_rocket, false);
	boss_add_attack_task(boss, AT_Normal, "", 40, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.moths_to_a_flame, false);
	boss_add_attack_task(boss, AT_Normal, "", 40, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_3), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.firefly_storm, false);
	boss_add_attack_from_info(boss, &stage3_spells.extra.light_singularity, false);

	boss_engage(boss);
}

TASK(flower_swirls_alternating) {
	int interval = 20;
	int cnt = 4;
	int altinterval = interval * cnt * 2;

	INVOKE_TASK(flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 160 * I,
		.move = flower_swirl_move(-2 + 0.0 * I, -0.09, 120),
		.count = cnt,
		.interval = interval,
	);

	WAIT(altinterval);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = - 10 + 160 * I,
		.move = flower_swirl_move(2 + 0.0 * I, -0.09, 120),
		.count = 4,
		.interval = interval,
	);

	WAIT(altinterval);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 160 * I,
		.move = flower_swirl_move(-1 + 1.0 * I, -0.09, 120),
		.count = cnt,
		.interval = interval,
	);

	WAIT(altinterval);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = - 10 + 160 * I,
		.move = flower_swirl_move(1 + 1.0 * I, -0.09, 120),
		.count = 4,
		.interval = interval,
	);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 160 * I,
		.move = flower_swirl_move(-2 + 0.0 * I, -0.09, 120),
		.count = cnt,
		.interval = interval,
	);
}

TASK(horde_fairies_intertwined, { int count; int interval; }) {
	for(int i = 0; i < ARGS.count; ++i) {
		INVOKE_TASK(horde_fairy,
			.pos = -20 + 60*I,
			.velocity = 1,
		);

		INVOKE_TASK(horde_fairy,
			.pos = VIEWPORT_W-+0 + 60*I,
			.velocity = -1,
			.blue = 1
		);

		WAIT(ARGS.interval);
	}
}

static void welcome_swirls(void) {
	int interval = 60;
	int lr_stagger = 0;
	int ud_stagger = interval / 2;

	INVOKE_TASK_DELAYED(0, flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 200 * I,
		.move = flower_swirl_move(-3 + 0.5 * I, -0.1, 20),
		.count = 12,
		.interval = interval,
	);

	INVOKE_TASK_DELAYED(ud_stagger, flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 200 * I,
		.move = flower_swirl_move(-3 - 1 * I, 0.1, 20),
		.count = 12,
		.interval = interval,
	);

	INVOKE_TASK_DELAYED(lr_stagger, flower_swirl_spawn,
		.pos = -10 + 200 * I,
		.move = flower_swirl_move(3 + 0.5 * I, 0.1, 20),
		.count = 12,
		.interval = interval,
	);

	INVOKE_TASK_DELAYED(lr_stagger + ud_stagger, flower_swirl_spawn,
		.pos = -10 + 200 * I,
		.move = flower_swirl_move(3 - 1 * I, -0.1, 20),
		.count = 12,
		.interval = interval,
	);
}

TASK(set_swing, { float v; }) {
	Stage3DrawData *dd = stage3_get_draw_data();
	dd->target_swing_strength = ARGS.v;
}

DEFINE_EXTERN_TASK(stage3_timeline) {
	stage_start_bgm("stage3");
	stage_set_voltage_thresholds(50, 125, 300, 600);

	welcome_swirls();

	INVOKE_TASK_DELAYED(400, swarm_trail_fairy_spawn, 5);

	STAGE_BOOKMARK_DELAYED(800, horde);
	INVOKE_TASK_DELAYED(800, horde_fairy_spawn,
		.count = difficulty_value(20, 20, 30, 30),
		.interval = 20,
		.velocity = 2*I,
	);

	INVOKE_TASK_DELAYED(880, laserball_fairy,
		.pos = 3*VIEWPORT_W/4 + 200*I,
		.freq_factor = 1,
	);

	STAGE_BOOKMARK_DELAYED(1300, circle-twist);
	INVOKE_TASK_DELAYED(1300, circle_twist_fairy,
		.pos = 0,
		.target_pos = VIEWPORT_W/2.0 + I*VIEWPORT_H/3.0,
	);

	if(global.diff > D_Normal) {
		INVOKE_TASK_DELAYED(1700, laserball_fairy,
			.pos = VIEWPORT_W/3 + 140*I,
			.freq_factor = 1,
		);

		INVOKE_TASK_DELAYED(1700, laserball_fairy,
			.pos = 2*VIEWPORT_W/3 + 140*I,
			.freq_factor = -1,
		);
	} else {
		INVOKE_TASK_DELAYED(1800, swarm_trail_fairy,           -20 + 140*I, move_linear( 9));
		INVOKE_TASK_DELAYED(1860, swarm_trail_fairy, VIEWPORT_W+20 + 110*I, move_linear(-9));
	}

	INVOKE_TASK_DELAYED(1900, flower_swirls_alternating);
	INVOKE_TASK_DELAYED(2390, swarm_trail_fairy, VIEWPORT_W+20 + VIEWPORT_H*0.24*I, move_linear(-9));
	INVOKE_TASK_DELAYED(2450, swarm_trail_fairy,           -20 + VIEWPORT_H*0.32*I, move_linear( 9));

	STAGE_BOOKMARK_DELAYED(2500, pre-midboss);

	INVOKE_TASK_DELAYED(2700, spawn_midboss);

	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);
	STAGE_BOOKMARK(post-midboss);

	INVOKE_TASK(bulletring_fairy_spawn,
		.count = 10,
	);

	INVOKE_TASK_DELAYED(60, set_swing, 1.0f);

	INVOKE_TASK_DELAYED(300, horde_fairy_spawn,
		.count = 20,
		.interval = 30,
		.velocity = I,
	);

	if(global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(900, common_call_func, welcome_swirls);
	}

	INVOKE_TASK_DELAYED(1200, circle_twist_fairy,
		.pos = VIEWPORT_W/2.0 - 40*I,
		.target_pos = VIEWPORT_W/2.0 + I*VIEWPORT_H/3.0,
	);

	INVOKE_TASK_DELAYED(1600, laserball_fairy,
		.pos = VIEWPORT_W/3 + 140*I,
		.freq_factor = -1,
	);

	INVOKE_TASK_DELAYED(1600, laserball_fairy,
		.pos = 2*VIEWPORT_W/3 + 140*I,
		.freq_factor = 1,
	);

	INVOKE_TASK_DELAYED(2080, horde_fairies_intertwined,
		.count = 10,
		.interval = 60,
	);

	INVOKE_TASK_DELAYED(2240, laserball_fairy,
		.pos = VIEWPORT_W/4 + 240*I,
		.freq_factor = -1,
	);

	INVOKE_TASK_DELAYED(2240, laserball_fairy,
		.pos = 3*VIEWPORT_W/4 + 240*I,
		.freq_factor = 1,
	);

	INVOKE_TASK_DELAYED(2880, swarm_trail_fairy,
		.pos = -40 + (VIEWPORT_H/2 + 20) * I,
		.move = {
			.velocity = 10,
			.retention = 1/cdir(M_TAU/256),
		},
	);

	INVOKE_TASK_DELAYED(2980, swarm_trail_fairy,
		.pos = VIEWPORT_W+40 + (VIEWPORT_H/2 + 20) * I,
		.move = {
			.velocity = -10,
			.retention = cdir(M_TAU/256),
		},
	);

	STAGE_BOOKMARK_DELAYED(2950, pre-boss);
	INVOKE_TASK_DELAYED(3200, set_swing, 0.2f);
	WAIT(3450);

	stage_unlock_bgm("stage3boss");

	INVOKE_TASK(spawn_boss);

	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(240);
	stage3_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
