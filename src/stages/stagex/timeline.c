/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "timeline.h"   // IWYU pragma: keep
#include "background_anim.h"
#include "common_tasks.h"
#include "coroutine/taskdsl.h"
#include "enemy_classes.h"
#include "global.h"
#include "move.h"
#include "nonspells/nonspells.h"
#include "stagex.h"
#include "yumemi.h"

#include "stages/common_imports.h"

#define BEATS 86

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
	cmplx pos; cmplx dir;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(VIEWPORT_W/2-10*I, ITEMS(
		.power = 3,
		.points = 5,
	)));

	YIELD;

	for(int i = 0; i < 80; ++i) {
		e->pos += cnormalize(ARGS.pos-e->pos)*2;
		YIELD;
	}

	for(int i = 0; i < 3; i++) {
		real aim = carg(global.plr.pos - e->pos);
		INVOKE_TASK(glider_bullet, e->pos, aim-0.7, 20, 6);
		INVOKE_TASK(glider_bullet, e->pos, aim, 25, 3);
		INVOKE_TASK(glider_bullet, e->pos, aim+0.7, 20, 6);
		WAIT(80-20*i);
	}

	for(;;) {
		e->pos += 2*(re(e->pos)-VIEWPORT_W/2 > 0)-1;
		YIELD;
	}
}

TASK(aimgrind_fairy, {
	cmplx pos;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, NULL));
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

	boss_add_attack_from_info(boss, &stagex_spells.midboss.trap_representation, false);
	boss_add_attack_from_info(boss, &stagex_spells.midboss.fork_bomb, false);
	boss_engage(global.boss);
}

