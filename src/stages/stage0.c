/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage0.h"

#include "../stage.h"
#include "../global.h"

Dialog *test_dialog() {
	Dialog *d = create_dialog("dialog/marisa", "dialog/youmu");
		
	dadd_msg(d, Left, "Hello");
	dadd_msg(d, Right, "Hello you");
	dadd_msg(d, Right, "Uhm ... who are you?\nNew line.\nAnother longer line.");
	dadd_msg(d, Left, "idk");
	
	return d;
}

void stage0_draw() {
	glPushMatrix();
	
	glTranslatef(-(VIEWPORT_X+VIEWPORT_W/2.0), -(VIEWPORT_Y+VIEWPORT_H/2.0),0);
	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glTranslatef(-(VIEWPORT_X+VIEWPORT_W/2.0)/SCREEN_W, -(VIEWPORT_Y+VIEWPORT_H/2.0)/SCREEN_H,0);
		gluPerspective(45, 5.0/6.0, 500, 5000);
		glTranslatef(0,0,-500);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	
	glBegin(GL_QUADS);
		glVertex3f(-1000,0,-3000);
		glVertex3f(1500,-0,-3000);
		glVertex3f(1500,2500,-3000);
		glVertex3f(-1000,2500,-3000);		
	glEnd();
	
	glPushMatrix();
	glRotatef(60, -1, 0, 0);
	
	Texture *water = get_tex("stage0/water");
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, water->gltex);
	
	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(0,global.frames/(float)water->h, 0);
		glScalef(2,2,2);
	glMatrixMode(GL_MODELVIEW);
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex3f(-500,0,0);
		glTexCoord2i(1,0);
		glVertex3f(1000,0,0);
		glTexCoord2i(1,1);
		glVertex3f(1000,3000,0);
		glTexCoord2i(0,1);
		glVertex3f(-500,3000,0);
	glEnd();
	
	glPopMatrix();
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);	
	glPopMatrix();
	
	glDisable(GL_TEXTURE_2D);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glViewport(0,0,SCREEN_W,SCREEN_H);
}

void cirno_intro(Boss *c, int time) {
	if(time == EVENT_BIRTH) {
		boss_add_waypoint(c->current, VIEWPORT_W/2-100 + 30I, 100);
		boss_add_waypoint(c->current, VIEWPORT_W/2+100 + 30I, 200);
		return;
	}
	
}

void cirno_perfect_freeze(Boss *c, int time) {
	if(time == EVENT_BIRTH) {
		boss_add_waypoint(c->current, VIEWPORT_W/2 + 100I, 110);
		boss_add_waypoint(c->current, VIEWPORT_W/2 + 100I, 160);
		boss_add_waypoint(c->current, VIEWPORT_W/2+90 + 130I, 190);
		boss_add_waypoint(c->current, VIEWPORT_W/2-90 + 130I, 320);
		return;
	}
	
	if(time > 10 && time < 80) {
		float r = rand()/(float)RAND_MAX;
		float g = rand()/(float)RAND_MAX;
		float b = rand()/(float)RAND_MAX;
		
// 		create_projectile2c("ball", c->pos, rgb(r, g, b), cirno_pfreeze_frogs, 4*cexp(I*rand()), add_ref(&global.boss))->parent=&global.boss;
	}
	
	if(time > 160 && time < 220 && !(time % 7)) {
		create_projectile1c("rice", c->pos + 60, rgb(0.3, 0.4, 0.9), linear, 5*cexp(I*carg(global.plr.pos - c->pos)));
		create_projectile1c("rice", c->pos - 60, rgb(0.3, 0.4, 0.9), linear, 5*cexp(I*carg(global.plr.pos - c->pos)));
	}
		
}

void cirno_pfreeze_bg(Boss *c, int time) {
	glColor4f(0.5,0.5,0.5,1);	
	fill_screen(time/700.0, time/700.0, 1, "stage0/cirnobg");	
	glColor4f(0.7,0.7,0.7,0.5);	
	fill_screen(-time/700.0 + 0.5, time/700.0+0.5, 0.4, "stage0/cirnobg");
	fill_screen(0, -time/100.0, 0, "stage0/snowlayer");
	
	glColor4f(1,1,1,1);
}
	

Boss *create_cirno() {
	Boss* cirno = create_boss("Cirno", "cirno", VIEWPORT_W/2 + 30I);
	boss_add_attack(cirno, AT_Normal, "Introduction", 10, 100, cirno_intro, NULL);
	boss_add_attack(cirno, AT_Spellcard, "Freeze Sign ~ Perfect Freeze", 20, 300, cirno_perfect_freeze, cirno_pfreeze_bg);
	
	start_attack(cirno, cirno->attacks);
	return cirno;
}


void stage0_enemy0(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3,0,0,0);
		return;
	}	
	
	FROM_TO(0, 60, 1)
		e->pos += 2I;
		
	AT(60) {
		int i = 0;
		for(i = -1; i <= 1; i++)
			create_projectile2c("crystal", e->pos, rgb(0.2, 0.3, 0.5), asymptotic, 2*cexp(I*(carg(global.plr.pos - e->pos) + 0.2*i)), 5);
		
		e->moving = 1;
		e->dir = creal(e->args[0]) < 0;
		
		e->pos0 = e->pos;
	}
	
	FROM_TO(70, 900, 1)
		e->pos = e->pos0 + (0.04*e->args[0] + 0.06I)*_i*_i;		
}

