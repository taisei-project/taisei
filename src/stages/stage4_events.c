/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"
#include "laser.h"

Dialog *stage4_dialog(void) {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "dialog/kurumi");

	dadd_msg(d, Right, "Ah! Intruder! Stop being so persistent!");

	if(global.plr.cha == Marisa) {
		dadd_msg(d, Left, "What? I mean, where am I?");
		dadd_msg(d, Right, "You are in the…");
		dadd_msg(d, Right, "STOP! I will never tell intruders like you!");
		dadd_msg(d, Left, "…in the mansion of the\nevil mastermind, right?");
		dadd_msg(d, Right, "AHH! Anyway! You won’t reach\nthe end of this corridor!");
	} else {
		dadd_msg(d, Left, "So you are the owner of this place?");
		dadd_msg(d, Right, "No, I’m just the guardian!");
		dadd_msg(d, Left, "What is there to be guarded?");
		dadd_msg(d, Right, "My master… I mean, that’s a secret!");
		dadd_msg(d, Left, "…");
		dadd_msg(d, Right, "So stop asking questions!\nSecrets are secret!\n…\nAnd I will beat you now!");
	}

	dadd_msg(d, BGM, "bgm_stage4boss");
	return d;
}

Dialog *stage4_dialog_end(void) {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "dialog/kurumi");

	dadd_msg(d, Left, "Now, where is your master?");
	dadd_msg(d, Right, "Didn’t I tell you? At the end of this corridor,\nthere is a door.");
	dadd_msg(d, Right, "Just leave me alone.");

	return d;
}

int stage4_splasher(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1,3,1,0);
		return 1;
	}

	e->moving = true;
	e->dir = creal(e->args[0]) < 0;

	FROM_TO(0, 50, 1)
		e->pos += e->args[0]*(1-t/50.0);

	FROM_TO(60, 150, 5-global.diff) {
		tsrand_fill(4);
		create_projectile2c(afrand(0) > 0.5 ? "rice" : "thickrice", e->pos, rgb(1,0.6-0.2*afrand(1),0.8), accelerated, e->args[0]/2+(1-2*afrand(2))+(1-2*afrand(3))*I, 0.02I);
	}

	FROM_TO(200, 300, 1)
		e->pos -= creal(e->args[0])*(t-200)/100.0;

	return 1;
}

int stage4_fodder(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 0,1,0,0);
		return 1;
	}

	if(creal(e->args[0]) != 0)
		e->moving = true;
	e->dir = creal(e->args[0]) < 0;
	e->pos += e->args[0];

	FROM_TO(100, 200, 22-global.diff*3) {
		if(global.diff > D_Easy) {
			create_projectile2c("ball", e->pos, rgb(1, 0.3, 0.5), asymptotic, 2*cexp(I*M_PI*2*frand()), 3);
		}
	}

	return 1;
}


int stage4_partcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,1,0,0);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(30,60,1) {
		e->args[0] *= 0.9;
	}

	FROM_TO(60,76,1) {
		int i;
		for(i = 0; i < global.diff; i++) {
			complex n = cexp(I*M_PI/16.0*_i + I*carg(e->args[0])-I*M_PI/4.0 + 0.01I*i*(1-2*(creal(e->args[0]) > 0)));
			create_projectile2c("wave", e->pos + (30)*n, rgb(1-0.2*i,0.5,0.7), asymptotic, 1.5*n, 2+2*i);
		}
	}

	FROM_TO(160, 200, 1)
		e->args[0] += 0.05I;

	return 1;
}

int stage4_cardbuster(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1,2,0,0);
		return 1;
	}

	FROM_TO(0, 120, 1)
		e->pos += (e->args[0]-e->pos0)/120.0;

	FROM_TO(200, 300, 1)
		e->pos += (e->args[1]-e->args[0])/100.0;

	FROM_TO(400, 600, 1)
		e->pos += (e->args[2]-e->args[1])/200.0;

	complex n = cexp(I*carg(global.plr.pos - e->pos) + 0.3I*_i);

	FROM_TO(120, 120+20*global.diff, 1)
		create_projectile2c("card", e->pos + 30*n, rgb(0, 1, 0), asymptotic, 1.3*n, 0.4I);

	FROM_TO(300, 320+20*global.diff, 1)
		create_projectile2c("card", e->pos + 30*n, rgb(0, 1, 0.2), asymptotic, 1.3*n, 0.4I);

	return 1;
}

