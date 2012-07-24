/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4_events.h"
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
	
	if((t < 100 && creal(e->pos) > VIEWPORT_W-100) || t > 200)
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
	
	FROM_TO(100, 700, 7-global.diff) {
		complex n = cexp(I*carg(global.plr.pos-e->pos)+(0.2-0.02*global.diff)*I*_i-M_PI/2*I);
		create_lasercurve2c(e->pos, 100, 300, rgb(0.7, 0.3, 1), las_accel, 4*n, 0.05*n);
		create_projectile2c("plainball", e->pos, rgb(0.7, 0.3, 1), accelerated, 4*n, 0.05*n)->draw = ProjDrawAdd;
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
	
	FROM_TO(500, 800, 60-5*global.diff)
		create_laserline1c(e->pos, 10*cexp(I*carg(global.plr.pos-e->pos)+0.04I*(1-2*frand())), 30, 60, rgb(1, 0, 1), NULL, 0);
		
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
	Boss *b = create_boss(" ", "iku", VIEWPORT_W+800I);
	
	boss_add_attack(b, AT_Move, "Introduction", 16, 0, iku_mid_intro, NULL);
	
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
		
	FROM_TO(870, 1200, 50)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*(_i&1), 2000, BigFairy, stage4_limiter, I);
		
	AT(1000)
		create_enemy1c(VIEWPORT_W/2, 6000, BigFairy, stage4_laserfairy, 2I);
		
	FROM_TO(1800, 2200, 40)
		create_enemy1c(VIEWPORT_W+200I*frand(), 500, Swirl, stage4_miner, -3+2I*frand());
		
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
		create_enemy1c(VIEWPORT_W/4, 6000, BigFairy, stage4_laserfairy, 2I);
		create_enemy1c(VIEWPORT_W/4*3, 6000, BigFairy, stage4_laserfairy, 2I);
	}
	
	FROM_TO(4200, 5000, 20) {
		float f = frand();
		create_enemy3c(VIEWPORT_W/2, 600, Swirl, stage4_swirl, 2*cexp(I*M_PI*f)+I, 60 + 100I, cexp(0.01I*(1-2*(f<0.5))));
	}
}