/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage3_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"
#include "laser.h"

int stage3_splasher(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1,3,1,0);
		return 1;
	}
	
	e->moving = 1;
	e->dir = creal(e->args[0]) < 0;
	
	FROM_TO(0, 50, 1)
		e->pos += e->args[0]*(1-t/50.0);
	
	FROM_TO(60, 150, 5-global.diff)
		create_projectile2c(frand() > 0.5 ? "rice" : "thickrice", e->pos, rgb(1,0.6-0.2*frand(),0.8), accelerated, e->args[0]/2+(1-2*frand())+(1-2*frand())*I, 0.02I);
	
	FROM_TO(200, 300, 1)
		e->pos -= creal(e->args[0])*(t-200)/100.0;
	
	return 1;
}

int stage3_fodder(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 0,1,0,0);
		return 1;
	}	
		
	e->pos += e->args[0];
	
	FROM_TO(100, 200, 22-global.diff*2) {
		if(global.diff > D_Easy) {
			create_projectile2c("ball", e->pos, rgb(1, 0.3, 0.5), asymptotic, 1*cexp(I*M_PI*2*frand()), 3);
		}
	}
	
	return 1;
}


int stage3_partcircle(Enemy *e, int t) {
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

int stage3_cardbuster(Enemy *e, int t) {
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

int stage3_backfire(Enemy *e, int t) {
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

int stage3_explosive(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		int i;
		spawn_items(e->pos, 0,1,0,0);
		
		int n = 6*global.diff;
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
		
void stage3_events() {
	TIMER(&global.timer);
	
// 	AT(0)
// 		global.timer = 2000;
		
	AT(70) {
		create_enemy1c(VIEWPORT_H/4*3*I, 9000, BigFairy, stage3_splasher, 3-4I);
		create_enemy1c(VIEWPORT_W + VIEWPORT_H/4*3*I, 9000, BigFairy, stage3_splasher, -3-4I);
	}
	
	FROM_TO(300, 450, 20) {
		create_enemy1c(VIEWPORT_W*frand(), 1000, Fairy, stage3_fodder, 3I);
	}
	
	FROM_TO(500, 550, 10) {
		int d = _i&1;
		create_enemy1c(VIEWPORT_W*d, 2000, Fairy, stage3_partcircle, 2*cexp(I*M_PI/2.0*(0.2+0.6*frand()+d)));
	}
	
	FROM_TO(600, 1400, 100) {
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 4000, BigFairy, stage3_cardbuster, VIEWPORT_W/6.0 + VIEWPORT_W/3.0*2*(_i&1)+100I,
					VIEWPORT_W/4.0 + VIEWPORT_W/2.0*((_i+1)&1)+300I, VIEWPORT_W/2.0+VIEWPORT_H*I+200I);
	}
	
	AT(1800) {
		create_enemy1c(VIEWPORT_H/2.0*I, 2000, Swirl, stage3_backfire, 0.3);
		create_enemy1c(VIEWPORT_W+VIEWPORT_H/2.0*I, 2000, Swirl, stage3_backfire, -0.5);
	}
	
	FROM_TO(2000, 2400, 10)
		create_enemy1c(20+(VIEWPORT_W-40)*frand(), 100, Swirl, stage3_explosive, 3I);
}