int stage4_backfire(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3,2,0,0);
		return 1;
	}

	FROM_TO(0,20,1)
		e->args[0] -= 0.05I;

	FROM_TO(60,100,1)
		e->args[0] += 0.05I;

	if(t > 100)
		e->args[0] -= 0.02I;


	e->pos += e->args[0];

	FROM_TO(20,180+global.diff*20,2) {
		complex n = cexp(I*M_PI*frand()-I*copysign(M_PI/2.0, creal(e->args[0])));
		int i;
		for(i = 0; i < global.diff; i++)
			create_projectile2c("wave", e->pos, rgb(0.2, 0.2, 1-0.2*i), asymptotic, 2*n, 2+2*i);
	}

	return 1;
}

int stage4_bigcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1,3,0,0);

		return 1;
	}

	FROM_TO(0, 70, 1)
		e->pos += e->args[0];

	FROM_TO(200, 300, 1)
		e->pos -= e->args[0];


	FROM_TO(80,80+20*global.diff,20) {
		int i;
		int n = 10+3*global.diff;
		for(i = 0; i < n; i++)
			create_projectile2c("bigball", e->pos, rgb(0,0.8-0.4*_i,0), asymptotic, 2*cexp(2.0I*M_PI/n*i+I*M_PI*_i), 3*sin(6*M_PI/n*i));
	}
	return 1;
}

int stage4_explosive(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		int i;
		spawn_items(e->pos, 0,1,0,0);

		int n = 5*global.diff;
		for(i = 0; i < n; i++) {
			create_projectile2c("ball", e->pos, rgb(0, 0, 1-0.6*(i&1)), asymptotic, 1.4*cexp(I*2*M_PI*i/(float)n), 2);
		}

		return 1;
	}

	e->pos += e->args[0];

	if(!(t % 30) && global.diff > D_Normal && frand() < 0.1)
		e->hp = 0;

	return 1;
}

void KurumiSlave(Enemy *e, int t) {
	if(!(t%2)) {
		complex offset  = (frand()-0.5)*30;
				offset += (frand()-0.5)*20.0I;
		create_particle3c("lasercurve", offset, rgb(0.3,0.0,0.0), EnemyFlareShrink, enemy_flare, 50, (-50.0I-offset)/50.0, add_ref(e));
	}
}

void kurumi_intro(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2.0+200.0I, 0.01);
}

int kurumi_burstslave(Enemy *e, int t) {
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
		create_projectile2c("wave", e->pos + 10.0I*e->args[0], rgb(r,0,0), accelerated, 2.0I*e->args[0], -0.01*e->args[1]);
		create_projectile2c("wave", e->pos - 10.0I*e->args[0], rgb(r,0,0), accelerated, -2.0I*e->args[0], -0.01*e->args[1]);
	}

	FROM_TO(40, 100,1) {
		e->args[1] -= e->args[0]*0.02;
		e->args[1] *= cexp(0.02I);
	}

	return 1;
}


void kurumi_slaveburst(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time == EVENT_DEATH)
		killall(global.enemies);

	if(time < 0)
		return;

	AT(0) {
		int i;
		int n = 3+2*global.diff;
		for(i = 0; i < n; i++) {
			create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_burstslave, cexp(I*2*M_PI/n*i+0.2I*time/500), 0, add_ref(b));
		}
	}
}

int kurumi_spikeslave(Enemy *e, int t) {
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
	e->args[1] *= cexp(0.01I*e->args[0]);

	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;
		create_projectile2c("wave", e->pos + 10.0I*e->args[0], rgb(r,0,0), linear, 1.5I*e->args[1], -0.01*e->args[0]);
		create_projectile2c("wave", e->pos - 10.0I*e->args[0], rgb(r,0,0), linear, -1.5I*e->args[1], -0.01*e->args[0]);
	}

	return 1;
}

void kurumi_redspike(Boss *b, int time) {
	int t = time % 500;

	if(time == EVENT_DEATH)
		killall(global.enemies);

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
			for(i = 0; i < n; i++)
				create_projectile2c("bigball", b->pos, rgb(1,0,0), asymptotic, 3*cexp(2.0I*M_PI/n*i+I*carg(global.plr.pos-b->pos)), 3)->draw=ProjDrawAdd;
		}
	} else {
		FROM_TO(80, 500, 2+2*(global.diff == D_Hard)) {
			tsrand_fill(2);
			complex offset = 100*afrand(0)*cexp(2.0I*M_PI*afrand(1));
			complex n = cexp(I*carg(global.plr.pos-b->pos-offset));
			create_projectile2c("rice", b->pos+offset, rgb(1,0,0), accelerated, -1*n, 0.05*n)->draw=ProjDrawAdd;
		}
	}
}

