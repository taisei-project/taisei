/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage2_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"

static Dialog *stage2_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Dialog *d = create_dialog(pm->character->dialog_sprite_name, "dialog/hina");
	pm->dialog->stage2_pre_boss(d);
	dadd_msg(d, BGM, "stage2boss");
	return d;
}

static Dialog *stage2_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Dialog *d = create_dialog(pm->character->dialog_sprite_name, "dialog/hina");
	pm->dialog->stage2_post_boss(d);
	return d;
}

int stage2_great_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Point, 5, Power, 4, NULL);
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
			complex dir = cexp(I*(2*M_PI/c*n+partdist*(_i%c2-c2/2)+bunchdist*(_i/c2)));

			PROJECTILE("rice", e->pos+30*dir, RGB(0.6,0.0,0.3), asymptotic, {
				1.5*dir,
				_i%5
			});

			if(global.diff > D_Easy && _i%7 == 0) {
				play_sound("shot1");
				PROJECTILE("bigball", e->pos+30*dir, RGB(0.3,0.0,0.6), linear, {
					1.7*dir*cexp(0.3*I*frand())
				});
			}
		}
	}

	AT(210+global.diff*25) {
		e->hp = min(e->hp,200);
		e->args[0] = 2.0*I;
	}

	return 1;
}

int stage2_small_spin_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Point, 2, NULL);
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

		PROJECTILE("ball", e->pos, RGB(0.9,0.0,0.3), linear, {
			pow(global.diff,0.7)*(conj(e->pos-VIEWPORT_W/2)/100 + ((1-2*e->dir)+3.0*I))
		});
	}

	return 1;
}

int stage2_aim(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Power, 2, NULL);
		return 1;
	}

	if(t < 70)
		e->pos += e->args[0];
	if(t > 150)
		e->pos -= e->args[0];

	AT(90) {
		if(global.diff > D_Normal) {
			play_sound("shot1");
			PROJECTILE("plainball", e->pos, RGB(0.6,0.0,0.8), asymptotic, {
				5*cexp(I*carg(global.plr.pos-e->pos)),
				-1
			});

			PROJECTILE("plainball", e->pos, RGB(0.2,0.0,0.1), linear, {
				3*cexp(I*carg(global.plr.pos-e->pos))
			});
		}
	}

	return 1;
}

int stage2_sidebox_trail(Enemy *e, int t) { // creal(a[0]): velocity, cimag(a[0]): angle, a[1]: d angle/dt, a[2]: time of acceleration
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Point, 1, Power, 1, NULL);
		return 1;
	}

	e->pos += creal(e->args[0])*cexp(I*cimag(e->args[0]));

	FROM_TO((int) creal(e->args[2]),(int) creal(e->args[2])+M_PI*0.5/fabs(creal(e->args[1])),1)
		e->args[0] += creal(e->args[1])*I;

	FROM_TO(10,200,30-global.diff*4) {
		play_sound_ex("shot1", 5, false);

		float f = 0;
		if(global.diff > D_Normal)
			f = 0.03*global.diff*frand();

		PROJECTILE("rice", e->pos, RGB(0.9,0.0,0.9), linear, { 3*cexp(I*(cimag(e->args[0])+f+0.5*M_PI)) });
		PROJECTILE("rice", e->pos, RGB(0.9,0.0,0.9), linear, { 3*cexp(I*(cimag(e->args[0])-f-0.5*M_PI)) });
	}

	return 1;
}

int stage2_flea(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Point, 2, NULL);
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
			PROJECTILE("flea", e->pos, RGB(0.3,0.2,1), asymptotic, {
				1.5*cexp(2.0*I*M_PI*frand()),
				1.5
			});
		}
	}

	return 1;
}

