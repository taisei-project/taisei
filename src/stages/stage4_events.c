/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage4_events.h"
#include "stage4.h"
#include "stage6_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"
#include "laser.h"
#include "common_tasks.h"

PRAGMA(message "Remove when this stage is modernized")
DIAGNOSTIC(ignored "-Wdeprecated-declarations")

void kurumi_spell_bg(Boss*, int);
void kurumi_slaveburst(Boss*, int);
void kurumi_redspike(Boss*, int);
void kurumi_aniwall(Boss*, int);
void kurumi_blowwall(Boss*, int);
void kurumi_danmaku(Boss*, int);
void kurumi_extra(Boss*, int);

TASK(boss_appear_stub, NO_ARGS) {
	log_warn("FIXME");
}

static void stage4_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Stage4PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage4PreBossDialog, pm->dialog->Stage4PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear_stub);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage4boss");
}

static void stage4_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage4PostBossDialog, pm->dialog->Stage4PostBoss);
}

static int stage4_splasher(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 3, ITEM_POWER, 1, ITEM_BOMB, 1);
		return 1;
	}

	e->moving = true;
	e->dir = creal(e->args[0]) < 0;

	FROM_TO(0, 50, 1)
		e->pos += e->args[0]*(1-t/50.0);

	FROM_TO_SND("shot1_loop", 66-6*global.diff, 150, 5-global.diff) {
		tsrand_fill(4);
		PROJECTILE(
			.proto = afrand(0) > 0.5 ? pp_rice : pp_thickrice,
			.pos = e->pos,
			.color = RGB(0.8,0.3-0.1*afrand(1),0.5),
			.rule = accelerated,
			.args = {
				e->args[0]/2+(1-2*afrand(2))+(1-2*afrand(3))*I,
				0.02*I
			}
		);
	}

	FROM_TO(200, 300, 1)
		e->pos -= creal(e->args[0])*(t-200)/100.0;

	return 1;
}

static int stage4_fodder(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POWER, 1);
		return 1;
	}

	if(creal(e->args[0]) != 0)
		e->moving = true;
	e->dir = creal(e->args[0]) < 0;
	e->pos += e->args[0];

	FROM_TO(10, 200, 120) {
		cmplx fairy_halfsize = 21 * (1 + I);

		if(!rect_rect_intersect(
			(Rect) { e->pos - fairy_halfsize, e->pos + fairy_halfsize },
			(Rect) { 0, CMPLX(VIEWPORT_W, VIEWPORT_H) },
			true, true)
		) {
			return 1;
		}

		play_sound_ex("shot3", 5, false);
		cmplx aim = global.plr.pos - e->pos;
		aim /= cabs(aim);

		float speed = 3;
		float boost_factor = 1.2;
		float boost_base = 1;
		int chain_len = global.diff + 2;

		PROJECTILE(
			.proto = pp_wave,
			.pos = e->pos,
			.color = RGB(1, 0.3, 0.5),
			.rule = asymptotic,
			.args = {
				speed * aim,
				boost_base + (chain_len + 1) * boost_factor,
			},
			.max_viewport_dist = 32,
		);

		for(int i = chain_len; i; --i) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = e->pos,
				.color = RGB(i / (float)(chain_len+1), 0.3, 0.5),
				.rule = asymptotic,
				.args = {
					speed * aim,
					boost_base + i * boost_factor,
				},
				.max_viewport_dist = 32,
			);
		}

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0, 0.3, 0.5),
			.rule = asymptotic,
			.args = {
				speed * aim,
				boost_base,
			},
			.max_viewport_dist = 32,
		);
	}

	return 1;
}

static int stage4_partcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 3);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(30,60,1) {
		e->args[0] *= 0.9;
	}

	FROM_TO(60,76,1) {
		int i;
		for(i = 0; i < global.diff; i++) {
			play_sound("shot2");
			cmplx n = cexp(I*M_PI/16.0*_i + I*carg(e->args[0])-I*M_PI/4.0 + 0.01*I*i*(1-2*(creal(e->args[0]) > 0)));
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + (30)*n,
				.color = RGB(1-0.2*i,0.5,0.7),
				.rule = asymptotic,
				.args = { 2*n, 2+2*i }
			);
		}
	}

	FROM_TO(160, 200, 1)
		e->args[0] += 0.05*I;

	return 1;
}

static int stage4_cardbuster(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 3, ITEM_POWER, 1);
		return 1;
	}

	FROM_TO(0, 120, 1)
		e->pos += (e->args[0]-e->pos0)/120.0;

	FROM_TO(200, 300, 1)
		e->pos += (e->args[1]-e->args[0])/100.0;

	FROM_TO(400, 600, 1)
		e->pos += (e->args[2]-e->args[1])/200.0;

	int c = 40;
	cmplx n = cexp(I*carg(global.plr.pos - e->pos) + 4*M_PI/(c+1)*I*_i);

	FROM_TO_SND("shot1_loop", 60, 60+c*global.diff, 1) {
		for(int i = 0; i < 3; ++i) {
			PROJECTILE(
				.proto = pp_card,
				.pos = e->pos + 30*n,
				.color = RGB(0, 0.2, 0.4 + 0.2 * i),
				.rule = accelerated,
				.args = { (0.8+0.2*global.diff)*(1.0 + 0.1 * i)*n, 0.01*(1+0.01*_i)*n }
			);
		}
	}

	FROM_TO_SND("shot1_loop", 240, 260+20*global.diff, 1) {
		PROJECTILE(
			.proto = pp_card,
			.pos = e->pos + 30*n,
			.color = RGB(0, 0.7, 0.5),
			.rule = asymptotic,
			.args = { (0.8+0.2*global.diff)*n, 0.4*I }
		);
	}

	return 1;
}

static int stage4_backfire(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5);
		return 1;
	}

	FROM_TO(0,20,1)
		e->args[0] -= 0.05*I;

	FROM_TO(60,100,1)
		e->args[0] += 0.05*I;

	if(t > 100)
		e->args[0] -= 0.02*I;


	e->pos += e->args[0];

	FROM_TO(20,180+global.diff*20,2) {
		play_sound("shot2");
		cmplx n = cexp(I*M_PI*frand()-I*copysign(M_PI/2.0, creal(e->args[0])));
		for(int i = 0; i < global.diff; i++) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = RGB(0.2, 0.2, 1-0.2*i),
				.rule = asymptotic,
				.args = { 2*n, 2+2*i }
			);
		}
	}

	return 1;
}