void kurumi_spell_bg(Boss *b, int time) {
	float f = 0.5+0.5*sin(time/80.0);

	glPushMatrix();
	glTranslatef(VIEWPORT_W/2, VIEWPORT_H/2,0);
	glScalef(0.6,0.6,1);
	glColor3f(f,1-f,1-f);
	draw_texture(0, 0, "stage4/kurumibg1");
	glColor3f(1,1,1);
	glPopMatrix();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	fill_screen(time/300.0, time/300.0, 0.5, "stage4/kurumibg2");

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void kurumi_outro(Boss *b, int time) {
	b->pos += -5-I;
}

Boss *create_kurumi_mid(void) {
	Boss* b = create_boss("Kurumi", "kurumi", VIEWPORT_W/2-400.0I);
	boss_add_attack(b, AT_Move, "Introduction", 4, 0, kurumi_intro, NULL);
	boss_add_attack(b, AT_Spellcard, "Bloodless ~ Gate of Walachia", 25, 20000, kurumi_slaveburst, kurumi_spell_bg);
	boss_add_attack(b, AT_Spellcard, global.diff < D_Hard ? "Bloodless ~ Dry Fountain" : "Bloodless ~ Red Spike", 30, 30000, kurumi_redspike, kurumi_spell_bg);
	boss_add_attack(b, AT_Move, "Outro", 2, 1, kurumi_outro, NULL);
	start_attack(b, b->attacks);
	return b;
}

int splitcard(Projectile *p, int t) {
	if(t == creal(p->args[2])) {
		p->args[0] += p->args[3];
		p->clr->b = -p->clr->b;
	}

	return asymptotic(p, t);
}

int stage4_supercard(Enemy *e, int t) {
	int time = t % 150;

	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,3,0,0);
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
		int i;
		complex n = cexp(I*carg(global.plr.pos - e->pos) + 0.3I*_i);
		for(i = -1; i <= 1 && t; i++)
			create_projectile4c("card", e->pos + 30*n, rgb(1-_i/20.0, 0, 0.4), splitcard, 1*n, 0.4I, 100-time+70, 1.4*I*i*n);
	}

	return 1;
}

void kurumi_boss_intro(Boss *b, int t) {
	TIMER(&t);
	GO_TO(b, VIEWPORT_W/2.0+200.0I, 0.01);

	AT(120)
		global.dialog = stage4_dialog();
}

void kurumi_breaker(Boss *b, int time) {
	int t = time % 400;
	int i;

	int c = 10+global.diff*2;
	int kt = 20;

	if(time < 0)
		return;

	TIMER(&t);

	FROM_TO(50, 400, 50-7*global.diff) {
		complex p = b->pos + 150*sin(_i) + 100.0I*cos(_i);

		for(i = 0; i < c; i++) {
			complex n = cexp(2.0I*M_PI/c*i);
			create_projectile4c("rice", p, rgb(1,0,0.5), splitcard, 3*n, 0,
									kt, 1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-2.6*n);

		}
	}

	FROM_TO(60, 400, 100) {
		for(i = 0; i < 20; i++)
			create_projectile2c("bigball", b->pos, rgb(0.5,0,0.5), asymptotic, cexp(2.0I*M_PI/20.0*i), 3);
	}

}

int aniwall_bullet(Projectile *p, int t) {
	if(t < 0)
		return 1;

	if(t > creal(p->args[1])) {
		if(global.diff > D_Normal) {
			tsrand_fill(2);
			p->args[0] += (0.1-0.2*afrand(0) + 0.1I-0.2I*afrand(1))*(global.diff-2);
			p->args[0] += 0.005*cexp(I*carg(global.plr.pos - p->pos));
		}

		p->pos += p->args[0];
	}

	p->clr->r = cimag(p->pos)/VIEWPORT_H;

	return 1;
}

int aniwall_slave(Enemy *e, int t) {
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
			e->args[2] = 10.0I;
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
			complex v = e->args[2]/cabs(e->args[2])*I*copysign(1,creal(e->args[0]));
			create_projectile2c("ball", e->pos, rgb(1,0,0), aniwall_bullet, 1*v, 40);
		}
	}

	return 1;
}

