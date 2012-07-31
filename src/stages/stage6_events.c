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
			create_projectile2c("rice", e->pos+5*(i/2)*e->args[1], rgb(0, 0.5, 1), accelerated, (1I-2I*(i&1))*(0.7+0.2*global.diff), 0.001*(i/2)*e->args[1]);
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
		create_projectile2c("rice", e->pos + 40*cexp(I*0.6*_i+I*carg(e->args[0])), rgb(0.3, 0.8, creal(e->args[2])), wait_proj, I*cexp(I*0.6*_i)*(0.7+0.3*global.diff), 200)->angle = 0.6*_i;
	}
	
	return 1;
}

void ScaleFade(Projectile *p, int t) {
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glScalef(creal(p->args[1]), creal(p->args[1]), 1);
	glRotatef(180/M_PI*p->angle, 0, 0, 1);
	if(t/creal(p->args[0]) != 0)
		glColor4f(1,1,1, 1.0 - (float)t/p->args[0]);
	
	draw_texture_p(0, 0, p->tex);
	
	if(t/creal(p->args[0]) != 0)
		glColor4f(1,1,1,1);
	glPopMatrix();
}

int scythe_mid(Enemy *e, int t) {
	complex n;
	
	if(t < 0)
		return 1;
	if(t > 300)
		return ACTION_DESTROY;
		
	e->pos += 6-global.diff-0.005I*t;
	
	n = cexp(cimag(e->args[1])*I*t);
	create_projectile2c("bigball", e->pos + 80*n, rgb(carg(n), 1-carg(n), 1/carg(n)), wait_proj, global.diff*cexp(0.6I)*n, 100)->draw=ProjDrawAdd;
	
	if(global.diff > D_Normal && t&1)
		create_projectile2c("ball", e->pos + 80*n, rgb(0, 0.2, 0.5), accelerated, n, 0.01*global.diff*cexp(I*carg(global.plr.pos - e->pos - 80*n)))->draw=ProjDrawAdd;
	
	return 1;
}

void Scythe(Enemy *e, int t) {
	Projectile *p = create_projectile_p(&global.particles, get_tex("stage6/scythe"), e->pos, NULL, ScaleFade, timeout, 8, e->args[2], 0, 0);
	p->type = Particle;
	p->angle = creal(e->args[1]);
	e->args[1] += cimag(e->args[1]);
	
	create_particle2c("flare", e->pos+100*creal(e->args[2])*frand()*cexp(2I*M_PI*frand()), rgb(1,1,0.6), GrowFadeAdd, timeout_linear, 60, -I+1);
}

int scythe_intro(Enemy *e, int t) {
	if(t < 0)
		return 0;
	
	TIMER(&t);
	
	GO_TO(e,VIEWPORT_W/2+200I, 0.05);
	
	
	FROM_TO(60, 119, 1) {
		e->args[1] -= 0.00333333I;
		e->args[2] -= 0.007;
	}
			
	return 1;
}

void elly_intro(Boss *b, int t) {
	TIMER(&t);
	GO_TO(b, VIEWPORT_W/2+200I, 0.01);
	
	AT(200) {
		create_enemy3c(VIEWPORT_W+200+200I, ENEMY_IMMUNE, Scythe, scythe_intro, 0, 1+0.2I, 1);
	}
}

int scythe_infinity(Enemy *e, int t) {
	if(t < 0)
		return 1;
	
	TIMER(&t);
	FROM_TO(0, 40, 1) {
		GO_TO(e, VIEWPORT_W/2+200I, 0.01);
		e->args[2] = min(0.8, creal(e->args[2])+0.0003*t*t);
		e->args[1] = creal(e->args[1]) + I*min(0.2, cimag(e->args[1])+0.0001*t*t);
	}
	
	FROM_TO(40, 3000, 1) {
		float w = min(0.15, 0.0001*(t-40));
		e->pos = VIEWPORT_W/2 + 200I + 200*cos(w*(t-40)+M_PI/2.0) + I*80*sin(2*w*(t-40));
		
		create_projectile2c("ball", e->pos+80*cexp(I*creal(e->args[1])), rgb(cos(creal(e->args[1])), sin(creal(e->args[1])), cos(creal(e->args[1])+2.1)), asymptotic, (1+0.2*global.diff)*cexp(I*creal(e->args[1])), 3);
	}
	
	return 1;
}

