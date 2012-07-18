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
	
	e->moving = 1;
	e->dir = creal(e->args[0]) < 0;
	e->pos += e->args[0];
	
	FROM_TO(100, 200, 22-global.diff*3) {
		if(global.diff > D_Easy) {
			create_projectile2c("ball", e->pos, rgb(1, 0.3, 0.5), asymptotic, 2*cexp(I*M_PI*2*frand()), 3);
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

int stage3_bigcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1,3,0,0);
		
		return 1;
	}
	
	FROM_TO(0, 70, 1)
		e->pos += e->args[0];
	
	FROM_TO(200, 300, 1)
		e->pos -= e->args[0];
	
		
	FROM_TO(80,80+20*global.diff,20) {
		int i;
		int n = 10+3*global.diff;
		for(i = 0; i < n; i++)
			create_projectile2c("bigball", e->pos, rgb(0,0.8-0.4*_i,0), asymptotic, 2*cexp(2I*M_PI/n*i+I*M_PI*_i), 3*sin(6*M_PI/n*i));
	}
	return 1;
}

int stage3_explosive(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		int i;
		spawn_items(e->pos, 0,1,0,0);
		
		int n = 5*global.diff;
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

void KurumiSlave(Enemy *e, int t) {
	if(!(t%2)) {
		complex offset = (frand()-0.5)*30 + (frand()-0.5)*20I;
		create_particle3c("lasercurve", offset, rgb(0.3,0.0,0.0), EnemyFlareShrink, enemy_flare, 50, (-50I-offset)/50.0, add_ref(e));
	}
		
}
	

void kurumi_intro(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2.0+200I, 0.01);
}

int kurumi_burstslave(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH)
		e->args[1] = e->args[0];
	AT(EVENT_DEATH) {
		free_ref(e->args[2]);
		return 1;
	}
	
	
	if(t == 600 || REF(e->args[2]) == NULL)
		return ACTION_DESTROY;
	
	e->pos += 2*e->args[1]*(sin(t/10.0)+1.5);
		
	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;
		create_projectile2c("wave", e->pos + 10I*e->args[0], rgb(r,0,0), accelerated, 2I*e->args[0], -0.01*e->args[1]);
		create_projectile2c("wave", e->pos - 10I*e->args[0], rgb(r,0,0), accelerated, -2I*e->args[0], -0.01*e->args[1]);
	}
	
	FROM_TO(40, 100,1) {
		e->args[1] -= e->args[0]*0.02;
		e->args[1] *= cexp(0.02I);
	}
	
	return 1;
}
	

void kurumi_slaveburst(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);
	
	AT(0) {
		int i;
		int n = 3+2*global.diff;
		for(i = 0; i < n; i++) {
			create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_burstslave, cexp(I*2*M_PI/n*i+0.2I*time/500), 0, add_ref(b));
		}
	}
}

int kurumi_spikeslave(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH)
		e->args[1] = e->args[0];
	AT(EVENT_DEATH) {
		free_ref(e->args[2]);
		return 1;
	}
	
	
	if(t == 300+50*global.diff || REF(e->args[2]) == NULL)
		return ACTION_DESTROY;
	
	e->pos += e->args[1];
	e->args[1] *= cexp(0.01I*e->args[0]);	
	
	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;
		create_projectile2c("wave", e->pos + 10I*e->args[0], rgb(r,0,0), linear, 1.5I*e->args[1], -0.01*e->args[0]);
		create_projectile2c("wave", e->pos - 10I*e->args[0], rgb(r,0,0), linear, -1.5I*e->args[1], -0.01*e->args[0]);
	}
	
	return 1;
}

void kurumi_redspike(Boss *b, int time) {
	int t = time % 500;
	TIMER(&t);
	
	FROM_TO(0, 500, 60) {
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_spikeslave, 1-2*(_i&1), 0, add_ref(b));
	}
	
	if(global.diff < D_Hard) {
		FROM_TO(0, 500, 150-50*global.diff) {
			int i;
			int n = global.diff*8;
			for(i = 0; i < n; i++)
				create_projectile2c("bigball", b->pos, rgb(1,0,0), asymptotic, 3*cexp(2I*M_PI/n*i+I*carg(global.plr.pos-b->pos)), 3);
		}
	} else {
		FROM_TO(80, 500, 2+2*(global.diff == D_Hard)) {
			complex offset = 100*frand()*cexp(2I*M_PI*frand());
			complex n = cexp(I*carg(global.plr.pos-b->pos-offset));
			create_projectile2c("rice", b->pos+offset, rgb(1,0,0), accelerated, -1*n, 0.05*n);
		}
	}
}

void kurumi_spell_bg(Boss *b, int time) {
	float f = 0.5+0.5*sin(time/80.0);
	
	glPushMatrix();
	glTranslatef(VIEWPORT_W/2, VIEWPORT_H/2,0);
	glScalef(0.6,0.6,1);
	glColor3f(f,1-f,1-f);
	draw_texture(0, 0, "stage3/kurumibg1");
	glColor3f(1,1,1);
	glPopMatrix();
		
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	fill_screen(time/300.0, time/300.0, 0.5, "stage3/kurumibg2");
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
}

void kurumi_outro(Boss *b, int time) {
	b->pos += -5-I;
	
	if(time == 0) {
		Enemy *e;
		for(e = global.enemies; e; e = e->next)
			e->hp = 0;
		
		Projectile *p;
		for(p = global.projs; p; p = p->next)
			p->type = DeadProj;
	}		
}