int stage2_accel_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Point, 1, Power, 3, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(60,250, 20+10*(global.diff<D_Hard)) {
		e->args[0] *= 0.5;

		int i;
		for(i = 0; i < 6; i++) {
			play_sound("redirect");
			PROJECTILE("ball", e->pos, RGB(0.9,0.1,0.2), accelerated, {
				1.5*cexp(2.0*I*M_PI/6*i)+cexp(I*carg(global.plr.pos - e->pos)),
				-0.02*cexp(I*(2*M_PI/6*i+0.02*frand()*global.diff))
			});
		}
	}

	if(t > 270)
		e->args[0] -= 0.01*I;

	return 1;
}

static void wriggle_ani_flyin(Boss *w) {
	aniplayer_queue(&w->ani,"midbossflyintro",1);
	aniplayer_queue(&w->ani,"main",0);
}

static void wriggle_intro_stage2(Boss *w, int t) {
	if(t < 0)
		return;
	w->pos = VIEWPORT_W/2 + 100.0*I + 300*(1.0-t/(4.0*FPS))*cexp(I*(3-t*0.04));
}

int wriggle_bug(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->pos += p->args[0];
	p->angle = carg(p->args[0]);

	if(global.boss && global.boss->current && !((global.frames - global.boss->current->starttime - 30) % 200)) {
		play_sound("redirect");
		p->args[0] *= cexp(I*(M_PI/3)*nfrand());
		spawn_projectile_highlight_effect(p);
	}

	return ACTION_NONE;
}

void wriggle_small_storm(Boss *w, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time < 0)
		return;

	FROM_TO_SND("shot1_loop", 0,400,5-global.diff) {
		PROJECTILE("rice", w->pos, RGB(1,0.5,0.2), wriggle_bug, { 2*cexp(I*_i*2*M_PI/20) });
		PROJECTILE("rice", w->pos, RGB(1,0.5,0.2), wriggle_bug, { 2*cexp(I*_i*2*M_PI/20+I*M_PI) });
	}

	GO_AT(w, 60, 120, 1)
	GO_AT(w, 180, 240, -1)

	if(!((t+200-15)%200)) {
		aniplayer_queue(&w->ani,"specialshot_charge",1);
		aniplayer_queue(&w->ani,"specialshot_hold",20);
		aniplayer_queue(&w->ani,"specialshot_release",1);
		aniplayer_queue(&w->ani,"main",0);
	}

	if(!(t%200)) {
		int i;
		play_sound("shot_special1");

		for(i = 0; i < 10+global.diff; i++) {
			PROJECTILE("bigball", w->pos, RGB(0.1,0.3,0.0), asymptotic, {
				2*cexp(I*i*2*M_PI/(10+global.diff)),
				2
			});
		}
	}
}

void wiggle_mid_flee(Boss *w, int t) {
	if(t == 0)
		aniplayer_queue(&w->ani, "fly", 0);
	if(t >= 0) {
		GO_TO(w, VIEWPORT_W/2 - 3.0 * t - 300 * I, 0.01)
	}
}

Boss *create_wriggle_mid(void) {
	Boss* wriggle = create_boss("Wriggle", "wriggle", NULL, VIEWPORT_W + 150 - 30.0*I);
	wriggle->glowcolor = *RGB(0.2, 0.4, 0.5);
	wriggle->shadowcolor = *RGBA_MUL_ALPHA(0.4, 0.2, 0.6, 0.5);
	wriggle_ani_flyin(wriggle);
	boss_add_attack(wriggle, AT_Move, "Introduction", 4, 0, wriggle_intro_stage2, NULL);
	boss_add_attack(wriggle, AT_Normal, "Small Bug Storm", 20, 26000, wriggle_small_storm, NULL);
	boss_add_attack(wriggle, AT_Move, "Flee", 5, 0, wiggle_mid_flee, NULL);;

	boss_start_attack(wriggle, wriggle->attacks);
	return wriggle;
}

void hina_intro(Boss *h, int time) {
	TIMER(&time);

	AT(100)
		global.dialog = stage2_dialog_pre_boss();

	GO_TO(h, VIEWPORT_W/2 + 100.0*I, 0.05);
}

