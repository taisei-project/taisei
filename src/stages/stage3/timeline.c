/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage3.h"
#include "wriggle.h"
#include "scuttle.h"
#include "spells/spells.h"
#include "nonspells/nonspells.h"
#include "background_anim.h"

#include "global.h"
#include "stagetext.h"
#include "common_tasks.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

TASK(boss_appear_stub, NO_ARGS) {
	log_warn("FIXME");
}

static void stage3_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Stage3PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage3PreBossDialog, pm->dialog->Stage3PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear_stub);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage3boss");
}

static void stage3_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage3PostBossDialog, pm->dialog->Stage3PostBoss);
}

static int stage3_enterswirl(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1, ITEM_POWER, 1);

		float r, g;
		if(frand() > 0.5) {
			r = 0.3;
			g = 1.0;
		} else {
			r = 1.0;
			g = 0.3;
		}

		int cnt = 24 - (D_Lunatic - global.diff) * 4;
		for(int i = 0; i < cnt; ++i) {
			double a = (M_PI * 2.0 * i) / cnt;
			cmplx dir = cexp(I*a);

			PROJECTILE(
				.proto = e->args[1]? pp_ball : pp_rice,
				.pos = e->pos,
				.color = RGB(r, g, 1.0),
				.rule = asymptotic,
				.args = {
					1.5 * dir,
					10 - 10 * psin(2 * a + M_PI/2)
				}
			);
		}

		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	AT(60) {
		enemy_kill(e);
	}

	e->pos += e->args[0];

	return 0;
}

static int stage3_slavefairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1, ITEM_POWER, 3);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	if(t < 120)
		GO_TO(e, e->args[0], 0.03)

	FROM_TO_SND("shot1_loop", 30, 120, 5 - global.diff) {
		float a = _i * 0.5;
		cmplx dir = cexp(I*a);

		PROJECTILE(
			.proto = pp_wave,
			.pos = e->pos + dir * 10,
			.color = (_i % 2)? RGB(1.0, 0.3, 0.3) : RGB(0.3, 0.3, 1.0),
			.rule = accelerated,
			.args = {
				dir * 2,
				dir * -0.035,
			},
		);

		if(global.diff > D_Easy && e->args[1]) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos + dir * 10,
				.color = RGB(1.0, 0.6, 0.3),
				.rule = linear,
				.args = { dir * (1.0 + 0.5 * sin(a)) }
			);
		}
	}

	if(t >= 120) {
		e->pos += 3 * e->args[2] + 2.0*I;
	}

	return 0;
}

static int stage3_slavefairy2(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1, ITEM_POWER, 3);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	int lifetime = 160;
	if(t < lifetime)
		GO_TO(e, e->args[0], 0.03)

	FROM_TO_SND("shot1_loop", 30, lifetime, 1) {
		double a = _i/sqrt(global.diff);

		if(t > 90) {
			a *= -1;
		}

		cmplx dir = cexp(I*a);
		PROJECTILE(
			.proto = pp_wave,
			.pos = e->pos,
			.color = (_i&1) ? RGB(1.0,0.3,0.3) : RGB(0.3,0.3,1.0),
			.rule = linear,
			.args = { 2*dir },
		);

		if(global.diff > D_Normal && _i % 3 == 0) {
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = !(_i&1) ? RGB(1.0,0.3,0.3) : RGB(0.3,0.3,1.0),
				.rule = linear,
				.args = { -2*dir }
			);
		}
	}

	if(t >= lifetime) {
		e->pos += 3 * e->args[2] + 2.0*I;
	}

	return 1;
}

static void charge_effect(Enemy *e, int t, int chargetime) {
	TIMER(&t);

	FROM_TO(chargetime - 30, chargetime, 1) {
		cmplx n = cexp(2.0*I*M_PI*frand());
		float l = 50*frand()+25;
		float s = 4+_i*0.01;

		PARTICLE(
			.sprite = "flare",
			.pos = e->pos+l*n,
			.color = RGBA(0.5, 0.5, 0.25, 0),
			.draw_rule = Fade,
			.rule = linear,
			.timeout = l/s,
			.args = { -s*n },
		);
	}
}