static int stage4_bigcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 3, ITEM_POWER, 1);

		return 1;
	}

	FROM_TO(0, 70, 1)
		e->pos += e->args[0];

	FROM_TO(200, 300, 1)
		e->pos -= e->args[0];


	FROM_TO(80,100+30*global.diff,20) {
		play_sound("shot_special1");
		int i;
		int n = 10+3*global.diff;
		for(i = 0; i < n; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = e->pos,
				.color = RGBA(0, 0.8 - 0.4 * _i, 0, 0),
				.rule = asymptotic,
				.args = {
					2*cexp(2.0*I*M_PI/n*i+I*3*_i),
					3*sin(6*M_PI/n*i)
				},
			);

			if(global.diff > D_Easy) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos,
					.color = RGBA(0, 0.3 * _i, 0.4, 0),
					.rule = asymptotic,
					.args = {
						(1.5+global.diff*0.2)*cexp(I*3*(i+frand())),
						I*5*sin(6*M_PI/n*i)
					},
				);
			}
		}
	}
	return 1;
}

static int stage4_explosive(Enemy *e, int t) {
	if(t == EVENT_KILLED || (t >= 100 && global.diff >= D_Normal)) {
		int i;

		if(t == EVENT_KILLED)
			spawn_items(e->pos, ITEM_POWER, 1);

		int n = 10*global.diff;
		cmplx phase = global.plr.pos-e->pos;
		phase /= cabs(phase);

		for(i = 0; i < n; i++) {
			double angle = 2*M_PI*i/n+carg(phase);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGB(0.1+0.6*(i&1), 0.2, 1-0.6*(i&1)),
				.rule = accelerated,
				.args = {
					1.5*(1.1+0.3*global.diff)*cexp(I*angle),
					0.001*cexp(I*angle)
				}
			);
		}

		play_sound("shot1");
		return ACTION_DESTROY;
	}

	e->pos += e->args[0];

	return 1;
}

static void KurumiSlave(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	if(!(t%2)) {
		cmplx offset = (frand()-0.5)*30;
		offset += (frand()-0.5)*20.0*I;
		PARTICLE(
			.sprite = "smoothdot",
			.pos = offset,
			.color = RGBA(0.3, 0.0, 0.0, 0.0),
			.draw_rule = Shrink,
			.rule = enemy_flare,
			.timeout = 50,
			.args = { (-50.0*I-offset)/50.0, add_ref(e) },
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}
}

static void kurumi_intro(Boss *b, int t) {
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);
}

static int kurumi_burstslave(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH)
		e->args[1] = e->args[0];
	AT(EVENT_DEATH) {
		free_ref(e->args[2]);
		return 1;
	}


	if(t == 600 || REF(e->args[2]) == NULL)
		return ACTION_DESTROY;

	e->pos += 2*e->args[1]*(sin(t/10.0)+1.5);

	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;

		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + i*10.0*I*e->args[0],
				.color = RGB(r,0,0),
				.rule = accelerated,
				.args = { i*2.0*I*e->args[0], -0.01*e->args[1] }
			);
		}

		play_sound("shot1");
	}

	FROM_TO(40, 100,1) {
		e->args[1] -= e->args[0]*0.02;
		e->args[1] *= cexp(0.02*I);
	}

	return 1;
}

void kurumi_slaveburst(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time == EVENT_DEATH)
		enemy_kill_all(&global.enemies);

	if(time < 0)
		return;

	AT(0) {
		int i;
		int n = 3+2*global.diff;
		for(i = 0; i < n; i++) {
			create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_burstslave, cexp(I*2*M_PI/n*i+0.2*I*time/500), 0, add_ref(b));
		}
	}
}

static int kurumi_spikeslave(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH)
		e->args[1] = e->args[0];
	AT(EVENT_DEATH) {
		free_ref(e->args[2]);
		return 1;
	}


	if(t == 300+50*global.diff || REF(e->args[2]) == NULL)
		return ACTION_DESTROY;

	e->pos += e->args[1];
	e->args[1] *= cexp(0.01*I*e->args[0]);

	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;

		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos + 10.0*I*e->args[0],
				.color = RGB(r,0,0),
				.rule = linear,
				.args = { i*1.5*I*e->args[1] }
			);
		}

		play_sound("shot1");
	}

	return 1;
}

void kurumi_redspike(Boss *b, int time) {
	int t = time % 500;

	if(time == EVENT_DEATH)
		enemy_kill_all(&global.enemies);

	if(time < 0)
		return;

	TIMER(&t);

	FROM_TO(0, 500, 60) {
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_spikeslave, 1-2*(_i&1), 0, add_ref(b));
	}

	if(global.diff < D_Hard) {
		FROM_TO(0, 500, 150-50*global.diff) {
			int i;
			int n = global.diff*8;
			for(i = 0; i < n; i++) {
				PROJECTILE(
					.proto = pp_bigball,
					.pos = b->pos,
					.color = RGBA(1.0, 0.0, 0.0, 0.0),
					.rule = asymptotic,
					.args = {
						(1+0.1*(global.diff == D_Normal))*3*cexp(2.0*I*M_PI/n*i+I*carg(global.plr.pos-b->pos)),
						3
					},
				);
			}

			play_sound("shot_special1");
		}
	} else {
		AT(80) {
			aniplayer_queue(&b->ani, "muda", 0);
		}

		AT(499) {
			aniplayer_queue(&b->ani, "main", 0);
		}

		FROM_TO_INT(80, 500, 40,200,2+2*(global.diff == D_Hard)) {
			tsrand_fill(2);
			cmplx offset = 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1));
			cmplx n = cexp(I*carg(global.plr.pos-b->pos-offset));
			PROJECTILE(
				.proto = pp_rice,
				.pos = b->pos+offset,
				.color = RGBA(1, 0, 0, 0),
				.rule = accelerated,
				.args = { -1*n, 0.05*n },
			);
			play_sound_ex("warp",0,false);
		}
	}
}

