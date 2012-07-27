/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage1_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"

Dialog *stage1_dialog() {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "masterspark");
		
	dadd_msg(d, Left, "What are you doing here?\nYou the culprit?");
	dadd_msg(d, Right, "Huh? No, you? Everone is upset, you know?\nSo I came too.");
	dadd_msg(d, Left, "Why, what happened?");
	dadd_msg(d, Right, "The border has cracked.");
	dadd_msg(d, Left, "Is that even possible!?");
	dadd_msg(d, Right, "Look, there is a way outside\nright behind us.");
	dadd_msg(d, Left, "But I have the feeling that you\n won't let me pass, haha");
	
	return d;
}

int stage1_great_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 5,4,0,0);
		return 1;
	}
	
	e->pos += e->args[0];
	
	FROM_TO(50,70,1)
		e->args[0] *= 0.5;
		
	FROM_TO(70, 190+global.diff*25, 5) {
		int n, c = 7;
		for(n = 0; n < c; n++) {
			complex dir = cexp(I*(2*M_PI/c*n+0.0001*(_i%5-3)+0.5*_i/5));
			create_projectile2c("rice", e->pos+30*dir, rgb(0.6,0.0,0.3), asymptotic, 1.5*dir, _i%5);
			
			if(global.diff > D_Normal && _i%6 == 0)
				create_projectile1c("bigball", e->pos+30*dir, rgb(0.3,0.0,0.6), linear, 1.5*dir);
		}
	}
	
	AT(210+global.diff*25) {
		e->args[0] = 2I;
	}
	
	return 1;
}

int spin_circle(Projectile *p, int t) { // a[0]: angular velocity, a[1]: center, a[2]: center speed
	if(t < 0)
		return 1;
	
	p->pos += p->args[0]*cimag(p->args[1]-p->pos) - p->args[0]*creal(p->args[1]-p->pos)*I;
	
	p->args[1] += p->args[2];
	
	return 1;
}

int stage1_small_spin_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,0,0,0);
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
		
	FROM_TO(50,80+global.diff*5,5)
		create_projectile3c("ball", e->pos, rgb(0.9,0.0,0.3), spin_circle, 0.02 - 0.04*(!e->dir), e->pos0 + _i*10*((1-2*e->dir)+1I), (1-2*e->dir)+1I);
	
	return 1;
}

int stage1_aim(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 0,2,0,0);
		return 1;
	}
	
	if(t < 70)
		e->pos += e->args[0];
	if(t > 150)
		e->pos -= e->args[0];
	
	AT(90) {
		if(global.diff > D_Normal) {
			create_projectile2c("plainball", e->pos, rgb(0.6,0.0,0.8), asymptotic, 5*cexp(I*carg(global.plr.pos-e->pos)), -1);
			create_projectile1c("plainball", e->pos, rgb(0.2,0.0,0.1), linear, 3*cexp(I*carg(global.plr.pos-e->pos)));
		}
	}
	
	return 1;
}

int stage1_sidebox_trail(Enemy *e, int t) { // creal(a[0]): velocity, cimag(a[0]): angle, a[1]: d angle/dt, a[2]: time of acceleration
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1,1,0,0);
		return 1;
	}
	
	e->pos += creal(e->args[0])*cexp(I*cimag(e->args[0]));
	
	FROM_TO((int) creal(e->args[2]),(int) creal(e->args[2])+M_PI*0.5/fabs(creal(e->args[1])),1)
		e->args[0] += creal(e->args[1])*I;
		
	FROM_TO(10,200,27-global.diff*2) {
		float f = 0;
		if(global.diff > D_Normal)
			f = 0.03*global.diff*frand();
		
		create_projectile1c("rice", e->pos, rgb(0.9,0.0,0.9), linear, 3*cexp(I*(cimag(e->args[0])+f+0.5*M_PI)));
		create_projectile1c("rice", e->pos, rgb(0.9,0.0,0.9), linear, 3*cexp(I*(cimag(e->args[0])-f-0.5*M_PI)));
	}
		
	return 1;
}	
	
