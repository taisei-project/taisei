/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "timeline.h"   // IWYU pragma: keep
#include "background_anim.h"
#include "corruption.h"
#include "nonspells/nonspells.h"
#include "spells/spells.h"
#include "stagex.h"

#include "stages/common_imports.h"

#define BEATS 86

#define FAIRY_ENTER_DELAY 400

static void stagex_fairy_enter(FairyHandle fairy, StageXCorruption *corruption) {
	ecls_fairy_3d_move_in(fairy, &stage_3d_context.cam,
		(vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, FAIRY_ENTER_DELAY);
	stagex_corrupt_enemy(corruption, NOT_NULL(ENT_UNBOX(fairy.entity)));
}

static void stagex_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(StageExPostBossDialog, pm->dialog->StageExPostBoss);
}

TASK(glider_bullet, {
	cmplx pos; real dir; real spacing; int interval;
}) {
	const int nproj = 5;
	const int nstep = 4;
	BoxedProjectile projs[nproj];

	const cmplx positions[][5] = {
		{1+I, -1, 1, -I, 1-I},
		{2, I, 1, -I, 1-I},
		{2, 1+I, 2-I, -I, 1-I},
		{2, 0, 2-I, 1-I, 1-2*I},
	};

	cmplx trans = cdir(ARGS.dir+1*M_PI/4)*ARGS.spacing;

	for(int i = 0; i < nproj; i++) {
		projs[i] = ENT_BOX(PROJECTILE(
			.pos = ARGS.pos+positions[0][i]*trans,
			.proto = pp_ball,
			.color = RGBA(0,0,1,1),
		));
	}

	for(int step = 0;; step++) {
		int cur_step = step%nstep;
		int next_step = (step+1)%nstep;

		int dead_count = 0;
		for(int i = 0; i < nproj; i++) {
			Projectile *p = ENT_UNBOX(projs[i]);
			if(p == NULL) {
				dead_count++;
			} else {
				p->move.retention = 1;
				p->move.velocity = -(positions[cur_step][i]-(1-I)*(cur_step==3)-positions[next_step][i])*trans/ARGS.interval;
			}
		}
		if(dead_count == nproj) {
			return;
		}
		WAIT(ARGS.interval);
	}
}

TASK(glider_fairy, {
	StageXCorruption *corruption;
	cmplx pos;
}) {
	auto fairy = ecls_spawn_fairy_red(ARGS.pos, ITEMS(
		.power = 3,
		.points = 5,
	));
	auto e = TASK_BIND(fairy.entity);
	stagex_fairy_enter(fairy, ARGS.corruption);

	YIELD;

	for(int i = 0; i < 3; i++) {
		real aim = carg(global.plr.pos - e->pos);
		INVOKE_TASK(glider_bullet, e->pos, aim-0.7, 20, 6);
		INVOKE_TASK(glider_bullet, e->pos, aim, 25, 3);
		INVOKE_TASK(glider_bullet, e->pos, aim+0.7, 20, 6);
		play_sfx("shot_special1");
		// WAIT(80-20*i);
		WAIT(3*BEATS/2);
	}

	e->move = move_accelerated(0, -0.02*I);
}

TASK(aimgrind_fairy, {
	StageXCorruption *corruption;
	cmplx pos;
}) {
	auto fairy = ecls_spawn_big_fairy(ARGS.pos, ITEMS(.points = 3, .power = 1));
	auto e = TASK_BIND(fairy.entity);
	stagex_fairy_enter(fairy, ARGS.corruption);

	cmplx v = CMPLX(1-2*(re(ARGS.pos)<VIEWPORT_W/2), 1);

	for(int i = 0; i < 30; i++) {
		for(int k = 0; k < 2; k++) {
			cmplx d = global.plr.pos-e->pos;
			cmplx aim = cnormalize(d);
			real r = cabs(d)*(0.6-0.2*k);
			real v0 = 6;
			real phi = acos(1-0.5*v0*v0/r/r);
			for(int j = -1; j <= 1; j+=2) {
				PROJECTILE(
					.pos = e->pos,
					.proto = pp_bullet,
					.color = RGBA(0.1,0.4,1,1),
					.move = {-v0*j*aim*I,0,cdir(phi*j)},
					.timeout = M_PI/phi,
				);
			}
			PROJECTILE(
				.pos = e->pos,
				.proto = pp_ball,
				.color = RGBA(0.8,0.1,0.5,1),
				.move = move_asymptotic(0, aim*5*cdir(0.3*sin(i)),0.1),
			);
		}
		WAIT(10);

	}
	e->move = move_linear(v);

}

TASK(rocket_proj, { cmplx pos; cmplx dir; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.pos = ARGS.pos,
		.proto = pp_bigball,
		.color = RGBA(0,0.2,1,0.0),
		.move = move_accelerated(0,0.2*ARGS.dir)
	));
	real phase = rng_angle();
	real period = rng_range(0.2,0.5);
	for(int i = 0; ; i++) {
		PROJECTILE(
			.pos = p->pos,
			.proto = pp_bullet,
			.color = RGBA(0.1,0.2,0.9,1.0),
			.move = move_accelerated(0,0.01*cdir(period*i+phase)),
		);
		YIELD;
	}
}

TASK(rocket_fairy, { cmplx pos; }) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, NULL));

	e->move = move_linear(0.5*I);

	cmplx aim = rng_dir();
	int rockets = 3;
	for(int i = 0; i < rockets; i++) {
		INVOKE_TASK(rocket_proj, e->pos, aim*cdir(M_TAU/rockets*i));
		WAIT(10);
	}
}

TASK(ngoner_proj, { cmplx pos; cmplx target; int stop_time; int laser_time; cmplx laser_vel; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.pos = ARGS.pos,
		.proto = pp_bullet,
		.color = RGBA(0,0.2,1,0.0),
		.move = move_linear(ARGS.target/ARGS.stop_time)
	));
	WAIT(ARGS.stop_time);
	p->move = move_dampen(p->move.velocity, 0.5);

	WAIT(ARGS.laser_time-ARGS.stop_time);
	p->move = move_linear(ARGS.laser_vel);

	PROJECTILE(
		.pos = p->pos,
		.proto = pp_ball,
		.color = RGBA(0.1,0.9,1,0.0),
		.move = move_linear(2*cnormalize(ARGS.target)),
	);
}

TASK(ngoner_laser, { cmplx pos; cmplx dir; }) {
	create_laser(ARGS.pos, 60, 360, RGBA(0.1,0.9,1.0,0.1), laser_rule_linear(ARGS.dir));
	create_laser(ARGS.pos, 60, 360, RGBA(0.1,0.9,1.0,0.1), laser_rule_linear(ARGS.dir));
}

TASK(ngoner_fairy, { cmplx pos; }) {
	Enemy *e = TASK_BIND(espawn_fairy_red(ARGS.pos, NULL));

	cmplx rot = rng_dir();

	int corners = 6;
	int projs_per_site = 11;
	int assembly_time = 10;
	real site_length = 70;

	real b = site_length / 2 * tan(M_PI* (0.5 - 1.0 / corners));

	int laser_time = corners*projs_per_site + assembly_time + 10;

	for(int i = 0; i < corners; i++) {
		cmplx offset = rot*b*cdir(M_TAU/corners*i);
		INVOKE_TASK_DELAYED(laser_time, ngoner_laser, e->pos + offset, 5*I*cnormalize(offset));
	}

	for(int s = 0; s < projs_per_site; s++) {
		for(int i = 0; i < corners; i++) {
			real phase = M_TAU/corners*(s/(real)projs_per_site-0.5);
			real radius = b/cos(phase);
			cmplx target = rot*radius*cdir(phase+M_TAU/corners*i);

			int laser_delay = laser_time + 2*abs(s-projs_per_site/2) - i - s*corners;
			cmplx laser_vel = rot*3.5*cdir(M_TAU/corners*i+0.06*(s-projs_per_site/2.0));


			INVOKE_TASK(ngoner_proj, e->pos, target, assembly_time, laser_delay, laser_vel);
			YIELD;
		}
	}
	e->move = move_linear(I);
}