void kurumi_spell_bg(Boss *b, int time) {
	float f = 0.5+0.5*sin(time/80.0);

	r_mat_mv_push();
	r_mat_mv_translate(VIEWPORT_W/2, VIEWPORT_H/2,0);
	r_mat_mv_scale(0.6, 0.6, 1);
	r_color3(f, 1 - f, 1 - f);
	draw_sprite(0, 0, "stage4/kurumibg1");
	r_mat_mv_pop();
	r_color4(1, 1, 1, 0);
	fill_viewport(time/300.0, time/300.0, 0.5, "stage4/kurumibg2");
	r_color4(1, 1, 1, 1);
}

static void kurumi_outro(Boss *b, int time) {
	b->pos += -5-I;
}

static void kurumi_global_rule(Boss *b, int time) {
	// FIXME: avoid running this every frame!

	if(b->current && ATTACK_IS_SPELL(b->current->type)) {
		b->shadowcolor = *RGBA_MUL_ALPHA(0.0, 0.4, 0.5, 0.5);
	} else {
		b->shadowcolor = *RGBA_MUL_ALPHA(1.0, 0.1, 0.0, 0.5);
	}
}

Boss* stage4_spawn_kurumi(cmplx pos) {
	Boss* b = create_boss("Kurumi", "kurumi", pos);
	boss_set_portrait(b, "kurumi", NULL, "normal");
	b->glowcolor = *RGB(0.5, 0.1, 0.0);
	b->global_rule = kurumi_global_rule;
	return b;
}

static Boss* create_kurumi_mid(void) {
	Boss *b = stage4_spawn_kurumi(VIEWPORT_W/2-400.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 2, 0, kurumi_intro, NULL);
	boss_add_attack_from_info(b, &stage4_spells.mid.gate_of_walachia, false);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.mid.dry_fountain, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.mid.red_spike, false);
	}
	boss_add_attack(b, AT_Move, "Outro", 2, 1, kurumi_outro, NULL);
	boss_start_attack(b, b->attacks);
	return b;
}

static int splitcard(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t == creal(p->args[2])) {
		// projectile_set_prototype(p, pp_bigball);
		p->color = *RGB(p->color.b, 0.2, p->color.g);
		play_sound_ex("redirect", 10, false);
		spawn_projectile_highlight_effect(p);
	}

	if(t > creal(p->args[2])) {
		p->args[0] += 0.01*p->args[3];
		asymptotic(p, t);
	} else {
		p->angle = carg(p->args[0]);
	}

	return ACTION_NONE; // asymptotic(p, t);
}

static int stage4_supercard(Enemy *e, int t) {
	int time = t % 150;

	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 5);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(50, 70, 1) {
		e->args[0] *= 0.7;
	}

	if(t > 450) {
		e->pos -= I;
		return 1;
	}

	__timep = &time;

	FROM_TO(70, 70+20*global.diff, 1) {
		play_sound_ex("shot1",5,false);

		cmplx n = cexp(I*(2*M_PI/20.0*_i + (t / 150) * M_PI/4));
		for(int i = -1; i <= 1 && t; i++) {
			PROJECTILE(
				.proto = pp_card,
				.pos = e->pos + 30*n,
				.color = RGB(0,0.4,1-_i/40.0),
				.rule = splitcard,
				.args = {1*n, 0.1*_i, 100-time+70, 2*I*i*n}
			);
		}
	}

	return 1;
}

static void kurumi_boss_intro(Boss *b, int t) {
	TIMER(&t);
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.015);

	AT(120)
		stage4_dialog_pre_boss();
}

static int splitcard_elly(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t == creal(p->args[2])) {
		p->args[0]+=p->args[3];
		// p->color = derive_color(p->color, CLRMASK_B, RGB(0,0,-color_component(p->color,CLR_B)));
		// FIXME: what was even the intention here?
		p->color.b *= -1;
		play_sound_ex("redirect", 10, false);
		spawn_projectile_highlight_effect(p);
	}

	return asymptotic(p, t);
}

static void kurumi_breaker(Boss *b, int time) {
	int t = time % 400;
	int i;

	int c = 10+global.diff*2;
	int kt = 20;

	if(time < 0)
		return;

	GO_TO(b, VIEWPORT_W/2 + VIEWPORT_W/3*sin(time/220) + I*cimag(b->pos), 0.02);

	TIMER(&t);

	FROM_TO_SND("shot1_loop", 50, 400, 50-7*global.diff) {
		cmplx p = b->pos + 150*sin(_i) + 100.0*I*cos(_i);

		for(i = 0; i < c; i++) {
			cmplx n = cexp(2.0*I*M_PI/c*i);
			PROJECTILE(
				.proto = pp_rice,
				.pos = p,
				.color = RGB(1,0,0.5),
				.rule = splitcard_elly,
				.args = {
					3*n,
					0,
					kt,
					1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-2.6*n
			});
		}
	}

	FROM_TO(60, 400, 100) {
		play_sound("shot_special1");
		aniplayer_queue(&b->ani,"muda",1);
		aniplayer_queue(&b->ani,"main",0);
		for(i = 0; i < 20; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.5, 0.0, 0.5, 0.0),
				.rule = asymptotic,
				.args = { cexp(2.0*I*M_PI/20.0*i), 3 },
			);
		}
	}
}

static int aniwall_bullet(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t > creal(p->args[1])) {
		if(global.diff > D_Normal) {
			tsrand_fill(2);
			p->args[0] += 0.1*(0.1-0.2*afrand(0) + 0.1*I-0.2*I*afrand(1))*(global.diff-2);
			p->args[0] += 0.002*cexp(I*carg(global.plr.pos - p->pos));
		}

		p->pos += p->args[0];
	}

	p->color.r = cimag(p->pos)/VIEWPORT_H;
	return ACTION_NONE;
}