void hina_cards1(Boss *h, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	AT(0) {
		aniplayer_queue(&h->ani, "guruguru", 0);
	}
	FROM_TO(0, 500, 2-(global.diff > D_Normal)) {
		play_sound_ex("shot1", 4, false);
		PROJECTILE("card", h->pos+50*cexp(I*t/10), RGB(0.8,0.0,0.0), asymptotic, { (1.6+0.4*global.diff)*cexp(I*t/5.0), 3 });
		PROJECTILE("card", h->pos-50*cexp(I*t/10), RGB(0.0,0.0,0.8), asymptotic, {-(1.6+0.4*global.diff)*cexp(I*t/5.0), 3 });
	}
}

void hina_amulet(Boss *h, int time) {
	int t = time % 200;

	if(time < 0)
		return;

	if(time < 100)
		GO_TO(h, VIEWPORT_W/2 + 200.0*I, 0.02);

	TIMER(&t);

	complex d = global.plr.pos - h->pos;
	int loopduration = 200*(global.diff+0.5)/(D_Lunatic+0.5);
	AT(0) {
		aniplayer_queue_frames(&h->ani,"guruguru",loopduration);
	}
	FROM_TO_SND("shot1_loop", 0,loopduration,1) {
		float f = _i/30.0;
		complex n = cexp(I*2*M_PI*f+I*carg(d)+0.7*time/200*I)/sqrt(0.5+global.diff);

		float speed = 1.0 + 0.75 * max(0, (int)global.diff - D_Normal);
		float accel = 1.0 + 1.20 * max(0, (int)global.diff - D_Normal);

		complex p = h->pos+30*log(1+_i/2.0)*n;

		const char *t0 = "ball";
		const char *t1 = global.diff == D_Easy ? t0 : "crystal";

		PROJECTILE(t0, p, RGB(0.8, 0.0, 0.0), accelerated, { speed *  2*n*I, accel * -0.01*n });
		PROJECTILE(t1, p, RGB(0.8, 0.0, 0.5), accelerated, { speed * -2*n*I, accel * -0.01*n });
	}
}

void hina_cards2(Boss *h, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	hina_cards1(h, time);

	GO_AT(h, 100, 200, 2);
	GO_AT(h, 260, 460, -2);
	GO_AT(h, 460, 500, 5);

	AT(100) {
		int i;
		for(i = 0; i < 30; i++) {
			play_sound("shot_special1");
			PROJECTILE("bigball", h->pos, RGB(0.7, 0, 0.7), asymptotic, {
				2*cexp(I*2*M_PI*i/20.0),
				3
			});
		}
	}
}

#define SLOTS 5

static int bad_pick_bullet(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->pos += p->args[0];
	p->args[0] += p->args[1];

	// reflection
	int slot = (int)(creal(p->pos)/(VIEWPORT_W/SLOTS)+1)-1; // correct rounding for slot == -1
	int targetslot = creal(p->args[2])+0.5;
	if(slot != targetslot)
		p->args[0] = copysign(creal(p->args[0]),targetslot-slot)+I*cimag(p->args[0]);

	return ACTION_NONE;
}

void hina_bad_pick(Boss *h, int time) {
	int t = time % 500;
	int i, j;

	TIMER(&t);

	if(time < 0)
		return;

	GO_TO(h, VIEWPORT_W/SLOTS*((113*(time/500))%SLOTS+0.5)+ 100.0*I, 0.02);

	FROM_TO(100, 500, 5) {
		play_sound_ex("shot1", 4, false);

		for(i = 1; i < SLOTS; i++) {
			PROJECTILE("crystal", VIEWPORT_W/SLOTS*i, RGB(0.5,0,0.6), linear, { 7.0*I });
		}

		if(global.diff >= D_Hard) {
			double shift = 0;
			if(global.diff == D_Lunatic)
				shift = 0.3*max(0,t-200);
			for(i = 1; i < SLOTS; i++) {
				double height = VIEWPORT_H/SLOTS*i+shift;
				if(height > VIEWPORT_H-40)
					height -= VIEWPORT_H-40;
				PROJECTILE("crystal", (i&1)*VIEWPORT_W+I*height, RGB(0.5,0,0.6), linear, { 5.0*(1-2*(i&1)) });
			}
		}
	}

	AT(200) {
		aniplayer_queue(&h->ani,"guruguru",2);
		play_sound("shot_special1");

		int win = tsrand()%SLOTS;
		for(i = 0; i < SLOTS; i++) {
			if(i == win)
				continue;

			float cnt = (1+min(D_Normal,global.diff)) * 5;
			for(j = 0; j < cnt; j++) {
				complex o = VIEWPORT_W/SLOTS*(i + j/(cnt-1));

				PROJECTILE("ball", o, RGBA(0.7, 0.0, 0.0, 0.0), bad_pick_bullet,
					.args = {
						0,
						0.005*nfrand() + 0.005*I * (1 + psin(i + j + global.frames)),
						i
					},
				);
			}
		}
	}
}