static Boss *stagex_spawn_scuttle(cmplx pos0) {
	Boss *scuttle = create_boss("Scutƫle", "scuttle", pos0);
	boss_set_portrait(scuttle, "scuttle", NULL, "normal");
	scuttle->shadowcolor = *RGBA(0.5, 0.0, 0.22, 1);
	scuttle->glowcolor = *RGBA(0.30, 0.0, 0.12, 0);

	return scuttle;
}

TASK(scuttle_appear, { cmplx pos; }) {
	STAGE_BOOKMARK(midboss);
	Boss *boss = global.boss = TASK_BIND(stagex_spawn_scuttle(ARGS.pos));

	boss_add_attack_from_info(boss, &stagex_spells.midboss.stack_smashing, false);
	boss_add_attack_from_info(boss, &stagex_spells.midboss.fork_bomb, false);
	boss_engage(global.boss);
}

TASK(scuttleproj_appear) {
	STAGE_BOOKMARK(scuttleproj);

	Projectile *p = TASK_BIND(PROJECTILE(
		.pos = VIEWPORT_W/2,
		.proto = pp_soul,
		.color = RGBA(0,0.2,1,0),
		.move = move_towards(0, global.plr.pos, 0.005),
		.flags = PFLAG_NOCLEAR | PFLAG_NOCOLLISION | PFLAG_NOAUTOREMOVE,
	));

	WAIT(20);

	int num_spots = 32;
	for(int i = 0; i < BEATS * 3; i++) {
		int spot = rng_range(0,num_spots);
		cmplx offset = cdir(M_TAU/num_spots*spot);
		real clr = rng_range(0,1);

		cmplx vel = 2*rng_dir();
		PROJECTILE(
			.pos = p->pos + 50*offset,
			.proto = pp_bullet,
			.color = RGBA(clr,0.2,1,0),
			.flags = PFLAG_MANUALANGLE,
			.angle = carg(offset),
			.move = move_linear(vel),
			.timeout = rng_range(20,60),
		);
		YIELD;
		p->move.attraction_point = global.plr.pos;

		if(i % 5 == 0) {
			if(rng_chance(0.2)) {
				p->sprite = res_sprite("proj/bigball");
				p->pos += 30*rng_dir();
			} else {
				p->sprite = res_sprite("proj/soul");
			}
		}
	}
	kill_projectile(p);

	INVOKE_TASK(scuttle_appear, p->pos);
}

TASK(yumemi_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 180*I, 0.015);
}

TASK(spawn_boss) {
	STAGE_BOOKMARK(boss);

	Boss *boss = global.boss = stagex_spawn_yumemi(5*VIEWPORT_W/4 - 200*I);
	PlayerMode *pm = global.plr.mode;

	Attack *opening_attack = boss_add_attack(boss, AT_Normal, "Opening", 60, 40000, NULL);
	StageExPreBossDialogEvents *e;

	INVOKE_TASK_INDIRECT(StageExPreBossDialog, pm->dialog->StageExPreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, yumemi_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, stagex_boss_nonspell_1, ENT_BOX(boss), opening_attack);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stagexboss");

	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_from_info(boss, &stagex_spells.boss.sierpinski, false);
 	boss_add_attack_task(boss, AT_Normal, "non2", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.mem_copy, false);
	boss_add_attack_task(boss, AT_Normal, "non3", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_3), NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.infinity_network, false);
	boss_add_attack_task(boss, AT_Normal, "non4", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_4), NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.pipe_dream, false);
	boss_add_attack_task(boss, AT_Normal, "non5", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_5), NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.alignment, false);
	boss_add_attack_task(boss, AT_Normal, "non6", 60, 40000, TASK_INDIRECT(BossAttack, stagex_boss_nonspell_6), NULL);
	boss_add_attack_from_info(boss, &stagex_spells.boss.rings, false);

	boss_engage(boss);
}

#if 0
DEFINE_EXTERN_TASK(stagex_timeline) {
	for(int i = 0; i < 20; i++) {
		real rx = rng_range(-1,1)*100;
		real ry = rng_range(-1,1)*50;
		INVOKE_TASK_DELAYED(400+i*50, rocket_fairy, CMPLX(VIEWPORT_W*0.5+rx, VIEWPORT_H*0.3+ry));
	}
	for(int i = 0; i < 20; i++) {
		real rx = rng_range(-1,1)*100;
		real ry = rng_range(-1,1)*50;
		INVOKE_TASK_DELAYED(1000+i*100, aimgrind_fairy, CMPLX(VIEWPORT_W*0.5+rx, VIEWPORT_H*0.3+ry));
	}
	for(int i = 0; i < 20; i++) {
		real rx = rng_range(-1,1)*100;
		real ry = rng_range(-1,1)*50;
		INVOKE_TASK_DELAYED(1500+i*70, ngoner_fairy, CMPLX(VIEWPORT_W*0.5+rx, VIEWPORT_H*0.3+ry));
	}
	for(int i = 0; i < 4;i++) {
		INVOKE_TASK_DELAYED(2000+i*100, glider_fairy, CMPLX(VIEWPORT_W*(i&1), VIEWPORT_H*0.5), 3*I);
		WAIT(140);
	}

	INVOKE_TASK_DELAYED(2500, scuttleproj_appear);
	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(1000);

	stagex_get_draw_data()->tower_global_dissolution = 1;
	INVOKE_TASK(spawn_boss);
	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	stage_unlock_bgm("stagexboss");

	WAIT(240);
	stagex_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
#endif

TASK(rotate_velocity, {
	MoveParams *move;
	real angle;
	int duration;
}) {
	cmplx r = cdir(ARGS.angle / ARGS.duration);
	ARGS.move->retention *= r;
	WAIT(ARGS.duration);
	ARGS.move->retention /= r;
}

static void set_turning_motion(Enemy *e, cmplx v, real turn_angle, int turn_delay, int turn_duration) {
	e->move = move_linear(v);
	INVOKE_SUBTASK_DELAYED(turn_delay, rotate_velocity,
		.move = &e->move,
		.angle = turn_angle,
		.duration = turn_duration
  	);
}

TASK(midswirl_proj, { cmplx pos; real boost; }) {
	cmplx aim = cnormalize(global.plr.pos - ARGS.pos);
	auto p = PROJECTILE(
		.pos = ARGS.pos,
		.proto = pp_plainball,
		.color = RGB(0.2, 0.2, 1),
		.move = move_asymptotic_simple(-aim * 2, 12),
		.max_viewport_dist = 100,
	);

	WAIT(30);

	// real a = M_PI/6;
	// cmplx r = I;//cdir(a * 0.5);
	// cmplx d = -aim / r;
	// for(int i = 0; i < 2; ++i) {
	// 	PROJECTILE(
	// 		.pos = p->pos,
	// 		.proto = pp_rice,
	// 		.color = RGB(1, 1, 0),
	// 		.move = move_asymptotic_simple(d, ARGS.boost),
	// 	);
	// 	d *= r * r;
	// }


	// aim = cnormalize(global.plr.pos - ARGS.pos);
	p->move = move_accelerated(p->move.velocity, aim * 0.06);
}

TASK(midswirl_burst, { cmplx pos; }) {
	cmplx aim = cnormalize(global.plr.pos - ARGS.pos);

	cmplx spread = cdir(M_PI/10);

	PROJECTILE(
		.proto = pp_bigball,
		.color = RGBA(0.1, 0.6, 0.1, 1),
		.pos = ARGS.pos,
		.move = move_asymptotic_simple(aim * 6, 4),
	);

	int cnt = 12;
	for(int i = 0; i < cnt; ++i) {
		float c = i / (cnt - 1.0f);

		cmplx v = aim * 16;

		PROJECTILE(
			.proto = pp_bullet,
			.color = RGBA(1, 0.5 * c, 0, 1),
			.pos = ARGS.pos,
			.move = move_asymptotic_halflife(v*spread, -0.1*v*spread*cdir(0.15 * rng_sreal()), 30),
			.max_viewport_dist = 256,
		);

		PROJECTILE(
			.proto = pp_bullet,
			.color = RGBA(1, 0.5 * c, 0, 1),
			.pos = ARGS.pos,
			.move = move_asymptotic_halflife(v/spread, -0.1*v/spread*cdir(0.15 * rng_sreal()), 30),
			.max_viewport_dist = 256,
		);

		WAIT(2);
	}
}

TASK(midswirl, {
	StageXCorruption *corruption;
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.points = 1, .power = 1)));
	stagex_corrupt_enemy(ARGS.corruption, e);
	set_turning_motion(e, ARGS.vel, ARGS.turn_angle, ARGS.turn_delay, ARGS.turn_duration);

	int period = BEATS/2;
	WAIT(period - (global.frames) % period);

	for(;;WAIT(period)) {
		play_sfx("shot2");
		INVOKE_TASK(midswirl_burst, e->pos);
	}
}