static int aniwall_slave(Enemy *e, int t) {
	float re, im;

	if(t < 0)
		return 1;

	if(creal(e->pos) <= 0)
		e->pos = I*cimag(e->pos);
	if(creal(e->pos) >= VIEWPORT_W)
		e->pos = VIEWPORT_W + I*cimag(e->pos);
	if(cimag(e->pos) <= 0)
		e->pos = creal(e->pos);
	if(cimag(e->pos) >= VIEWPORT_H)
		e->pos = creal(e->pos) + I*VIEWPORT_H;

	re = creal(e->pos);
	im = cimag(e->pos);

	if(cabs(e->args[1]) <= 0.1) {
		if(re == 0 || re == VIEWPORT_W) {

			e->args[1] = 1;
			e->args[2] = 10.0*I;
		}

		e->pos += e->args[0]*t;
	} else {
		if((re <= 0) + (im <= 0) + (re >= VIEWPORT_W) + (im >= VIEWPORT_H) == 2) {
			float sign = 1;
			sign *= 1-2*(re > 0);
			sign *= 1-2*(im > 0);
			sign *= 1-2*(cimag(e->args[2]) == 0);
			e->args[2] *= sign*I;
		}

		e->pos += e->args[2];

		if(!(t % 7-global.diff-2*(global.diff > D_Normal))) {
			cmplx v = e->args[2]/cabs(e->args[2])*I*sign(creal(e->args[0]));
			if(cimag(v) > -0.1 || global.diff >= D_Normal) {
				play_sound("shot1");
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos+I*v*20*nfrand(),
					.color = RGB(1,0,0),
					.rule = aniwall_bullet,
					.args = { 1*v, 40 }
				);
			}
		}
	}

	return 1;
}

static void KurumiAniWallSlave(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	if(e->args[1]) {
		PARTICLE(
			.sprite = "smoothdot",
			.pos = e->pos,
			.color = RGBA(1, 1, 1, 0),
			.draw_rule = Fade,
			.timeout = 30,
		);
	}
}

void kurumi_aniwall(Boss *b, int time) {
	TIMER(&time);

	AT(EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
	}

	GO_TO(b, VIEWPORT_W/2 + VIEWPORT_W/3*sin(time/200) + I*cimag(b->pos),0.03)

	if(time < 0)
		return;

	AT(0) {
		aniplayer_queue(&b->ani, "muda", 0);
		play_sound("laser1");
		create_lasercurve2c(b->pos, 50, 80, RGBA(1.0, 0.8, 0.8, 0.0), las_accel, 0, 0.2*cexp(0.4*I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, aniwall_slave, 0.2*cexp(0.4*I));
		create_lasercurve2c(b->pos, 50, 80, RGBA(1.0, 0.8, 0.8, 0.0), las_accel, 0, 0.2*cexp(I*M_PI - 0.4*I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, aniwall_slave, 0.2*cexp(I*M_PI - 0.4*I));
	}
}

static void kurumi_sbreaker(Boss *b, int time) {
	if(time < 0)
		return;

	int dur = 300+50*global.diff;
	int t = time % dur;
	int i;
	TIMER(&t);

	int c = 10+global.diff*2;
	int kt = 40;

	FROM_TO_SND("shot1_loop", 50, dur, 2+(global.diff < D_Hard)) {
		cmplx p = b->pos + 150*sin(_i/8.0)+100.0*I*cos(_i/15.0);

		cmplx n = cexp(2.0*I*M_PI/c*_i);
		PROJECTILE(
			.proto = pp_rice,
			.pos = p,
			.color = RGB(1.0, 0.0, 0.5),
			.rule = splitcard_elly,
			.args = {
				2*n,
				0,
				kt,
				1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-1.7*n
			}
		);
	}

	FROM_TO(60, dur, 100) {
		play_sound("shot_special1");
		aniplayer_queue(&b->ani, "muda", 4);
		aniplayer_queue(&b->ani, "main", 0);

		for(i = 0; i < 20; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.5, 0.0, 0.5, 0.0),
				.rule = asymptotic,
				.args = { cexp(2.0*I*M_PI/20.0*i), 3 },
			);
		}
	}
}

static int blowwall_slave(Enemy *e, int t) {
	float re, im;

	if(t < 0)
		return 1;

	e->pos += e->args[0]*t;

	if(creal(e->pos) <= 0)
		e->pos = I*cimag(e->pos);
	if(creal(e->pos) >= VIEWPORT_W)
		e->pos = VIEWPORT_W + I*cimag(e->pos);
	if(cimag(e->pos) <= 0)
		e->pos = creal(e->pos);
	if(cimag(e->pos) >= VIEWPORT_H)
		e->pos = creal(e->pos) + I*VIEWPORT_H;

	re = creal(e->pos);
	im = cimag(e->pos);

	if(re <= 0 || im <= 0 || re >= VIEWPORT_W || im >= VIEWPORT_H) {
		int i, c;
		float f;
		ProjPrototype *type;

		c = 20 + global.diff*40;

		for(i = 0; i < c; i++) {
			f = frand();

			if(f < 0.3) {
				type = pp_soul;
			} else if(f < 0.6) {
				type = pp_bigball;
			} else {
				type = pp_plainball;
			}

			PROJECTILE(
				.proto = type,
				.pos = e->pos,
				.color = RGBA(1.0, 0.1, 0.1, 0.0),
				.rule = asymptotic,
				.args = { (1+3*f)*cexp(2.0*I*M_PI*frand()), 4 },
			);
		}

		play_sound("shot_special1");
		return ACTION_DESTROY;
	}

	return 1;
}

static void bwlaser(Boss *b, float arg, int slave) {
	create_lasercurve2c(b->pos, 50, 100, RGBA(1.0, 0.5+0.3*slave, 0.5+0.3*slave, 0.0), las_accel, 0, (0.1+0.1*slave)*cexp(I*arg));

	if(slave) {
		play_sound("laser1");
		create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, blowwall_slave, 0.2*cexp(I*arg));
	} else {
		// FIXME: needs a better sound
		play_sound("shot2");
		play_sound("shot_special1");
	}
}

void kurumi_blowwall(Boss *b, int time) {
	int t = time % 600;
	TIMER(&t);

	if(time == EVENT_DEATH)
		enemy_kill_all(&global.enemies);

	if(time < 0) {
		return;
	}

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.04)

	AT(0) {
		aniplayer_queue(&b->ani,"muda",0);
	}

	AT(50)
		bwlaser(b, 0.4, 1);

	AT(100)
		bwlaser(b, M_PI-0.4, 1);

	FROM_TO(200, 300, 50)
		bwlaser(b, -M_PI*frand(), 1);

	FROM_TO(300, 500, 10)
		bwlaser(b, M_PI/10*_i, 0);

}

static Projectile* vapor_particle(cmplx pos, const Color *clr) {
	return PARTICLE(
		.sprite = "stain",
		.color = clr,
		.timeout = 60,
		.draw_rule = ScaleFade,
		.args = { 0, 0, 0.2 + 2.0*I },
		.pos = pos,
		.angle = M_PI*2*frand(),
	);
}

