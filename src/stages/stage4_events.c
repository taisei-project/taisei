/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stage4_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"
#include "laser.h"

void kurumi_spell_bg(Boss*, int);
void kurumi_slaveburst(Boss*, int);
void kurumi_redspike(Boss*, int);
void kurumi_aniwall(Boss*, int);
void kurumi_blowwall(Boss*, int);
void kurumi_danmaku(Boss*, int);
void kurumi_extra(Boss*, int);

Dialog *stage4_dialog(void) {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "dialog/kurumi");

	dadd_msg(d, Right, "Ah! Intruder! Stop being so persistent!");

	if(global.plr.cha == Marisa) {
		dadd_msg(d, Left, "What? I mean, where am I?");
		dadd_msg(d, Right, "You are in the…");
		dadd_msg(d, Right, "Ha! you almost got me!\nI will never tell intruders like you!");
		dadd_msg(d, Left, "…in the mansion of the\nevil mastermind, right?");
		dadd_msg(d, Right, "AHH! Anyway! You won’t reach\nthe end of this corridor!");
	} else {
		dadd_msg(d, Left, "So you are the owner of this place?");
		dadd_msg(d, Right, "No, I’m just the guardian!");
		dadd_msg(d, Left, "What is there to be guarded?");
		dadd_msg(d, Right, "My master… I mean– that’s a secret!");
		dadd_msg(d, Left, "…");
		dadd_msg(d, Right, "So stop asking questions!\nSecrets are secret!\n…\nAnd I will beat you now!");
	}

	dadd_msg(d, BGM, "stage4boss");
	return d;
}

Dialog *stage4_dialog_end(void) {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "dialog/kurumi");

	dadd_msg(d, Left, "Now, where is your master?");
	dadd_msg(d, Right, "At the end of this corridor,\nthere is a door.");
	dadd_msg(d, Right, "Just leave me alone.");

	return d;
}

int stage4_splasher(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 3, Bomb, 1, NULL);
		return 1;
	}

	e->moving = true;
	e->dir = creal(e->args[0]) < 0;

	FROM_TO(0, 50, 1)
		e->pos += e->args[0]*(1-t/50.0);

	FROM_TO(66-6*global.diff, 150, 5-global.diff) {
		tsrand_fill(4);
		create_projectile2c(afrand(0) > 0.5 ? "rice" : "thickrice", e->pos, rgb(0.8,0.3-0.1*afrand(1),0.5), accelerated, e->args[0]/2+(1-2*afrand(2))+(1-2*afrand(3))*I, 0.02*I);
	}

	FROM_TO(200, 300, 1)
		e->pos -= creal(e->args[0])*(t-200)/100.0;

	return 1;
}

int stage4_fodder(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Power, 1, NULL);
		return 1;
	}

	if(creal(e->args[0]) != 0)
		e->moving = true;
	e->dir = creal(e->args[0]) < 0;
	e->pos += e->args[0];

	FROM_TO(100, 200, 22-global.diff*3) {
		create_projectile2c("ball", e->pos, rgb(1, 0.3, 0.5), asymptotic, 2*cexp(I*M_PI*2*frand()), 3);
	}

	return 1;
}


int stage4_partcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, Power, 1, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(30,60,1) {
		e->args[0] *= 0.9;
	}

	FROM_TO(60,76,1) {
		int i;
		for(i = 0; i < global.diff; i++) {
			complex n = cexp(I*M_PI/16.0*_i + I*carg(e->args[0])-I*M_PI/4.0 + 0.01*I*i*(1-2*(creal(e->args[0]) > 0)));
			create_projectile2c("wave", e->pos + (30)*n, rgb(1-0.2*i,0.5,0.7), asymptotic, 2*n, 2+2*i);
		}
	}

	FROM_TO(160, 200, 1)
		e->args[0] += 0.05*I;

	return 1;
}

