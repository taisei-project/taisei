/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "stage2_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"

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
	
	if(t < 120)
		GO_TO(e, e->args[0], 0.03)
	
	FROM_TO_INT(30, 120, 5 - global.diff, 1, 1) {
		float a = global.timer * 0.5;
		if(_i % 2)
			a = -a;
		complex dir = sin(a) + I * cos(a);
		
		create_projectile2c("wave", e->pos + dir * 10, (global.timer % 2)? rgb(1.0, 0.5, 0.5) : rgb(0.5, 0.5, 1.0), accelerated,
			dir * 3,
			dir * -0.05
		);
		
		if(e->args[1] && !(_i % 10)) create_projectile1c("ball", e->pos + dir * 10, rgb(0.3, 0.6, 0.3), linear,
			dir * (0.3 + 0.5 + 0.5 * sin(a * 3))
		);
	}
	
	if(t >= 120)
		e->pos += 3 * e->args[2] + 2I;
	
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
		create_enemy3c(e->pos, 900, Fairy, stage2_slavefairy, e->pos + 70 + 50 * I, e->args[0], +1);
		create_enemy3c(e->pos, 900, Fairy, stage2_slavefairy, e->pos - 70 + 50 * I, e->args[0], -1);
	}
	
	FROM_TO(120, 600, 270) {
		create_enemy3c(e->pos, 900, Fairy, stage2_slavefairy, e->pos + 70 - 50 * I, e->args[0], +1);
		create_enemy3c(e->pos, 900, Fairy, stage2_slavefairy, e->pos - 70 - 50 * I, e->args[0], -1);
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
		float c = 0.25 + 0.25 * sin(t / 15.0);
		
		create_projectile2c("flea", e->pos, rgb(0.5 + c, 0.8, 0.4), accelerated,
			2*cexp(I*carg(global.plr.pos - e->pos)),
			0.005*cexp(I*(M_PI*2 * frand()))
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
		int d = (D_Lunatic - global.diff + 2);
		if(!(t % d)) {
			float i; for(i = -M_PI; i <= M_PI; i += 1.0) {
				float c = 0.25 + 0.25 * sin(t / 5.0);
				
				create_projectile2c("thickrice", e->pos, rgb(1.0 - c, 0.6, 0.5 + c), asymptotic,
					//2*cexp(I*(carg(global.plr.pos - e->pos) + i)),
					2*cexp(I*(i+carg((VIEWPORT_W+I*VIEWPORT_H)/2 - e->pos))),
					1.5
				);
				
				if(global.diff > D_Easy && !(t % 3*d)) {
					create_projectile2c("flea", e->pos, rgb(0.5 + 1.2 * c, 0.8, 0.5), asymptotic,
						2*cexp(I*(carg(global.plr.pos - e->pos) + i)),
						1.5
					);
				}
			}
		}
	}
	
	AT(180)
		e->hp = 0;
	
	return 0;
}

Dialog* stage2_dialog() {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "masterspark");
	
	dadd_msg(d, Right, "Hurrrrr durr herp a derp!");
	dadd_msg(d, Left, "Fuck fuck fuckity fuck!");
	dadd_msg(d, Right, "HURR DURR A DERP A HERP HERP LOL DERP.");
	
	return d;
}

void stage2_mid_intro(Boss *boss, int time) {
	GO_TO(boss, VIEWPORT_W/2.0 + 100I, 0.03);
}

void stage2_mid_outro(Boss *boss, int time) {
	if(time == 0) {
		spawn_items(boss->pos, 10, 10, 0, 1);
		Projectile *p;
		for(p = global.projs; p; p = p->next)
			p->type = DeadProj;
	}
	
	boss->pos += pow(max(0, time)/30.0, 2) * cexp(I*(3*M_PI/2 + 0.5 * sin(time / 20.0)));
}