TASK(midswirls, {
	StageXCorruption *corruption;
	int count;
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	for(int i = 0; i < ARGS.count; ++i, WAIT(12)) {
		INVOKE_TASK(midswirl, ARGS.corruption,
			.pos = ARGS.pos,
			.vel = ARGS.vel,
			.turn_angle = ARGS.turn_angle,
			.turn_delay = ARGS.turn_delay,
			.turn_duration = ARGS.turn_duration
		);
	}
}

static int midboss_section(StageXCorruption *C) {
	int t = 0;

	stagex_bg_trigger_next_phase();
	t += WAIT(BEATS * 1.25);
	play_sfx("shot_special1");

	INVOKE_TASK(midswirls, C,
		.count = 16,
		.pos = 0 + 64*I,
		.vel = 8,
		.turn_angle = M_PI,
		.turn_delay = 20,
		.turn_duration = 30
	);

	t += WAIT(BEATS);

	INVOKE_TASK(midswirls, C,
		.count = 16,
		.pos = VIEWPORT_W + 64*I,
		.vel = -8,
		.turn_angle = -M_PI,
		.turn_delay = 20,
		.turn_duration = 30
	);

	t += WAIT(BEATS);

	INVOKE_TASK(midswirls, C,
		.count = 16,
		.pos = 0 + 128*I,
		.vel = 8,
		.turn_angle = 3*M_PI/2,
		.turn_delay = 20,
		.turn_duration = 30
	);

	INVOKE_TASK(midswirls, C,
		.count = 16,
		.pos = VIEWPORT_W + 128*I,
		.vel = -8,
		.turn_angle = -3*M_PI/2,
		.turn_delay = 20,
		.turn_duration = 30
	);

	t += WAIT(BEATS * 2);

	INVOKE_TASK(scuttleproj_appear);
	INVOKE_TASK(ngoner_fairy, 140 + 140 * I);
	t += WAIT(BEATS);
	INVOKE_TASK(ngoner_fairy, VIEWPORT_W - 140 + 140 * I);
	t += WAIT(BEATS);
	while(!global.boss) {
		++t;
		YIELD;
	}
	STAGE_BOOKMARK(midboss);
	log_debug("midboss spawn: %i", t);
	t += WAIT_EVENT_OR_DIE(&NOT_NULL(global.boss)->events.defeated).frames;
	log_debug("midboss defeat: %i", t);
	STAGE_BOOKMARK(post-midboss);

	return t;
}

TASK(laser45, { cmplx origin; cmplx dir; cmplx r; const Color *clr; int d0; int d1;}) {
	play_sfx("laser1");

	MoveParams *move;
	auto l = TASK_BIND(create_dynamic_laser(ARGS.origin, 120, (ARGS.d0+ARGS.d1) * 4, ARGS.clr, &move));
	l->width_exponent = 0.5;

	cmplx pos = ARGS.origin;
	INVOKE_SUBTASK(common_move_ext, .pos = &pos, .move_params = move);

	cmplx r = ARGS.r;
	*move = move_linear(ARGS.dir * 3);

	for(int i = 0; i < 4; i++) {
		WAIT(ARGS.d0);
		cmplx aim = cnormalize(global.plr.pos - pos);
		PROJECTILE(pp_ball, &l->color, .pos = pos, .move = move_accelerated(0, 0.01*aim));
		play_sfx("shot3");
		move->velocity *= r;
		WAIT(ARGS.d1);
		aim = cnormalize(global.plr.pos - pos);
		PROJECTILE(pp_ball, &l->color, .pos = pos, .move = move_accelerated(0, 0.01*aim));
		play_sfx("shot3");
		move->velocity *= r;
	}
}

TASK(laser45_big_fairy, { cmplx origin; }) {
	auto e = TASK_BIND(ecls_fairy_summon(ecls_spawn_huge_fairy(ARGS.origin, ITEMS(.points = 5)), 60).entity);

	for(int i = 0; i < 3; ++i) {
		RADIAL_LOOP(l, 8, I) {
			INVOKE_TASK(laser45, e->pos, l.dir, cdir(M_PI/4), RGBA(1, 0.3, 0.0, 0), .d0 = 60, .d1 = 45);
			INVOKE_TASK_DELAYED(30, laser45, e->pos, l.dir, cdir(-M_PI/4), RGBA(0.1, 0.1, 1,0), .d0 =60, .d1=15);
		}
		WAIT(400);
	}

	e->move = move_linear(-I);

	enemy_kill(e);
}

TASK(inert_swirl, { StageXCorruption *corruption; cmplx origin; cmplx dir; }) {
	auto swirl = ecls_spawn_swirl(ARGS.origin, ITEMS(.power_mini = 1, .points = 1));
	auto e = TASK_BIND(swirl.entity);
	ecls_swirl_3d_move_in(swirl, &stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, 180);
	stagex_corrupt_enemy(ARGS.corruption, e);
	e->move = move_accelerated(0, ARGS.dir * 0.02);
}

TASK(intro_swirls, { StageXCorruption *corruption; }) {
	int t = BEATS * 4;
	int interval = 8;
	cmplx r = cdir(0.3);
	cmplx dir = -I;

	real w = 0;
	real dw = 0.05;

	for(;t - interval > 0; t -= interval, w += dw) {
		real o = 10;
		cmplx p0 = o + o*I;
		cmplx p1 = p0 + VIEWPORT_W - 2*o;
		cmplx p = clerp(p0, p1, 0.5 + 0.5 * triangle(w));
		dir = cnormalize(VIEWPORT_W/2 + VIEWPORT_H*5*I - p);

		INVOKE_TASK(inert_swirl, ARGS.corruption, p, dir);
		dir *= r;
		WAIT(interval);
	}

	// WAIT(BEATS * 2);
	t = BEATS * 4;
	w = 0;

	for(;t - interval > 0; t -= interval, w += dw) {
		real o = 30;
		cmplx p0 = o + (VIEWPORT_H-o) *I;
		cmplx p1 = p0 + VIEWPORT_W - 2*o;
		cmplx p = clerp(p1, p0, 0.5 + 0.5 * triangle(w));
		dir = cnormalize(VIEWPORT_W/2 - VIEWPORT_H*4*I - p);

		INVOKE_TASK(inert_swirl, ARGS.corruption, p, dir);
		dir *= r;
		WAIT(interval);
	}
}

TASK(intro_fairy, { StageXCorruption *corruption; cmplx origin; }) {
	auto fairy = ecls_spawn_big_fairy(ARGS.origin, ITEMS(.power = 2, .points = 2));
	auto e = TASK_BIND(fairy.entity);
	// ecls_fairy_3d_move_in(fairy, &stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, BEATS);
	ecls_fairy_3d_move_in(fairy, &stage_3d_context.cam,
		(vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, FAIRY_ENTER_DELAY);
	// ecls_fairy_summon(fairy, 120);
	stagex_corrupt_enemy(ARGS.corruption, e);

	int burst = 3;

	cmplx dir1 = I;
	cmplx dir2 = -I;
	cmplx r1 = cdir(2*0.15);
	cmplx r2 = cdir(2*0.15);

	real s = 3;
	real boost = 5;

	for(int i = 0; i < 3; ++i) {
		for(int t = 0; t < burst; t++) {
			#if 0
			cmplx aim, pos;
			real ofs = 40;

			pos = e->pos + ofs * dir1;
			aim = cnormalize(global.plr.pos - e->pos);

			RADIAL_LOOP(l, 1, aim) {
				PROJECTILE(
					.proto = pp_thickrice,
					.pos = pos,
					.color = RGB(1, 0.2, 0.2),
					.move = move_asymptotic_simple(l.dir * s, boost),
				);
			}

			WAIT(2);

			pos = e->pos + ofs * dir2;
			aim = cnormalize(global.plr.pos - e->pos);

			RADIAL_LOOP(l, 1, aim) {
				PROJECTILE(
					.proto = pp_thickrice,
					.pos = pos,
					.color = RGB(0.2, 0.2, 1),
					.move = move_asymptotic_simple(l.dir * s, boost),
				);
			}

			dir1 *= r1;
			dir2 *= r2;

			play_sfx_loop("shot1_loop");
			#endif

			int cnt = 13;
			real spread = M_PI;
			cmplx aim = cnormalize(global.plr.pos - e->pos);
			cmplx dir = aim * cdir(-spread/2);
			cmplx r = cdir(spread/(cnt-1));

			for(int i = 0; i < cnt; ++i) {
				PROJECTILE(
					.proto = pp_bullet,
					.pos = e->pos - dir * 42,
					.color = RGB(1, 0.8, 0.2),
					// .move = move_asymptotic_simple(dir * 3, 10),
					.move = move_asymptotic(aim * 5, dir * (3 + 2 * fabs(0.5 - i/(cnt-1.0) )), 0.99),
					// .move = move_asymptotic(aim * 5, dir * 0.1, 0.95),
				);
				PROJECTILE(
					.proto = pp_bullet,
					.pos = e->pos + dir * 42,
					.color = RGB(1, 0.2, 0.8),
					// .move = move_asymptotic_simple(dir * 3, 10),
					.move = move_asymptotic(aim * 5, -dir * (3 + 2 * fabs(0.5 - i/(cnt-1.0) )), 0.99),
					// .move = move_asymptotic(aim * 5, dir * 0.1, 0.95),
				);
				dir *= r;
			}

			play_sfx("shot_special1");
			WAIT(12);
		}

		WAIT(BEATS*2 - 12 * burst);
	}

	e->move = move_linear(2*I);
}

TASK(square_fairy, { StageXCorruption *corruption; cmplx origin; int distort; }) {
	auto fairy = ecls_spawn_fairy_red(ARGS.origin, ITEMS(.power = 1));
	auto e = TASK_BIND(fairy.entity);

	cmplx center = 0.5*(VIEWPORT_W+I*VIEWPORT_H);

	e->move = move_linear(-0.5 * cnormalize(center-e->pos));
	e->move.retention = cdir(0.01);

	stagex_fairy_enter(fairy, ARGS.corruption);

	// e->move = move_linear(I*cnormalize(center-e->pos));
	e->move.acceleration = e->move.velocity * -I * 0.01;
	e->move.retention = 1;

	for(;;) {
		INVOKE_SUBTASK(common_charge, e->pos, *RGBA(1.0,0.3,0.0,0.5), BEATS/2, .sound = COMMON_CHARGE_SOUNDS);
		WAIT(BEATS/2);

		int num = 5;
		for(int side = 0; side < 4; side++) {
			cmplx aim = cdir(M_PI/2*side + 0.1*ARGS.distort);

			for(int n = -num/2; n <= num/2; n++) {
				cmplx dir = aim * (2+1*I*n);
				PROJECTILE(
					.proto = pp_bigball,
					.pos = e->pos,
					.color = RGBA(0,0.2,1, 0),
					.move = move_linear(dir)
				);
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos,
					.color = RGBA(1,0.2,0,0.5),
					.move = move_linear((1.2+0.1*ARGS.distort*I*sin(n))*dir)
				);
			}
		}
		play_sfx("shot_special1");
		WAIT(BEATS*4.5);
	}
}

TASK(stream_fairy_motion, { BoxedEnemy e; real move_dist; real turn_angle; }) {
	auto e = TASK_BIND(ARGS.e);

	real speed = cabs(e->move.velocity);
	real move_dist = VIEWPORT_W * 0.75;
	int move_duration = round(move_dist / speed);
	int turn_duration = 60;
	real turn_angle = ARGS.turn_angle;

	for(;;) {
		WAIT(move_duration);
		common_rotate_velocity(&e->move, turn_angle, turn_duration);
		turn_angle = -turn_angle;
	}
}

TASK(stream_bullet, { BoxedProjectile p; }) {
	auto p = TASK_BIND(ARGS.p);

	WAIT(30);
	p->color.r = p->color.g;
	spawn_projectile_highlight_effect(p);

	cmplx aim = cnormalize(global.plr.pos - p->pos + rng_dir());
	p->move = move_accelerated(p->move.velocity * 0.0, 0.04 * aim);
}

TASK(stream_fairy, { StageXCorruption *corruption; cmplx origin; cmplx dir; real move_dist; real turn_angle; }) {
	auto fairy = ecls_spawn_fairy_blue(ARGS.origin, ITEMS(.points = 2));
	auto e = TASK_BIND(fairy.entity);

	stagex_fairy_enter(fairy, ARGS.corruption);
	e->move = move_linear(ARGS.dir);

	INVOKE_SUBTASK(stream_fairy_motion, ENT_BOX(e), ARGS.move_dist, ARGS.turn_angle);

	cmplx ofs = -16*I;
	cmplx r = cdir(1);

	for(;;) {
		int cnt = 15;

		for(int i = 0; i < cnt; ++i) {
			if(cabs(global.plr.pos - e->pos) > 128) {
				cmplx aim = cnormalize(global.plr.pos - e->pos);
				auto p = PROJECTILE(
					.proto = pp_rice,
					.color = RGB(0, 0.5, 0),
					.pos = e->pos + ofs,
					.move = move_asymptotic_simple(aim * 2, 20),
				);
				INVOKE_TASK(stream_bullet, ENT_BOX(p));
				play_sfx_loop("shot1_loop");
			}

			WAIT(4);
			ofs *= r;
		}

		WAIT(120);
	}
}

TASK(stream_fairies, { StageXCorruption *corruption; }) {
	int cnt = 16;

	real dist = VIEWPORT_W * 0.75;
	real ofs = (VIEWPORT_W - dist) * 0.5;
	cmplx orig = 64*I + ofs;

	for(int i = 0; i < cnt; ++i) {
		INVOKE_TASK(stream_fairy, ARGS.corruption,
			orig, 1.5, dist, M_PI);
		WAIT(BEATS/2);
	}
}

TASK(transition_swirl, { cmplx origin; cmplx dir; int corruption; }) {
	auto swirl = ecls_spawn_swirl(ARGS.origin, ITEMS(.power = 0));
	auto e = TASK_BIND(swirl.entity);
	ecls_swirl_3d_move_in(swirl, &stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, BEATS/2);

	e->move = move_accelerated(ARGS.dir, 0.01*cdir(0.1)*ARGS.dir);

	for(int t = 0; t < 3; t++) {
		for(int side = -1; side <= 1; side+=2) {
			PROJECTILE(
				.proto = ARGS.corruption ? pp_wave : pp_bullet,
				.pos = e->pos,
				.color = RGB(1*ARGS.corruption,0.2*ARGS.corruption,1-ARGS.corruption),
				.move = move_accelerated(side*I*ARGS.dir, 0.01*ARGS.corruption*-ARGS.dir)
			);
		}
		play_sfx("shot1");

		WAIT(10);
	}
}

TASK(transition_swirls) {
	for(int t = 0; t < 1.5*BEATS; t++) {
		cmplx pos = 50*I*cos(t) + 50*sin(sin(sin(t)));
		cmplx dir = 3*cnormalize(pos);
		INVOKE_SUBTASK(transition_swirl, VIEWPORT_W/2 + I*VIEWPORT_H/2 + pos, dir, 0);
		WAIT(2);
	}
	WAIT(BEATS/4);
	for(int t = 0; t < BEATS; t++) {
		cmplx pos = 50*I*cos(t) + 50*sin(t^(342345));
		cmplx dir = -3*cnormalize(pos);
		INVOKE_SUBTASK(transition_swirl, VIEWPORT_W/2 + I*VIEWPORT_H/2 + pos, dir, 1);
		WAIT(2);
	}
	AWAIT_SUBTASKS;
}

TASK(wheat_laser_proj, { cmplx pos; cmplx dir; cmplx turn; int delay; }) {
	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_rice,
		.color = RGBA(0, 0.3, 1, 0.5),
		.pos = ARGS.pos,
		.angle = carg(ARGS.dir),
		.flags = PFLAG_MANUALANGLE,
		.max_viewport_dist = 64,
	));

	play_sfx("shot2");

	WAIT(ARGS.delay);
	play_sfx("redirect");
	p->flags &= ~PFLAG_MANUALANGLE;
	p->move = (MoveParams){
		.velocity = 3*cnormalize(ARGS.dir),
		.retention = ARGS.turn,
		.attraction_point = global.plr.pos,
		.attraction = 0.0,
		.attraction_exponent = 0.2
	};

	for(int t = 0; t < 2*BEATS; ++t, YIELD) {
		capproach_asymptotic_p(&p->move.attraction, 0.1, 0.1, 1e-5);
	}

	// WAIT(2*BEATS);

	play_sfx("redirect");
	p->color = *RGBA(1, 0.3, 0,0.5);
	spawn_projectile_highlight_effect(p);
	p->move.attraction = 0;
	// p->move.retention = 1;
	p->move.acceleration = 0.1 * cdir(p->angle);

	WAIT(120);
	p->move.acceleration *= 0.5;
	p->move.retention = 1;
}