static int kdanmaku_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	int time = creal(p->args[0]);

	if(t == time) {
		p->color = *RGBA(0.6, 0.3, 1.0, 0.0);
		projectile_set_prototype(p, pp_bullet);
		p->args[1] = (global.plr.pos - p->pos) * 0.001;

		if(frand() < 0.5) {
			Projectile *v = vapor_particle(p->pos, color_mul_scalar(COLOR_COPY(&p->color), 0.5));

			if(frand() < 0.5) {
				v->flags |= PFLAG_REQUIREDPARTICLE;
			}
		}

		PARTICLE(
			.sprite = "flare",
			.color = RGB(1, 1, 1),
			.timeout = 30,
			.draw_rule = ScaleFade,
			.args = { 0, 0, 3.0 },
			.pos = p->pos,
		);

		play_sound("shot3");
	}

	if(t > time && cabs(p->args[1]) < 2) {
		p->args[1] *= 1.02;
		fapproach_p(&p->color.a, 1, 0.025);
	}

	p->pos += p->args[1];
	p->angle = carg(p->args[1]);

	return ACTION_NONE;
}

static int kdanmaku_slave(Enemy *e, int t) {
	float re;

	if(t < 0)
		return 1;

	if(!e->args[1])
		e->pos += e->args[0]*t;
	else
		e->pos += 5.0*I;

	if(creal(e->pos) <= 0)
		e->pos = I*cimag(e->pos);
	if(creal(e->pos) >= VIEWPORT_W)
		e->pos = VIEWPORT_W + I*cimag(e->pos);

	re = creal(e->pos);

	if(re <= 0 || re >= VIEWPORT_W)
		e->args[1] = 1;

	if(cimag(e->pos) >= VIEWPORT_H)
		return ACTION_DESTROY;

	if(e->args[2] && e->args[1]) {
		int i, n = 3+imax(D_Normal,global.diff);
		float speed = 1.5+0.1*global.diff;

		for(i = 0; i < n; i++) {
			cmplx p = VIEWPORT_W/(float)n*(i+psin(t*t*i*i+t*t)) + I*cimag(e->pos);
			if(cabs(p-global.plr.pos) > 60) {
				PROJECTILE(
					.proto = pp_thickrice,
					.pos = p,
					.color = RGBA(1.0, 0.5, 0.5, 0.0),
					.rule = kdanmaku_proj,
					.args = { 160, speed*0.5*cexp(2.0*I*M_PI*sin(245*t+i*i*3501)) },
					.flags = PFLAG_NOSPAWNFLARE,
				);

				if(frand() < 0.5) {
					vapor_particle(p, RGBA(0.5, 0.125 * frand(), 0.125 * frand(), 0.1));
				}
			}
		}
		play_sound_ex("redirect", 3, false);
	}

	return 1;
}

void kurumi_danmaku(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time == EVENT_DEATH)
		enemy_kill_all(&global.enemies);
	if(time < 0)
		return;

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.04)

	AT(260) {
		aniplayer_queue(&b->ani,"muda",4);
		aniplayer_queue(&b->ani,"main",0);
	}

	AT(50) {
		play_sound("laser1");
		create_lasercurve2c(b->pos, 50, 100, RGBA(1.0, 0.8, 0.8, 0.0), las_accel, 0, 0.2*cexp(I*carg(-b->pos)));
		create_lasercurve2c(b->pos, 50, 100, RGBA(1.0, 0.8, 0.8, 0.0), las_accel, 0, 0.2*cexp(I*carg(VIEWPORT_W-b->pos)));
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, kdanmaku_slave, 0.2*cexp(I*carg(-b->pos)), 0, 1);
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, kdanmaku_slave, 0.2*cexp(I*carg(VIEWPORT_W-b->pos)), 0, 0);
	}
}

static void kurumi_extra_shield_pos(Enemy *e, int time) {
	double dst = 75 + 100 * fmax((60 - time) / 60.0, 0);
	double spd = cimag(e->args[0]) * fmin(time / 120.0, 1);
	e->args[0] += spd;
	e->pos = global.boss->pos + dst * cexp(I*creal(e->args[0]));
}

static bool kurumi_extra_shield_expire(Enemy *e, int time) {
	if(time > creal(e->args[1])) {
		e->hp = 0;
		return true;
	}

	return false;
}

static int kurumi_extra_dead_shield_proj(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	p->color = *color_lerp(
		RGBA(2.0, 0.0, 0.0, 0.0),
		RGBA(0.2, 0.1, 0.5, 0.0),
	fmin(time / 60.0f, 1.0f));

	return asymptotic(p, time);
}

static int kurumi_extra_dead_shield(Enemy *e, int time) {
	if(time < 0) {
		return 1;
	}

	if(!(time % 6)) {
		// complex dir = cexp(I*(M_PI * 0.5 * nfrand() + carg(global.plr.pos - e->pos)));
		// complex dir = cexp(I*(carg(global.plr.pos - e->pos)));
		cmplx dir = cexp(I*creal(e->args[0]));
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.rule = kurumi_extra_dead_shield_proj,
			.args = { 2*dir, 10 }
		);
		play_sound("shot1");
	}

	time += cimag(e->args[1]);

	kurumi_extra_shield_pos(e, time);

	if(kurumi_extra_shield_expire(e, time)) {
		int cnt = 10;
		for(int i = 0; i < cnt; ++i) {
			cmplx dir = cexp(I*M_PI*2*i/(double)cnt);
			tsrand_fill(2);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.rule = kurumi_extra_dead_shield_proj,
				.args = { 1.5 * (1 + afrand(0)) * dir, 4 + anfrand(1) },
			);
		}

		play_sound("boom");
	}

	return 1;
}

static int kurumi_extra_shield(Enemy *e, int time) {
	if(time == EVENT_DEATH) {
		if(global.boss && !(global.gameover > 0) && !boss_is_dying(global.boss) && e->args[2] == 0) {
			create_enemy2c(e->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_extra_dead_shield, e->args[0], e->args[1]);
		}
		return 1;
	}

	if(time < 0) {
		return 1;
	}

	e->args[1] = creal(e->args[1]) + time*I;

	kurumi_extra_shield_pos(e, time);
	kurumi_extra_shield_expire(e, time);

	return 1;
}