static int stage3_burstfairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 2, ITEM_POWER, 5);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	int bursttime = creal(e->args[3]);
	int prebursttime = cimag(e->args[3]);
	int burstspan = 30;

	if(t < bursttime) {
		GO_TO(e, e->args[0], 0.08)
	}

	if(prebursttime > 0) {
		AT(prebursttime) {
			play_sound("shot_special1");

			int cnt = 6 + 4 * global.diff;
			for(int p = 0; p < cnt; ++p) {
				cmplx dir = cexp(I*M_PI*2*p/cnt);
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->args[0],
					.color = RGB(0.2, 0.1, 0.5),
					.rule = asymptotic,
					.args = {
						dir,
						10 + 4 * global.diff
					},
				);
			}
		}
	}

	AT(bursttime) {
		play_sound("shot_special1");
	}

	int step = 3 - (global.diff > D_Normal);

	charge_effect(e, t, bursttime);

	FROM_TO_SND("shot1_loop", bursttime, bursttime + burstspan, step) {
		double phase = (t - bursttime) / (double)burstspan;
		cmplx dir = cexp(I*M_PI*phase);

		int cnt = 5 + global.diff;
		for(int p = 0; p < cnt; ++p) {
			for(int i = -1; i < 2; i += 2) {
				PROJECTILE(
					.proto = pp_bullet,
					.pos = e->pos + dir * 10,
					.color = color_lerp(RGB(0.0, 0.0, 1.0), RGB(1.0, 0.0, 0.0), psin(M_PI * phase)),
					.rule = asymptotic,
					.args = {
						1.5 * dir * (1 + p / (cnt - 1.0)) * i,
						3 * global.diff
					},
				);
			}
		}

	}

	if(t >= bursttime + burstspan) {
		e->pos += 3 * e->args[2] + 2.0*I;
	} else if(t > bursttime) {
		// GO_TO(e, VIEWPORT_W+VIEWPORT_H*I - e->args[0], 0.05)
	}

	return 0;
}

static int stage3_chargefairy_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	/*
	if(t == 0) {
		p->type = FakeProj;
	} else if(t == 20) {
		p->type = EnemyProj;
	}
	*/

	p->angle = carg(p->args[0]);
	t -= creal(p->args[2]);

	if(t == 0) {
		play_sound("redirect");
		play_sound("shot_special1");
		spawn_projectile_highlight_effect(p);
	} else if(t > 0) {
		p->args[1] *= 0.8;
		p->pos += p->args[0] * (p->args[1] + 1);
	}

	return ACTION_NONE;
}

static int stage3_chargefairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 3);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	AT(30) {
		e->args[3] = global.plr.pos;

	}

	int chargetime = e->args[1];
	int cnt = 19 - 4 * (D_Lunatic - global.diff);
	int step = 1;

	charge_effect(e, t, chargetime);

	FROM_TO_SND("shot1_loop", chargetime, chargetime + cnt * step - 1, step) {
		cmplx aim = e->args[3] - e->pos;
		aim /= cabs(aim);
		cmplx aim_norm = -cimag(aim) + I*creal(aim);

		int layers = 1 + global.diff;
		int i = _i;

		for(int layer = 0; layer < layers; ++layer) {
			if(layer&1) {
				i = cnt - 1 - i;
			}

			double f = i / (cnt - 1.0);
			int w = 100 - 20 * layer;
			cmplx o = e->pos + w * psin(M_PI*f) * aim + aim_norm * w*0.8 * (f - 0.5);
			cmplx paim = e->pos + (w+1) * aim - o;
			paim /= cabs(paim);

			PROJECTILE(
				.proto = pp_wave,
				.pos = o,
				.color = color_lerp(RGB(0.0, 0.0, 1.0), RGB(1.0, 0.0, 0.0), f),
				.rule = stage3_chargefairy_proj,
				.args = {
					paim, 6 + global.diff - layer,
					chargetime + 30 - t
				},
			);
		}
	}

	if(t > chargetime + step * cnt * 2) {
		/*
		complex dir = e->pos - (VIEWPORT_W+VIEWPORT_H*I)/2;
		dir /= cabs(dir);
		e->pos += dir;
		*/
		e->pos += e->args[2];
	} else {
		GO_TO(e, e->args[0], 0.03);
	}

	return 0;
}

