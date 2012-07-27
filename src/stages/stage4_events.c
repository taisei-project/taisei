/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4_events.h"
#include "stage4.h"
#include <global.h>

Dialog *stage4_post_mid_dialog() {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", NULL);
		
	dadd_msg(d, Left, "Hey! Stop!");
		
	return d;
}


int stage4_greeter(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,2,0,0);
		return 1;
	}
	
	e->moving = 1;
	e->dir = creal(e->args[0]) < 0;
	
	if(t < 50 || t > 200)
		e->pos += e->args[0];
		
	FROM_TO(100, 180, 20) {
		int i;
		
		for(i = -(int)global.diff; i <= (int)global.diff; i++) {			
			create_projectile2c("bullet", e->pos, rgb(0.0,0.0,1.0), asymptotic, (3.5+(global.diff == D_Lunatic))*cexp(I*carg(global.plr.pos-e->pos) + 0.06I*i), 5);
		}
	}
	
	return 1;
}
	
int stage4_lightburst(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 4, 1, 0, 0);
		return 1;
	}
	
	if(t < 70 || t > 200)
		e->pos += e->args[0];
	
	FROM_TO(20, 170, 5) {
		int i;
		int c = 5+global.diff;
		for(i = 0; i < c; i++) {
			complex n = cexp(I*carg(global.plr.pos) + 2I*M_PI/c*i);
			create_projectile2c("ball", e->pos + 50*n*cexp(-0.4I*_i*global.diff), rgb(0.3, 0, 0.7), asymptotic, 3*n, 3);
		}
	}
	
	return 1;
}

int stage4_swirl(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1, 0, 0, 0);
		return 1;
	}
	
	if(t > creal(e->args[1]) && t < cimag(e->args[1]))
		e->args[0] *= e->args[2];
	
	e->pos += e->args[0];
	
	FROM_TO(0, 400, 20-global.diff*3) {
		create_projectile2c("bullet", e->pos, rgb(0.3, 0.4, 0.5), asymptotic, 2*e->args[0]*I/cabs(e->args[0]), 3);
		create_projectile2c("bullet", e->pos, rgb(0.3, 0.4, 0.5), asymptotic, -2*e->args[0]*I/cabs(e->args[0]), 3);
	}
	
	return 1;
}

int stage4_limiter(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 4, 2, 0, 0);
		return 1;
	}
	
	e->pos += e->args[0];
	
	FROM_TO(0, 1200, 3) {
		create_projectile2c("rice", e->pos, rgb(0.5,0.1,0.2), asymptotic, 10*cexp(I*carg(global.plr.pos-e->pos)+0.2I-0.1I*(global.diff/4)+3.0I/(_i+1)), 2);
		create_projectile2c("rice", e->pos, rgb(0.5,0.1,0.2), asymptotic, 10*cexp(I*carg(global.plr.pos-e->pos)-0.2I+0.1I*(global.diff/4)-3.0I/(_i+1)), 2);
	}
	
	
	return 1;
}
	
int stage4_laserfairy(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 5, 5, 0, 0);
		return 1;
	}
	
	FROM_TO(0, 100, 1)
		e->pos += e->args[0];
		
	if(t > 700)
		e->pos -= e->args[0];
	
	FROM_TO(100, 700, (7-global.diff)*(1+(int)creal(e->args[1]))) {
		complex n = cexp(I*carg(global.plr.pos-e->pos)+(0.2-0.02*global.diff)*I*_i);
		float fac = (0.5+0.2*global.diff);
		create_lasercurve2c(e->pos, 100, 300, rgb(0.7, 0.3, 1), las_accel, fac*4*n, fac*0.05*n);
		create_projectile2c("plainball", e->pos, rgb(0.7, 0.3, 1), accelerated, fac*4*n, fac*0.05*n)->draw = ProjDrawAdd;
	}
	return 1;
}

