/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage0.h"

#include "stage.h"
#include "global.h"
#include "stageutils.h"

static Stage3D bgcontext;

Dialog *stage0_dialog() {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "masterspark");
		
	dadd_msg(d, Right, "Hey! Who is there?");
	dadd_msg(d, Left, "Just someone?");
	dadd_msg(d, Right, "How dare you pass the lake of the fairies!\nIt's a dangerous area for weak humans.");
	dadd_msg(d, Left, "I'm just walking by. Any problem with that?");
	dadd_msg(d, Right, "Of course! It's not right!");
	dadd_msg(d, Right, "I'll just freeze you!");
	
	return d;
}

void stage0_bg_draw(Vector pos) {
	glPushMatrix();
	glTranslatef(pos[0],pos[1],pos[0]);
	glRotatef(180,1,0,0);
	glScalef(1200,3000,1);
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage0/water")->gltex);
	
	draw_quad();
	
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();	
}

Vector **stage0_bg_pos(Vector p, float maxrange) {
	Vector q = {0,0,0};
	Vector r = {0,3000,0};
	return linear3dpos(p, maxrange, q, r);
}

void stage0_smoke_draw(Vector pos) {
	float d = abs(pos[1]-bgcontext.cx[1]);
	
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glTranslatef(pos[0]+200*sin(pos[1]), pos[1], pos[2]+200*sin(pos[1]/25.0));
	glRotatef(90,-1,0,0);
	glScalef(3.5,2,1);		
	glRotatef(global.frames,0,0,1);
	
	glColor4f(.2,.2,.2,((d-500)*(d-500))/1.5e6);
	draw_texture(0,0,"stage0/fog");
	glColor4f(1,1,1,1);
	
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

Vector **stage0_smoke_pos(Vector p, float maxrange) {
	Vector q = {0,0,-300};
	Vector r = {0,300,0};
	return linear3dpos(p, maxrange/2.0, q, r);
}

void stage0_fog(int fbonum) {
	Shader *shader = get_shader("zbuf_fog");
	
	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "tex"), 0);
	glUniform1i(uniloc(shader, "depth"), 1);
	glUniform4f(uniloc(shader, "fog_color"),0.1, 0.1, 0.1, 1.0);
	glUniform1f(uniloc(shader, "start"),0.0);
	glUniform1f(uniloc(shader, "end"),0.4);
	glUniform1f(uniloc(shader, "exponent"),3.0);
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, resources.fbg[fbonum].depth);
	glActiveTexture(GL_TEXTURE0);
	
	draw_fbo_viewport(&resources.fbg[fbonum]);
	glUseProgram(0);
}

void stage0_draw() {
	set_perspective(&bgcontext, 500, 5000);
	
	draw_stage3d(&bgcontext, 7000);	
}


void cirno_intro(Boss *c, int time) {

	GO_TO(c, VIEWPORT_W/2.0 + 100I, 0.035);
}

void cirno_icy(Boss *c, int time) {
	int t = time % 280;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	FROM_TO(0, 200, 5-global.diff) {
		create_projectile2c("crystal", VIEWPORT_W/2.0 + 10*_i*(0.5-frand()) + cimag(c->pos)*I, rgb(0.2,0.5,0.4+0.5*frand()), accelerated, 1.7*cexp(I*_i/10.0)*(1-2*(_i&1)), 0.0001I*_i + (0.0025 - 0.005*frand()));
		create_projectile2c("crystal", VIEWPORT_W/2.0 + 10*_i*(0.5-frand()) + cimag(c->pos)*I, rgb(0.2,0.5,0.4+0.5*frand()), accelerated, 1.7*cexp(I*_i/10.0)*(1-2*(_i&1)), 0.0001I*_i + (0.0025 - 0.005*frand()));
	}
}
	