static int stage3_bigfairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 5);
		if(creal(e->args[0]) && global.timer > 2800)
			spawn_items(e->pos, ITEM_BOMB, 1);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	EnemyLogicRule slave = cimag(e->args[0]) ? stage3_slavefairy2 : stage3_slavefairy;

	FROM_TO(30, 600, 270) {
		create_enemy3c(e->pos, 900, Fairy, slave, e->pos + 70 + 50 * I, e->args[0], +1);
		create_enemy3c(e->pos, 900, Fairy, slave, e->pos - 70 + 50 * I, e->args[0], -1);
	}

	FROM_TO(120, 600, 270) {
		create_enemy3c(e->pos, 900, Fairy, slave, e->pos + 70 - 50 * I, e->args[0], +1);
		create_enemy3c(e->pos, 900, Fairy, slave, e->pos - 70 - 50 * I, e->args[0], -1);
	}

	AT(creal(e->args[1])) {
		enemy_kill(e);
	}

	return 0;
}

static int stage3_bitchswirl(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_BIRTH) {
		return 1;
	}

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1, ITEM_POWER, 1);
		return -1;
	}

	FROM_TO(0, 120, 20) {
		PROJECTILE(
			.proto = pp_flea,
			.pos = e->pos,
			.color = RGB(0.7, 0.0, 0.5),
			.rule = accelerated,
			.args = {
				2*cexp(I*carg(global.plr.pos - e->pos)),
				0.005*cexp(I*(M_PI*2 * frand())) * (global.diff > D_Easy)
			},
		);
		play_sound("shot1");
	}

	e->pos += e->args[0] + e->args[1] * creal(e->args[2]);
	e->args[2] = creal(e->args[2]) * cimag(e->args[2]) + I * cimag(e->args[2]);

	return 0;
}

static int stage3_cornerfairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5, ITEM_POWER, 5);
		return -1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	FROM_TO(0, 120, 1)
		GO_TO(e, e->args[0], 0.01)

	FROM_TO_SND("shot1_loop", 140, 240, 1) {
		GO_TO(e, e->args[1], 0.025 * fmin((t - 120) / 42.0, 1))
		int d = 5; //(D_Lunatic - global.diff + 3);
		if(!(t % d)) {
			int i, cnt = 7+global.diff;

			for(i = 0; i < cnt; ++i) {
				float c = psin(t / 15.0);
				bool wave = global.diff > D_Easy && cabs(e->args[2]);

				PROJECTILE(
					.proto = wave ? pp_wave : pp_thickrice,
					.pos = e->pos,
					.color = cabs(e->args[2])
							? RGB(0.5 - c*0.2, 0.3 + c*0.7, 1.0)
							: RGB(1.0 - c*0.5, 0.6, 0.5 + c*0.5),
					.rule = asymptotic,
					.args = {
						(1.8-0.4*wave*!!(e->args[2]))*cexp(I*((2*i*M_PI/cnt)+carg((VIEWPORT_W+I*VIEWPORT_H)/2 - e->pos))),
						1.5
					},
				);
			}
		}
	}

	AT(260) {
		return ACTION_DESTROY;
	}

	return 0;
}

static void scuttle_intro(Boss *boss, int time) {
	GO_TO(boss, VIEWPORT_W/2.0 + 100.0*I, 0.04);
}

static void scuttle_outro(Boss *boss, int time) {
	if(time == 0) {
		spawn_items(boss->pos, ITEM_POINTS, 10, ITEM_POWER, 10, ITEM_LIFE, 1);
	}

	boss->pos += pow(fmax(0, time)/30.0, 2) * cexp(I*(3*M_PI/2 + 0.5 * sin(time / 20.0)));
}

static Boss *stage3_create_midboss(void) {
	Boss *scuttle = stage3_spawn_scuttle(VIEWPORT_W/2 - 200.0*I);

	boss_add_attack(scuttle, AT_Move, "Introduction", 1, 0, scuttle_intro, NULL);
	boss_add_attack(scuttle, AT_Normal, "Lethal Bite", 11, 20000, stage3_midboss_nonspell1, NULL);
	boss_add_attack_from_info(scuttle, &stage3_spells.mid.deadly_dance, false);
	boss_add_attack(scuttle, AT_Move, "Runaway", 2, 1, scuttle_outro, NULL);
	scuttle->zoomcolor = *RGB(0.4, 0.1, 0.4);

	boss_engage(scuttle);
	return scuttle;
}

