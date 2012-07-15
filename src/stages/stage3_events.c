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
		spawn_items(e->pos, 1,3,0,0);
		return 1;
	}
	
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
	
	FROM_TO(0, 60, 1)
		e->pos += e->args[0]*(1-t/60.0);
	
	AT(100) {
		if(global.diff > D_Easy) {
			int i;
			for(i = 0; i <= global.diff/2; i++) {
				create_projectile2c("ball", e->pos, rgb(0, 0, 0.4), asymptotic, 3*cexp(I*carg(global.plr.pos - e->pos)+I*0.2*(i-global.diff/2.0)), 3);
			}
		}
	}
	
	FROM_TO(200, 300, 1) {
		e->pos -= 3*e->args[0]*(t-200)/100.0;
	}
	
	return 1;
}
	

void stage3_events() {
	TIMER(&global.timer);
	
// 	FROM_TO(30, 200, 20) {
// 		create_laser(LT_Line, VIEWPORT_W/2, 20*cexp(I*0.3*_i), 40, 200, rgb(0.9,0.9,0.0), NULL, 0);
// 	}
	
// 	AT(70) {
// 		create_enemy1c(VIEWPORT_H/4*3*I, 9000, BigFairy, stage3_splasher, 3-4I);
// 		create_enemy1c(VIEWPORT_W + VIEWPORT_H/4*3*I, 9000, BigFairy, stage3_splasher, -3-4I);
// 	}
// 	
// 	FROM_TO(300, 350, 10) {
// 		create_enemy1c(VIEWPORT_H/3*2*I-60*_i*I, 1000, Fairy, stage3_fodder, 2);
// 		create_enemy1c(VIEWPORT_W + VIEWPORT_H/3*2*I-60*_i*I, 1000, Fairy, stage3_fodder, -2);
// 	}
}