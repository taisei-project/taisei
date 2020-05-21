/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "timeline.h"
#include "wriggle.h"
#include "hina.h"
#include "nonspells/nonspells.h"
#include "stage2.h"

#include "stage.h"
#include "global.h"
#include "common_tasks.h"

TASK(great_circle_fairy, { cmplx pos; MoveParams move_enter; MoveParams move_exit; }) {
	Enemy *e = TASK_BIND(create_enemy1c(ARGS.pos, 7500, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 5,
		.power = 4,
	});

	e->move = ARGS.move_enter;

	WAIT(50);

	int step = difficulty_value(5, 4, 4, 3);
	int duration = difficulty_value(145, 170, 195, 220);
	real partdist = 0.04*global.diff;
	real bunchdist = 0.5;
	int c2 = 5;
	int shotnum = difficulty_value(5, 5, 7, 7);

	for(int t = 0, bunch = 0; t < duration; t += WAIT(step), ++bunch) {
		play_sfx_loop("shot1_loop");

		for(int i = 0; i < shotnum; ++i) {
			cmplx dir = cdir(i * M_TAU / shotnum + partdist * (bunch % c2 - c2/2) + bunchdist * (bunch / c2));

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos + 30 * dir,
				.color = RGB(0.6, 0.0, 0.3),
				.move = move_asymptotic_simple(1.5 * dir, bunch % 5),
			);

			if(global.diff > D_Easy && bunch % 7 == 0) {
				play_sound("shot1");
				PROJECTILE(
					.proto = pp_bigball,
					.pos = e->pos + 30 * dir,
					.color = RGB(0.3, 0.0, 0.6),
					.move = move_linear(1.7 * dir * cdir(0.3 * rng_real())),
				);
			}
		}
	}

	WAIT(60);

	e->move = ARGS.move_exit;
}

static int stage2_great_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 4);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(50,70,1)
		e->args[0] *= 0.5;

	FROM_TO_SND("shot1_loop", 70, 190+global.diff*25, 5-global.diff/2) {
		int n, c = 5+2*(global.diff>D_Normal);

		const double partdist = 0.04*global.diff;
		const double bunchdist = 0.5;
		const int c2 = 5;


		for(n = 0; n < c; n++) {
			cmplx dir = cexp(I*(2*M_PI/c*n+partdist*(_i%c2-c2/2)+bunchdist*(_i/c2)));

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos+30*dir,
				.color = RGB(0.6,0.0,0.3),
				.rule = asymptotic,
				.args = {
					1.5*dir,
					_i%5
				}
			);

			if(global.diff > D_Easy && _i%7 == 0) {
				play_sound("shot1");
				PROJECTILE(
					.proto = pp_bigball,
					.pos = e->pos+30*dir,
					.color = RGB(0.3,0.0,0.6),
					.rule = linear,
					.args = {
						1.7*dir*cexp(0.3*I*frand())
					}
				);
			}
		}
	}

	AT(210+global.diff*25) {
		e->hp = fmin(e->hp,200);
		e->args[0] = 2.0*I;
	}

	return 1;
}

static int stage2_small_spin_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2);
		return 1;
	}

	if(creal(e->args[0]) < 0)
		e->dir = 1;
	else
		e->dir = 0;

	e->pos += e->args[0];

	if(t < 100)
		e->args[0] += 0.0002*(VIEWPORT_W/2+I*VIEWPORT_H/2-e->pos);

	AT(50)
		e->pos0 = e->pos;

	FROM_TO(30,80+global.diff*5,5-global.diff/2) {
		if(_i % 2) {
			play_sound("shot1");
		}

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.9,0.0,0.3),
			.rule = linear,
			.args = {
				pow(global.diff,0.7)*(conj(e->pos-VIEWPORT_W/2)/100 + ((1-2*e->dir)+3.0*I))
			}
		);
	}

	return 1;
}

static int stage2_aim(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POWER, 2);
		return 1;
	}

	if(t < 70)
		e->pos += e->args[0];
	if(t > 150)
		e->pos -= e->args[0];

	AT(90) {
		if(global.diff > D_Normal) {
			play_sound("shot1");
			PROJECTILE(
				.proto = pp_plainball,
				.pos = e->pos,
				.color = RGB(0.6,0.0,0.8),
				.rule = asymptotic,
				.args = {
					5*cexp(I*carg(global.plr.pos-e->pos)),
					-1
				}
			);

			PROJECTILE(
				.proto = pp_plainball,
				.pos = e->pos,
				.color = RGB(0.2,0.0,0.1),
				.rule = linear,
				.args = {
					3*cexp(I*carg(global.plr.pos-e->pos))
				}
			);
		}
	}

	return 1;
}