static void stage3_boss_intro(Boss *boss, int time) {
	if(time == 110)
		stage3_dialog_pre_boss();

	GO_TO(boss, VIEWPORT_W/2.0 + 100.0*I, 0.03);
}

static Boss* stage3_create_boss(void) {
	Boss *wriggle = stage3_spawn_wriggle(VIEWPORT_W/2 - 200.0*I);

	boss_add_attack(wriggle, AT_Move, "Introduction", 2, 0, stage3_boss_intro, NULL);
	boss_add_attack(wriggle, AT_Normal, "", 11, 35000, stage3_boss_nonspell1, NULL);
	boss_add_attack_from_info(wriggle, &stage3_spells.boss.moonlight_rocket, false);
	boss_add_attack(wriggle, AT_Normal, "", 40, 35000, stage3_boss_nonspell2, NULL);
	boss_add_attack_from_info(wriggle, &stage3_spells.boss.wriggle_night_ignite, false);
	boss_add_attack(wriggle, AT_Normal, "", 40, 35000, stage3_boss_nonspell3, NULL);
	boss_add_attack_from_info(wriggle, &stage3_spells.boss.firefly_storm, false);
	boss_add_attack_from_info(wriggle, &stage3_spells.extra.light_singularity, false);

	boss_engage(wriggle);
	return wriggle;
}