int scythe_reset(Enemy *e, int t) {
	if(t < 0)
		return 1;
	
	if(t == 1)
		e->args[1] = fmod(creal(e->args[1]), 2*M_PI) + I*cimag(e->args[1]);
	
	GO_TO(e, VIEWPORT_W/2.0+200I, 0.02);
	e->args[2] = max(0.6, creal(e->args[2])-0.01*t);
	e->args[1] += (0.19-creal(e->args[1]))*0.05;
	e->args[1] = creal(e->args[1]) + I*0.9*cimag(e->args[1]);
	return 1;
}

void elly_frequency(Boss *b, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_infinity;
	}
	AT(EVENT_DEATH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_reset;
	}
		
}

int scythe_newton(Enemy *e, int t) {
	if(t < 0)
		return 1;
	
	TIMER(&t);
	
	FROM_TO(0, 100, 1)
		e->pos -= 0.2I*_i;
	
	AT(100) {
		e->args[1] = 0.2I;
// 		e->args[2] = 1;
	}
	
	FROM_TO(100, 2000, 1) {
		e->pos = VIEWPORT_W/2+I*VIEWPORT_H/2 + 500*cos(_i*0.06)*cexp(I*_i*0.01);
	}
	
	
	FROM_TO(100, 2000, 3) {
		float f = carg(global.plr.pos-e->pos);
		Projectile *p;
		for(p = global.projs; p; p = p->next) {
			if(p->type == FairyProj && cabs(p->pos-e->pos) < 50) {
				p->args[1] = 0.01*global.diff*cexp(I*f);
				p->clr->r = frand();
			}
		}
	}
			
	
	return 1;
}

void elly_newton(Boss *b, int t) {
	TIMER(&t);
	
	AT(0) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_newton;
	}
	
	FROM_TO(0, 2000, 20) {
		float a = 2.7*_i;
		int x, y;
		int w = 3;
		
		for(x = -w; x <= w; x++) {
			for(y = -w; y <= w; y++) {
				create_projectile2c("plainball", b->pos+(x+I*y)*(18)*cexp(I*a), rgb(0, 0.1, 0.4), accelerated, 2*cexp(I*a), 0);
			}
		}
	}	
}

void elly_spellbg(Boss *b, int t) {
	fill_screen(0,0,0.7,"stage6/spellbg_classic");
	glBlendFunc(GL_ZERO,GL_SRC_COLOR);
	glColor4f(1,1,1,0);
	fill_screen(0,-t*0.005,0.7,"stage6/spellbg_chalk");
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);
}

Boss *create_elly() {
	Boss *b = create_boss("Elly", "elly", -200I);
	
	boss_add_attack(b, AT_Move, "Catch the Scythe", 6, 0, elly_intro, NULL);
// 	boss_add_attack(b, AT_Normal, "Frequency", 20, 23000, elly_frequency, NULL);
	boss_add_attack(b, AT_Spellcard, "Newton Sign ~ 2.5 Laws of Movement", 30, 30000, elly_newton, elly_spellbg);
	
	start_attack(b, b->attacks);
	
	return b;
}
	
void stage6_events() {
	TIMER(&global.timer);
	
	AT(0)
		global.timer = 3800;
	
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
	
	AT(2300)
		create_enemy3c(200I-200, ENEMY_IMMUNE, Scythe, scythe_mid, 0, 0.2I, 1);
		
	AT(3800)
		global.boss = create_elly();
}