static int stage2_sidebox_trail(Enemy *e, int t) { // creal(a[0]): velocity, cimag(a[0]): angle, a[1]: d angle/dt, a[2]: time of acceleration
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1, ITEM_POWER, 1);
		return 1;
	}

	e->pos += creal(e->args[0])*cexp(I*cimag(e->args[0]));

	FROM_TO((int) creal(e->args[2]),(int) creal(e->args[2])+M_PI*0.5/fabs(creal(e->args[1])),1)
		e->args[0] += creal(e->args[1])*I;

	FROM_TO(10,200,30-global.diff*4) {
		play_sound_ex("shot1", 5, false);

		float f = 0;
		if(global.diff > D_Normal) {
			f = 0.03*global.diff*frand();
		}

		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.9,0.0,0.9),
				.rule = linear,
				.args = { 3*cexp(I*(cimag(e->args[0])+i*f+0.5*M_PI)) }
			);
		}
	}

	return 1;
}

static int stage2_flea(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2);
		return 1;
	}

	e->pos += e->args[0]*(1-e->args[1]);

	FROM_TO(80,90,1)
		e->args[1] += 0.1;

	FROM_TO(200,205,1)
		e->args[1] -= 0.2;


	FROM_TO(10, 400, 30-global.diff*3-t/70) {
		if(global.diff != D_Easy) {
			play_sound("shot1");
			PROJECTILE(
				.proto = pp_flea,
				.pos = e->pos,
				.color = RGB(0.3,0.2,1),
				.rule = asymptotic,
				.args = {
					1.5*cexp(2.0*I*M_PI*frand()),
					1.5
				}
			);
		}
	}

	return 1;
}

static int stage2_accel_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1, ITEM_POWER, 3);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(60,250, 20+10*(global.diff<D_Hard)) {
		e->args[0] *= 0.5;

		int i;
		for(i = 0; i < 6; i++) {
			play_sound("redirect");
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGB(0.9,0.1,0.2),
				.rule = accelerated,
				.args = {
					1.5*cexp(2.0*I*M_PI/6*i)+cexp(I*carg(global.plr.pos - e->pos)),
					-0.02*cexp(I*(2*M_PI/6*i+0.02*frand()*global.diff))
				}
			);
		}
	}

	if(t > 270)
		e->args[0] -= 0.01*I;

	return 1;
}

TASK(boss_appear_stub, NO_ARGS) {
	log_warn("FIXME");
}

static void stage2_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Stage2PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage2PreBossDialog, pm->dialog->Stage2PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear_stub);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage2boss");
}

static void stage2_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage2PostBossDialog, pm->dialog->Stage2PostBoss);
}

static void wriggle_ani_flyin(Boss *w) {
	aniplayer_queue(&w->ani,"midbossflyintro",1);
	aniplayer_queue(&w->ani,"main",0);
}

static void wriggle_intro_stage2(Boss *w, int t) {
	if(t < 0)
		return;
	t = smoothstep(0.0, 1.0, t / 240.0) * 240;
	w->pos = CMPLX(VIEWPORT_W/2, 100.0) + (1.0 - t / 240.0) * (300 * cexp(I * (3 - t * 0.04)) - 128);
}

static void wiggle_mid_flee(Boss *w, int t) {
	if(t == 0)
		aniplayer_queue(&w->ani, "fly", 0);
	if(t >= 0) {
		GO_TO(w, VIEWPORT_W/2 - 3.0 * t - 300 * I, 0.01)
	}
}

static Boss *create_wriggle_mid(void) {
	Boss *wriggle = stage2_spawn_wriggle(VIEWPORT_W + 150 - 30.0*I);
	wriggle_ani_flyin(wriggle);

	boss_add_attack(wriggle, AT_Move, "Introduction", 4, 0, wriggle_intro_stage2, NULL);
	boss_add_attack_task(wriggle, AT_Normal, "Small Bug Storm", 20, 26000, TASK_INDIRECT(BossAttack, stage2_midboss_nonspell_1), NULL);
	boss_add_attack(wriggle, AT_Move, "Flee", 5, 0, wiggle_mid_flee, NULL);;

	boss_start_attack(wriggle, wriggle->attacks);
	return wriggle;
}

static void hina_intro(Boss *h, int time) {
	TIMER(&time);

	AT(100)
	stage2_dialog_pre_boss();

	GO_TO(h, VIEWPORT_W/2 + 100.0*I, 0.05);
}