int cirno_pfreeze_frogs(Projectile *p, int t) {
	if(t == EVENT_DEATH)
		free_ref(p->args[1]);
	if(t < 0)
		return 1;
	
	Boss *parent = REF(p->args[1]);
	
	if(parent == NULL)
		return ACTION_DESTROY;
	
	int boss_t = (global.frames - parent->current->starttime) % 320;
	
	if(boss_t < 110)
		linear(p, t);
	else if(boss_t == 110) {
		free(p->clr);
		p->clr = rgb(0.7,0.7,0.7);
	}
	
	if(t == 240) {
		p->pos0 = p->pos;
		p->args[0] = (1.8+0.2*global.diff)*cexp(I*2*M_PI*frand());
	}
	
	if(t > 240)
		linear(p, t-240);
	
	return 1;
}

void cirno_perfect_freeze(Boss *c, int time) {
	int t = time % 320;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	FROM_TO(-40, 0, 1)
		GO_TO(c, VIEWPORT_W/2.0 + 100I, 0.04);
		
	FROM_TO(20,80,1) {
		float r = frand();
		float g = frand();
		float b = frand();
		
		create_projectile2c("ball", c->pos, rgb(r, g, b), cirno_pfreeze_frogs, 4*cexp(I*rand()), add_ref(global.boss));
		if(global.diff > D_Normal)
			create_projectile2c("ball", c->pos, rgb(r, g, b), cirno_pfreeze_frogs, 4*cexp(I*rand()), add_ref(global.boss));
	}
	
	GO_AT(c, 160, 190, 2 + 1I);

	FROM_TO(160, 220, 6) {
		create_projectile2c("rice", c->pos + 60, rgb(0.3, 0.4, 0.9), asymptotic, 3*cexp(I*(carg(global.plr.pos - c->pos) + frand() - 0.5)), 2.5);
		create_projectile2c("rice", c->pos - 60, rgb(0.3, 0.4, 0.9), asymptotic, 3*cexp(I*(carg(global.plr.pos - c->pos) + frand() - 0.5)), 2.5);
	}
	
	GO_AT(c, 190, 220, -2)
	
	FROM_TO(280, 320, 1)
		GO_TO(c, VIEWPORT_W/2.0 + 100I, 0.04);
}

void cirno_pfreeze_bg(Boss *c, int time) {
	glColor4f(0.5,0.5,0.5,1);	
	fill_screen(time/700.0, time/700.0, 1, "stage0/cirnobg");	
	glColor4f(0.7,0.7,0.7,0.5);	
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	fill_screen(-time/700.0 + 0.5, time/700.0+0.5, 0.4, "stage0/cirnobg");	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	fill_screen(0, -time/100.0, 0, "stage0/snowlayer");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);
}
	
Boss *create_cirno_mid() {
	Boss* cirno = create_boss("Cirno", "cirno", VIEWPORT_W + 150 + 30I);
	boss_add_attack(cirno, AT_Move, "Introduction", 2, 0, cirno_intro, NULL);
	boss_add_attack(cirno, AT_Normal, "Icy Storm", 20, 20000, cirno_icy, NULL);
	boss_add_attack(cirno, AT_Spellcard, "Freeze Sign ~ Perfect Freeze", 32, 20000, cirno_perfect_freeze, cirno_pfreeze_bg);
	
	start_attack(cirno, cirno->attacks);
	return cirno;
}

void cirno_intro_boss(Boss *c, int time) {
	if(time < 0)
		return;
	TIMER(&time);
	GO_TO(c, VIEWPORT_W/2.0 + 100I, 0.035);
		
	AT(100)
		global.dialog = stage0_dialog();
}

