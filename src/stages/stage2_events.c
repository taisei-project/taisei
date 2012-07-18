/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage2_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"

float nfrand() {
	return (frand() - 0.5) * 2;
}

int stage2_enterswirl(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1, 1, 0, 0);
		
		float r, g;
		if(frand() > 0.5) {
			r = 0.3 + 0.3 * frand();
			g = 0.7;
		} else {
			r = 0.7;
			g = 0.3 + 0.3 * frand();
		}
		
		float a; for(a = 0; a < M_PI * 2; a += 1.3 - global.diff * 0.2) {
			complex dir = sin(a) + I * cos(a);
			float spd = 1 + 0.5 * sin(10 * a);
			
			create_projectile2c("rice", e->pos, rgb(r, g, 0.7), accelerated,
				dir * 2,
				dir * spd * -0.03
			);
		}
		
		return 1;
	}
	
	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}
	
	AT(60) {
		e->hp = 0;
	}
	
	e->pos += e->args[0];
	
	return 0;
}

int stage2_slavefairy(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1, 3, 0, 0);
		return 1;
	}
	
	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}
	
	GO_TO(e, e->args[0], 0.03)
	
	FROM_TO_INT(30, 120, 5 - global.diff, 1, 1) {
		float a = global.timer * 0.5;
		if(_i % 2)
			a = -a;
		complex dir = sin(a) + I * cos(a);
		
		create_projectile2c("wave", e->pos + dir * 10, (global.timer % 2)? rgb(1.0, 0.6, 0.6) : rgb(0.6, 0.6, 1.0), accelerated,
			dir * 3,
			dir * -0.05
		);
	}
	
	AT(120) {
		e->hp = 0;
	}
	
	return 0;
}

int stage2_bigfairy(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		if(e->args[0])
			spawn_items(e->pos, 5, 5, 1, 0);
		return 1;
	}
	
	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}
	
	FROM_TO(30, 600, 270) {
		create_enemy1c(e->pos, 300, Fairy, stage2_slavefairy, e->pos + 70 + 50 * I);
		create_enemy1c(e->pos, 300, Fairy, stage2_slavefairy, e->pos - 70 + 50 * I);
	}
	
	FROM_TO(120, 600, 270) {
		create_enemy1c(e->pos, 300, Fairy, stage2_slavefairy, e->pos + 70 - 50 * I);
		create_enemy1c(e->pos, 300, Fairy, stage2_slavefairy, e->pos - 70 - 50 * I);
	}
	
	AT(600)
		e->hp = 0;
	
	return 0;
}

int stage2_bitchswirl(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1, 1, 0, 0);
		return -1;
	}
	
	FROM_TO(0, 120, 20) {
		float c = 0.25 + 0.25 * sin(t / 5.0);
		
		create_projectile2c("flea", e->pos, rgb(1.0 - c, 0.6, 0.5 + c), accelerated,
			2*cexp(I*carg(global.plr.pos - e->pos)),
			0.005*cexp(I*(M_PI_2 * nfrand()))
		);
	}
	
	e->pos -= 5I;
	
	AT(120) {
		e->hp = 0;
	}
	
	return 0;
}

int stage2_cornerfairy(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 5, 5, 0, 0);
		return -1;
	}
	
	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}
	
	FROM_TO(0, 60, 1)
		GO_TO(e, e->args[0], 0.01)
	
	FROM_TO(60, 120, 1) {
		GO_TO(e, e->args[1], 0.05)
		
		if(!(t % (D_Lunatic - global.diff + 2))) {
			float i; for(i = -M_PI; i <= M_PI; i += 1.0) {
				float c = 0.25 + 0.25 * sin(t / 5.0);
				
				create_projectile2c("thickrice", e->pos, rgb(1.0 - c, 0.6, 0.5 + c), asymptotic,
					//2*cexp(I*(carg(global.plr.pos - e->pos) + i)),
					2*cexp(I*(i+carg((VIEWPORT_W+I*VIEWPORT_H)/2 - e->pos))),
					1.5
				);
			}
		}
	}
	
	AT(180)
		e->hp = 0;
	
	return 0;
}

void stage2_events() {
	TIMER(&global.timer);
	
	//AT(0)
	//	global.timer = 1460;
	
	FROM_TO(160, 300, 10) {
		create_enemy1c(VIEWPORT_W/2 + 20 * nfrand() + (VIEWPORT_H/4 + 20 * nfrand())*I, 200, Swirl, stage2_enterswirl, I * 3 + nfrand() * 3);
	}
	
	AT(400) {
		create_enemy1c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage2_bigfairy, 0);
	}
	
	FROM_TO(1100, 1300, 10) {
		create_enemy1c(20 + (VIEWPORT_H+20)*I, 50, Swirl, stage2_bitchswirl, 0);
		create_enemy1c(VIEWPORT_W-20 + (VIEWPORT_H+20)*I, 50, Swirl, stage2_bitchswirl, 0);
	}
	
	AT(1500) {
		create_enemy1c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage2_bigfairy, 1);
	}
	
	FROM_TO(2300, 2520, 130) {
		double offs = -50;
		
		complex p1 = 0+0I;
		complex p2 = VIEWPORT_W+0I;
		complex p3 = VIEWPORT_W + VIEWPORT_H*I;
		complex p4 = 0+VIEWPORT_H*I;
		
		create_enemy2c(p1, 500, Fairy, stage2_cornerfairy, p1 - offs - offs*I, p2 + offs - offs*I);
		create_enemy2c(p2, 500, Fairy, stage2_cornerfairy, p2 + offs - offs*I, p3 + offs + offs*I);
		create_enemy2c(p3, 500, Fairy, stage2_cornerfairy, p3 + offs + offs*I, p4 - offs + offs*I);
		create_enemy2c(p4, 500, Fairy, stage2_cornerfairy, p4 - offs + offs*I, p1 - offs - offs*I);
	}
}