int stage4_cardbuster(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 2, NULL);
		return 1;
	}

	FROM_TO(0, 120, 1)
		e->pos += (e->args[0]-e->pos0)/120.0;

	FROM_TO(200, 300, 1)
		e->pos += (e->args[1]-e->args[0])/100.0;

	FROM_TO(400, 600, 1)
		e->pos += (e->args[2]-e->args[1])/200.0;

	complex n = cexp(I*carg(global.plr.pos - e->pos) + 0.3*I*_i);

	FROM_TO(120, 120+20*global.diff, 1)
		create_projectile2c("card", e->pos + 30*n, rgb(0, 1, 0), asymptotic, (1.1+0.2*global.diff)*n, 0.4*I);

	FROM_TO(300, 320+20*global.diff, 1)
		create_projectile2c("card", e->pos + 30*n, rgb(0, 1, 0.2), asymptotic, (1.1+0.2*global.diff)*n, 0.4*I);

	return 1;
}

int stage4_backfire(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 3, Power, 2, NULL);
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
		spawn_items(e->pos, Point, 1, Power, 3, NULL);

		return 1;
	}

	FROM_TO(0, 70, 1)
		e->pos += e->args[0];

	FROM_TO(200, 300, 1)
		e->pos -= e->args[0];


	FROM_TO(80,100+30*global.diff,20) {
		int i;
		int n = 10+3*global.diff;
		for(i = 0; i < n; i++) {
			create_projectile2c("bigball", e->pos, rgb(0,0.8-0.4*_i,0), asymptotic, 2*cexp(2.0*I*M_PI/n*i+I*3*_i), 3*sin(6*M_PI/n*i))->draw=ProjDrawAdd;
			if(global.diff > D_Easy)
				create_projectile2c("ball", e->pos, rgb(0,0.3*_i,0.4), asymptotic, (1.5+global.diff*0.2)*cexp(I*3*(i+frand())), I*5*sin(6*M_PI/n*i))->draw=ProjDrawAdd;
		}
	}
	return 1;
}

int stage4_explosive(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		int i;
		spawn_items(e->pos, Power, 1, NULL);

		int n = 5*global.diff;
		complex phase = global.plr.pos-e->pos;
		phase /= cabs(phase);

		for(i = 0; i < n; i++) {
			create_projectile2c("ball", e->pos, rgb(0.1, 0.2, 1-0.6*(i&1)), asymptotic, (1.1+0.3*global.diff)*cexp(I*2*M_PI*(i+frand())/(float)n)*phase, 2);
		}

		return 1;
	}

	e->pos += e->args[0];

	if(!(t % 30) && global.diff >= D_Normal && frand() < 0.1)
		e->hp = 0;

	return 1;
}

void KurumiSlave(Enemy *e, int t) {
	if(!(t%2)) {
		complex offset = (frand()-0.5)*30;
		offset += (frand()-0.5)*20.0*I;
		create_particle3c("lasercurve", offset, rgb(0.3,0.0,0.0), EnemyFlareShrink, enemy_flare, 50, (-50.0*I-offset)/50.0, add_ref(e));
	}
}

void kurumi_intro(Boss *b, int t) {
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);
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
		create_projectile2c("wave", e->pos + 10.0*I*e->args[0], rgb(r,0,0), accelerated, 2.0*I*e->args[0], -0.01*e->args[1]);
		create_projectile2c("wave", e->pos - 10.0*I*e->args[0], rgb(r,0,0), accelerated, -2.0*I*e->args[0], -0.01*e->args[1]);
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
		killall(global.enemies);

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
	e->args[1] *= cexp(0.01*I*e->args[0]);

	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;
		create_projectile2c("wave", e->pos + 10.0*I*e->args[0], rgb(r,0,0), linear, 1.5*I*e->args[1], -0.01*e->args[0]);
		create_projectile2c("wave", e->pos - 10.0*I*e->args[0], rgb(r,0,0), linear, -1.5*I*e->args[1], -0.01*e->args[0]);
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
				create_projectile2c("bigball", b->pos, rgb(1,0,0), asymptotic, (1+0.1*(global.diff == D_Normal))*3*cexp(2.0*I*M_PI/n*i+I*carg(global.plr.pos-b->pos)), 3)->draw=ProjDrawAdd;
		}
	} else {
		FROM_TO_INT(80, 500, 40,200,2+2*(global.diff == D_Hard)) {
			tsrand_fill(2);
			complex offset = 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1));
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
	Boss* b = create_boss("Kurumi", "kurumi", "dialog/kurumi", VIEWPORT_W/2-400.0*I);
	boss_add_attack(b, AT_Move, "Introduction", 4, 0, kurumi_intro, NULL);
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