int stage4_miner(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2, 0, 0, 0);
		return 1;
	}
	
	e->pos += e->args[0];
	
	FROM_TO(0, 600, 5-global.diff/2) {
		create_projectile1c("rice", e->pos + 20*cexp(2I*M_PI*frand()), rgb(0,0,cabs(e->args[0])), linear, cexp(2I*M_PI*frand()));
	}
	
	return 1;
}

int stage4_explosion(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 5, 5, 0, creal(e->args[1]));
		return 1;
	}
	
	FROM_TO(0, 80, 1)
		e->pos += e->args[0];
	
	FROM_TO(90, 300, 7-global.diff)
		create_projectile2c("soul", e->pos, rgb(0,0,1), asymptotic, 4*cexp(0.5I*_i), 3)->draw = ProjDrawAdd;
	
	FROM_TO(200, 720, 6-global.diff) {
		create_projectile2c("rice", e->pos, rgb(1,0,0), asymptotic, 2*cexp(-0.3I*_i+frand()*I), 3);
		create_projectile2c("rice", e->pos, rgb(1,0,0), asymptotic, -2*cexp(-0.3I*_i+frand()*I), 3);
	}
	
	FROM_TO(500, 800, 80-5*global.diff)
		create_laserline(e->pos, 10*cexp(I*carg(global.plr.pos-e->pos)+0.04I*(1-2*frand())), 60, 120, rgb(1, 0.3, 1));
		
	return 1;
}
		

void iku_mid_intro(Boss *b, int t) {
	TIMER(&t);
	
	b->pos += -1-7I+10*t*(cimag(b->pos)<-200);
	
	FROM_TO(90, 110, 10) {
		create_enemy2c(b->pos, ENEMY_IMMUNE, Swirl, stage4_explosion, -2-0.5*_i+I*_i, _i == 1);
	}
	
	AT(960)
		killall(global.enemies);
}

Boss *create_iku_mid() {
	Boss *b = create_boss("Bombs?", "iku", VIEWPORT_W+800I);
	
	boss_add_attack(b, AT_SurvivalSpell, "Static Bombs", 16, 10, iku_mid_intro, NULL);
	
	return b;
}

int stage4_lightburst2(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 4, 3, 0, 0);
		return 1;
	}
	
	if(t < 70 || t > 200)
		e->pos += e->args[0];
	
	FROM_TO(20, 170, 5) {
		int i;
		int c = 5+global.diff;
		for(i = 0; i < c; i++) {
			complex n = cexp(I*carg(global.plr.pos-e->pos) + 2I*M_PI/c*i);
			create_projectile2c("bigball", e->pos + 50*n*cexp(-1I*_i*global.diff), rgb(0.3, 0, 0.7+0.3*(_i&1)), asymptotic, 2.5*n+0.25*global.diff*frand()*cexp(2I*M_PI*frand()), 3);
		}
	}
	
	return 1;
}

int stage4_superbullet(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 4, 3, 0, 0);
		return 1;
	}
	
	FROM_TO(0, 70, 1)
		e->pos += e->args[0];
		
	FROM_TO(60, 200, 1) {
		complex n = cexp(I*M_PI*sin(_i/(8.0+global.diff)+frand()*0.1)+I*carg(global.plr.pos-e->pos));
		
		create_projectile2c("bullet", e->pos + 50*n, rgb(0.6, 0, 0), asymptotic, 2*n, 10);
	}
	
	FROM_TO(260, 400, 1)
		e->pos -= e->args[0];
	return 1;
}

void iku_intro(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2+300I, 0.01);
}

