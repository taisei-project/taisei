/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage6_events.h"
#include "stage6.h"
#include <global.h>

int stage6_hacker(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3, 4, 0, 0);
		return 1;
	}
	
	FROM_TO(0, 70, 1)
		e->pos += e->args[0];
		
	FROM_TO(100, 180+40*global.diff, 3) {
		int i;
		for(i = 0; i < 6; i++) {
			complex n = sin(_i*0.2)*cexp(I*0.3*(i/2-1))*(1-2*(i&1));
			create_projectile1c("wave", e->pos + 120*n, rgb(1.0, 0.2-0.01*_i, 0.0), linear, (0.25-0.5*frand())*global.diff+creal(n)+2I);
		}
	}
	
	FROM_TO(180+40*global.diff+60, 2000, 1)
		e->pos -= e->args[0];
	return 1;
}

int stage6_side(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3, 4, 0, 0);
		return 1;
	}
	
	if(t < 60 || t > 120)
		e->pos += e->args[0];
	
	AT(70) {
		int i;
		int c = 15+10*global.diff;
		for(i = 0; i < c; i++) {
			create_projectile2c("rice", e->pos+5*(i/2)*e->args[1], rgb(0, 0.5, 1), accelerated, 1I-2I*(i&1), (0.001-0.0001*global.diff)*(i/2)*e->args[1]);
		}
	}
	
	return 1;
}

int wait_proj(Projectile *p, int t) {
	if(t > creal(p->args[1])) {
		p->angle = carg(p->args[0]);
		p->pos += p->args[0];
	}
	
	return 1;
}

int stage6_flowermine(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 4, 3, 0, 0);
		return 1;
	}
	
	e->pos += e->args[0];
	
	FROM_TO(70, 200, 1)
		e->args[0] += 0.07*cexp(I*carg(e->args[1]-e->pos));
		
	FROM_TO(0, 1000, 7-global.diff) {
		create_projectile2c("rice", e->pos + 40*cexp(I*0.6*_i+I*carg(e->args[0])), rgb(0.3, 0.8, creal(e->args[2])), wait_proj, I*cexp(I*0.6*_i), 200)->angle = 0.6*_i;
	}
	
	return 1;
}

void stage6_events() {
	TIMER(&global.timer);
	
// 	AT(0)
// 		global.timer = 1300;
	
	AT(100)
		create_enemy1c(VIEWPORT_W/2, 6000, BigFairy, stage6_hacker, 2I);
		
	FROM_TO(500, 700, 10)
		create_enemy2c(VIEWPORT_W*(_i&1), 2000, Fairy, stage6_side, 2I+0.1*(1-2*(_i&1)),1-2*(_i&1));
		
	FROM_TO(720, 940, 10) {
		complex p = VIEWPORT_W/2+(1-2*(_i&1))*20*(_i%10);
		create_enemy2c(p, 2000, Fairy, stage6_side, 2I+1*(1-2*(_i&1)),I*cexp(I*carg(global.plr.pos-p))*(1-2*(_i&1)));
	}
	
	FROM_TO(1380, 1660, 20)
		create_enemy2c(200I, 600, Fairy, stage6_flowermine, 2*cexp(0.5I*M_PI/9*_i)+1, 0);
		
	FROM_TO(1600, 2000, 20)
		create_enemy3c(VIEWPORT_W/2, 600, Fairy, stage6_flowermine, 2*cexp(0.5I*M_PI/9*_i+I*M_PI/2)+1I, VIEWPORT_H/2*I+VIEWPORT_W, 1);
}