int stage1_flea(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,0,0,0);
		return 1;
	}
	
	e->pos += e->args[0]*(1-e->args[1]);
	
	FROM_TO(80,90,1)
		e->args[1] += 0.1;
		
	FROM_TO(200,205,1)
		e->args[1] -= 0.2;
	
		
	FROM_TO(10, 400, 40-global.diff*5-t/70) {
		create_projectile2c("flea", e->pos, rgb(0.2,0.2,1), asymptotic, 1.5*cexp(2I*M_PI*frand()), 1.5);
	}
	
	return 1;
}

int stage1_accel_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1,3,0,0);
		return 1;
	}
	
	e->pos += e->args[0];
		
	FROM_TO(60,250, 10) {
		e->args[0] *= 0.5;
		
		int i;
		for(i = 0; i < 6; i++) {
			create_projectile2c("ball", e->pos, rgb(0.6,0.1,0.2), accelerated, 1.5*cexp(2I*M_PI/6*i)+cexp(I*carg(global.plr.pos - e->pos)), -0.02*cexp(I*(2*M_PI/6*i+0.02*frand()*global.diff)));
		}
	}
	
	if(t > 270)
		e->args[0] -= 0.01I;

	return 1;
}

void wriggle_intro(Boss *w, int t) {
	if(t < 0)
		return;
	
	w->pos = VIEWPORT_W/2 + 100I + 400*(1.0-t/(4.0*FPS))*cexp(I*(3-t*0.04));
}

int wriggle_bug(Projectile *p, int t) {
	if(t < 0)
		return 1;
	
	p->pos += p->args[0];
	p->angle = carg(p->args[0]);
	
	
	if(t > 70 && frand() < 0.01)
		p->args[0] *= cexp(I*M_PI/3);
	
	return 1;
}

void wriggle_small_storm(Boss *w, int time) {
	int t = time % 400;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	FROM_TO(0,400,5) {
		create_projectile1c("rice", w->pos, rgb(1,0.5,0.2), wriggle_bug, 2*cexp(I*_i*2*M_PI/20));
		create_projectile1c("rice", w->pos, rgb(1,0.5,0.2), wriggle_bug, 2*cexp(I*_i*2*M_PI/20+I*M_PI));
	}
	
	GO_AT(w, 60, 120, 1)
	GO_AT(w, 180, 240, -1)
	
	if(!(t%200)) {
		int i;
		for(i = 0; i < 10; i++)
			create_projectile2c("bigball", w->pos, rgb(0.1,0.3,0.0), asymptotic, 2*cexp(I*i*2*M_PI/10), 2);
	}
}

Boss *create_wriggle_mid() {
	Boss* wriggle = create_boss("Wriggle", "wriggle", VIEWPORT_W + 150 - 30I);
	boss_add_attack(wriggle, AT_Move, "Introduction", 4, 0, wriggle_intro, NULL);
	boss_add_attack(wriggle, AT_Normal, "Small Bug Storm", 20, 20000, wriggle_small_storm, NULL);
	
	start_attack(wriggle, wriggle->attacks);
	return wriggle;
}

void hina_intro(Boss *h, int time) {
	TIMER(&time);	
	
	if(time < 0)
		return;
	
	AT(100)
		global.dialog = stage1_dialog();
	
	GO_TO(h, VIEWPORT_W/2 + 100I, 0.05);
}

void hina_cards1(Boss *h, int time) {
	int t = time % 500;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	FROM_TO(0, 500, 2-(global.diff > D_Normal)) {
		create_projectile2c("card", h->pos+50*cexp(I*t/10), rgb(0.8,0.0,0.0),  asymptotic, 2*cexp(I*t/5.0), 3);
		create_projectile2c("card", h->pos-50*cexp(I*t/10), rgb(0.0,0.0,0.8),  asymptotic, -2*cexp(I*t/5.0), 3);
	}
}