void cirno_iceplosion0(Boss *c, int time) {
	int t = time % 350;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	FROM_TO(20,30,2) {
		int i;
		for(i = 0; i < 8+global.diff; i++) {
			create_projectile2c("plainball", c->pos, rgb(0,0,0.5), asymptotic, (3+_i/3.0)*cexp(I*(2*M_PI/8.0*i - 0.3)), _i*0.7);
		}
	}
	
	FROM_TO(40,100,1) {
		create_projectile2c("crystal", c->pos, rgb(0.3,0.3,0.8), accelerated, 2*cexp(2I*M_PI*frand()) + 2I, 0.002*cexp(I*(M_PI/10.0*(_i%20))));
	}
	
	FROM_TO(150, 300, 30) {
		float dif = M_PI*2*frand();
		int i;
		for(i = 0; i < 20; i++) {			
			create_projectile2c("plainball", c->pos, rgb(0.04*_i,0.04*_i,0.4+0.04*_i), asymptotic, (3+_i/3.0)*cexp(I*(2*M_PI/8.0*i + dif)), 2.5);
		}
	}
}

void cirno_crystal_rain(Boss *c, int time) {
	int t = time % 500;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	if(frand() > 0.9-0.07*global.diff)
		create_projectile2c("crystal", VIEWPORT_W*frand(), rgb(0.2,0.2,0.4), accelerated, 1I, 0.01I + (0.01+0.003*global.diff)*(1-2*frand()));
	
	FROM_TO(100, 400, 120-20*global.diff)
		create_projectile2c("bigball", c->pos, rgb(0.2,0.2,0.9), asymptotic, 2*cexp(I*carg(global.plr.pos-c->pos)), 2.3);
	
	GO_AT(c, 20, 70, 1+0.6I);
	GO_AT(c, 120, 170, -1+0.2I);
	GO_AT(c, 230, 300, -1+0.6I);
	
	FROM_TO(400, 500, 1)
		GO_TO(c, VIEWPORT_W/2.0 + 100I, 0.01);
}

void cirno_iceplosion1(Boss *c, int time) {
	int t = time % 360;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	if(time < 0)
		GO_TO(c, VIEWPORT_W/2.0 + 100I, 0.02);
	
	FROM_TO(20,30,2) {
		int i;
		for(i = 0; i < 15+global.diff; i++) {
			create_projectile2c("plainball", c->pos, rgb(0,0,0.5), asymptotic, (3+_i/3.0)*cexp(I*((2)*M_PI/8.0*i + (0.1+0.03*global.diff)*(1 - 2*frand()))), _i*0.7);
		}		
	}
	
	FROM_TO(40,100,2) {
		create_projectile2c("crystal", c->pos + 100, rgb(0.3,0.3,0.8), accelerated, 1.5*cexp(2I*M_PI*frand()) - 0.4 + 2I, 0.002*cexp(I*(M_PI/10.0*(_i%20))));
		create_projectile2c("crystal", c->pos - 100, rgb(0.3,0.3,0.8), accelerated, 1.5*cexp(2I*M_PI*frand()) + 0.4 + 2I, 0.002*cexp(I*(M_PI/10.0*(_i%20))));
	}
	
	FROM_TO(150, 300, 30) {
		float dif = M_PI*2*frand();
		int i;
		for(i = 0; i < 20; i++) {			
			create_projectile2c("plainball", c->pos, rgb(0.04*_i,0.04*_i,0.4+0.04*_i), asymptotic, (3+_i/3.0)*cexp(I*(2*M_PI/8.0*i + dif)), 2.5);
		}
	}
}

int cirno_icicles(Projectile *p, int t) {
	int turn = 60;
	
	if(t < 0)
		return 1;
	
	if(t < turn) {
		p->pos += p->args[0]*pow(0.9,t);
	} else if(t == turn) {
		p->args[0] = 2.5*cexp(I*(carg(p->args[0])-M_PI/2.0+M_PI*(creal(p->args[0]) > 0)));
	} else if(t > turn) {
		p->pos += p->args[0];
	}
	
	p->angle = carg(p->args[0]);
	
	return 1;
}