TASK(wheat_fairy, { StageXCorruption *corruption; cmplx pos; MoveParams move; }) {
	auto fairy = ecls_spawn_big_fairy(ARGS.pos, ITEMS(.power = 2, .points = 2));
	auto e = TASK_BIND(fairy.entity);
	ecls_fairy_3d_move_in(fairy, &stage_3d_context.cam,
		(vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, FAIRY_ENTER_DELAY);
	stagex_corrupt_enemy(ARGS.corruption, e);
	e->move = ARGS.move;

	struct {
		cmplx dir;
		cmplx turn;
	} leaf_params[2] = {
		{ cdir( 1), 0.9 * cdir(-0.1) },
		{ cdir(-1), 0.9 * cdir( 0.1) },
	};

	for(int t = 0; t < 2; t++) {
		INVOKE_SUBTASK(common_charge, .anchor = &e->pos, .color = *RGBA(1.0,0.1,0.0,0.5), BEATS/2, .sound = COMMON_CHARGE_SOUNDS);
		WAIT(BEATS/2);
		int points = 8;
		real length = 100;

		MoveParams move = e->move;
		e->move.retention = 0.9;

		RADIAL_LOOP(l, points, I) {
			int count = 10;
			int interval = BEATS/count;
			for(int j = 0; j < count; j++) {
				for(int d = 0; d < 2; d++) {
					cmplx v = l.dir * leaf_params[d].dir;
					cmplx pos = e->pos + l.dir * length/count * j + 5 * v;
					INVOKE_TASK_DELAYED(interval*j, wheat_laser_proj,
						pos, v, leaf_params[d].turn, (count - j) * interval + 4 * j);
				}
			}
		}

		WAIT(BEATS);
		e->move = move;
		WAIT(2*BEATS);
	}
}

TASK(amaranth_proj, { cmplx pos; MoveParams move; }) {
	auto p = TASK_BIND(PROJECTILE(pp_bigball, .color = RGBA(1.0,0.3,0,1), .pos = ARGS.pos, .move = ARGS.move));

	for(;;) {
		int count = 1;
		real radius = 10;
		for(int i = 0; i < count; i++) {
			real x, y;
			do {
				x = rng_f64s();
				y = rng_f64s();
			} while(x*x + y*y > 1);

			cmplx pos = p->pos + radius*(x + I*y);
			cmplx dir = cpow(cnormalize(x + I*y), 3);
			PROJECTILE(
				.proto = pp_flea,
				.color = RGBA(1.0,0.3,0,0),
				.pos = pos,
				.move = (MoveParams){.velocity = 0.02*dir, .retention = 1.01}
				// .move = move_asymptotic_halflife(0.01*dir, 3*dir, 160),
			);
		}
		play_sfx_loop("shot1_loop");
		YIELD;
	}
}

TASK(amaranth_fairy, { StageXCorruption *corruption; cmplx pos; MoveParams move; }) {
	auto fairy = ecls_spawn_big_fairy(ARGS.pos, ITEMS(.points = 10));
	auto e = TASK_BIND(fairy.entity);
	stagex_fairy_enter(fairy, ARGS.corruption);

	// INVOKE_SUBTASK_DELAYED(BEATS/2, common_charge, 0, *RGBA(0.0,0.0,1.0,0.0), BEATS/2, .anchor = &e->pos, .sound = COMMON_CHARGE_SOUNDS);
	//
	e->move = ARGS.move;
	common_charge(120, &e->pos, 0, *RGBA(0.0, 0.0, 1.0, 0.0));


	WAIT(5);
	for(int t = 0; t < 3; t++) {
		cmplx aim = cnormalize(global.plr.pos-e->pos);
		INVOKE_TASK(amaranth_proj, e->pos, move_linear(4*aim));
		play_sfx("shot_special1");
		WAIT(BEATS);
	}
}

TASK(octahedron_proj, { vec3 *vertices; int *path; cmplx *shift; real offset; }) {
	auto p = TASK_BIND(PROJECTILE(.proto = pp_flea, .color = RGBA(1,0.3,0,0)));

	real speed = 0.01;
	for(int t=0;;t += WAIT(1)) {
		real f = t*speed + ARGS.offset;
		int idx = f;
		f -= idx;
		idx %= 12;
		vec3 tmp = {};
		glm_vec3_lerp(ARGS.vertices[ARGS.path[idx]], ARGS.vertices[ARGS.path[idx+1]], f, tmp);
		p->pos = *ARGS.shift + tmp[0] + I * tmp[1];
	}
}

TASK(octahedron, { cmplx pos; MoveParams move; vec3 axis; real final_size; real size_timescale; }) {

	// this is float, can/should we make it double?
	vec3 vertices[] = {
		{1,0,0},
		{-1,0,0},
		{0,1,0},
		{0,-1,0},
		{0,0,1},
		{0,0,-1}
	};

	// for(int i = 0; i < ARRAY_SIZE(vertices); i++) {
	// 	glm_vec3_scale(vertices[i], 100, vertices[i]);
	// }

	cmplx shift = ARGS.pos;
	float scale = 1;
	INVOKE_SUBTASK(common_easing_animate, &scale, ARGS.final_size, ARGS.size_timescale, glm_ease_quad_out);


	int path[] = {
		0,2,4,0,3,4,1,3,5,1,2,5,0
	};

	int count = 80;

	for(int i = 0; i < count; i++) {
		INVOKE_SUBTASK(octahedron_proj, vertices, path, &shift, i*12.0/count);
	}
	for(;;) {
		for(int i = 0; i < ARRAY_SIZE(vertices); i++) {
			glm_vec3_rotate(vertices[i], 0.01, ARGS.axis);
			glm_vec3_scale_as(vertices[i], scale, vertices[i]);
		}

		move_update(&shift, &ARGS.move);

		WAIT(1);
	}
}

TASK(octahedron_fairy, { StageXCorruption *corruption; cmplx origin; }) {
	auto fairy = ecls_spawn_super_fairy(ARGS.origin, ITEMS(.points = 30, .power = 10, .life = 1));
	auto e = TASK_BIND(fairy.entity);
	stagex_fairy_enter(fairy, ARGS.corruption);

	common_charge(120, &e->pos, 0, *RGB(2.0, 1.0, 0.0));
	stage_clear_hazards(CLEAR_HAZARDS_ALL);

	for(int t = 0; t < 600; t += WAIT(BEATS/8)) {
		cmplx aim = cdir(t*0.1);
		vec3 axis = { im(aim), re(aim), 1 };
		INVOKE_TASK(octahedron, .pos = e->pos, .move = move_linear(2*aim), .final_size = 100, .size_timescale = 3*BEATS, .axis={axis[0], axis[1],axis[2]});
		play_sfx("shot3");
	}

	e->move = move_linear(-I*0.5);
}

TASK(assist_laser, { cmplx pos; cmplx accel; }) {

	PROJECTILE(pp_ball, .max_viewport_dist=100, .pos = ARGS.pos, .color = RGBA(0.0,0.0,1.0,0.0), .move = move_accelerated(0,ARGS.accel));
	create_laser(ARGS.pos, 10, 10000, RGBA(0.0,0.0,1.0,0.0), laser_rule_accelerated(0, ARGS.accel));
	WAIT(10);
	PROJECTILE(pp_ball, .max_viewport_dist=100, .pos = ARGS.pos, .color = RGBA(0.0,0.0,1.0,0.0), .move = move_accelerated(0,ARGS.accel));
}

TASK(assist_fairy, { StageXCorruption *corruption; cmplx origin; MoveParams move; }) {
	auto fairy = ecls_spawn_fairy_blue(ARGS.origin, ITEMS(.points = 1));
	auto e = TASK_BIND(fairy.entity);
	ecls_fairy_3d_move_in(fairy,
		&stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, FAIRY_ENTER_DELAY);
	stagex_corrupt_enemy(ARGS.corruption, e);

	e->move = ARGS.move;
	WAIT(5);

	for(;;) {
		play_sfx("laser1");

		RADIAL_LOOP(l, 10, cnormalize(global.plr.pos-e->pos)) { // * cdir(2*M_PI/4)
			INVOKE_TASK(assist_laser, e->pos, 0.07 * l.dir);
			// WAIT(1);
		}

		WAIT(2*BEATS);
	}
}

TASK(transition_swirl2, { cmplx origin; cmplx dir; }) {
	auto swirl = ecls_spawn_swirl(ARGS.origin, ITEMS(.power = 0));
	auto e = TASK_BIND(swirl.entity);
	ecls_swirl_3d_move_in(swirl,
		&stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, 2.5 * BEATS);
	e->move = move_linear(ARGS.dir);

	WAIT(5);
	for(int i = 0; i < 3; i++) {
		cmplx aim = cnormalize(global.plr.pos - e->pos) * cdir(M_TAU/3*i);
		PROJECTILE(pp_wave, .color = RGBA(0,0.2,1.0,1), .pos = e->pos, .move = move_accelerated(3*aim, 0.01*aim));
	}
	play_sfx_loop("shot1_loop");
}

TASK(transition_swirls2) {
	int sections = 5;
	for(int sec = 0; sec < sections; sec++) {
		for(int t = 0; t < BEATS/2; t++) {
			cmplx pos = 50*(1-2*(t&1))*cdir(M_TAU/sections * sec + t);
			cmplx dir = 6*cnormalize(pos);
			INVOKE_SUBTASK(transition_swirl2, VIEWPORT_W/2 + 200*I + pos, dir);
			WAIT(1);
		}
	}
	AWAIT_SUBTASKS;
}

TASK(scissor_fairy, { StageXCorruption *corruption; cmplx origin; MoveParams move; int dir; }) {
	auto fairy = ecls_spawn_huge_fairy(ARGS.origin, ITEMS(.power = 1));
	auto e = TASK_BIND(fairy.entity);
	stagex_fairy_enter(fairy, ARGS.corruption);
	// ecls_fairy_3d_move_in(fairy, &stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, BEATS/2);

	// INVOKE_SUBTASK(common_charge, 0, *RGBA(0.0,0.0,1.0,0.0), BEATS/2, .anchor = &e->pos, .sound = COMMON_CHARGE_SOUNDS);

	real scissor = 0;
	for(int t = 0; t < BEATS; t++) {
		real spread = -0.5+0.04*t;
		scissor += 0.003;
		cmplx aim = cnormalize(global.plr.pos - e->pos);

		RADIAL_LOOP(l, 2, aim) {
			PROJECTILE(
				.proto = pp_ball,
				.color = RGBA(1, 0.3, 0, 0.5),
				.pos = e->pos,
				.move = move_accelerated(4*l.dir*cdir(spread*ARGS.dir), scissor * I * l.dir * ARGS.dir)
			);
		}

		play_sfx_loop("shot1_loop");
		WAIT(3);
	}
	e->move = ARGS.move;
}


TASK(funk_small_bullet, { cmplx *pos; versor *rot; real f; }) {
	auto p = TASK_BIND(PROJECTILE(.proto = pp_rice, .color = RGBA(1.0,0.5,0.0,0.0)));

	real radius  = 30;
	for(int t = 0; t < 2*BEATS;t++) {
		real speed = 0.001;
		real phi = speed * t + M_TAU*ARGS.f;
		vec3 pos = {cos(phi), sin(phi), 0};
		vec3 vel = {-sin(phi), cos(phi), 0};
		vec3 rotated;
		glm_quat_rotatev(*ARGS.rot, pos, rotated);
		p->pos = *ARGS.pos + radius * (rotated[0] + I * rotated[1]);
		glm_quat_rotatev(*ARGS.rot, vel, rotated);
		p->move = move_linear(4*(rotated[0] + I * rotated[1]));
		WAIT(1);
	}
	play_sfx("noise1");
}

TASK(funk_bullet, { cmplx pos; MoveParams move; }) {
	auto p = TASK_BIND(PROJECTILE(.proto = pp_soul, .color = RGBA(0.1,0.1,0.8,0.1), .pos = ARGS.pos, .move = ARGS.move, .flags = PFLAG_MANUALANGLE));

	vec3 axis;
	cmplx dir1 = rng_dir();
	cmplx dir2 = rng_dir();
	axis[0] = re(dir1)*re(dir2);
	axis[1] = im(dir1)*re(dir2);
	axis[2] = im(dir2);

	versor rot1, rot2;
	glm_quat_identity(rot1);
	glm_quatv(rot2,rng_angle(), axis);

	int count = 20;
	for(int i = 0; i < count; i++) {
		INVOKE_SUBTASK(funk_small_bullet, &p->pos, &rot1, i/(real)count);
		INVOKE_SUBTASK(funk_small_bullet, &p->pos, &rot2, i/(real)count);
	}

	for(;;) {
		cmplx dir1 = rng_dir();
		cmplx dir2 = rng_dir();
		versor q, tmp;
		p->angle = rng_angle();
		glm_quat(q, 0.12, re(dir1)*re(dir2), im(dir1)*re(dir2), im(dir2));
		glm_quat_mul(rot1, q, tmp);
		glm_quat_copy(tmp, rot1);
		q[2] = rng_f64s();
		glm_quat_mul(rot2, q, tmp);
		glm_quat_copy(tmp, rot2);

		WAIT(1);
	}
}

TASK(funk_fairy, { cmplx pos; MoveParams move; }) {
	auto fairy = ecls_spawn_big_fairy(ARGS.pos, ITEMS(.power = 2, .points = 2));
	auto e = TASK_BIND(fairy.entity);
	ecls_fairy_3d_move_in(fairy, &stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, BEATS);

	INVOKE_SUBTASK(common_charge, 0, *RGBA(0.0,0.0,1.0,0.0), BEATS, .anchor = &e->pos, .sound = COMMON_CHARGE_SOUNDS);
	WAIT(BEATS);
	int count = 4;

	for(int i = 0; i < count; i++) {
		cmplx aim = 2*cnormalize(global.plr.pos-e->pos)*cdir(0.5*(i-count/2.0));
		INVOKE_TASK(funk_bullet, e->pos, move_asymptotic_simple(aim, 4));

		play_sfx("shot_special1");
		WAIT(BEATS/2);
	}
	e->move = ARGS.move;

}

TASK(funk_fairies) {
	cmplx corners[] = {
		50+50*I,
		VIEWPORT_W-50+50*I,
		VIEWPORT_W-50+I*VIEWPORT_H-50*I,
		50 + I*VIEWPORT_H-50*I,
	};
	for(int i = 0; i < 6; i++) {
		INVOKE_SUBTASK_DELAYED( i*BEATS, funk_fairy, .pos = corners[i%4], . move = move_linear(cnormalize((VIEWPORT_W + I*VIEWPORT_H)/2 - corners[i%4])));
	}
	AWAIT_SUBTASKS;
}

TASK(drum_fairy, { cmplx pos; }) {
	auto fairy = ecls_spawn_huge_fairy(ARGS.pos, ITEMS(.points = 3));
	auto e = TASK_BIND(fairy.entity);
	ecls_fairy_3d_move_in(fairy, &stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, BEATS);

	INVOKE_SUBTASK(common_charge, e->pos, *RGBA(1.0,1.0,0.0,0.5), BEATS/2, .sound = COMMON_CHARGE_SOUNDS);
	WAIT(BEATS/2);

	for(int t = 0; t < 20; t++) {
		int count = 40;

		for(int i = 0; i < count; i++) {
			cmplx aim = cdir(M_TAU/count * i + 0*t);

			PROJECTILE(pp_rice, .color = RGBA(1.0, 0.3, 0.05, 1.0), .pos = e->pos + 20*aim, .move = move_accelerated(0.5*aim, 0.02*aim));
		}
		play_sfx("shot2");
		WAIT(BEATS/8);
	}
	e->move = move_linear(2*I);
}

typedef struct LaserRuleStaircaseData {
	cmplx tread;
	cmplx riser;
	real velocity;
} LaserRuleStaircaseData;

static cmplx staircase_laser_rule_impl(Laser *l, real t, void *ruledata) {
	LaserRuleStaircaseData *rd = ruledata;

	real ltread = cabs(rd->tread);
	real lriser = cabs(rd->riser);

	real f = rd->velocity * t;
	int num_steps = f/(ltread + lriser);
	f -= num_steps * (ltread + lriser);
	cmplx last = cnormalize(rd->tread) * f;
	if(f > ltread) {
		last = rd->tread + cnormalize(rd->riser) * (f - ltread);
	}
	return l->pos +  num_steps * (rd->tread + rd->riser) + last;
}

static LaserRule staircase_laser_rule(cmplx tread, cmplx riser, real velocity) {
	LaserRuleStaircaseData rd = {
		tread,
		riser,
		velocity,
	};
	return MAKE_LASER_RULE(staircase_laser_rule_impl, rd);
}

TASK(staircase_swirl, { cmplx pos; cmplx dir; bool cross;}) {
	auto swirl = ecls_spawn_swirl(ARGS.pos, ITEMS(.points = 1));
	auto e = TASK_BIND(swirl.entity);
	ecls_swirl_3d_move_in(swirl, &stage_3d_context.cam, (vec3) { 0, 0, stage_3d_context.cam.pos[2] - 150 }, BEATS);

	play_sfx("laser1");
	if(!ARGS.cross) {
		create_laser(e->pos, 80, 500, RGBA(0.1,0.3,1,0), staircase_laser_rule(-20, 20*I, 4));
	} else {
		create_laser(e->pos, 80, 500, RGBA(0.1,0.3,1,0), laser_rule_linear(4*ARGS.dir));

	}
	e->move = move_linear(2*I*ARGS.dir);
}

TASK(staircase_swirls, { bool cross; }) {
	real offset = 10;
	real total_length = VIEWPORT_W + VIEWPORT_H - 4*offset;

	WAIT(BEATS);
	int count = total_length/40;
	for(int i = 0; i < count; i++) {
		real l = total_length/count*i;
		cmplx dir;
		cmplx pos;
		if(l < VIEWPORT_W-2*offset) {
			pos = offset + l + offset*I;
			dir = I;
		} else {
			pos = VIEWPORT_W-offset + (offset + l-(VIEWPORT_W-offset*2))*I;
			dir = -1;
		}

		if(ARGS.cross) {
			pos = VIEWPORT_W + I*VIEWPORT_H - pos;
			dir *= -1;
		}

		INVOKE_SUBTASK(staircase_swirl, pos, dir, .cross = ARGS.cross);
	}
	AWAIT_SUBTASKS;
}

TASK(aimed_laser45, { cmplx origin; cmplx dir; int delay; int warpid; const Color *clr;}) {

	MoveParams *move;

	real offset = 9;
	real speed = 3;

	cmplx pos = ARGS.origin + offset * ARGS.warpid * I * ARGS.dir;

	auto l = TASK_BIND(create_dynamic_laser(pos, 120, (ARGS.delay) * 8, ARGS.clr, &move));
	l->width_exponent = 0.5;

	*move = move_linear(ARGS.dir * speed);
	INVOKE_SUBTASK(common_move_ext, .pos = &pos, .move_params = move);

	for(int i = 0; i < 4; i++) {
		WAIT(ARGS.delay/(1+(i&1)));
		cmplx dir = cnormalize(move->velocity);
		cmplx midpos = pos - offset * ARGS.warpid * I * dir;
		cmplx r = 1;
		real min_dist = INFINITY;
		for(int d = -1; d <= 1; d++) {
			cmplx normal = I*dir*cdir(d*M_PI/4);

			real distance_from_line = fabs(creal(conj(normal)*(global.plr.pos-midpos)));

			if(distance_from_line < min_dist) {
				min_dist = distance_from_line;
				r = cdir(d*M_PI/4);
			}
		}

		if(re(midpos) < 0 || re(midpos) > VIEWPORT_W || im(midpos) < 0 || im(midpos) > VIEWPORT_H) {
			break;
		}
		int waittime = (-ARGS.warpid * sign(im(r)) + 2) * (offset/speed - 1);
		WAIT(waittime);
		play_sfx("shot3");
		cmplx aim = cnormalize(global.plr.pos - pos);
		PROJECTILE(pp_flea, RGBA(1,0.2,0,0), .pos = pos, .move = move_accelerated(0, 0.01*aim));

		move->velocity *= r;
		WAIT(waittime);
	}
}

TASK(aimed_laser45_warp, { cmplx origin; cmplx dir; int delay; const Color *clr; }) {
	play_sfx("laser1");

	for(int i = -2; i <= 2; i++) {
		INVOKE_SUBTASK(aimed_laser45, .origin = ARGS.origin, .dir = ARGS.dir, .warpid = i, .clr = ARGS.clr, .delay = ARGS.delay);
	}
	AWAIT_SUBTASKS;
}

TASK(laser45_warp_fairy, { cmplx origin; cmplx final_pos; }) {
	auto fairy = ecls_spawn_huge_fairy(ARGS.origin, ITEMS(.points = 5));
	auto e = TASK_BIND(fairy.entity);
	ecls_fairy_summon(fairy, BEATS);
	e->move = move_towards(ARGS.final_pos, 0.03);
}

TASK(corrupted_fairy_test, { cmplx pos; StageXCorruption *corruption; }) {
	auto fairy = ecls_spawn_big_fairy(ARGS.pos, ITEMS(.power = 2, .points = 3));
	auto e = TASK_BIND(fairy.entity);
	stagex_corrupt_enemy(ARGS.corruption, e);
	ecls_fairy_summon(fairy, 120);
}

DEFINE_EXTERN_TASK(stagex_timeline) {
	// TEMPORARY TESTING HACK
	global.plr.lives = 2;
	global.plr.power_stored = 0;

	StageXCorruption *C = stagex_corruption_create();

	#if 0
	// INVOKE_SUBTASK(transition_swirls);


	// INVOKE_SUBTASK(stream_fairy, C, 64*I, 1.5, M_PI);
	INVOKE_SUBTASK(stream_fairies, C);

	// INVOKE_SUBTASK_DELAYED(0*BEATS, wheat_fairy, C, 200+100*I, move_accelerated(-1 + I, 0.01));
	// INVOKE_SUBTASK_DELAYED(3*BEATS, wheat_fairy, C, 100+200*I, move_accelerated(1 + I, 0.01));
	// INVOKE_SUBTASK_DELAYED(5*BEATS, wheat_fairy, C, 300+200*I, move_accelerated(1 + I, -0.01));
	STALL;
	#endif

	INVOKE_SUBTASK(intro_swirls, C);

	// INVOKE_TASK(corrupted_fairy_test,
	// 	.pos = VIEWPORT_W/2 + VIEWPORT_H/2*I,
	// 	.corruption = corruption,
	// );
	//
	// WAIT(600);
	// stage_load_quicksave();

	int ofs = BEATS*8 - FAIRY_ENTER_DELAY;

	INVOKE_SUBTASK_DELAYED(ofs +  2*BEATS, glider_fairy, C, VIEWPORT_W-100 + 400*I);
	INVOKE_SUBTASK_DELAYED(ofs +  4*BEATS, glider_fairy, C, 100 + 400*I);
	INVOKE_SUBTASK_DELAYED(ofs +  6*BEATS, glider_fairy, C, VIEWPORT_W/2 + 340*I);

	INVOKE_SUBTASK_DELAYED(ofs +  8*BEATS, wheat_fairy, C, 130 + 200*I);
	INVOKE_SUBTASK_DELAYED(ofs + 10*BEATS, wheat_fairy, C, VIEWPORT_W-130 + 200*I);
	INVOKE_SUBTASK_DELAYED(ofs + 12*BEATS, wheat_fairy, C, VIEWPORT_W/2 + 240*I);
	// INVOKE_SUBTASK_DELAYED(ofs + 11*BEATS, wheat_fairy, C, 90 + 260*I);
	// INVOKE_SUBTASK_DELAYED(ofs + 12*BEATS, wheat_fairy, C, VIEWPORT_W-90 + 260*I);

	WAIT(ofs + 8*BEATS);

	// for(int i = 0; i < 5; i++) {
	RADIAL_LOOP(l, 5, -I) {
		INVOKE_SUBTASK_DELAYED(14*BEATS+BEATS*l.i - 120, amaranth_fairy, C,
			0.5*(VIEWPORT_W + VIEWPORT_H*I) + 150 * l.dir, move_accelerated(1 + I, -0.01));
	}
	// }

	// INVOKE_SUBTASK_DELAYED(12*BEATS, stream_fairies, C);

	real delay = 2;
	for(int repeat = 0; repeat < 5; repeat++) {
		RADIAL_LOOP(l, 7, -I) {
			INVOKE_SUBTASK_DELAYED((8+delay*repeat)*BEATS, square_fairy, C,
				0.5*(VIEWPORT_W + VIEWPORT_H*I) + 3 * cos(repeat) * l.dir, repeat);
		}
		delay -= 0.2;
	}

	WAIT(21*BEATS - 120);
	cmplx octahedron_pos = 0.5*VIEWPORT_W + I*0.4*VIEWPORT_H;
	INVOKE_SUBTASK_DELAYED(0, octahedron_fairy, C, .origin = octahedron_pos);
	WAIT(120);

	RADIAL_LOOP(l, 6, -I * cdir(0.023)) {
		cmplx aim = l.dir * 100;
		for(int i = -1; i < 2; i+=2) {
			cmplx pos = aim*i + octahedron_pos;
			INVOKE_SUBTASK_DELAYED(BEATS+l.i*BEATS, assist_fairy, C,
					.origin = pos, .move = move_linear(cnormalize(aim)*i));
		}
	}

	// INVOKE_SUBTASK_DELAYED(6*BEATS + FAIRY_ENTER_DELAY, transition_swirls2);

	// WAIT(14 * BEATS);

	if(0){
		int count = 7;
		for(int i = 0; i < count; i++) {
			cmplx dir = cdir(-M_PI/count * i);
			cmplx pos = VIEWPORT_W*0.5 + 300*I + 200*dir;
			INVOKE_SUBTASK_DELAYED((11+i*0.5)*BEATS, scissor_fairy, C, .origin = pos, .move = move_linear(-2*I), .dir = rng_sign());

		}
	}

	WAIT(12* BEATS);
	int midboss_time = midboss_section(C);
	stagex_bg_trigger_next_phase();

	STALL;

	delay = 2;
	for(int repeat = 0; repeat < 5; repeat++) {
		RADIAL_LOOP(l, 7, -I) {
			INVOKE_SUBTASK_DELAYED((8+delay*repeat)*BEATS, square_fairy, C,
				0.5*(VIEWPORT_W + VIEWPORT_H*I) + 3 * cos(repeat) * l.dir, repeat);
		}
		delay -= 0.2;
	}

	STALL;

	/*
	 * OLD STUFF FOR REFERENCE
	 */

	INVOKE_SUBTASK(intro_swirls, C);
	// INVOKE_SUBTASK_DELAYED(2*BEATS, intro_fairy, 100 + 200*I);
	// INVOKE_SUBTASK_DELAYED(4*BEATS, intro_fairy, VIEWPORT_W-100 + 200*I);
	// INVOKE_SUBTASK_DELAYED(6*BEATS, intro_fairy, VIEWPORT_W/2 + 200*I);

	INVOKE_SUBTASK_DELAYED(16*BEATS, transition_swirls);
	STAGE_BOOKMARK_DELAYED(16*BEATS, transition1);

	INVOKE_SUBTASK_DELAYED(20*BEATS, wheat_fairy, C, 200+100*I, move_accelerated(-1 + I, 0.01));
	INVOKE_SUBTASK_DELAYED(23*BEATS, wheat_fairy, C, 100+200*I, move_accelerated(1 + I, 0.01));
	INVOKE_SUBTASK_DELAYED(25*BEATS, wheat_fairy, C, 300+200*I, move_accelerated(1 + I, -0.01));

	for(int i = 0; i < 5; i++) {
		INVOKE_SUBTASK_DELAYED(28*BEATS+BEATS*i, amaranth_fairy, C, 0.5*(VIEWPORT_W + VIEWPORT_H*I) + 150*cdir(M_TAU/4*i), move_accelerated(1 + I, -0.01));
	}
	// WAIT(400);

	// INVOKE_SUBTASK_DELAYED(27*BEATS, fairy_laser45, 0.5*(VIEWPORT_W+VIEWPORT_H*I));
	WAIT(36*BEATS);
	STAGE_BOOKMARK(octahedron-fairy);

	INVOKE_SUBTASK_DELAYED(0, octahedron_fairy, C, .origin = 0.5*(VIEWPORT_W + I * VIEWPORT_H));
	for(int t = 0; t < 6; t++) {
		cmplx aim = cdir(M_TAU/6*t)*100;
		for(int i = -1; i < 2; i+=2) {
			cmplx pos = aim*i + 0.5*(VIEWPORT_W + I * VIEWPORT_H);
			INVOKE_SUBTASK_DELAYED(BEATS+t*BEATS, assist_fairy, C, .origin = pos, .move = move_linear(cnormalize(aim)*i));
		}
	}
	INVOKE_SUBTASK_DELAYED(8*BEATS, transition_swirls2);
	STAGE_BOOKMARK_DELAYED(10.5*BEATS, scissor-fairies);
	int count = 7;
	for(int i = 0; i < count; i++) {
		cmplx dir = cdir(-M_PI/count * i);
		cmplx pos = VIEWPORT_W*0.5 + 300*I + 200*dir;
		INVOKE_SUBTASK_DELAYED((11+i*0.5)*BEATS, scissor_fairy, .origin = pos, .move = move_linear(-2*I), .dir = rng_sign());

	}

	STAGE_BOOKMARK_DELAYED(14*BEATS, funk-fairies);
	INVOKE_SUBTASK_DELAYED(14*BEATS, funk_fairies);

	STAGE_BOOKMARK_DELAYED(23*BEATS, drum-fairy);
	INVOKE_SUBTASK_DELAYED(23*BEATS, drum_fairy, .pos = 40+40*I);
	INVOKE_SUBTASK_DELAYED(24*BEATS, staircase_swirls);
	INVOKE_SUBTASK_DELAYED(26*BEATS, drum_fairy, .pos = VIEWPORT_W-40+(VIEWPORT_H-40)*I);
	INVOKE_SUBTASK_DELAYED(27*BEATS, staircase_swirls, .cross=true);

	WAIT(5762-36*BEATS);
	midboss_time = midboss_section(C);
	stagex_bg_trigger_next_phase();
	WAIT(4140 - midboss_time);
	stagex_bg_trigger_tower_dissolve();
	STAGE_BOOKMARK(post-midboss-filler);
	RADIAL_LOOP(l, 10, I) {
	INVOKE_SUBTASK_DELAYED(3*BEATS, aimed_laser45_warp, 0.5*(VIEWPORT_W + I*VIEWPORT_W), .clr = RGBA(0.1,0.1,1, 0), .dir = l.dir, .delay=30);
	}
	// INVOKE_SUBTASK_DELAYED(3*BEATS, laser45_big_fairy, 0.5*(VIEWPORT_W + I*VIEWPORT_W));
	WAIT(BEATS * 24);
	stagex_bg_trigger_next_phase();

	STAGE_BOOKMARK(pre-boss);
	WAIT(BEATS * 6);

	INVOKE_TASK(spawn_boss);
	while(!global.boss) YIELD;
	WAIT_EVENT_OR_DIE(&global.boss->events.defeated);

	stage_unlock_bgm("stagexboss");

	WAIT(240);
	stagex_dialog_post_boss();
	WAIT_EVENT_OR_DIE(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