void hina_amulet(Boss *h, int time) {
	int t = time % 200;
	
	if(time < 0)
		return;
	
	if(time < 100)
		GO_TO(h, VIEWPORT_W/2 + 200I, 0.02);
	
	TIMER(&t);
	
	FROM_TO(0,30*global.diff,1) {
			float f = _i/30.0;
			complex n = cexp(I*2*M_PI*f+0.7*time/200*I);		
			
			create_projectile2c("crystal", h->pos+30*log(1+_i/2.0)*n, rgb(0.8,0,0), accelerated, 2*n*I, -0.01*n);
			create_projectile2c("crystal", h->pos+30*log(1+_i/2.0)*n, rgb(0.8,0,0.5), accelerated, -2*n*I, -0.01*n);
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
			create_projectile2c("bigball", h->pos, rgb(0.7, 0, 0.7), asymptotic, 2*cexp(I*2*M_PI*i/20.0), 3);
		}
	}
}

void hina_bad_pick(Boss *h, int time) {
	int t = time % 400;
	int i, j;
	
	TIMER(&t);
	
	if(time < 0)
		return;
	
	GO_TO(h, VIEWPORT_W/5*(time/400+0.6)+ 100I, 0.02);
	
	FROM_TO(100, 500, 5) {
		
		for(i = 1; i < 5; i++) {
			create_projectile1c("crystal", VIEWPORT_W/5*i, rgb(0.2,0,0.2), linear, 7I);
		}
	}
	
	AT(200) {
		int win = tsrand()%5;
		for(i = 0; i < 5; i++) {
			if(i == win)
				continue;
			
			for(j = 0; j < 2+global.diff; j++)
				create_projectile1c("bigball", VIEWPORT_W/5*(i+0.2+0.6*frand()), rgb(0.7,0,0.0), linear, 2I);
		}
	}
}

void hina_wheel(Boss *h, int time) {
	int t = time % 400;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	GO_TO(h, VIEWPORT_W/2+VIEWPORT_H/2*I, 0.02);
	
	if(time < 60)
		return;
	
	FROM_TO(0, 400, 5-global.diff) {
		int i;
		float speed = 10;
		if(time > 500)
			speed = 1+9*exp(-(time-500)/100.0);
						
		for(i = 1; i < 6; i++) {
			create_projectile1c("crystal", h->pos, rgb(log(1+time*1e-3),0,0.2), linear, speed*cexp(I*2*M_PI/5*(i+time/100.0+frand()*time/1700.0)));
		}
	}
}

void hina_spell_bg(Boss *h, int time) {
	
	glPushMatrix();
	glTranslatef(VIEWPORT_W/2, VIEWPORT_H/2,0);
	glPushMatrix();
	glScalef(0.6,0.6,1);
	draw_texture(0, 0, "stage1/spellbg1");
	glPopMatrix();
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	glRotatef(time*5, 0,0,1);
	draw_texture(0, 0, "stage1/spellbg2");	
	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	draw_animation(creal(h->pos), cimag(h->pos), 0, "fire");
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
}

Boss *create_hina() {
	Boss* hina = create_boss("Kagiyama Hina", "hina", VIEWPORT_W + 150 + 100I);
	boss_add_attack(hina, AT_Move, "Introduction", 2, 0, hina_intro, NULL);
	boss_add_attack(hina, AT_Normal, "Cards1", 20, 15000, hina_cards1, NULL);
	boss_add_attack(hina, AT_Spellcard, "Shard ~ Amulet of Harm", 26, 25000, hina_amulet, hina_spell_bg);
	boss_add_attack(hina, AT_Normal, "Cards2", 17, 15000, hina_cards2, NULL);
	boss_add_attack(hina, AT_Spellcard, "Lottery Sign ~ Bad Pick", 30, 36000, hina_bad_pick, hina_spell_bg);
	boss_add_attack(hina, AT_Spellcard, "Lottery Sign ~ Wheel of Fortune", 20, 36000, hina_wheel, hina_spell_bg);
	
	start_attack(hina, hina->attacks);
	return hina;
}