void cirno_icicle_fall(Boss *c, int time) {
	int t = time % 400;
	TIMER(&t);
	
	if(time < 0)
		return;
	
	GO_TO(c, VIEWPORT_W/2.0+120I, 0.01);
		
	FROM_TO(20,200,30) {
		int i;
		for(i = 2; i < 5; i++) {
			create_projectile1c("crystal", c->pos, rgb(0.3,0.3,0.9), cirno_icicles, 6*i*cexp(I*(-0.1+0.1*_i)));
			create_projectile1c("crystal", c->pos, rgb(0.3,0.3,0.9), cirno_icicles, 6*i*cexp(I*(M_PI+0.1-0.1*_i)));
		}
	}
}

Boss *create_cirno() {
	Boss* cirno = create_boss("Cirno", "cirno", -150 + 100I);
	boss_add_attack(cirno, AT_Move, "Introduction", 2, 0, cirno_intro_boss, NULL);
	boss_add_attack(cirno, AT_Normal, "Iceplosion 0", 20, 20000, cirno_iceplosion0, NULL);
	boss_add_attack(cirno, AT_Spellcard, "Freeze Sign ~ Crystal Rain", 28, 28000, cirno_crystal_rain, cirno_pfreeze_bg);
	boss_add_attack(cirno, AT_Normal, "Iceplosion 1", 20, 20000, cirno_iceplosion1, NULL);
	boss_add_attack(cirno, AT_Spellcard, "Doom Sign ~ Icicle Fall", 35, 40000, cirno_icicle_fall, cirno_pfreeze_bg);
	
	start_attack(cirno, cirno->attacks);
	return cirno;
}

int stage0_burst(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3,0,0,0);
		return 1;
	}	
	
	FROM_TO(0, 60, 1)
		e->pos += 2I;
	
	AT(60) {
		int i = 0;
		int n = global.diff+1;
		
		for(i = -n/2; i <= n/2; i++) {
			create_projectile2c("crystal", e->pos, rgb(0.2, 0.3, 0.5), asymptotic, (2+0.1*global.diff)*cexp(I*(carg(global.plr.pos - e->pos) + 0.2*i)), 5);
		}
		
		e->moving = 1;
		e->dir = creal(e->args[0]) < 0;
		
		e->pos0 = e->pos;
	}
	
	
	FROM_TO(70, 900, 1)
		e->pos = e->pos0 + (0.04*e->args[0])*_i*_i;		
		
	return 1;
}

int stage0_circletoss(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,2,0,0);
		return 1;
	}
	
	e->pos += e->args[0];
				
	FROM_TO(60,100,2) {
		e->args[0] = 0.5*e->args[0];
		create_projectile2c("rice", e->pos, rgb(0.6, 0.2, 0.7), asymptotic, 2*cexp(I*M_PI/10*_i), _i/2.0);
	}
	
	
	if(global.diff > D_Easy) {
		FROM_TO_INT(90,500,150,15,1)
			create_projectile2c("thickrice", e->pos, rgb(0.2, 0.4, 0.8), asymptotic, (1+frand()*2)*cexp(I*carg(global.plr.pos - e->pos)), 3);
	}
	
	FROM_TO(global.diff > D_Easy ? 500 : 240, 900, 1)
		e->args[0] += 0.03*e->args[1] - 0.04I;
		
	return 1;
}

int stage0_sinepass(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, frand()>0.5, frand()>0.2,0,0);
		return 1;
	}
	
	e->args[1] -= cimag(e->pos-e->pos0)*0.03I;
	e->pos += e->args[1]*0.4 + e->args[0];
	
	if(frand() > 0.993-0.001*global.diff)
		create_projectile1c("ball", e->pos, rgb(0.8,0.8,0.4), linear, (1+frand())*cexp(I*carg(global.plr.pos - e->pos)));
	
	return 1;
}

int stage0_drop(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,1,0,0);
		return 1;
	}
	if(t < 0)
		return 1;
	
	e->pos = e->pos0 + e->args[0]*t + e->args[1]*t*t;
	
	FROM_TO(10,1000,1)
		if(frand() > 0.995-0.002*global.diff)
			create_projectile1c("ball", e->pos, rgb(0.8,0.8,0.4), linear, (1+frand())*cexp(I*carg(global.plr.pos - e->pos)));
		
	return 1;
}