static Boss *create_hina(void) {
	Boss *hina = stage2_spawn_hina(VIEWPORT_W + 150 + 100.0*I);

	aniplayer_queue(&hina->ani,"main",3);
	aniplayer_queue(&hina->ani,"guruguru",1);
	aniplayer_queue(&hina->ani,"main",0);

	boss_add_attack(hina, AT_Move, "Introduction", 2, 0, hina_intro, NULL);
	boss_add_attack(hina, AT_Normal, "Cards1", 30, 25000, hina_cards1, NULL);
	boss_add_attack_from_info(hina, &stage2_spells.boss.amulet_of_harm, false);
	boss_add_attack(hina, AT_Normal, "Cards2", 40, 30000, hina_cards2, NULL);
	boss_add_attack_from_info(hina, &stage2_spells.boss.bad_pick, false);
	boss_add_attack_from_info(hina, &stage2_spells.boss.wheel_of_fortune, false);
	boss_add_attack_from_info(hina, &stage2_spells.extra.monty_hall_danmaku, false);

	boss_start_attack(hina, hina->attacks);
	return hina;
}

void stage2_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage2");
		stage_set_voltage_thresholds(75, 175, 400, 720);
	}

	AT(300) {
		// create_enemy1c(VIEWPORT_W/2-10.0*I, 7500, BigFairy, stage2_great_circle, 2.0*I);
	}

	FROM_TO(650-50*global.diff, 750+25*(4-global.diff), 50) {
		create_enemy1c(VIEWPORT_W*((_i)%2), 1000, Fairy, stage2_small_spin_circle, 2-4*(_i%2)+1.0*I);
	}

	FROM_TO(850, 1000, 15)
	create_enemy1c(VIEWPORT_W/2+25*(_i-5)-20.0*I, 200, Fairy, stage2_aim, (2+frand()*0.3)*I);

	FROM_TO(960, 1200, 20)
	create_enemy3c(VIEWPORT_W-80+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 3 - 0.5*M_PI*I, -0.02, 90);

	FROM_TO(1140, 1400, 20)
	create_enemy3c(200-20.0*I, 200, Fairy, stage2_sidebox_trail, 3+0.5*I*M_PI, -0.05, 70);

	AT(1300)
	create_enemy1c(150-10.0*I, 4000, BigFairy, stage2_great_circle, 2.5*I);

	AT(1500)
	create_enemy1c(VIEWPORT_W-150-10.0*I, 4000, BigFairy, stage2_great_circle, 2.5*I);

	FROM_TO(1700, 2000, 30)
	create_enemy1c(VIEWPORT_W*frand()-10.0*I, 200, Fairy, stage2_flea, 1.7*I);

	if(global.diff > D_Easy) {
		FROM_TO(1950, 2500, 60) {
			create_enemy3c(VIEWPORT_W-40+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 5 - 0.5*M_PI*I, -0.02, 83-global.diff*3);
			create_enemy3c(40+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 5 - 0.5*M_PI*I, 0.02, 80-global.diff*3);
		}
	}

	AT(2300) {
		create_enemy1c(VIEWPORT_W/4-10.0*I, 2000, BigFairy, stage2_accel_circle, 2.0*I);
		create_enemy1c(VIEWPORT_W/4*3-10.0*I, 2000, BigFairy, stage2_accel_circle, 2.0*I);
	}

	AT(2700)
	global.boss = create_wriggle_mid();

	FROM_TO(2900, 3400, 50) {
		if(global.diff > D_Normal)
			create_enemy3c(VIEWPORT_W-80+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 3 - 0.5*M_PI*I, -0.02, 90);
		create_enemy3c(80+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 3 - 0.5*M_PI*I, 0.02, 90);
	}

	FROM_TO(3000, 3600, 300) {
		create_enemy1c(VIEWPORT_W/2-60*(_i-1)-10.0*I, 7000+500*global.diff, BigFairy, stage2_great_circle, 2.0*I);
	}

	FROM_TO(3700, 4500, 40)
	create_enemy1c(VIEWPORT_W*(0.1+0.8*frand())-10.0*I, 150, Fairy, stage2_flea, 2.5*I);

	FROM_TO(4000, 4600, 100+100*(global.diff<D_Hard))
	create_enemy1c(VIEWPORT_W*(0.5+0.1*sqrt(_i)*(1-2*(_i&1)))-10.0*I, 2000, BigFairy, stage2_accel_circle, 2.0*I);

	AT(5100) {
		stage_unlock_bgm("stage2");
		global.boss = create_hina();
	}

	AT(5180) {
		stage_unlock_bgm("stage2boss");
		stage2_dialog_post_boss();
	}

	AT(5185) {
		stage_finish(GAMEOVER_SCORESCREEN);
	}
}

DEFINE_EXTERN_TASK(stage2_timeline) {
	YIELD;

	INVOKE_TASK_DELAYED(300, great_circle_fairy,
		.pos = VIEWPORT_W/2 - 10.0*I,
		.move_enter = move_towards(VIEWPORT_W/2 + 150.0*I, 0.04),
		.move_exit = move_asymptotic_halflife(0, 2*I, 60)
	);
}