void KurumiAniWallSlave(Enemy *e, int t) {
	if(e->args[1])
		create_particle1c("part/lasercurve", e->pos, rgb(1,1,1), FadeAdd, timeout, 30);
}

void kurumi_aniwall(Boss *b, int time) {
	TIMER(&time);

	AT(EVENT_DEATH) {
		killall(global.enemies);
	}

	if(time < 0)
		return;

	AT(60) {
		create_lasercurve2c(b->pos, 50, 80, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(0.4I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, aniwall_slave, 0.2*cexp(0.4I));
		create_lasercurve2c(b->pos, 50, 80, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(I*M_PI - 0.4I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, aniwall_slave, 0.2*cexp(I*M_PI - 0.4I));
	}
}

void kurumi_sbreaker(Boss *b, int time) {
	if(time < 0)
		return;

	int t = time % 400;
	int i;
	TIMER(&t);

	int c = 10+global.diff*2;
	int kt = 40;

	FROM_TO(50, 400, 2) {
		complex p = b->pos + 150*sin(_i/8.0)+100.0I*cos(_i/15.0);

		complex n = cexp(2.0I*M_PI/c*_i);
		create_projectile4c("rice", p, rgb(1,0,0.5), splitcard, 2*n, 0,
								kt, 1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-1.7*n);

	}

	FROM_TO(60, 400, 100) {
		for(i = 0; i < 20; i++)
			create_projectile2c("bigball", b->pos, rgb(0.5,0,0.5), asymptotic, cexp(2.0I*M_PI/20.0*i), 3);
	}

}

int blowwall_slave(Enemy *e, int t) {
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
		char *type;

		c = 20 + global.diff*40;

		for(i = 0; i < c; i++) {
			f = frand();

			if(f < 0.3)
				type = "soul";
			else if(f < 0.6)
				type = "bigball";
			else
				type = "plainball";

			create_projectile2c(type, e->pos, rgb(1, 0.1, 0.1), asymptotic, (1+3*f)*cexp(2.0I*M_PI*frand()), 4)->draw=ProjDrawAdd;
		}

		return ACTION_DESTROY;
	}

	return 1;
}



static void bwlaser(Boss *b, float arg, int slave) {
	create_lasercurve2c(b->pos, 50, 100, rgb(1, 0.5+0.3*slave, 0.5+0.3*slave), las_accel, 0, (0.1+0.1*slave)*cexp(I*arg));
	if(slave)
		create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, blowwall_slave, 0.2*cexp(I*arg));
}

void kurumi_blowwall(Boss *b, int time) {
	int t = time % 600;
	TIMER(&t);

	if(time == EVENT_DEATH)
		killall(global.enemies);

	if(time < 0)
		return;

	AT(50)
		bwlaser(b, 0.4, 1);

	AT(100)
		bwlaser(b, M_PI-0.4, 1);

	FROM_TO(200, 300, 50)
		bwlaser(b, -M_PI*frand(), 1);

	FROM_TO(300, 500, 10)
		bwlaser(b, M_PI/10*_i, 0);

}

int kdanmaku_slave(Enemy *e, int t) {
	float re;

	if(t < 0)
		return 1;

	if(!e->args[1])
		e->pos += e->args[0]*t;
	else
		e->pos += 5.0I;

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
		int i, n = 3*global.diff;

		if(!(t % 1)) {
			for(i = 0; i < n; i++) {
				complex p = VIEWPORT_W/(float)n*(i+frand()) + I*cimag(e->pos);
				if(cabs(p-global.plr.pos) > 60)
					create_projectile1c("thickrice", p, rgb(1, 0.5, 0.5), linear, 0.5*cexp(2.0I*M_PI*frand()))->draw = ProjDrawAdd;
			}
		}
	}

	return 1;
}

void kurumi_danmaku(Boss *b, int time) {
	int t = time % 600;
	TIMER(&t);

	if(time == EVENT_DEATH)
		killall(global.enemies);
	if(time < 0)
		return;

	AT(50) {
		create_lasercurve2c(b->pos, 50, 100, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(I*carg(-b->pos)));
		create_lasercurve2c(b->pos, 50, 100, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(I*carg(VIEWPORT_W-b->pos)));
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, kdanmaku_slave, 0.2*cexp(I*carg(-b->pos)), 0, 1);
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, kdanmaku_slave, 0.2*cexp(I*carg(VIEWPORT_W-b->pos)), 0, 0);
	}
}