int stage0_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3,4,0,0);
		return 1;
	}
	
	FROM_TO(0, 150, 1)
		e->pos += (e->args[0] - e->pos)*0.02;
	
	FROM_TO_INT(150, 550, 40, 40, 2)
		create_projectile2c("rice", e->pos, rgb(0.6, 0.2, 0.7), asymptotic, (2+0.1*global.diff)*cexp(I*M_PI/10*_ni), _ni/2.0);
	
	FROM_TO(560,1000,1)
		e->pos += e->args[1];
		
	return 1;
}

int stage0_multiburst(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3,4,0,0);
		return 1;
	}
	
	FROM_TO(0, 50, 1)
		e->pos += 2I;
	
	FROM_TO_INT(60, 300, 70, 40, 12-global.diff) {
		int i;
		for(i = -1; i <= 1; i++)
			create_projectile1c("crystal", e->pos, rgb(0.2, 0.3, 0.5), linear, 2.5*cexp(I*(carg(global.plr.pos - e->pos) + i/5.0)));
	}
	
	FROM_TO(320, 700, 1) {
		e->args[1] += 0.03;
		e->pos += e->args[0]*e->args[1] + 1.4I;
	}
	
	return 1;
}

int stage0_instantcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,4,0,0);
		return 1;
	}
	
	FROM_TO(0, 110, 1) {
		e->pos += e->args[0];
	}
	
	int i;
	
	AT(150) {
		for(i = 0; i < 20+global.diff; i++)
			create_projectile2c("rice", e->pos, rgb(0.6, 0.2, 0.7), asymptotic, 1.5*cexp(I*2*M_PI/(20.0+global.diff)*i), 2.0);
	}
	
	AT(170) {
		for(i = 0; i < 20+global.diff; i++)
			create_projectile2c("rice", e->pos, rgb(0.6, 0.2, 0.7), asymptotic, 3*cexp(I*2*M_PI/(20.0+global.diff)*i), 3.0);
	}
	
	if(t > 200)
		e->pos += e->args[1];
	
	return 1;
}

int stage0_tritoss(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 5,5,0,0);
		return 1;
	}
	
	FROM_TO(0, 100, 1) {
		e->pos += e->args[0];
	}
	
	FROM_TO(120, 800,8-global.diff) {
		float a = M_PI/30.0*((_i/7)%30)+0.1*(1-2*frand());
		
		create_projectile2c("thickrice", e->pos, rgb(0.2, 0.4, 0.8), asymptotic, 2*cexp(I*a), 3);
		create_projectile2c("thickrice", e->pos, rgb(0.2, 0.4, 0.8), asymptotic, 2*cexp(I*(a+2*M_PI/3.0)), 3);
		create_projectile2c("thickrice", e->pos, rgb(0.2, 0.4, 0.8), asymptotic, 2*cexp(I*(a+4*M_PI/3.0)), 3);
	}
	
	FROM_TO(480, 800, 300) {
		int i, n = 20 + global.diff*2;
		for(i = 0; i < n; i++) {
			create_projectile2c("rice", e->pos, rgb(0.6, 0.2, 0.7), asymptotic, 1.5*cexp(I*2*M_PI/n*i), 2.0);
			create_projectile2c("rice", e->pos, rgb(0.6, 0.2, 0.7), asymptotic, 3*cexp(I*2*M_PI/n*i), 3.0);
		}
	}
	
	if(t > 820)
		e->pos += e->args[1];
	
	return 1;
}