int splitcard(Projectile *p, int t) {
	if(t == creal(p->args[2])) {
		p->args[0] += p->args[3];
		p->clr = derive_color(p->clr, CLRMASK_B, rgb(0, 0, -color_component(p->clr, CLR_B)));
	}

	return asymptotic(p, t);
}

int stage4_supercard(Enemy *e, int t) {
	int time = t % 150;

	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, Power, 3, NULL);
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
		complex n = cexp(I*carg(global.plr.pos - e->pos) + 0.3*I*_i);
		for(i = -1; i <= 1 && t; i++)
			create_projectile4c("card", e->pos + 30*n, rgb(1-_i/20.0, 0, 0.4), splitcard, 1*n, 0.4*I, 100-time+70, 1.4*I*i*n);
	}

	return 1;
}

void kurumi_boss_intro(Boss *b, int t) {
	TIMER(&t);
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.015);

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

	GO_TO(b, VIEWPORT_W/2 + VIEWPORT_W/3*sin(time/220) + I*cimag(b->pos), 0.02);

	TIMER(&t);

	FROM_TO(50, 400, 50-7*global.diff) {
		complex p = b->pos + 150*sin(_i) + 100.0*I*cos(_i);

		for(i = 0; i < c; i++) {
			complex n = cexp(2.0*I*M_PI/c*i);
			create_projectile4c("rice", p, rgb(1,0,0.5), splitcard, 3*n, 0,
									kt, 1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-2.6*n);

		}
	}

	FROM_TO(60, 400, 100) {
		for(i = 0; i < 20; i++)
			create_projectile2c("bigball", b->pos, rgb(0.5,0,0.5), asymptotic, cexp(2.0*I*M_PI/20.0*i), 3)->draw=ProjDrawAdd;
	}

}

int aniwall_bullet(Projectile *p, int t) {
	if(t < 0)
		return 1;

	if(t > creal(p->args[1])) {
		if(global.diff > D_Normal) {
			tsrand_fill(2);
			p->args[0] += 0.1*(0.1-0.2*afrand(0) + 0.1*I-0.2*I*afrand(1))*(global.diff-2);
			p->args[0] += 0.002*cexp(I*carg(global.plr.pos - p->pos));
		}

		p->pos += p->args[0];
	}

	float r, g, b, a;
	parse_color(p->clr,&r,&g,&b,&a);
	p->clr = rgb(cimag(p->pos)/VIEWPORT_H, g, b);

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
			complex v = e->args[2]/cabs(e->args[2])*I*sign(creal(e->args[0]));
			if(cimag(v) > -0.1 || global.diff >= D_Normal)
				create_projectile2c("ball", e->pos+I*v*20*nfrand(), rgb(1,0,0), aniwall_bullet, 1*v, 40);
		}
	}

	return 1;
}

void KurumiAniWallSlave(Enemy *e, int t) {
	if(e->args[1])
		create_particle1c("lasercurve", e->pos, rgb(1,1,1), FadeAdd, timeout, 30);
}