void iku_bolts(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);
	
	FROM_TO(0, 400, 2)
		create_projectile2c("bigball", VIEWPORT_W*frand()-15I, rgb(1,0,0), accelerated, 1-2*frand()+1.7I, -0.01I)->draw = ProjDrawSub;
		
	FROM_TO(60, 400, 50) {
		int i, c = 10+global.diff;
		for(i = 0; i < c; i++) {
			create_projectile2c("ball", b->pos, rgb(0.4, 1, 1), asymptotic, (i+2)*0.4*cexp(I*carg(global.plr.pos-b->pos))+0.2*(global.diff-1)*frand(), 3)->draw = ProjDrawAdd;
		}
	}
	
	FROM_TO(0, 70, 1)
		GO_TO(b, 100+300I, 0.02);
		
	FROM_TO(100, 200, 1)
		GO_TO(b, VIEWPORT_W/2+100I, 0.02);
		
	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+300I, 0.02);
		
	FROM_TO(330, 400, 1)
		GO_TO(b, VIEWPORT_W/2+100I, 0.02);
		
}

void iku_atmospheric(Boss *b, int time) {
	if(time < 0) {
		GO_TO(b, VIEWPORT_W/2+200I, 0.06);
		return;
	}
	
	int t = time % 500;
	TIMER(&t);
	
	FROM_TO(0, 500, 23-2*global.diff) {
		complex p1 = VIEWPORT_W*frand() + VIEWPORT_H/2*I*frand();
		complex p2 = p1 + (120+20*global.diff)*cexp(0.5I-frand()*I)*(1-2*(frand() > 0.5));
		
		int i;
		int c = 10;
		
		for(i = -c*0.5; i <= c*0.5; i++) {
			create_projectile2c("ball", p1+(p2-p1)/c*i, rgb(1/(1+fabs(0.3*i)), 1, 1), accelerated, 0, (0.005+0.001*global.diff)*cexp(I*carg(p2-p1)+I*M_PI/2+0.2I*i))->draw = ProjDrawAdd;
		}
	}
	
	FROM_TO(0, 500, 7-global.diff)
		create_projectile2c("thickrice", VIEWPORT_W*frand(), rgb(0,0.3,0.7), accelerated, 0, 0.01I);
}

complex bolts2_laser(Laser *l, float t) {
	return creal(l->args[0])+I*cimag(l->pos) + copysign(1,cimag(l->args[0]-l->pos))*0.06I*t*t + (20+4*global.diff)*sin(t*0.1+creal(l->args[0]));
}

void iku_bolts2(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);
	
	FROM_TO(0, 400, 2)
		create_projectile2c("bigball", VIEWPORT_W*frand()-15I, rgb(1,0,0), accelerated, 1-2*frand()+1.7I, -0.01I)->draw = ProjDrawSub;
		
	FROM_TO(0, 400, 60)
		create_lasercurve1c(creal(global.plr.pos), 100, 200, rgb(0.3,1,1), bolts2_laser, global.plr.pos);
	
	FROM_TO(0, 400, 5-global.diff)
		if(frand() < 0.9)
			create_projectile1c("plainball", b->pos, rgb(0.2,0,0.8), linear, cexp(0.1I*_i));
	
	FROM_TO(0, 70, 1)
		GO_TO(b, 100+200I, 0.02);
				
	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+200I, 0.02);
		
}

int lightning_slave(Enemy *e, int t) {
	if(t < 0)
		return 1;
	if(t > 200)
		return ACTION_DESTROY;
	
	TIMER(&t);
	
	e->pos += e->args[0];
	
	FROM_TO(0,200,20)
		e->args[0] *= cexp(2I*M_PI*frand());
	
	FROM_TO(0, 200, 3)
		if(cabs(e->pos-global.plr.pos) > 60)
			create_projectile2c("wave", e->pos, rgb(0, 1, 1), accelerated, 0.5*e->args[0]/cabs(e->args[0])*I, 0.005*(1-2*frand()+I*frand()))->draw = ProjDrawAdd;

	return 1;
}
	

