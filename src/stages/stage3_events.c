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
	
	FROM_TO(100, 200, 20) {
		if(global.diff > D_Easy) {
			create_projectile2c("ball", e->pos, rgb(1, 0.3, 0.3), asymptotic, 2*cexp(I*carg(global.plr.pos - e->pos)+I*0.2*(global.diff/2.0)), 3);
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
	
	FROM_TO(60,100,5) {
		int i;
		for(i = 0; i < global.diff; i++) {
			complex n = cexp(I*M_PI/16.0*_i + I*carg(e->args[0])-I*M_PI/4.0);
			create_projectile2c("wave", e->pos + (30+15*i)*n, rgb(1-0.2*i,0.5,0.7), asymptotic, 2*n, 2);
		}
	}
	
	FROM_TO(160, 200, 1)
		e->args[0] += 0.1I;
	
	return 1;
}

void stage3_events() {
	TIMER(&global.timer);
	
	
	AT(70) {
		create_enemy1c(VIEWPORT_H/4*3*I, 9000, BigFairy, stage3_splasher, 3-4I);
		create_enemy1c(VIEWPORT_W + VIEWPORT_H/4*3*I, 9000, BigFairy, stage3_splasher, -3-4I);
	}
	
	FROM_TO(300, 450, 20) {
		create_enemy1c(VIEWPORT_W*frand(), 1000, Fairy, stage3_fodder, 3I);
	}
	
	FROM_TO(500, 570, 10) {
		int d = frand() < 0.5;
		create_enemy1c(VIEWPORT_W*d+VIEWPORT_H*I, 2000, Fairy, stage3_partcircle, 3*cexp(-I*M_PI/2.0*(frand()+d)));
	}
}