/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4_events.h"
#include <global.h>


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
	
	FROM_TO(100, 700, 5)
		create_laser(LT_Curve, e->pos, 0, 100, 800, rgb(0.7, 0.3, 1), las_accel, 4*cexp(0.2I*_i));
		
	return 1;
}
	
void stage4_events() {
	TIMER(&global.timer);
	
	AT(0)
		global.timer = 1000;
	
	FROM_TO(60, 120, 10) {
		create_enemy1c(VIEWPORT_W+70I+50*_i*I, 300, Fairy, stage4_greeter, -3);
	}
	
	FROM_TO(270, 320, 25)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*_i, 2000, BigFairy, stage4_lightburst, 2I);
		
	FROM_TO(500, 600, 10)
		create_enemy3c(200I*frand(), 500, Swirl, stage4_swirl, 4+I, 70+20*frand()+200I, cexp(-0.05I));
		
	FROM_TO(700, 800, 10)
		create_enemy3c(VIEWPORT_W+200I*frand(), 500, Swirl, stage4_swirl, -4+frand()*I, 70+20*frand()+200I, cexp(0.05I));
		
	FROM_TO(870, 1200, 50)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*(_i&1), 2000, BigFairy, stage4_limiter, I);
		
	AT(1000)
		create_enemy1c(VIEWPORT_W/2, 3000, BigFairy, stage4_laserfairy, 2I);
}