#undef SLOTS

void hina_wheel(Boss *h, int time) {
	int t = time % 400;
	TIMER(&t);

	static int dir = 0;

	if(time < 0)
		return;

	GO_TO(h, VIEWPORT_W/2+VIEWPORT_H/2*I, 0.02);

	AT(0) {
		aniplayer_queue(&h->ani,"guruguru",0);
	}
	if(time < 60) {
		if(time == 0) {
			if(global.diff > D_Normal) {
				dir = 1 - 2 * (tsrand()%2);
			} else {
				dir = 1;
			}
		}

		return;
	}

	FROM_TO_SND("shot1_loop", 0, 400, 5-(int)min(global.diff, D_Normal)) {
		int i;
		float speed = 10;
		if(time > 500)
			speed = 1+9*exp(-(time-500)/100.0);

		float d = max(0, (int)global.diff - D_Normal);

		for(i = 1; i < 6+d; i++) {
			float a = dir * 2*M_PI/(5+d)*(i+(1 + 0.4 * d)*time/100.0+(1 + 0.2 * d)*frand()*time/1700.0);
			PROJECTILE("crystal", h->pos, RGB(log(1+time*1e-3),0,0.2), linear, { speed*cexp(I*a) });
		}
	}
}

static int timeout_deadproj_linear(Projectile *p, int time) {
	// TODO: maybe add a "clear on timeout" flag?

	if(time < 0) {
		return ACTION_ACK;
	}

	if(time > creal(p->args[0])) {
		p->type = DeadProj;
	}

	p->pos += p->args[1];
	p->angle = carg(p->args[1]);
	return ACTION_NONE;
}

int hina_monty_slave(Enemy *s, int time) {
	if(time < 0) {
		return 1;
	}

	if(time > 60 && time < 720-140 + 20*(global.diff-D_Lunatic) && !(time % (int)(max(2 + (global.diff < D_Normal), (120 - 0.5 * time))))) {
		play_loop("shot1_loop");

		PROJECTILE("crystal", s->pos,
			.color = RGB(0.5 + 0.5 * psin(time*0.2), 0.3, 1.0 - 0.5 * psin(time*0.2)),
			.rule = asymptotic,
			.args = {
				5*I + 1 * (sin(time) + I * cos(time)),
				4
			}
		);

		if(global.diff > D_Easy) {
			PROJECTILE("crystal", s->pos,
				.color = RGB(0.5 + 0.5 * psin(time*0.2), 0.3, 1.0 - 0.5 * psin(time*0.2)),
				.rule = timeout_deadproj_linear,
				.args = {
					500,
					-0.5*I + 1 * (sin(time) + I * cos(time))
				}
			);
		}
	}

	return 1;
}