static int kurumi_extra_bigfairy1(Enemy *e, int time) {
	if(time < 0) {
		return 1;
	}
	TIMER(&time);

	int escapetime = 400+4000*(global.diff == D_Lunatic);
	if(time < escapetime) {
		GO_TO(e, e->args[0], 0.02);
	} else  {
		GO_TO(e, e->args[0]-I*VIEWPORT_H,0.01)
	}

	FROM_TO(50,escapetime,60) {
		int count = 5;
		cmplx phase = cexp(I*2*M_PI*frand());
		for(int i = 0; i < count; i++) {
			cmplx arg = cexp(I*2*M_PI*i/count);
			if(global.diff == D_Lunatic)
				arg *= phase;
			create_lasercurve2c(e->pos, 20, 200, RGBA(1.0, 0.3, 0.7, 0.0), las_accel, arg, 0.1*arg);
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(1.0, 0.3, 0.7),
				.rule = accelerated,
				.args = { arg, 0.1*arg },
			);
		}

		play_sound("laser1");
	}

	return 1;
}

DEPRECATED_DRAW_RULE
static void kurumi_extra_drainer_draw(Projectile *p, int time, ProjDrawRuleArgs args) {
	cmplx org = p->pos;
	cmplx targ = p->args[1];
	double a = 0.5 * creal(p->args[2]);
	Texture *tex = r_texture_get("part/sinewave");

	r_shader_standard();

	r_color4(1.0 * a, 0.5 * a, 0.5 * a, 0);
	loop_tex_line_p(org, targ, 16, time * 0.01, tex);

	r_color4(0.4 * a, 0.2 * a, 0.2 * a, 0);
	loop_tex_line_p(org, targ, 18, time * 0.0043, tex);

	r_color4(0.5 * a, 0.2 * a, 0.5 * a, 0);
	loop_tex_line_p(org, targ, 24, time * 0.003, tex);
}

static int kurumi_extra_drainer(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return ACTION_ACK;
	}

	if(time < 0) {
		return ACTION_ACK;
	}

	Enemy *e = REF(p->args[0]);
	p->pos = global.boss->pos;

	if(e) {
		p->args[1] = e->pos;
		p->args[2] = approach(p->args[2], 1, 0.5);

		if(time > 40 && e->hp > 0) {
			// TODO: maybe add a special sound for this?

			float drain = fmin(4, e->hp);
			ent_damage(&e->ent, &(DamageInfo) { .amount = drain });
			global.boss->current->hp = fmin(global.boss->current->maxhp, global.boss->current->hp + drain * 2);
		}
	} else {
		p->args[2] = approach(p->args[2], 0, 0.5);
		if(!creal(p->args[2])) {
			return ACTION_DESTROY;
		}
	}

	return ACTION_NONE;
}

static void kurumi_extra_create_drainer(Enemy *e) {
	PROJECTILE(
		.size = 1+I,
		.pos = e->pos,
		.rule = kurumi_extra_drainer,
		.draw_rule = kurumi_extra_drainer_draw,
		.args = { add_ref(e) },
		.shader = "sprite_default",
		.flags = PFLAG_NOCLEAR | PFLAG_NOCOLLISION,
	);
}

static void kurumi_extra_swirl_visual(Enemy *e, int time, bool render) {
	if(!render) {
		// why the hell was this here?
		// Swirl(e, time, render);
		return;
	}

	// FIXME: blend
	r_blend(BLEND_SUB);
	Swirl(e, time, render);
	r_blend(BLEND_PREMUL_ALPHA);
}

static void kurumi_extra_fairy_visual(Enemy *e, int time, bool render) {
	if(!render) {
		Fairy(e, time, render);
		return;
	}

	// r_blend(BLEND_ADD);
	r_shader("sprite_negative");
	Fairy(e, time, render);
	r_shader("sprite_default");
	// r_blend(BLEND_ALPHA);
}

static void kurumi_extra_bigfairy_visual(Enemy *e, int time, bool render) {
	if(!render) {
		BigFairy(e, time, render);
		return;
	}

	// r_blend(BLEND_ADD);
	r_shader("sprite_negative");
	BigFairy(e, time, render);
	r_shader("sprite_default");
	// r_blend(BLEND_ALPHA);
}

static int kurumi_extra_fairy(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1);
		return 1;
	}

	if(e->flags & EFLAG_NO_AUTOKILL && t > 50)
		e->flags &= ~EFLAG_NO_AUTOKILL;

	if(creal(e->args[0]-e->pos) != 0)
		e->moving = true;
	e->dir = creal(e->args[0]-e->pos) < 0;

	FROM_TO(0,90,1) {
		GO_TO(e,e->args[0],0.1)
	}

	/*FROM_TO(100, 200, 22-global.diff*3) {
		PROJECTILE("ball", e->pos, RGB(1, 0.3, 0.5), asymptotic, 2*cexp(I*M_PI*2*frand()), 3);
	}*/

	int attacktime = creal(e->args[1]);
	int flytime = cimag(e->args[1]);
	FROM_TO_SND("shot1_loop", attacktime-20,attacktime+20,20) {
		cmplx vel = cexp(I*frand()*2*M_PI)*(2+0.1*(global.diff-D_Easy));
		if(e->args[2] == 0) { // attack type
			int corners = 5;
			double len = 50;
			int count = 5;
			for(int i = 0; i < corners; i++) {
				for(int j = 0; j < count; j++) {
					cmplx pos = len/2/tan(2*M_PI/corners)*I+(j/(double)count-0.5)*len;
					pos *= cexp(I*2*M_PI/corners*i);
					PROJECTILE(
						.proto = pp_flea,
						.pos = e->pos+pos,
						.color = RGB(1, 0.3, 0.5),
						.rule = linear,
						.args = { vel+0.1*I*pos/cabs(pos) }
					);
				}
			}
		} else {
			int count = 30;
			double rad = 20;
			for(int j = 0; j < count; j++) {
				double x = (j/(double)count-0.5)*2*M_PI;
				cmplx pos = 0.5*cos(x)+sin(2*x) + (0.5*sin(x)+cos(2*x))*I;
				pos*=vel/cabs(vel);
				PROJECTILE(
					.proto = pp_flea,
					.pos = e->pos+rad*pos,
					.color = RGB(0.5, 0.3, 1),
					.rule = linear,
					.args = { vel+0.1*pos },
				);
			}
		}
	}
	AT(attacktime) {
		e->args[0] = global.plr.pos-e->pos;
		kurumi_extra_create_drainer(e);
		play_sound("redirect");
	}
	FROM_TO(attacktime,attacktime+flytime,1) {
		e->pos += e->args[0]/flytime;
	}

	FROM_TO(attacktime,attacktime+flytime,20-global.diff*3) {
		if(global.diff>D_Easy) {
			Color *clr = RGBA_MUL_ALPHA(0.1 + 0.07 * _i, 0.3, 1 - 0.05 * _i, 0.8);
			clr->a = 0;

			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = clr,
				.timeout = 50,
			);
		}
	}
	if(t > attacktime + flytime + 20 && global.boss) {
		GO_TO(e,global.boss->pos,0.04)
	}

	return 1;
}