void stage3_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage3");
		stage_set_voltage_thresholds(115, 250, 510, 860);
	}

	FROM_TO(160, 300, 10) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W/2 + 20 * anfrand(0) + (VIEWPORT_H/4 + 20 * anfrand(1))*I, 200, Swirl, stage3_enterswirl, 3 * (I + sin(M_PI*global.frames/15.0)));
	}

	AT(360) {
		create_enemy2c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage3_bigfairy, 0+1*I, 600 - 30 * (D_Lunatic - global.diff));
	}

	FROM_TO(600, 800-30*(D_Lunatic-global.diff), 20) {
		create_enemy3c(-20 + 20*I, 50, Swirl, stage3_bitchswirl, 5, 1*I, 5+0.95*I);
	}

	FROM_TO(800, 1000-30*(D_Lunatic-global.diff), 20) {
		cmplx f = 5 + (0.93 + 0.01 * _i) * I;
		create_enemy3c(-20 + 20*I, 50, Swirl, stage3_bitchswirl, 5, 1*I, f);
		create_enemy3c(VIEWPORT_W+20 + 20*I, 50, Swirl, stage3_bitchswirl, -5, 1*I, f);
	}

	{
		int cnt = 4;
		int step = 20;
		int start = 1030;
		FROM_TO(start, start + step * cnt - 1, step) {
			int i = _i % cnt;
			double span = 300 - 60 * (i/2);
			cmplx pos1 = VIEWPORT_W/2;
			cmplx pos2 = VIEWPORT_W/2 + span * (-0.5 + (i&1)) + (VIEWPORT_H/3 + 100*(i/2))*I;
			create_enemy4c(pos1, 700, Fairy, stage3_burstfairy, pos2, i&1, 0, start + step * 6 - global.timer);
		}
	}

	{
		int cnt = 6;
		int step = 60;
		int start = 1200;
		FROM_TO(start, start + cnt * step - 1, step) {
			int i = _i % 4;
			double span = 300 - 60 * (i/2);
			cmplx pos = VIEWPORT_W/2 + span * (-0.5 + (i&1)) + (VIEWPORT_H/3 + 100*(i/2))*I;

			cmplx exitdir = pos - (VIEWPORT_W+VIEWPORT_H*I)/2;
			exitdir /= cabs(exitdir);

			create_enemy3c(pos, 1000, Fairy, stage3_chargefairy, pos, 30, exitdir);
		}
	}

	FROM_TO(1530, 1575, 10) {
		create_enemy3c(20 - 20*I, 50, Swirl, stage3_bitchswirl, 5*I, 0, 0);
	}

	FROM_TO(1575, 1620, 10) {
		create_enemy3c(VIEWPORT_W-20 - 20*I, 50, Swirl, stage3_bitchswirl, 5*I, 0, 0);
	}

	AT(1600) {
		create_enemy2c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage3_bigfairy, 1, 600 - 30 * (D_Lunatic - global.diff));
	}

	FROM_TO(1800, 2200, 10) {
		if(global.enemies.first == NULL) {
			int cnt = 2;
			for(int i = 0; i <= cnt;i++) {
				cmplx pos1 = VIEWPORT_W/2+VIEWPORT_W/3*nfrand() + VIEWPORT_H/5*I;
				cmplx pos2 = VIEWPORT_W/2+50*(i-cnt/2)+VIEWPORT_H/3*I;
				create_enemy3c(pos1, 700, Fairy, stage3_slavefairy2, pos2, i&1,0.5*(i-cnt/2));
			}
		}
	}

	{
		int cnt = 3;
		int step = 60;
		int start = 2000;

		FROM_TO(start, start + cnt * step - 1, step) {
			create_enemy3c(-20 + (VIEWPORT_H+20)*I, 1000, Fairy, stage3_chargefairy, 30 + VIEWPORT_H/2.0*I, 60, -1*I);
			create_enemy3c(VIEWPORT_W+20 + (VIEWPORT_H+20)*I, 1000, Fairy, stage3_chargefairy, VIEWPORT_W-30 + VIEWPORT_H/2.0*I, 60, -1*I);
		}
	}

	AT(2400) {
		double offs = -50;

		cmplx p1 = 0+0*I;
		cmplx p2 = VIEWPORT_W+0*I;
		cmplx p3 = VIEWPORT_W+VIEWPORT_H*I;
		cmplx p4 = 0+VIEWPORT_H*I;

		create_enemy2c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs - offs*I, p2 + offs - offs*I);
		create_enemy2c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs - offs*I, p3 + offs + offs*I);
		create_enemy2c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs + offs*I, p4 - offs + offs*I);
		create_enemy2c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs + offs*I, p1 - offs - offs*I);
	}

	if(global.diff > D_Normal) AT(2490) {
		double offs = -50;

		cmplx p1 = VIEWPORT_W/2+0*I;
		cmplx p2 = VIEWPORT_W+VIEWPORT_H/2*I;
		cmplx p3 = VIEWPORT_W/2+VIEWPORT_H*I;
		cmplx p4 = 0+VIEWPORT_H/2*I;

		create_enemy2c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs*I, p2 + offs);
		create_enemy2c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs,   p3 + offs*I);
		create_enemy2c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs*I, p4 - offs);
		create_enemy2c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs,   p1 + offs*I);
	}

	int midboss_time = STAGE3_MIDBOSS_TIME;

	AT(2740) {
		global.boss = stage3_create_midboss();
	}

	AT(2741) {
		if(global.boss) {
			global.timer += global.frames - global.boss->birthtime - 1;
		}
	}

	FROM_TO(2741, 2740 + midboss_time - 60, 10) {
		if(_i&1) {
			create_enemy3c(-20 + (VIEWPORT_H-20)*I, 50, Swirl, stage3_bitchswirl, 5, -1*I, 25+0.95*I);
		} else {
			create_enemy3c(VIEWPORT_W+20 + (VIEWPORT_H-20)*I, 50, Swirl, stage3_bitchswirl, -5, -1*I, 25+0.95*I);
		}
	}

	FROM_TO(2740 + midboss_time, 3000 + midboss_time, 10+2*(D_Lunatic-global.diff)) {
		tsrand_fill(3);
		create_enemy2c(VIEWPORT_W/2 + 20 * anfrand(0) + (VIEWPORT_H/4 + 20 * anfrand(1))*I, 200, Swirl, stage3_enterswirl, I * 3 + anfrand(2) * 3, 1);
	}

	AT(3000 + midboss_time) {
		create_enemy2c(VIEWPORT_W - VIEWPORT_W/3 + (VIEWPORT_H/5)*I, 8000, BigFairy, stage3_bigfairy, 2, 480 - 30 * (D_Lunatic - global.diff));
	}

	if(global.diff > D_Easy) {
		FROM_TO(3000 + midboss_time, 3100 + midboss_time, 20+4*(D_Lunatic-global.diff)) {
			create_enemy3c(VIEWPORT_W-20 + (VIEWPORT_H+20)*I, 50, Swirl, stage3_bitchswirl, -5*I, 0, 0);
		}
	}

	AT(3500 + midboss_time) {
		create_enemy2c(VIEWPORT_W/3 + (VIEWPORT_H/5)*I, 8000, BigFairy, stage3_bigfairy, 2, 480 - 30 * (D_Lunatic - global.diff));
	}

	{
		int cnt = 4;
		int step = 60;
		FROM_TO(3500 + midboss_time, 3500 + cnt * step - 1 + midboss_time, step) {
			cmplx pos = VIEWPORT_W - VIEWPORT_W/3 + (VIEWPORT_H/5)*I;
			cmplx spos = creal(pos) - 20 * I;
			create_enemy3c(spos, 1000, Fairy, stage3_chargefairy, pos, 30, 2*I);
		}
	}

	{
		int cnt = 4;
		int step = 70;
		int start = 4000 + midboss_time;
		FROM_TO(start, start + step * cnt - 1, step) {
			int i = _i % cnt;
			double span = 300 - 60 * (1-i/2);
			cmplx pos1 = VIEWPORT_W/2;
			cmplx pos2 = VIEWPORT_W/2 + span * (-0.5 + (i&1)) + (VIEWPORT_H/3 + 100*(i/2))*I;
			create_enemy4c(pos1, 3000, BigFairy, stage3_burstfairy, pos2, i&1, 0,
				(start + 300 - global.timer) + I *
				(30)
			);
		}
	}

	FROM_TO(3700 + midboss_time, 3800 + midboss_time, 20+4*(D_Lunatic-global.diff)) {
		create_enemy3c(20 + (VIEWPORT_H+20)*I, 50, Swirl, stage3_bitchswirl, -5*I, 0, 0);
	}

	AT(4330 + midboss_time) {
		double offs = -50;

		cmplx p1 = 0+0*I;
		cmplx p2 = VIEWPORT_W+0*I;
		cmplx p3 = VIEWPORT_W+VIEWPORT_H*I;
		cmplx p4 = 0+VIEWPORT_H*I;

		create_enemy3c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs - offs*I, p2 + offs - offs*I, 1);
		create_enemy3c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs - offs*I, p3 + offs + offs*I, 1);
		create_enemy3c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs + offs*I, p4 - offs + offs*I, 1);
		create_enemy3c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs + offs*I, p1 - offs - offs*I, 1);
	}

	if(global.diff > D_Normal) AT(4480 + midboss_time) {
		double offs = -50;

		cmplx p1 = VIEWPORT_W/2+0*I;
		cmplx p2 = VIEWPORT_W+VIEWPORT_H/2*I;
		cmplx p3 = VIEWPORT_W/2+VIEWPORT_H*I;
		cmplx p4 = 0+VIEWPORT_H/2*I;

		create_enemy3c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs*I, p2 + offs, 0);
		create_enemy3c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs,   p3 + offs*I, 0);
		create_enemy3c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs*I, p4 - offs, 0);
		create_enemy3c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs,   p1 + offs*I, 0);
	}

	FROM_TO(4760 + midboss_time, 4940 + midboss_time, 10+2*(D_Lunatic-global.diff)) {
		create_enemy3c(VIEWPORT_W-20 - 20.0*I, 50, Swirl, stage3_bitchswirl, 5*I, 0, 0);
		create_enemy3c(20 + -20.0*I, 50, Swirl, stage3_bitchswirl, 5*I, 0, 0);
	}

	AT(5300 + midboss_time) {
		stage_unlock_bgm("stage3");
		global.boss = stage3_create_boss();
	}

	AT(5400 + midboss_time) {
		stage_unlock_bgm("stage3boss");
		stage3_dialog_post_boss();
	}

	AT(5405 + midboss_time) {
		stage_finish(GAMEOVER_SCORESCREEN);
	}
}