void stage0_events() {	
	TIMER(&global.timer);
	
	// opening. projectile bursts
	FROM_TO(100, 160, 25) {
		create_enemy1c(VIEWPORT_W/2 + 70, 700, Fairy, stage0_burst, 1 + 0.6I);
		create_enemy1c(VIEWPORT_W/2 - 70, 700, Fairy, stage0_burst, -1 + 0.6I);
	}
	
	// more bursts. fairies move / \ like
	FROM_TO(240, 300, 30) {
		create_enemy1c(70 + _i*40, 700, Fairy, stage0_burst, -1 + 0.6I);
		create_enemy1c(VIEWPORT_W - (70 + _i*40), 700, Fairy, stage0_burst, 1 + 0.6I);
	}
		
	// big fairies, circle + projectile toss
	FROM_TO(400, 460, 50)
		create_enemy2c(VIEWPORT_W*_i + VIEWPORT_H/3*I, 1500, BigFairy, stage0_circletoss, 2-4*_i-0.3I, 1-2*_i);	
	
	
	// swirl, sine pass
	FROM_TO(380, 1000, 20)
		create_enemy2c(VIEWPORT_W*(_i&1) + frand()*100I + 70I, 100, Swirl, stage0_sinepass, 3.5*(1-2*(_i&1)), frand()*7I);
	
	// swirl, drops
	FROM_TO(1100, 1600, 20)
		create_enemy2c(VIEWPORT_W/3, 100, Swirl, stage0_drop, 4I, 0.06);
		
	FROM_TO(1500, 2000, 20)
		create_enemy2c(VIEWPORT_W+200I, 100, Swirl, stage0_drop, -2, -0.04-0.03I);
	
	// bursts
	FROM_TO(1250, 1800, 60)
		create_enemy1c(VIEWPORT_W/2 + frand()*500-250, 500, Fairy, stage0_burst, frand()*2-1);
			
	// circle - multi burst combo
	FROM_TO(1700, 2300, 300)
		create_enemy2c(VIEWPORT_W/2, 1400, BigFairy, stage0_circle, VIEWPORT_W/4 + VIEWPORT_W/2*frand()+200I, 3-6*(frand()>0.5)+frand()*2I);
	
	FROM_TO(2000, 2500, 200) {
		int i, t = global.diff + 1;
		for(i = 0; i < t; i++)
			create_enemy1c(VIEWPORT_W/2 - 40*t + 80*i, 1000, Fairy, stage0_multiburst, i - 2.5);
	}
	
	AT(2700)
		global.boss = create_cirno_mid();
	
	// some chaotic swirls + instant circle combo
	FROM_TO(2760, 3800, 20)
		create_enemy2c(VIEWPORT_W/2 - 200*(1-2*frand()), 100, Swirl, stage0_drop, 1I, 0.001I + 0.02+0.06*(1-2*frand()));
	
	FROM_TO(2900, 3750, 190)
		create_enemy2c(VIEWPORT_W*frand(), 1200, Fairy, stage0_instantcircle, 2I, 3.0 - 6*frand() - 1I);
	
		
	// multiburst + normal circletoss, later tri-toss
	FROM_TO(3900, 4800, 200)
		create_enemy1c(VIEWPORT_W*frand(), 1000, Fairy, stage0_multiburst, 2.5*frand());
		
	FROM_TO(4000, 4100, 20)
		create_enemy2c(VIEWPORT_W*_i + VIEWPORT_H/3*I, 1700, Fairy, stage0_circletoss, 2-4*_i-0.3I, 1-2*_i);	
		
	AT(4200)
		create_enemy2c(VIEWPORT_W/2.0, 1800, BigFairy, stage0_tritoss, 2I, -2.6I);
		
	AT(5000)
		global.boss = create_cirno();
	
}

void stage0_start() {
	init_stage3d(&bgcontext);
	add_model(&bgcontext, stage0_bg_draw, stage0_bg_pos);
	add_model(&bgcontext, stage0_smoke_draw, stage0_smoke_pos);

	
	bgcontext.crot[0] = 60;
	bgcontext.cx[2] = 700;
	bgcontext.cv[1] = 7;
}

void stage0_end() {
	free_stage3d(&bgcontext);
}

void stage0_loop() {
	ShaderRule list[] = { stage0_fog, NULL };
	stage_loop(stage_get(1), stage0_start, stage0_end, stage0_draw, stage0_events, list, 5200);
}