void iku_lightning(Boss *b, int time) {
	int t = time % 151;
	
	if(time == EVENT_DEATH) {
		killall(global.enemies);
		return;
	}
	
	if(time < 0) {
		GO_TO(b, VIEWPORT_W/2.0+200I, 0.03);
		return;
	}
	
	TIMER(&t);	
			
	FROM_TO(0, 100, 1) {
		complex n = cexp(2I*M_PI*frand());
		float l = 200*frand()+100;
		float s = 4+_i*0.01;
		create_particle2c("flare", b->pos+l*n, rgb(0.7,1,1), Shrink, timeout_linear, l/s, -s*n);
	}
	
	AT(150) {
		int i;
		for(i = 0; i < global.diff+1; i++)
			create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, lightning_slave, 10*cexp(I*carg(global.plr.pos - b->pos)+2I*M_PI/(global.diff+1)*i));
	}
}

void iku_bolts3(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);
	
	FROM_TO(0, 400, 2)
		create_projectile2c("bigball", VIEWPORT_W*frand()-15I, rgb(1,0,0), accelerated, 1-2*frand()+1.8I, -0.01I)->draw = ProjDrawSub;
		
	FROM_TO(60, 400, 60) {
		int i, c = 10+global.diff;
		complex n = cexp(I*carg(global.plr.pos-b->pos)+0.1I-0.2I*frand());
		for(i = 0; i < c; i++) {
			create_projectile2c("ball", b->pos, rgb(0.4, 1, 1), asymptotic, (i+2)*0.4*n+0.2*(global.diff-1)*frand(), 3)->draw = ProjDrawAdd;
		}
	}
	
	FROM_TO(0, 400, 5-global.diff)
		if(frand() < 0.9)
			create_projectile1c("plainball", b->pos, rgb(0.2,0,0.8), linear, cexp(0.1I*_i));
	
	FROM_TO(0, 70, 1)
		GO_TO(b, 100+200I, 0.02);
				
	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+200I, 0.02);
		
}


int induction_bullet(Projectile *p, int t) {
	if(t < 0)
		return 1;
	
	p->pos = p->pos0 + p->args[0]*t*cexp(p->args[1]*t);
	p->angle = carg(p->args[0]*cexp(p->args[1]*t)*(1+p->args[1]*t));
	return 1;
}

complex cathode_laser(Laser *l, float t) {
	return l->pos + l->args[0]*t*cexp(l->args[1]*t);
}

void iku_cathode(Boss *b, int t) {
	if(t < 0) {
		GO_TO(b, VIEWPORT_W/2+200I, 0.02);
		return;
	}
	
	TIMER(&t)
	
	FROM_TO(50, 1800, 70-global.diff*10) {
		int i;
		int c = 5+global.diff;
		
		for(i = 0; i < c; i++) {
			create_projectile2c("bigball", b->pos, rgb(0.2, 0.4, 1), induction_bullet, 2*cexp(2I*M_PI*frand()), 0.01*I*(1-2*(_i&1)))->draw = ProjDrawAdd;
			create_lasercurve2c(b->pos, 60, 200, rgb(0.4, 1, 1), cathode_laser, 2*cexp(2I*M_PI*M_PI*frand()), 0.015*I*(1-2*(_i&1)));
		}
	}
}

void iku_induction(Boss *b, int t) {
	if(t < 0) {
		GO_TO(b, VIEWPORT_W/2+200I, 0.02);
		return;
	}
		
	TIMER(&t);

	FROM_TO(0, 1800, 8) {
		int i,j;
		int c = 5+global.diff;
		for(i = 0; i < c; i++) {
			for(j = 0; j < 2; j++)
				create_projectile2c("ball", b->pos, rgb(0, 1, 1), induction_bullet, 2*cexp(2I*M_PI/c*i+I*M_PI/2+0.6I*(_i/6)), (0.01+0.001*global.diff)*I*(1-2*j))->draw = ProjDrawAdd;			
		}
		
	}
}

void iku_spell_bg(Boss *b, int t);