void hina_monty_slave_visual(Enemy *s, int time, bool render) {
	Swirl(s, time, render);

	if(!render) {
		return;
	}

	Sprite *soul = get_sprite("proj/soul");
	double scale = fabs(swing(clamp(time / 60.0, 0, 1), 3)) * 1.25;

	Color *clr1 = RGBA(1.0, 0.0, 0.0, 0.0);
	Color *clr2 = RGBA(0.0, 0.0, 1.0, 0.0);
	Color *clr3 = RGBA(psin(time*0.05), 0.0, 1.0 - psin(time*0.05), 0.0);

	r_mat_push();
	r_mat_translate(creal(s->pos), cimag(s->pos), 0);
	r_shader("sprite_bullet");

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = soul,
		.rotation.angle = time * DEG2RAD,
		.scale.x = scale * (0.6 + 0.6 * psin(time*0.1)),
		.scale.y = scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)),
		.color = clr1,
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = soul,
		.rotation.angle = time * DEG2RAD,
		.scale.x = scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)),
		.scale.y = scale * (0.6 + 0.6 * psin(time*0.1)),
		.color = clr2,
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = soul,
		.rotation.angle = -time * DEG2RAD,
		.scale.both = scale,
		.color = clr3,
	});

	r_mat_pop();
	r_shader("sprite_default");
}

void hina_monty(Boss *h, int time) {
	int t = time % 720;
	TIMER(&t);

	static short slave_pos, bad_pos, good_pos, plr_pos;
	static int cwidth = VIEWPORT_W / 3.0;
	static complex targetpos;

	if(time == EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 0) {
		targetpos = VIEWPORT_W/2.0 + VIEWPORT_H/2.0 * I;
		return;
	}

	AT(0) {
		plr_pos = creal(global.plr.pos) / cwidth;
		bad_pos = tsrand() % 3;
		do good_pos = tsrand() % 3; while(good_pos == bad_pos);

		play_sound("laser1");

		for(int i = 0; i < 2; ++i) {
			int x = cwidth * (1 + i);
			create_laserline_ab(x, x + VIEWPORT_H*I, 15, 30, 60, RGBA(0.3, 1.0, 1.0, 0.0));
			create_laserline_ab(x, x + VIEWPORT_H*I, 20, 240, 600, RGBA(1.0, 0.3, 1.0, 0.0));
		}

		enemy_kill_all(&global.enemies);
	}

	AT(120) {
		do slave_pos = tsrand() % 3; while(slave_pos == plr_pos || slave_pos == good_pos);
		while(bad_pos == slave_pos || bad_pos == good_pos) bad_pos = tsrand() % 3;

		complex o = cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I - 200.0*I;

		play_sound("laser1");
		create_laserline_ab(h->pos, o, 15, 30, 60, RGBA(1.0, 0.3, 0.3, 0.0));
		aniplayer_queue(&h->ani,"guruguru",1);
		aniplayer_queue(&h->ani,"main",0);
	}

	AT(140) {
		play_sound("shot_special1");
		create_enemy4c(cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I - 200.0*I, ENEMY_IMMUNE, hina_monty_slave_visual, hina_monty_slave, 0, 0, 0, 1);
	}

	AT(190) {
		targetpos = cwidth * (0.5 + good_pos) + VIEWPORT_H/2.0*I - 200.0*I;
	}

	AT(240) {
		// main laser barrier activation
		play_sound("laser1");
	}

	FROM_TO(220, 360 + 60 * max(0, (double)global.diff - D_Easy), 60) {
		play_sound("shot_special1");

		float cnt = (2.0+global.diff) * 5;
		for(int i = 0; i < cnt; i++) {
			bool top = ((global.diff > D_Hard) && (_i % 2));
			complex o = !top*VIEWPORT_H*I + cwidth*(bad_pos + i/(double)(cnt - 1));

			PROJECTILE("ball", o,
				.color = top ? RGBA(0, 0, 0.7, 0) : RGBA(0.7, 0, 0, 0),
				.rule = accelerated,
				.args = {
					0,
					(top ? -0.5 : 1) * 0.004 * (sin((M_PI * 4 * i / (cnt - 1)))*0.1*global.diff - I*(1 + psin(i + global.frames)))
				},
			);
		}
	}

	{
		const int step = 2;
		const int cnt = 10;
		const int burst_dur = (cnt - 1) * step;
		const int cycle_dur = burst_dur+10*(D_Hard-global.diff);
		const int start = 210;
		const int end = 540;
		const int ncycles = (end - start) / cycle_dur;

		AT(start) {
			aniplayer_queue_frames(&h->ani,"guruguru",end-start);
			aniplayer_queue(&h->ani,"main",0);
		}

		FROM_TO_INT(start, start + cycle_dur * ncycles - 1, cycle_dur, burst_dur, step) {
			play_sound("shot1");

			double p = _ni / (double)(cnt-1);
			double c = p;
			double m = 0.60 + 0.01 * global.diff;

			p *= m;
			if(_i % 2) {
				p = 1.0 - p;
			}

			complex o = cwidth * (p + 0.5/(cnt-1) - 0.5) + h->pos;

			if(global.diff > D_Normal) {
				PROJECTILE("card", o, RGB(c * 0.8, 0, (1 - c) * 0.8), accelerated, { -2.5*I, 0.05*I });
			} else {
				PROJECTILE("card", o, RGB(c * 0.8, 0, (1 - c) * 0.8), linear, { 2.5*I });
			}
		}
	}

	FROM_TO(240, 390, 5) {
		create_item(VIEWPORT_H*I + cwidth*(good_pos + frand()), -50.0*I, _i % 2 ? Point : Power);
	}

	AT(600) {
		targetpos = cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I;
	}

	GO_TO(h, targetpos, 0.06);
}