void kurumi_extra(Boss *b, int time) {
	int length = 400;
	int t = time % length;
	int direction = (time/length)%2;

	int castlimit = b->current->maxhp * 0.05;
	int shieldlimit = b->current->maxhp * 0.1;

	TIMER(&t);

	if(time == EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 120)
		GO_TO(b, VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I, 0.01)

	if(t == 0 && b->current->hp >= shieldlimit) {
		int cnt = 12;
		for(int i = 0; i < cnt; ++i) {
			double a = M_PI * 2 * i / (double)cnt;
			int hp = 500;
			create_enemy2c(b->pos, hp, kurumi_extra_swirl_visual, kurumi_extra_shield, a + 0.05*I, 800);
			create_enemy2c(b->pos, hp, kurumi_extra_swirl_visual, kurumi_extra_shield, a - 0.05*I, 800);
		}
	}

	AT(90) {
		int cnt = 20;
		for(int i = 0; i < cnt; i++) {
			b->current->hp -= 600;
			if(b->current->hp < castlimit)
				b->current->hp = castlimit;
			tsrand_fill(2);
			cmplx pos = VIEWPORT_W/2*afrand(0)+I*afrand(1)*VIEWPORT_H*2/3;
			if(direction)
				pos = VIEWPORT_W-creal(pos)+I*cimag(pos);
			// immune so they don’t get killed while they are still offscreen.
			Enemy *fairy = create_enemy3c(pos-300*(1-2*direction),500,kurumi_extra_fairy_visual,kurumi_extra_fairy,pos,100+20*i+100*(1.1-0.05*global.diff)*I,direction);
			fairy->flags |= EFLAG_NO_AUTOKILL;
		}

		// XXX: maybe add a special sound for this?
		play_sound("shot_special1");
	}

	cmplx sidepos = VIEWPORT_W * (0.5+0.3*(1-2*direction)) + VIEWPORT_H * 0.28 * I;
	FROM_TO(90,120,1) {
		GO_TO(b, sidepos,0.1)
	}

	FROM_TO(190,200,1) {
		GO_TO(b, sidepos+30*I,0.1)
	}

	AT(190){
		aniplayer_queue_frames(&b->ani, "muda", 110);
		aniplayer_queue(&b->ani, "main", 0);
	}

	FROM_TO(300,400,1) {
		GO_TO(b,VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I,0.1)
	}

	if(global.diff >= D_Hard) {
		AT(300) {
			double ofs = VIEWPORT_W * 0.5;
			cmplx pos = 0.5 * VIEWPORT_W + I * (VIEWPORT_H - 100);
			cmplx targ = 0.5 *VIEWPORT_W + VIEWPORT_H * 0.3 * I;
			create_enemy1c(pos + ofs, 3300, kurumi_extra_bigfairy_visual, kurumi_extra_bigfairy1, targ + 0.8*ofs);
			create_enemy1c(pos - ofs, 3300, kurumi_extra_bigfairy_visual, kurumi_extra_bigfairy1, targ - 0.8*ofs);
		}
	}
	if((t == length-20 && global.diff == D_Easy)|| b->current->hp < shieldlimit) {
		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->logic_rule == kurumi_extra_shield) {
				e->args[2] = 1; // discharge extra shield
				e->hp = 0;
				continue;
			}
		}
	}
}


static Boss *create_kurumi(void) {
	Boss* b = stage4_spawn_kurumi(-400.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 4, 0, kurumi_boss_intro, NULL);
	boss_add_attack(b, AT_Normal, "Sin Breaker", 25, 33000, kurumi_sbreaker, NULL);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.boss.animate_wall, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.boss.demon_wall, false);
	}
	boss_add_attack(b, AT_Normal, "Cold Breaker", 25, 36000, kurumi_breaker, NULL);
	boss_add_attack_from_info(b, &stage4_spells.boss.bloody_danmaku, false);
	boss_add_attack_from_info(b, &stage4_spells.boss.blow_the_walls, false);
	boss_add_attack_from_info(b, &stage4_spells.extra.vlads_army, false);
	boss_start_attack(b, b->attacks);

	return b;
}

static int scythe_post_mid(Enemy *e, int t) {
	TIMER(&t);

	int fleetime = creal(e->args[3]);

	if(t == EVENT_DEATH) {
		if(fleetime >= 300) {
			spawn_items(e->pos, ITEM_LIFE, 1);
		}

		return 1;
	}

	if(t < 0) {
		return 1;
	}

	AT(fleetime) {
		return ACTION_DESTROY;
	}

	double scale = fmin(1.0, t / 60.0) * (1.0 - clamp((t - (fleetime - 60)) / 60.0, 0.0, 1.0));
	double alpha = scale * scale;
	double spin = (0.2 + 0.2 * (1.0 - alpha)) * 1.5;

	cmplx opos = VIEWPORT_W/2+160*I;
	double targ = (t-300) * (0.5 + psin(t/300.0));
	double w = fmin(0.15, 0.0001*targ);

	cmplx pofs = 150*cos(w*targ+M_PI/2.0) + I*80*sin(2*w*targ);
	pofs += ((VIEWPORT_W/2+VIEWPORT_H/2*I - opos) * (global.diff - D_Easy)) / (D_Lunatic - D_Easy);

	e->pos = opos + pofs * (1.0 - clamp((t - (fleetime - 120)) / 60.0, 0.0, 1.0)) * smooth(smooth(scale));
	e->args[2] = 0.5 * scale + (1.0 - alpha) * I;
	e->args[1] = creal(e->args[1]) + spin * I;

	FROM_TO(90, fleetime - 120, 1) {
		cmplx shotorg = e->pos+80*cexp(I*creal(e->args[1]));
		cmplx shotdir = cexp(I*creal(e->args[1]));

		struct projentry { ProjPrototype *proj; char *snd; } projs[] = {
			{ pp_ball,       "shot1"},
			{ pp_bigball,    "shot1"},
			{ pp_soul,       "shot_special1"},
			{ pp_bigball,    "shot1"},
		};

		struct projentry *pe = &projs[_i % (sizeof(projs)/sizeof(struct projentry))];

		double ca = creal(e->args[1]) + _i/60.0;
		Color *c = RGB(cos(ca), sin(ca), cos(ca+2.1));

		play_sound_ex(pe->snd, 3, true);

		PROJECTILE(
			.proto = pe->proj,
			.pos = shotorg,
			.color = c,
			.rule = asymptotic,
			.args = {
				(1.2-0.1*global.diff)*shotdir,
				5 * sin(t/150.0)
			},
		);
	}

	FROM_TO(fleetime - 120, fleetime, 1) {
		stage_clear_hazards(CLEAR_HAZARDS_ALL);
	}

	scythe_common(e, t);
	return 1;
}

