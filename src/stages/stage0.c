/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage0.h"

#include <SDL/SDL_opengl.h>
#include "../stage.h"
#include "../global.h"

void simpleEnemy(Enemy *e, int t) {
	if(t == EVENT_DEATH && global.plr.power < 6) {
		create_item(e->pos, 6*cexp(I*rand()), Power);
		return;
	} else if(t < 0) {
		return;
	}
	
	if(!(t % 50))
		create_projectile("ball", e->pos, rgb(0,0,1), linear,3 + 2I);
	
	e->moving = 1;
	e->dir = creal(e->args[0]) < 0;
	
	e->pos = e->pos0 + e->args[0]*t + I*sin(t/10.0f)*20; // TODO: do this way cooler.
}

Dialog *test_dialog() {
	Dialog *d = create_dialog("dialog/youmu", "dialog/youmu");
		
	dadd_msg(d, Left, "Hello");
	dadd_msg(d, Right, "Hello you");
	dadd_msg(d, Right, "Uhm ... who are you?\nNew line.\nAnother longer line.");
	
	return d;
}

void stage0_draw() {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, global.rtt.fbo);
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
	
	Texture *water = get_tex("wasser");
	
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
	
	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(-global.frames/1000.0,global.frames/200.0, 0);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage1/border")->gltex);
	
// 	glColor3f(1,0.75,0.75);
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex3f(VIEWPORT_W,0,0);
		glTexCoord2i(1,0);
		glVertex3f(VIEWPORT_W,0,1000);
		glTexCoord2i(1,1);
		glVertex3f(VIEWPORT_W,3000,1000);
		glTexCoord2i(0,1);
		glVertex3f(VIEWPORT_W,3000,0);		
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
		boss_add_waypoint(c->current, VIEWPORT_W/2+100 + 30I, 400);
		return;
	}
	
	if(!(time % 50))
		create_laser(LaserLine, c->pos, 10*cexp(I*carg(global.plr.pos - c->pos)+I*0.1), 30, 200, rgb(1,0,0), NULL, 0);
}

complex lolsin(Laser *l, float t) {
	complex pos;
	pos = l->pos0 + (1-t/200)*t*cexp(I*l->args[0])*3 + t*2I;
	
	return pos;
}

void cirno_test(Boss *c, int time) {
	if(time == EVENT_BIRTH) {
		boss_add_waypoint(c->current, 220 + 100I, 50);
		boss_add_waypoint(c->current, 12 + 100I, 60);
		boss_add_waypoint(c->current, 200 + 90I, 100);
		return;
	}
	int i;
	if(!(time % 50))
		for(i = 0; i < 6; i++)
			create_laser(LaserCurve, c->pos, c->pos, 50, 200, rgba(0.5,0.5,1,0.4), lolsin, i);
}

Boss *create_cirno() {
	Boss* cirno = create_boss("Cirno", "cirno", VIEWPORT_W/2 + 30I);
	boss_add_attack(cirno, Normal, "Introduction", 10, 100, cirno_intro);
	boss_add_attack(cirno, Spellcard, "Test Sign ~ Strongest Implementation", 10, 100, cirno_test);
	
	return cirno;
}

void stage0_events() {
	if(global.dialog)
		return;	
	if(global.timer == 200)
		global.dialog = test_dialog();
	if(global.timer == 300)
		global.boss = create_cirno();
	if(global.boss == NULL && !(global.timer % 100)) create_enemy(&global.enemies, Fairy, simpleEnemy, 0 + I*100, 3, NULL, 2);
	
// 	if(!(global.timer % 100))
// 		create_laser(LaserCurve, 300, 300, 60, 500, ((ColorA){0.6,0.6,1,0.4}), lolsin, 0);
}

void stage0_loop() {	
	stage_start();
	glEnable(GL_FOG);
	GLfloat clr[] = { 0.1, 0.1, 0.1, 0 };
	glFogfv(GL_FOG_COLOR, clr);
	glFogf(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 0);
	glFogf(GL_FOG_END, 1500);
	
	while(!global.game_over) {
		stage0_events();
		stage_input();
		stage_logic();		
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		stage0_draw();
		stage_draw();				
		
		SDL_GL_SwapBuffers();
		frame_rate();
	}
	
	glDisable(GL_FOG);
	stage_end();
}