void stage0_enemy1(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,2,0,0);
		return;
	}
	
	e->pos += e->args[0];
			
	
	FROM_TO(60,100,2) {
		e->args[0] = 0.5*e->args[0];
		create_projectile2c("rice", e->pos, rgb(0.6, 0.2, 0.7), asymptotic, 2*cexp(I*M_PI/10*_i), _i/2.0);
	}
	
	
	FROM_TO(90, 500, 20);
	if(!(_i % 9))
		create_projectile2c("thickrice", e->pos, rgb(0.2, 0.4, 0.8), asymptotic, (1+frand()*3)*cexp(I*carg(global.plr.pos - e->pos)), 3);
		
	FROM_TO(500, 900, 1)
		e->args[0] += 0.03*e->args[1] - 0.04I;
}

void stage0_enemy2(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, frand()>0.5, frand()>0.2,0,0);
		return;
	}
	
	e->args[1] -= cimag(e->pos-e->pos0)*0.03I;
	e->pos += e->args[1]*0.4 + e->args[0];
	
}

void stage0_enemy3(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 2,1,0,0);
		return;
	}
	
	e->pos = e->pos0 + e->args[0]*t + e->args[1]*t*t;
	
	FROM_TO(10,1000,1)
		if(frand() > 0.98)
			create_projectile1c("ball", e->pos, rgb(0.8,0.8,0.4), linear, (1+frand())*cexp(I*carg(global.plr.pos - e->pos)));
}

void stage0_enemy4(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3,4,0,0);
		return;
	}
	
	FROM_TO(0, 150, 1)
		e->pos += (e->args[0] - e->pos)*0.02;
	
	FROM_TO_INT(150, 550, 40, 40, 2)
		create_projectile2c("rice", e->pos, rgb(0.6, 0.2, 0.7), asymptotic, 2*cexp(I*M_PI/10*_ni), _ni/2.0);
	
	FROM_TO(560,1000,1)
		e->pos += e->args[1];
}

void stage0_enemy5(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 3,4,0,0);
		return;
	}
	
	FROM_TO(0, 50, 1)
		e->pos += 2I;
		
	FROM_TO_INT(60, 300, 70, 40, 12) {
		int i;
		for(i = -1; i <= 1; i++)
			create_projectile1c("crystal", e->pos, rgb(0.2, 0.3, 0.5), linear, 2.5*cexp(I*(carg(global.plr.pos - e->pos) + i/5.0)));
	}
	
	FROM_TO(320, 700, 1) {
		e->args[1] += 0.03;
		e->pos += e->args[0]*e->args[1] + 1.4I;
	}
}
void stage0_events() {
	if(global.dialog)
		return;	
		
	TIMER(&global.timer);
	
	FROM_TO(100, 160, 25) {
		create_enemy1c(VIEWPORT_W/2 + 70, 8, Fairy, stage0_enemy0, 1);
		create_enemy1c(VIEWPORT_W/2 - 70, 8, Fairy, stage0_enemy0, -1);
	}
		
	FROM_TO(240, 300, 30) {
		create_enemy1c(70 + _i*40, 8, Fairy, stage0_enemy0, -1);
		create_enemy1c(VIEWPORT_W - (70 + _i*40), 8, Fairy, stage0_enemy0, 1);
	}
	
	FROM_TO(400, 460, 50)
		create_enemy2c(VIEWPORT_W*_i + VIEWPORT_H/3*I, 25, Fairy, stage0_enemy1, 2-4*_i-0.3I, 1-2*_i);	
	
	FROM_TO(380, 1000, 20)
		create_enemy2c(VIEWPORT_W*(_i&1) + frand()*100I + 70I, 2, Swirl, stage0_enemy2, 3.5*(1-2*(_i&1)), frand()*7I);
		
	FROM_TO(1100, 1600, 20)
		create_enemy2c(VIEWPORT_W/3, 1, Swirl, stage0_enemy3, 4I, 0.06);
		
	FROM_TO(1500, 2000, 20)
		create_enemy2c(VIEWPORT_W+200I, 2, Swirl, stage0_enemy3, -2, -0.04-0.03I);
	
	FROM_TO(1250, 1800, 60)
		create_enemy1c(VIEWPORT_W/2 + frand()*500-250 , 8, Fairy, stage0_enemy0, frand()*2-1);
		
	FROM_TO(1700, 2700, 300)
		create_enemy2c(VIEWPORT_W/2, 35, Fairy, stage0_enemy4, VIEWPORT_W/4 + VIEWPORT_W/2*frand()+200I, 3-6*(frand()>0.5)+frand()*2I);
	
	FROM_TO(2000,2800, 200) {
		int i;
		for(i = 0; i < 5; i++)
			create_enemy1c(VIEWPORT_W/4 + VIEWPORT_H/10*i, 8, Fairy, stage0_enemy5, i - 2.5);
	}
// 	if(!(global.timer % 100))
// 		create_laser(LaserCurve, 300, 300, 60, 500, ((ColorA){0.6,0.6,1,0.4}), lolsin, 0);
}

void stage0_start() {
	stage_start();
	glEnable(GL_FOG);
	GLfloat clr[] = { 0.1, 0.1, 0.1, 0 };
	glFogfv(GL_FOG_COLOR, clr);
	glFogf(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 0);
	glFogf(GL_FOG_END, 1500);
}

void stage0_end() {
	glDisable(GL_FOG);
}

void stage0_loop() {	
	stage_loop(stage0_start, stage0_end, stage0_draw, stage0_events);
}