void stage1_events() {
	TIMER(&global.timer);
		
	AT(300) {
		create_enemy1c(VIEWPORT_W/2-10I, 7000+500*global.diff, BigFairy, stage1_great_circle, 2I);
	}
	
	FROM_TO(650-50*global.diff, 750+25*(4-global.diff), 50) {
		create_enemy1c(VIEWPORT_W*((_i)%2)+50I, 2000, Fairy, stage1_small_spin_circle, 2-4*(_i%2)+1I);
	}
	
	FROM_TO(850, 1000, 15)
		create_enemy1c(VIEWPORT_W/2+25*(_i-5)-20I, 200, Fairy, stage1_aim, (2+frand()*0.3)*I);
		
	FROM_TO(960, 1200, 20)
		create_enemy3c(VIEWPORT_W-80+(VIEWPORT_H+20)*I, 200, Fairy, stage1_sidebox_trail, 3 - 0.5*M_PI*I, -0.02, 90);
	
	FROM_TO(1140, 1400, 20)
		create_enemy3c(200-20I, 200, Fairy, stage1_sidebox_trail, 3+0.5I*M_PI, -0.05, 70);
		
	AT(1300)
		create_enemy1c(150-10I, 4000, BigFairy, stage1_great_circle, 2.5I);
		
	AT(1500)
		create_enemy1c(VIEWPORT_W-150-10I, 4000, BigFairy, stage1_great_circle, 2.5I);
		
	FROM_TO(1700, 2000, 30)
		create_enemy1c(VIEWPORT_W*frand()-20I, 200, Fairy, stage1_flea, 1.7I);
		
	FROM_TO(1950, 2500, 60) {
		create_enemy3c(VIEWPORT_W-40+(VIEWPORT_H+20)*I, 200, Fairy, stage1_sidebox_trail, 5 - 0.5*M_PI*I, -0.02, 83-global.diff*3);
		create_enemy3c(40+(VIEWPORT_H+20)*I, 200, Fairy, stage1_sidebox_trail, 5 - 0.5*M_PI*I, 0.02, 80-global.diff*3);
	}
	
	AT(2500) {
		create_enemy1c(VIEWPORT_W/4-10I, 2000, Fairy, stage1_accel_circle, 2I);
		create_enemy1c(VIEWPORT_W/4*3-10I, 2000, Fairy, stage1_accel_circle, 2I);
	}
		
	AT(2800)
		global.boss = create_wriggle_mid();
		
	FROM_TO(3100, 3400, 50) {
		create_enemy3c(VIEWPORT_W-80+(VIEWPORT_H+20)*I, 200, Fairy, stage1_sidebox_trail, 3 - 0.5*M_PI*I, -0.02, 90);
		create_enemy3c(80+(VIEWPORT_H+20)*I, 200, Fairy, stage1_sidebox_trail, 3 - 0.5*M_PI*I, 0.02, 90);
	}
	
	AT(3600) {
		create_enemy1c(VIEWPORT_W/2-10I, 7000+500*global.diff, BigFairy, stage1_great_circle, 2I);
	}
	
	FROM_TO(3700, 4500, 40)
		create_enemy1c(VIEWPORT_W*frand()-10I, 150, Fairy, stage1_flea, 2.5I);
		
	FROM_TO(4000, 4600, 100)
		create_enemy1c(VIEWPORT_W/2+100-200*frand()-10I, 2000, Fairy, stage1_accel_circle, 2I);
		
	AT(5100) {
		global.boss = create_hina();
	}
}