void hina_spell_bg(Boss *h, int time) {
	r_mat_push();
	r_mat_translate(VIEWPORT_W/2, VIEWPORT_H/2,0);
	r_mat_push();
	r_mat_scale(0.6,0.6,1);
	draw_sprite(0, 0, "stage2/spellbg1");
	r_mat_pop();
	r_blend(BLEND_MOD);
	r_mat_rotate_deg(time*5, 0,0,1);
	draw_sprite(0, 0, "stage2/spellbg2");
	r_mat_pop();
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(1, 1, 1, 0);
	Animation *fireani = get_ani("fire");
	draw_sprite_p(creal(h->pos), cimag(h->pos), animation_get_frame(fireani, get_ani_sequence(fireani, "main"), global.frames));
	r_color4(1, 1, 1, 1);
}

Boss* stage2_spawn_hina(complex pos) {
	Boss *hina = create_boss("Kagiyama Hina", "hina", "dialog/hina", pos);
	hina->glowcolor = *RGBA_MUL_ALPHA(0.7, 0.2, 0.3, 0.5);
	hina->shadowcolor = hina->glowcolor;
	return hina;
}

Boss *create_hina(void) {
	Boss *hina = stage2_spawn_hina(VIEWPORT_W + 150 + 100.0*I);

	aniplayer_queue(&hina->ani,"main",3);
	aniplayer_queue(&hina->ani,"guruguru",1);
	aniplayer_queue(&hina->ani,"main",0);

	boss_add_attack(hina, AT_Move, "Introduction", 2, 0, hina_intro, NULL);
	boss_add_attack(hina, AT_Normal, "Cards1", 30, 25000, hina_cards1, NULL);
	boss_add_attack_from_info(hina, &stage2_spells.boss.amulet_of_harm, false);
	boss_add_attack(hina, AT_Normal, "Cards2", 40, 30000, hina_cards2, NULL);
	boss_add_attack_from_info(hina, &stage2_spells.boss.bad_pick, false);

	if(global.diff < D_Hard) {
		boss_add_attack_from_info(hina, &stage2_spells.boss.wheel_of_fortune_easy, false);
	} else {
		boss_add_attack_from_info(hina, &stage2_spells.boss.wheel_of_fortune_hard, false);
	}

	boss_add_attack_from_info(hina, &stage2_spells.extra.monty_hall_danmaku, false);

	boss_start_attack(hina, hina->attacks);
	return hina;
}

void stage2_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage2");
	}

	AT(300) {
		create_enemy1c(VIEWPORT_W/2-10.0*I, 7500, BigFairy, stage2_great_circle, 2.0*I);
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
		global.boss = create_hina();
	}

	AT(5180) {
		global.dialog = stage2_dialog_post_boss();
	}

	AT(5340 - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