Boss *create_iku() {
	Boss *b = create_boss("Nagae Iku", "iku", VIEWPORT_W/2-200I);
	
	boss_add_attack(b, AT_Move, "Introduction", 3, 0, iku_intro, NULL);
	boss_add_attack(b, AT_Normal, "Bolts1", 20, 20000, iku_bolts, NULL);
	boss_add_attack(b, AT_Spellcard, "High Voltage ~ Atmospheric Discharge", 30, 30000, iku_atmospheric, iku_spell_bg);
	boss_add_attack(b, AT_Normal, "Bolts2", 25, 20000, iku_bolts2, NULL);
	boss_add_attack(b, AT_Spellcard, "Charge Sign ~ Artificial Lightning", 30, 35000, iku_lightning, iku_spell_bg);
	boss_add_attack(b, AT_Normal, "Bolts3", 20, 20000, iku_bolts3, NULL);
	boss_add_attack(b, AT_Spellcard, "Spark Sign ~ Natural Cathode", 30, 35000, iku_cathode, iku_spell_bg);
	boss_add_attack(b, AT_Spellcard, "Current Sign ~ Induction", 30, 35000, iku_induction, iku_spell_bg);
		
	return b;
}

void stage4_events() {
	TIMER(&global.timer);
		
	FROM_TO(60, 120, 10)
		create_enemy1c(VIEWPORT_W+70I+50*_i*I, 300, Fairy, stage4_greeter, -3);
	
	FROM_TO(270, 320, 25)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*_i, 2000, BigFairy, stage4_lightburst, 2I);
		
	FROM_TO(500, 600, 10)
		create_enemy3c(200I*frand(), 500, Swirl, stage4_swirl, 4+I, 70+20*frand()+200I, cexp(-0.05I));
		
	FROM_TO(700, 800, 10)
		create_enemy3c(VIEWPORT_W+200I*frand(), 500, Swirl, stage4_swirl, -4+frand()*I, 70+20*frand()+200I, cexp(0.05I));
		
	FROM_TO(870, 1000, 50)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*(_i&1), 2000, BigFairy, stage4_limiter, I);
		
	AT(1000)
		create_enemy1c(VIEWPORT_W/2, 9000, BigFairy, stage4_laserfairy, 2I);
		
	FROM_TO(1900, 2200, 40)
		create_enemy1c(VIEWPORT_W+200I*frand(), 500, Swirl, stage4_miner, -3+2I*frand());
	
	FROM_TO(1500, 2400, 80)
		create_enemy1c(VIEWPORT_W*(_i&1)+100I, 300, Fairy, stage4_greeter, 3-6*(_i&1));
		
	FROM_TO(2200, 2600, 40)
		create_enemy1c(VIEWPORT_W/10*_i, 200, Swirl, stage4_miner, 3I);
		
	AT(2900)
		global.boss = create_iku_mid();
		
	AT(2920)
		global.dialog = stage4_post_mid_dialog();
		
	FROM_TO(3000, 3200, 100)
		create_enemy1c(VIEWPORT_W/2 + VIEWPORT_W/6*(1-2*(_i&1)), 2000, BigFairy, stage4_lightburst2, -1+2*(_i&1) + 2I);
		
	FROM_TO(3300, 4000, 50)
		create_enemy1c(200I+VIEWPORT_W*(_i&1), 1500, Fairy, stage4_superbullet, 3-6*(_i&1));
	
	AT(3400) {
		create_enemy2c(VIEWPORT_W/4, 6000, BigFairy, stage4_laserfairy, 2I, 1);
		create_enemy2c(VIEWPORT_W/4*3, 6000, BigFairy, stage4_laserfairy, 2I, 1);
	}
	
	FROM_TO(4200, 5000, 20) {
		float f = frand();
		create_enemy3c(VIEWPORT_W/2, 400, Swirl, stage4_swirl, 2*cexp(I*M_PI*f)+I, 60 + 100I, cexp(0.01I*(1-2*(f<0.5))));
	}
	
	AT(5300)
		global.boss = create_iku();
}