Boss *create_kurumi_mid() {
	Boss* b = create_boss("Kurumi", "kurumi", VIEWPORT_W/2-400I);
	boss_add_attack(b, AT_Move, "Introduction", 4, 0, kurumi_intro, NULL);
	boss_add_attack(b, AT_Spellcard, "Bloodless ~ Gate of Walachia", 25, 20000, kurumi_slaveburst, kurumi_spell_bg);
	boss_add_attack(b, AT_Spellcard, global.diff < D_Hard ? "Bloodless ~ Dry Fountain" : "Bloodless ~ Red Spike", 30, 30000, kurumi_redspike, kurumi_spell_bg);
	boss_add_attack(b, AT_Move, "Outro", 2, 1, kurumi_outro, NULL);
	start_attack(b, b->attacks);
	return b;
}

int splitcard(Projectile *p, int t) {
	if(t == creal(p->args[2])) {
		p->args[0] += p->args[3];
		p->clr->b = -p->clr->b;
	}
	
	return asymptotic(p, t);
}

int stage3_supercard(Enemy *e, int t) {
	int time = t % 150;
	
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,3,0,0);		
		return 1;
	}
	
	e->pos += e->args[0];
	
	FROM_TO(50, 70, 1) {
		e->args[0] *= 0.7;
	}
	
	if(t > 450) {
		e->pos -= I;
		return 1;
	}
	
	__timep = &time;
	
	FROM_TO(70, 70+20*global.diff, 1) {
		int i;
		complex n = cexp(I*carg(global.plr.pos - e->pos) + 0.3I*_i);
		for(i = -1; i <= 1 && t; i++)
			create_projectile4c("card", e->pos + 30*n, rgb(1-_i/20.0, 0, 0.4), splitcard, 1*n, 0.4I, 100-time+70, 1.4*I*i*n);		
	}
	 
	return 1;
}
	
		
void stage3_events() {
	TIMER(&global.timer);
	
	AT(0)
		global.timer = 3200;
		
	AT(70) {
		create_enemy1c(VIEWPORT_H/4*3*I, 3000, BigFairy, stage3_splasher, 3-4I);
		create_enemy1c(VIEWPORT_W + VIEWPORT_H/4*3*I, 3000, BigFairy, stage3_splasher, -3-4I);
	}
	
	FROM_TO(300, 450, 20) {
		create_enemy1c(VIEWPORT_W*frand(), 200, Fairy, stage3_fodder, 3I);
	}
	
	FROM_TO(500, 550, 10) {
		int d = _i&1;
		create_enemy1c(VIEWPORT_W*d, 1000, Fairy, stage3_partcircle, 2*cexp(I*M_PI/2.0*(0.2+0.6*frand()+d)));
	}
	
	FROM_TO(600, 1400, 100) {
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 3000, BigFairy, stage3_cardbuster, VIEWPORT_W/6.0 + VIEWPORT_W/3.0*2*(_i&1)+100I,
					VIEWPORT_W/4.0 + VIEWPORT_W/2.0*((_i+1)&1)+300I, VIEWPORT_W/2.0+VIEWPORT_H*I+200I);
	}
	
	AT(1800) {
		create_enemy1c(VIEWPORT_H/2.0*I, 2000, Swirl, stage3_backfire, 0.3);
		create_enemy1c(VIEWPORT_W+VIEWPORT_H/2.0*I, 2000, Swirl, stage3_backfire, -0.5);
	}
	
	FROM_TO(2000, 2600, 20)
		create_enemy1c(300I*frand(), 200, Fairy, stage3_fodder, 2);
	
	FROM_TO(2000, 2400, 200)
		create_enemy1c(VIEWPORT_W/2+200-400*frand(), 1000, BigFairy, stage3_bigcircle, 2I);
		
	FROM_TO(2600, 2800, 10)
		create_enemy1c(20I+VIEWPORT_H/3*I*frand()+VIEWPORT_W, 100, Swirl, stage3_explosive, -3);
	
	AT(3200)
		global.boss = create_kurumi_mid();
		
	FROM_TO(3201, 3601, 10)
		create_enemy1c(VIEWPORT_W*(_i&1)+VIEWPORT_H/2*I-300I*frand(), 200, Fairy, stage3_fodder, 2-4*(_i&1)+1I);
	
	FROM_TO(3500, 4000, 100)
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 1000, BigFairy, stage3_cardbuster, VIEWPORT_W/6.0*(_i&1)+100I,
					VIEWPORT_W/4.0+VIEWPORT_W/2.0*((_i+1)&1)+300I, VIEWPORT_W/2.0-200I);
		
	AT(3800)
		create_enemy1c(VIEWPORT_W/2, 7000, BigFairy, stage3_supercard, 4I);
		
	FROM_TO(4300, 4600, 95-10*global.diff)
		create_enemy1c(VIEWPORT_W*(_i&1)+100*I, 200, Swirl, stage3_backfire, frand()*(1-2*(_i&1)));
		
	FROM_TO(4800, 5200, 10)
		create_enemy1c(20I+I*VIEWPORT_H/3*frand()+VIEWPORT_W*(_i&1), 100, Swirl, stage3_explosive, (1-2*(_i&1))*3+I);
}