TASK(scuttleproj_appear) {
	STAGE_BOOKMARK(scuttleproj);

	Projectile *p = TASK_BIND(PROJECTILE(
		.pos = VIEWPORT_W/2,
		.proto = pp_soul,
		.color = RGBA(0,0.2,1,0),
		.move = move_towards(0, global.plr.pos, 0.01),
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

TASK(midswirl, {
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(.points = 1)));
	set_turning_motion(e, ARGS.vel, ARGS.turn_angle, ARGS.turn_delay, ARGS.turn_duration);

	for(;;WAIT(6)) {
// 		play_sfx("shot1");

		cmplx aim = cnormalize(global.plr.pos - e->pos);

		PROJECTILE(
			.pos = e->pos,
			.proto = pp_ball,
			.color = RGB(0, 0, 1),
			.move = move_asymptotic_simple(aim * 5, 6),
		);
	}

	STALL;
}

TASK(midswirls, {
	int count;
	cmplx pos;
	cmplx vel;
	real turn_angle;
	int turn_delay;
	int turn_duration;
}) {
	for(int i = 0; i < ARGS.count; ++i, WAIT(12)) {
		INVOKE_TASK(midswirl,
			.pos = ARGS.pos,
			.vel = ARGS.vel,
			.turn_angle = ARGS.turn_angle,
			.turn_delay = ARGS.turn_delay,
			.turn_duration = ARGS.turn_duration
		);
	}
}

static int midboss_section(void) {
	int t = 0;

	STAGE_BOOKMARK(midboss);
	stagex_bg_trigger_next_phase();
	t += WAIT(BEATS * 1.25);
	play_sfx("shot_special1");

	INVOKE_TASK(midswirls,
		.count = 8,
		.pos = 0 + 64*I,
		.vel = 8,
		.turn_angle = M_PI,
		.turn_delay = 20,
		.turn_duration = 30
	);

	t += WAIT(BEATS);

	INVOKE_TASK(midswirls,
		.count = 8,
		.pos = VIEWPORT_W + 64*I,
		.vel = -8,
		.turn_angle = -M_PI,
		.turn_delay = 20,
		.turn_duration = 30
	);

	t += WAIT(BEATS);

	INVOKE_TASK(midswirls,
		.count = 8,
		.pos = 0 + 260*I,
		.vel = 8,
		.turn_angle = 3*M_PI/2,
		.turn_delay = 20,
		.turn_duration = 30
	);

	INVOKE_TASK(midswirls,
		.count = 8,
		.pos = VIEWPORT_W + 260*I,
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
	log_debug("midboss spawn: %i", t);
	t += WAIT_EVENT_OR_DIE(&NOT_NULL(global.boss)->events.defeated).frames;
	log_debug("midboss defeat: %i", t);
	STAGE_BOOKMARK(post-midboss);

	return t;
}

TASK(laser45, { cmplx origin; cmplx dir; cmplx r; const Color *clr; }) {
	int d0 = 60;
	int d1 = 15;

	play_sfx("laser1");

	MoveParams *move;
	auto l = TASK_BIND(create_dynamic_laser(ARGS.origin, 120, (d0+d1) * 4, ARGS.clr, &move));
	l->width_exponent = 0.5;

	cmplx r = ARGS.r;
	*move = move_linear(ARGS.dir * 3);

	for(;;) {
		WAIT(d0);
		move->velocity *= r;
		WAIT(d1);
		move->velocity *= r;
	}
}

TASK(fairy_laser45, { cmplx origin; }) {
	auto e = TASK_BIND(espawn_huge_fairy(ARGS.origin, ITEMS(.points = 5)));
	ecls_anyfairy_summon(e, 60);

	for(int i = 0; i < 3; ++i) {
		RADIAL_LOOP(l, 8, I) {
			INVOKE_TASK(laser45, e->pos, l.dir, cdir(M_PI/4), RGB(0.5, 0.1, 0.8));
			INVOKE_TASK_DELAYED(30, laser45, e->pos, l.dir, cdir(-M_PI/4), RGB(0.1, 0.5, 0.8));
		}
		WAIT(400);
	}

	enemy_kill(e);
}

TASK(intro_swirl, { cmplx origin; cmplx dir; }) {
	auto e = TASK_BIND(espawn_swirl(ARGS.origin, ITEMS(.points = 0)));
	vec3 p = { 0, 0, stage_3d_context.cam.pos[2] - 150 };
	ecls_anyenemy_fake3dmovein(e, &stage_3d_context.cam, p, 180);
	e->move = move_accelerated(0, ARGS.dir * -0.05);
	WAIT(5);
	PROJECTILE(
		.proto = pp_bullet,
		.pos = e->pos,
		.move = move_asymptotic_simple(2 * cnormalize(global.plr.pos - e->pos), 2),
		.color = RGB(0, 0, 1),
	);
	// enemy_kill(e);
}

TASK(intro_fairy, { cmplx origin; }) {
	auto e = TASK_BIND(espawn_big_fairy(ARGS.origin, ITEMS(.power = 2, .points = 2)));
	vec3 p = { 0, 0, stage_3d_context.cam.pos[2] - 150 };

	ecls_anyenemy_fake3dmovein(e, &stage_3d_context.cam, p, BEATS);

	int duration = BEATS;
	for(int t = 0; t < duration; t++) {
		real f = t/(real)duration;
		real range = 1+4 * pcos(M_PI+2*M_PI*f);
		int x, y;
		do {
			x = rng_range(-range, range);
			y = rng_range(-range, range);
		} while(x*x + y*y > range*range);

		cmplx aim = cnormalize(global.plr.pos-e->pos);
		cmplx ppos = e->pos + 20 * (x + I * y);
		cmplx paim = cnormalize(global.plr.pos - ppos);
		PROJECTILE(
			.proto = pp_ball,
			.pos = ppos,
			.color = RGB(2*f,1*f,1),
			.move = move_accelerated((2+f)*aim, 0.02*paim)
		);

		play_sfx_loop("shot1_loop");

		WAIT(4);
	}
	e->move = move_linear(2*I);
}

TASK(square_fairy, { cmplx origin; int corruption; }) {
	auto e = TASK_BIND(espawn_fairy_red(ARGS.origin, ITEMS(.power = 1)));
	vec3 p = { 0, 0, stage_3d_context.cam.pos[2] - 150 };

	ecls_anyenemy_fake3dmovein(e, &stage_3d_context.cam, p, BEATS);

	e->move = move_linear(I*cnormalize(0.5*(VIEWPORT_W+I*VIEWPORT_H)-e->pos));

	for(;;) {
		INVOKE_SUBTASK(common_charge, e->pos, RGBA(1.0,1.0,0.0,0.5), BEATS/2, .sound = COMMON_CHARGE_SOUNDS);
		WAIT(BEATS/2);

		int num = 5;
		for(int side = 0; side < 4; side++) {
			cmplx aim = cdir(M_PI/2*side + 0.1*ARGS.corruption);

			for(int n = -num/2; n <= num/2; n++) {
				cmplx dir = aim * (2+1*I*n);
				PROJECTILE(
					.proto = pp_bigball,
					.pos = e->pos,
					.color = RGB(0,0,1),
					.move = move_linear(dir)
				);
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos,
					.color = RGB(1,0,0),
					.move = move_linear((1.2+0.1*ARGS.corruption*I*sin(n))*dir)
				);
			}
		}
		play_sfx("special_shot1");
		WAIT(BEATS*4.5);
	}


}

TASK(transition_swirl, { cmplx origin; cmplx dir; int corruption; }) {
	auto e = TASK_BIND(espawn_swirl(ARGS.origin, ITEMS(.power = 0)));
	vec3 p = { 0, 0, stage_3d_context.cam.pos[2] - 150 };
	ecls_anyenemy_fake3dmovein(e, &stage_3d_context.cam, p, BEATS/2);

	e->move = move_accelerated(ARGS.dir, 0.01*cdir(0.1)*ARGS.dir);

	for(int t = 0; t < 10; t++) {
		for(int side = -1; side <= 1; side+=2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = RGB(0.5*ARGS.corruption,0.2*ARGS.corruption,1-ARGS.corruption),
				.move = move_accelerated(side*I*ARGS.dir, 0.01*ARGS.corruption*-ARGS.dir)
			);
		}
		play_sfx("shot1");

		WAIT(10);
	}
}


TASK(intro_swirls) {
	int t = BEATS * 10;
	int interval = 8;
	cmplx r = cdir(0.3);
	cmplx dir = -I;
	for(;t - interval > 0; t -= interval) {
		INVOKE_SUBTASK(intro_swirl, 0.5*(VIEWPORT_W+VIEWPORT_H*I) + 80 * dir, dir);
		dir *= r;
		WAIT(interval);
	}
	AWAIT_SUBTASKS;
}

TASK(transition_swirls) {
	int duration = 3*BEATS;
	int count = 100;

	for(int t = 0; t < 1.5*BEATS; t++) {
		cmplx pos = 50*I*cos(t) + 50*sin(sin(sin(t)));
		cmplx dir = 3*cnormalize(pos);
		INVOKE_SUBTASK(transition_swirl, VIEWPORT_W/2 + I*VIEWPORT_H/2 + pos, dir, 0);
		WAIT(1);
	}
	WAIT(BEATS/4);
	for(int t = 0; t < 1.5*BEATS; t++) {
		cmplx pos = 50*I*cos(t) + 50*sin(t^(342345));
		cmplx dir = -3*cnormalize(pos);
		INVOKE_SUBTASK(transition_swirl, VIEWPORT_W/2 + I*VIEWPORT_H/2 + pos, dir, 1);
		WAIT(1);
	}
	AWAIT_SUBTASKS;
}

DEFINE_EXTERN_TASK(stagex_timeline) {
	WAIT(BEATS);

	INVOKE_SUBTASK(intro_swirls);
	INVOKE_SUBTASK_DELAYED(2*BEATS, intro_fairy, 100 + 200*I);
	INVOKE_SUBTASK_DELAYED(4*BEATS, intro_fairy, VIEWPORT_W-100 + 200*I);
	INVOKE_SUBTASK_DELAYED(6*BEATS, intro_fairy, VIEWPORT_W/2 + 200*I);

	real delay = 2;
	for(int repeat = 0; repeat < 5; repeat++) {
		for(int n = 0; n < 7; n++) {
			INVOKE_SUBTASK_DELAYED((8+delay*repeat)*BEATS, square_fairy, 0.5*(VIEWPORT_W + VIEWPORT_H*I) + 100*cos(repeat)*I*cexp(-I*n), repeat);
		}
		delay -= 0.2;
	}

	INVOKE_SUBTASK_DELAYED(16*BEATS, transition_swirls);

	// WAIT(400);

	// INVOKE_TASK(fairy_laser45, 0.5*(VIEWPORT_W+VIEWPORT_H*I));

	WAIT(5762);
	int midboss_time = midboss_section();
	stagex_bg_trigger_next_phase();
	WAIT(4140 - midboss_time);
	stagex_bg_trigger_tower_dissolve();
	STAGE_BOOKMARK(post-midboss-filler);
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