void kurumi_aniwall(Boss *b, int time) {
	TIMER(&time);

	AT(EVENT_DEATH) {
		killall(global.enemies);
	}

	GO_TO(b, VIEWPORT_W/2 + VIEWPORT_W/3*sin(time/200) + I*cimag(b->pos),0.03)


	if(time < 0)
		return;

	AT(0) {
		create_lasercurve2c(b->pos, 50, 80, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(0.4*I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, aniwall_slave, 0.2*cexp(0.4*I));
		create_lasercurve2c(b->pos, 50, 80, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(I*M_PI - 0.4*I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, aniwall_slave, 0.2*cexp(I*M_PI - 0.4*I));
	}
}

void kurumi_sbreaker(Boss *b, int time) {
	if(time < 0)
		return;

	int dur = 300+50*global.diff;
	int t = time % dur;
	int i;
	TIMER(&t);

	int c = 10+global.diff*2;
	int kt = 40;

	FROM_TO(50, dur, 2+(global.diff < D_Hard)) {
		complex p = b->pos + 150*sin(_i/8.0)+100.0*I*cos(_i/15.0);

		complex n = cexp(2.0*I*M_PI/c*_i);
		create_projectile4c("rice", p, rgb(1,0,0.5), splitcard, 2*n, 0,
								kt, 1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-1.7*n);

	}

	FROM_TO(60, dur, 100) {
		for(i = 0; i < 20; i++)
			create_projectile2c("bigball", b->pos, rgb(0.5,0,0.5), asymptotic, cexp(2.0*I*M_PI/20.0*i), 3)->draw=ProjDrawAdd;
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

			create_projectile2c(type, e->pos, rgb(1, 0.1, 0.1), asymptotic, (1+3*f)*cexp(2.0*I*M_PI*frand()), 4)->draw=ProjDrawAdd;
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

	if(time < 0) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.04)
		return;
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

int kdanmaku_slave(Enemy *e, int t) {
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
		int i, n = 3*global.diff;

		if(!(t % 1)) {
			for(i = 0; i < n; i++) {
				complex p = VIEWPORT_W/(float)n*(i+frand()) + I*cimag(e->pos);
				if(cabs(p-global.plr.pos) > 60)
					create_projectile1c("thickrice", p, rgb(1, 0.5, 0.5), linear, 0.5*cexp(2.0*I*M_PI*frand()))->draw = ProjDrawAdd;
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

void kurumi_extra_shield_pos(Enemy *e, int time) {
	double dst = 75 + 100 * max((60 - time) / 60.0, 0);
	double spd = cimag(e->args[0]) * min(time / 120.0, 1);
	e->args[0] += spd;
	e->pos = global.boss->pos + dst * cexp(I*creal(e->args[0]));
}

bool kurumi_extra_shield_expire(Enemy *e, int time) {
	if(time > creal(e->args[1])) {
		e->hp = 0;
		return true;
	}

	return false;
}

int kurumi_extra_dead_shield_proj(Projectile *p, int time) {
	if(time < 0) {
		return 1;
	}

	p->clr = mix_colors(
		rgb(0.2, 0.1, 0.5),
		rgb(2.0, 0.0, 0.0),
	min(time / 60.0f, 1.0f));

	return asymptotic(p, time);
}



int kurumi_extra_dead_shield(Enemy *e, int time) {
	if(time < 0) {
		return 1;
	}

	if(!(time % 6)) {
		// complex dir = cexp(I*(M_PI * 0.5 * nfrand() + carg(global.plr.pos - e->pos)));
		// complex dir = cexp(I*(carg(global.plr.pos - e->pos)));
		complex dir = cexp(I*creal(e->args[0]));
		create_projectile2c("rice", e->pos, 0, kurumi_extra_dead_shield_proj, 2*dir, 10);
	}

	time += cimag(e->args[1]);

	kurumi_extra_shield_pos(e, time);

	if(kurumi_extra_shield_expire(e, time)) {
		int cnt = 10;
		for(int i = 0; i < cnt; ++i) {
			complex dir = cexp(I*M_PI*2*i/(double)cnt);
			tsrand_fill(2);
			create_projectile2c("ball", e->pos, 0, kurumi_extra_dead_shield_proj, 1.5 * (1 + afrand(0)) * dir, 4 + anfrand(1))->draw = ProjDrawAdd;
		}
	}

	return 1;
}

int kurumi_extra_shield(Enemy *e, int time) {
	if(time == EVENT_DEATH) {
		if(!boss_is_dying(global.boss) && e->args[2] == 0) {
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

int kurumi_extra_bigfairy1(Enemy *e, int time) {
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
		complex phase = cexp(I*2*M_PI*frand());
		for(int i = 0; i < count; i++) {
			complex arg = cexp(I*2*M_PI*i/count);
			if(global.diff == D_Lunatic)
				arg *= phase;
			create_lasercurve2c(e->pos,20,200,rgb(1,0.3,0.7),las_accel,arg,0.1*arg);
			create_projectile2c("bullet",e->pos,rgb(1,0.3,0.7),accelerated,arg,0.1*arg);
		}
	}

	return 1;
}

void kurumi_extra_drainer_draw(Projectile *p, int time) {
	complex org = p->pos;
	complex targ = p->args[1];
	double a = 0.5 * creal(p->args[2]);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(1.0, 0.5, 0.5, a);
	loop_tex_line_p(org, targ, 16, time * 0.01, p->tex);
	glColor4f(0.4, 0.2, 0.2, a);
	loop_tex_line_p(org, targ, 18, time * 0.0043, p->tex);
	glColor4f(0.5, 0.2, 0.5, a);
	loop_tex_line_p(org, targ, 24, time * 0.003, p->tex);
	glColor4f(1,1,1,1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int kurumi_extra_drainer(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return 1;
	}

	if(time < 0) {
		return 1;
	}

	Enemy *e = REF(p->args[0]);
	p->pos = global.boss->pos;

	if(e) {
		p->args[1] = e->pos;
		p->args[2] = approach(p->args[2], 1, 0.5);

		if(time > 40) {
			int drain = clamp(4, 0, e->hp);
			e->hp -= drain;
			global.boss->current->hp = min(global.boss->current->maxhp, global.boss->current->hp + drain * 2);
		}
	} else {
		p->args[2] = approach(p->args[2], 0, 0.5);
		if(!creal(p->args[2])) {
			return ACTION_DESTROY;
		}
	}

	return 1;
}

void kurumi_extra_create_drainer(Enemy *e) {
	create_particle1c("sinewave", e->pos, 0, kurumi_extra_drainer_draw, kurumi_extra_drainer, add_ref(e));
}

void kurumi_swirl_draw(Enemy *e, int time) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
	Swirl(e, time);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void kurumi_extra_fairy_draw(Enemy *e, int time) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glUseProgram(get_shader("negative")->prog);
	Fairy(e, time);
	glUseProgram(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void kurumi_bigfairy_draw(Enemy *e, int time) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glUseProgram(get_shader("negative")->prog);
	BigFairy(e, time);
	glUseProgram(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int kurumi_extra_fairy(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, NULL);
		return 1;
	}

	if(e->hp == ENEMY_IMMUNE && t > 50)
		e->hp = 500;

	if(creal(e->args[0]-e->pos) != 0)
		e->moving = true;
	e->dir = creal(e->args[0]-e->pos) < 0;

	FROM_TO(0,90,1) {
		GO_TO(e,e->args[0],0.1)
	}

	/*FROM_TO(100, 200, 22-global.diff*3) {
		create_projectile2c("ball", e->pos, rgb(1, 0.3, 0.5), asymptotic, 2*cexp(I*M_PI*2*frand()), 3);
	}*/

	int attacktime = creal(e->args[1]);
	int flytime = cimag(e->args[1]);
	FROM_TO(attacktime-20,attacktime+20,20) {
		complex vel = cexp(I*frand()*2*M_PI)*(2+0.1*(global.diff-D_Easy));
		if(e->args[2] == 0) { // attack type
			int corners = 5;
			double len = 50;
			int count = 5;
			for(int i = 0; i < corners; i++) {
				for(int j = 0; j < count; j++) {
					complex pos = len/2/tan(2*M_PI/corners)*I+(j/(double)count-0.5)*len;
					pos *= cexp(I*2*M_PI/corners*i);
					create_projectile1c("flea", e->pos+pos, rgb(1, 0.3, 0.5), linear, vel+0.1*I*pos/cabs(pos));
				}
			}
		} else {
			int count = 30;
			double rad = 20;
			for(int j = 0; j < count; j++) {
				double x = (j/(double)count-0.5)*2*M_PI;
				complex pos = 0.5*cos(x)+sin(2*x) + (0.5*sin(x)+cos(2*x))*I;
				pos*=vel/cabs(vel);
				create_projectile1c("flea", e->pos+rad*pos, rgb(0.5, 0.3, 1), linear, vel+0.1*pos);
			}
		}
	}
	AT(attacktime) {
		e->args[0] = global.plr.pos-e->pos;

		kurumi_extra_create_drainer(e);
	}
	FROM_TO(attacktime,attacktime+flytime,1) {
		e->pos += e->args[0]/flytime;
	}

	FROM_TO(attacktime,attacktime+flytime,20-global.diff*3) {
		if(global.diff>D_Easy) {
			create_projectile1c("ball", e->pos, rgba(0.1+0.07*_i, 0.3, 1-0.05*_i,0.8), timeout, 50)->draw=ProjDrawAdd;
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

	int shieldlimit = 1000;
	TIMER(&t);

	if(time == EVENT_DEATH) {
		killall(global.enemies);
		return;
	}

	if(time < 120)
		GO_TO(b, VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I, 0.01)

	if(t == 0 && b->current->hp >= shieldlimit) {
		int cnt = 12;
		for(int i = 0; i < cnt; ++i) {
			double a = M_PI * 2 * i / (double)cnt;
			int hp = 500;
			create_enemy2c(b->pos, hp, kurumi_swirl_draw, kurumi_extra_shield, a + 0.05*I, 800);
			create_enemy2c(b->pos, hp, kurumi_swirl_draw, kurumi_extra_shield, a - 0.05*I, 800);
		}
	}

	AT(90) {
		int cnt = 20;
		for(int i = 0; i < cnt; i++) {
			b->current->hp -= 600;
			if(b->current->hp < shieldlimit)
				b->current->hp = shieldlimit;
			tsrand_fill(2);
			complex pos = VIEWPORT_W/2*afrand(0)+I*afrand(1)*VIEWPORT_H*2/3;
			if(direction)
				pos = VIEWPORT_W-creal(pos)+I*cimag(pos);
			// immune so they don’t get killed while they are still offscreen.
			create_enemy3c(pos-300*(1-2*direction),ENEMY_IMMUNE,kurumi_extra_fairy_draw,kurumi_extra_fairy,pos,100+20*i+100*(1.1-0.05*global.diff)*I,direction);
		}
	}

	complex sidepos = VIEWPORT_W * (0.5+0.3*(1-2*direction)) + VIEWPORT_H * 0.28 * I;
	FROM_TO(90,120,1) {
		GO_TO(b, sidepos,0.1)
	}

	FROM_TO(190,200,1) {
		GO_TO(b, sidepos+30*I,0.1)
	}

	FROM_TO(300,400,1) {
		GO_TO(b,VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I,0.1)
	}

	if(global.diff >= D_Hard) {
		AT(300) {
			double ofs = VIEWPORT_W * 0.5;
			complex pos = 0.5 * VIEWPORT_W + I * (VIEWPORT_H - 100);
			complex targ = 0.5 *VIEWPORT_W + VIEWPORT_H * 0.3 * I;
			create_enemy1c(pos + ofs, 3300, kurumi_bigfairy_draw, kurumi_extra_bigfairy1, targ + 0.8*ofs);
			create_enemy1c(pos - ofs, 3300, kurumi_bigfairy_draw, kurumi_extra_bigfairy1, targ - 0.8*ofs);
		}
	}
	if((t == length-20 && global.diff == D_Easy)|| b->current->hp < shieldlimit) {
		for(Enemy *e = global.enemies; e; e = e->next) {
			if(e->logic_rule == kurumi_extra_shield) {
				e->args[2] = 1; // discharge extra shield
				e->hp = 0;
				continue;
			}
		}
	}
}


Boss *create_kurumi(void) {
	Boss* b = create_boss("Kurumi", "kurumi", "dialog/kurumi", -400.0*I);
	boss_add_attack(b, AT_Move, "Introduction", 4, 0, kurumi_boss_intro, NULL);
	boss_add_attack(b, AT_Normal, "Sin Breaker", 20, 33000, kurumi_sbreaker, NULL);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.boss.animate_wall, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.boss.demon_wall, false);
	}
	boss_add_attack(b, AT_Normal, "Cold Breaker", 20, 36000, kurumi_breaker, NULL);
	boss_add_attack_from_info(b, &stage4_spells.boss.blow_the_walls, false);
	if(global.diff > D_Normal) {
		boss_add_attack_from_info(b, &stage4_spells.boss.bloody_danmaku, false);
	}

	boss_add_attack_from_info(b, &stage4_spells.extra.vlads_army, false);
	boss_start_attack(b, b->attacks);

	return b;
}



void stage4_events(void) {
	TIMER(&global.timer);

	AT(0) {
		start_bgm("stage4");
	}

	AT(70) {
		create_enemy1c(VIEWPORT_H/4*3*I, 3000, BigFairy, stage4_splasher, 3-4.0*I);
		create_enemy1c(VIEWPORT_W + VIEWPORT_H/4*3*I, 3000, BigFairy, stage4_splasher, -3-4.0*I);
	}

	FROM_TO(300, 450, 20) {
		create_enemy1c(VIEWPORT_W*frand(), 200, Fairy, stage4_fodder, 3.0*I);
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

	FROM_TO(2000, 2600, 20)
		create_enemy1c(300.0*I*frand(), 200, Fairy, stage4_fodder, 2);

	FROM_TO(2000, 2400, 200)
		create_enemy1c(VIEWPORT_W/2+200-400*frand(), 1000, BigFairy, stage4_bigcircle, 2.0*I);

	FROM_TO(2600, 3000, 10)
		create_enemy1c(20.0*I+VIEWPORT_H/3*I*frand()+VIEWPORT_W, 100, Swirl, stage4_explosive, -3);

	AT(3200)
		global.boss = create_kurumi_mid();

	FROM_TO(3201, 3601, 10)
		create_enemy1c(VIEWPORT_W*(_i&1)+VIEWPORT_H/2*I-300.0*I*frand(), 200, Fairy, stage4_fodder, 2-4*(_i&1)+1.0*I);

	FROM_TO(3500, 4000, 100)
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 1000, BigFairy, stage4_cardbuster, VIEWPORT_W/6.0*(_i&1)+100.0*I,
					VIEWPORT_W/4.0+VIEWPORT_W/2.0*((_i+1)&1)+300.0*I, VIEWPORT_W/2.0-200.0*I);

	AT(3800)
		create_enemy1c(VIEWPORT_W/2, 7000, BigFairy, stage4_supercard, 4.0*I);

	FROM_TO(4300, 4600, 95-10*global.diff)
		create_enemy1c(VIEWPORT_W*(_i&1)+100*I, 200, Swirl, stage4_backfire, frand()*(1-2*(_i&1)));

	FROM_TO(4800, 5200, 10)
		create_enemy1c(20.0*I+I*VIEWPORT_H/3*frand()+VIEWPORT_W*(_i&1), 100, Swirl, stage4_explosive, (1-2*(_i&1))*3+I);

	AT(5300)
		global.boss = create_kurumi();

	AT(5400)
		global.dialog = stage4_dialog_end();

	AT(5550 - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
