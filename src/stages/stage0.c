/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage0.h"

#include "../stage.h"
#include "../global.h"

void simpleEnemy(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		create_item(e->pos, 6*cexp(I*rand()), Power);
		return;
	} else if(t < 0) {
		return;
	}
	
	if(!(t % 50))
		create_projectile1c("ball", e->pos, rgb(0,0,1), linear, 3 + 2I);
	
	e->moving = 1;
	e->dir = creal(e->args[0]) < 0;
	
	e->pos = e->pos0 + e->args[0]*t + I*sin(t/10.0f)*20; // TODO: do this way cooler.
}

Dialog *test_dialog() {
	Dialog *d = create_dialog("dialog/marisa", "dialog/youmu");
		
	dadd_msg(d, Left, "Hello");
	dadd_msg(d, Right, "Hello you");
	dadd_msg(d, Right, "Uhm ... who are you?\nNew line.\nAnother longer line.");
	dadd_msg(d, Left, "idk");
	
	return d;
}

void stage0_draw() {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, global.fbg.fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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


void stage0_fairy0(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_DEATH) {
		create_item(e->pos, 6*cexp(I*rand()), Power);
		return;
	}	
	
	FROM_TO(0, 60, 1)
		e->pos += 2I;
		
	AT(60) {
		int i = 0;
		for(i = -1; i <= 1; i++)
			create_projectile1c("ball", e->pos, rgb(0.2, 0.3, 0.5), linear, 4*cexp(I*(carg(global.plr.pos - e->pos) + 0.2*i)));
		
		e->moving = 1;
		e->dir = creal(e->args[0]) < 0;
		
		e->pos0 = e->pos;
	}
	
	FROM_TO(70, 900, 1)
		e->pos = e->pos0 + (0.04*e->args[0] + 0.06I)*_i*_i;		
}

void stage0_statfairy1(Enemy *e, int time) {
	TIMER(&time);
	AT(EVENT_DEATH) {
		create_item(e->pos, 6*cexp(I*rand()), Power);
		create_item(e->pos, 6*cexp(I*rand()), Power);
		create_item(e->pos, 6*cexp(I*rand()), Point);
		return;
	}
	
	e->pos += e->args[0];
			
	
	FROM_TO(60,100,2) {
		e->args[0] = 0.5*e->args[0];
		create_projectile1c("rice", e->pos, rgb(0.2, 0.4, 0.8), linear, 3*cexp(I*M_PI/10*_i));
	}
	
	FROM_TO(90, 500, 60) {
		int i;
		for(i = 4; i < 8; i++)
			create_projectile1c("ball", e->pos, rgb(0.2,0.4,0.8), linear, i*cexp(I*carg(global.plr.pos- e->pos)));
	}
	
	FROM_TO(500, 900, 1)
		e->args[0] += 0.03*e->args[1] - 0.04I;
}

void Swirl(Enemy *e, int t) {
	glPushMatrix();
	glTranslatef(creal(e->pos), cimag(e->pos),0);
	glRotatef(t*15,0,0,1);
	draw_texture(0,0, "swirl");
	glPopMatrix();
}

void stage0_events() {
	if(global.dialog)
		return;	
	
	TIMER(&global.timer);
	
	FROM_TO(100, 160, 25) {
		create_enemy1c(VIEWPORT_W/2 + 70, 8, Fairy, stage0_fairy0, 1);
		create_enemy1c(VIEWPORT_W/2 - 70, 8, Fairy, stage0_fairy0, -1);
	}
		
	FROM_TO(240, 300, 30) {
		create_enemy1c(70 + _i*40, 8, Fairy, stage0_fairy0, -1);
		create_enemy1c(VIEWPORT_W - (70 + _i*40), 8, Fairy, stage0_fairy0, 1);
	}
	
	FROM_TO(400, 460, 50)
		create_enemy2c(VIEWPORT_W*_i + VIEWPORT_H/3*I, 15, Swirl, stage0_statfairy1, 2-4*_i-0.3I, 1-2*_i);	
	
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