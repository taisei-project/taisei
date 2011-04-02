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

void simpleFairy(Fairy *f) {
	if(!((global.frames - f->birthtime) % 50))
		create_projectile("rice", f->pos, ((Color){0,0,1}), linear,3+ 2*I);
	
	f->moving = 1;
	f->dir = creal(f->args[0]) < 0;
	
	int t = global.frames - f->birthtime;
	f->pos = f->pos0 + f->args[0]*t + I*sin((global.frames - f->birthtime)/10.0f)*20; // TODO: do this way cooler.
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
}

void stage0_events() {
	if(!(global .frames % 100)) {
// 		int i;
// 		for(i = 0; i < VIEWPORT_W/15; i++)
// 			create_projectile(&_projs.ball, i*VIEWPORT_W/15, 0, 90 + i*10, ((Color) {0,0,1}), simple, 2);
		create_fairy(0 + I*100, 3, simpleFairy, 2);
// 		create_fairy(VIEWPORT_W-1, 10, -1, 180, 3, simpleFairy);
// 		create_fairy(VIEWPORT_W-1, 200, -1, 180, 3, simpleFairy);
		create_projectile("ball", VIEWPORT_W/2, ((Color) {0,0,1}), linear, 2*I);
		play_sound("shot_special1");
	}
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
		stage_logic();
		stage_input();
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		stage0_draw();
		stage_draw();
				
		
		SDL_GL_SwapBuffers();
		frame_rate();
	}
	
	glDisable(GL_FOG);
	stage_end();
}