int stage2_mid_poison(Projectile *p, int time) {
	int result = accelerated(p, time);
	
	if(!(time % (57 - global.diff * 3))) {
		float a = p->args[2];
		float t = p->args[3] + time;
		
		create_projectile2c((frand() > 0.5)? "thickrice" : "rice", p->pos, rgb(0.3, 0.7 + 0.3 * psin(a/3.0 + t/20.0), 0.3), accelerated,
				0,
				0.005*cexp(I*(M_PI*2 * sin(a/5.0 + t/20.0)))
		);
	}
	
	return result;
}

void stage2_mid_a1(Boss *boss, int time) {
	int i;
	TIMER(&time);
	
	FROM_TO(0, 120, 1)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.03)
	
	if(time > 120) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2 + sin(time/30.0) * time/5.0 * cexp(I * M_PI_2 * time/100.0), 0.03)
		
		if(!(time % 70)) {
			for(i = 0; i < 15; ++i) {
				float a = M_PI/(5 + global.diff) * i * 2;
				create_projectile4c("wave", boss->pos, rgb(0.3, 0.3 + 0.7 * psin(a*3 + time/50.0), 0.3), stage2_mid_poison,
					0,
					0.02 * cexp(I*(a+time/10.0)),
					a,
					time
				);
			}
		}
		
		if(global.diff > D_Easy && !(time % 35)) {
			int cnt = global.diff * 3;
			for(i = 0; i < cnt; ++i) create_projectile2c("ball", boss->pos, rgb(1.0, 1.0, 0.3), asymptotic,
				(0.5 + 3 * psin(time + M_PI/3*2*i)) * cexp(I*(time / 20.0 + M_PI/cnt*i*2)),
				1.5
			);
		}
		
		if(!(time % 3)) {
			for(i = -1; i < 2; i += 2) {
				float c = psin(time/10.0);
				create_projectile1c("crystal", boss->pos, rgba(0.3 + c * 0.7, 0.6 - c * 0.3, 0.3, 0.7), linear,
					10 * cexp(I*(carg(global.plr.pos - boss->pos) + (M_PI/4.0 * i * (1-time/2300.0)) * (1 - 0.5 * psin(time/10.0))))
				);
			}
		}
	}
}

void stage2_mid_spellbg(Boss *h, int time) {
	float b = (0.3 + 0.2 * (sin(time / 50.0) * sin(time / 25.0f + M_PI))) * min(time/20.0, 1);
	glColor4f(b, b, b, 1);
	fill_screen(-time/50.0 + 0.5, time/100.0+0.5, 1, "stage2/spellbg1");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	fill_screen(time/50.0 + 0.5, time/90.0+0.5, 1, "stage2/spellbg1");
	fill_screen(-time/55.0 + 0.5, -time/100.0+0.5, 1, "stage2/spellbg1");
	fill_screen(time/55.0 + 0.5, -time/90.0+0.5, 1, "stage2/spellbg1");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1, 1, 1, 1);
}

Boss* stage2_create_midboss() {
	Boss* scuttle = create_boss("Scuttle", "scuttle", VIEWPORT_W/2 - 200I);
	boss_add_attack(scuttle, AT_Move, "Introduction", 2, 0, stage2_mid_intro, NULL);
	boss_add_attack(scuttle, AT_Spellcard, "Venom Sign ~ Deadly Dance", 30, 20000, stage2_mid_a1, stage2_mid_spellbg);
	boss_add_attack(scuttle, AT_Move, "Runaway", 2, 1, stage2_mid_outro, NULL);
	scuttle->zoomcolor = rgb(0.4, 0.5, 0.4);
	
	start_attack(scuttle, scuttle->attacks);
	return scuttle;
}

void stage2_events() {
	TIMER(&global.timer);
	
//	AT(0)
//		global.timer = 2300;
	
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
	
	AT(1600) {
		create_enemy1c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage2_bigfairy, 1);
	}
	
	FROM_TO(2400, 2620, 130) {
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
	
	AT(2800)
		global.boss = stage2_create_midboss();
}