void stage4_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage4");
		stage4_skip(env_get("STAGE4_TEST", 0));
		stage_set_voltage_thresholds(170, 340, 660, 1040);
	}

	AT(70) {
		create_enemy1c(VIEWPORT_H/4*3*I, 3000, BigFairy, stage4_splasher, 3-4.0*I);
		create_enemy1c(VIEWPORT_W + VIEWPORT_H/4*3*I, 3000, BigFairy, stage4_splasher, -3-4.0*I);
	}

	FROM_TO(300, 450, 10) {
		float span = VIEWPORT_W * 0.5;
		float phase = 2*_i/M_PI;

		if(_i & 1) {
			phase *= -1;
		}

		create_enemy1c((VIEWPORT_W + span * sin(phase)) * 0.5 - 32*I, 200, Fairy, stage4_fodder, 2.0*I*cexp(I*phase*0.1));
	}

	FROM_TO(500, 550, 10) {
		int d = _i&1;
		create_enemy1c(VIEWPORT_W*d, 1000, Fairy, stage4_partcircle, 2*cexp(I*M_PI/2.0*(0.2+0.6*frand()+d)));
	}

	FROM_TO(600, 1400, 100) {
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 3000, BigFairy, stage4_cardbuster, VIEWPORT_W/6.0 + VIEWPORT_W/3.0*2*(_i&1)+100.0*I,
					VIEWPORT_W/4.0 + VIEWPORT_W/2.0*((_i+1)&1)+300.0*I, VIEWPORT_W/2.0+VIEWPORT_H*I+200.0*I);
	}

	AT(1800) {
		create_enemy1c(VIEWPORT_H/2.0*I, 2000, Swirl, stage4_backfire, 0.3);
		create_enemy1c(VIEWPORT_W+VIEWPORT_H/2.0*I, 2000, Swirl, stage4_backfire, -0.5);
	}

	FROM_TO(2060, 2600, 15) {
		float span = 300;
		float phase = 2*_i/M_PI;

		if(_i & 1) {
			phase *= -1;
		}

		create_enemy1c(I * span * psin(phase) - 24, 200, Fairy, stage4_fodder, 2.0*cexp(I*phase*0.005));
	}

	FROM_TO(2000, 2400, 200)
		create_enemy1c(VIEWPORT_W/2+200-400*frand(), 1000, BigFairy, stage4_bigcircle, 2.0*I);

	FROM_TO(2600, 3000, 10)
		create_enemy1c(20.0*I+VIEWPORT_H/3*I*frand()+VIEWPORT_W, 100, Swirl, stage4_explosive, -3);

	AT(3200)
		global.boss = create_kurumi_mid();

	int midboss_time = STAGE4_MIDBOSS_MUSIC_TIME;

	AT(3201) {
		if(global.boss) {
			global.timer += fmin(midboss_time, global.frames - global.boss->birthtime) - 1;
		}
	}

	FROM_TO(3201, 3201 + midboss_time - 1, 1) {
		if(!global.enemies.first) {
			create_enemy4c(VIEWPORT_W/2+160.0*I, ENEMY_IMMUNE, Scythe, scythe_post_mid, 0, 1+0.2*I, 0+1*I, 3201 + midboss_time - global.timer);
		}
	}

	FROM_TO(3201 + midboss_time, 3501 + midboss_time, 10) {
		float span = 120;
		float phase = 2*_i/M_PI;
		create_enemy1c(VIEWPORT_W*(_i&1)+span*I*psin(phase), 200, Fairy, stage4_fodder, 2-4*(_i&1)+1.0*I);
	}

	FROM_TO(3350 + midboss_time, 4000 + midboss_time, 60) {
		int d = _i&1;
		create_enemy1c(VIEWPORT_W*d, 1000, Fairy, stage4_partcircle, 2*cexp(I*M_PI/2.0*(0.2+0.6*frand()+d)));
	}

	FROM_TO(3550 + midboss_time, 4200 + midboss_time, 200) {
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 3000, BigFairy, stage4_cardbuster, VIEWPORT_W/2.+VIEWPORT_W*0.4*(1-2*(_i&1))+100.0*I,
					VIEWPORT_W/4.0+VIEWPORT_W/2.0*((_i+1)&1)+300.0*I, VIEWPORT_W/2.0-200.0*I);
	}

	AT(3800 + midboss_time)
		create_enemy1c(VIEWPORT_W/2, 9000, BigFairy, stage4_supercard, 4.0*I);

	FROM_TO(4300 + midboss_time, 4600 + midboss_time, 95-10*global.diff)
		create_enemy1c(VIEWPORT_W*(_i&1)+100*I, 200, Swirl, stage4_backfire, frand()*(1-2*(_i&1)));

	FROM_TO(4800 + midboss_time, 5200 + midboss_time, 10)
		create_enemy1c(20.0*I+I*VIEWPORT_H/3*frand()+VIEWPORT_W*(_i&1), 100, Swirl, stage4_explosive, (1-2*(_i&1))*3+I);

	AT(5300 + midboss_time) {
		stage_unlock_bgm("stage4");
		global.boss = create_kurumi();
	}

	AT(5400 + midboss_time) {
		stage_unlock_bgm("stage4boss");
		stage4_dialog_post_boss();
	}

	AT(5405 + midboss_time) {
		stage_finish(GAMEOVER_SCORESCREEN);
	}
}