Boss *create_kurumi(void) {
	Boss* b = create_boss("Kurumi", "kurumi", -400.0I);
	boss_add_attack(b, AT_Move, "Introduction", 4, 0, kurumi_boss_intro, NULL);
	boss_add_attack(b, AT_Normal, "Sin Breaker", 20, 20000, kurumi_sbreaker, NULL);
	boss_add_attack(b, AT_Spellcard, global.diff < D_Hard ? "Limit ~ Animate Wall" : "Summoning ~ Demon Wall", 30, 40000, kurumi_aniwall, kurumi_spell_bg);
	boss_add_attack(b, AT_Normal, "Cold Breaker", 20, 20000, kurumi_breaker, NULL);
	boss_add_attack(b, AT_Spellcard, "Power Sign ~ Blow the Walls", 30, 32000, kurumi_blowwall, kurumi_spell_bg);
	if(global.diff > D_Normal)
		boss_add_attack(b, AT_Spellcard, "Fear Sign ~ Bloody Danmaku", 30, 32000, kurumi_danmaku, kurumi_spell_bg);
	start_attack(b, b->attacks);

	return b;
}



void stage4_events(void) {
	TIMER(&global.timer);

	AT(70) {
		create_enemy1c(VIEWPORT_H/4*3*I, 3000, BigFairy, stage4_splasher, 3-4.0I);
		create_enemy1c(VIEWPORT_W + VIEWPORT_H/4*3*I, 3000, BigFairy, stage4_splasher, -3-4.0I);
	}

	FROM_TO(300, 450, 20) {
		create_enemy1c(VIEWPORT_W*frand(), 200, Fairy, stage4_fodder, 3.0I);
	}

	FROM_TO(500, 550, 10) {
		int d = _i&1;
		create_enemy1c(VIEWPORT_W*d, 1000, Fairy, stage4_partcircle, 2*cexp(I*M_PI/2.0*(0.2+0.6*frand()+d)));
	}

	FROM_TO(600, 1400, 100) {
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 3000, BigFairy, stage4_cardbuster, VIEWPORT_W/6.0 + VIEWPORT_W/3.0*2*(_i&1)+100.0I,
					VIEWPORT_W/4.0 + VIEWPORT_W/2.0*((_i+1)&1)+300.0I, VIEWPORT_W/2.0+VIEWPORT_H*I+200.0I);
	}

	AT(1800) {
		create_enemy1c(VIEWPORT_H/2.0*I, 2000, Swirl, stage4_backfire, 0.3);
		create_enemy1c(VIEWPORT_W+VIEWPORT_H/2.0*I, 2000, Swirl, stage4_backfire, -0.5);
	}

	FROM_TO(2000, 2600, 20)
		create_enemy1c(300.0I*frand(), 200, Fairy, stage4_fodder, 2);

	FROM_TO(2000, 2400, 200)
		create_enemy1c(VIEWPORT_W/2+200-400*frand(), 1000, BigFairy, stage4_bigcircle, 2.0I);

	FROM_TO(2600, 2800, 10)
		create_enemy1c(20.0I+VIEWPORT_H/3*I*frand()+VIEWPORT_W, 100, Swirl, stage4_explosive, -3);

	AT(3200)
		global.boss = create_kurumi_mid();

	FROM_TO(3201, 3601, 10)
		create_enemy1c(VIEWPORT_W*(_i&1)+VIEWPORT_H/2*I-300.0I*frand(), 200, Fairy, stage4_fodder, 2-4*(_i&1)+1.0I);

	FROM_TO(3500, 4000, 100)
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 1000, BigFairy, stage4_cardbuster, VIEWPORT_W/6.0*(_i&1)+100.0I,
					VIEWPORT_W/4.0+VIEWPORT_W/2.0*((_i+1)&1)+300.0I, VIEWPORT_W/2.0-200.0I);

	AT(3800)
		create_enemy1c(VIEWPORT_W/2, 7000, BigFairy, stage4_supercard, 4.0I);

	FROM_TO(4300, 4600, 95-10*global.diff)
		create_enemy1c(VIEWPORT_W*(_i&1)+100*I, 200, Swirl, stage4_backfire, frand()*(1-2*(_i&1)));

	FROM_TO(4800, 5200, 10)
		create_enemy1c(20.0I+I*VIEWPORT_H/3*frand()+VIEWPORT_W*(_i&1), 100, Swirl, stage4_explosive, (1-2*(_i&1))*3+I);

	AT(5300)
		global.boss = create_kurumi();

	AT(5400)
		global.dialog = stage4_dialog_end();
}
