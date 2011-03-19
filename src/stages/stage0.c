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
	if(!((global.frames - f->birthtime) % 50)) {
		create_projectile(&_projs.rice, f->pos, ((Color){0,0,1}), linear,3+ 2*I);
	}
	f->moving = 1;
	f->dir = creal(f->args[0]) < 0;
	
	int t = global.frames - f->birthtime;
	f->pos = f->pos0 + f->args[0]*t + I*sin((global.frames - f->birthtime)/10.0f)*20; // TODO: do this way cooler.
}

void stage0_draw() {
	glPushMatrix();
	glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);
		
	glBegin(GL_QUADS);
		glVertex3f(0,0,1000);
		glVertex3f(0,VIEWPORT_H,1000);
		glVertex3f(VIEWPORT_W,VIEWPORT_H,1000);
		glVertex3f(VIEWPORT_W,0,1000);		
	glEnd();
	
	glPushMatrix();
	glTranslatef(0,VIEWPORT_H,0);
	glRotatef(110, 1, 0, 0);
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, global.textures.water.gltex);
	
	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(0,global.frames/(float)global.textures.water.h, 0);
	glMatrixMode(GL_MODELVIEW);
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex3f(0,0,0);
		glTexCoord2i(1,0);
		glVertex3f(VIEWPORT_W,0,0);
		glTexCoord2i(1,1);
		glVertex3f(VIEWPORT_W,1000,0);
		glTexCoord2i(0,1);
		glVertex3f(0,1000,0);
	glEnd();
	
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, global.textures.border.gltex);
	
// 	glColor3f(1,0.75,0.75);
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex3f(VIEWPORT_W,-500,0);
		glTexCoord2i(1,0);
		glVertex3f(VIEWPORT_W,-500,1000);
		glTexCoord2i(1,1);
		glVertex3f(VIEWPORT_W,1000,1000);
		glTexCoord2i(0,1);
		glVertex3f(VIEWPORT_W*0.75,1000,0);		
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
		create_projectile(&_projs.ball, VIEWPORT_W/2, ((Color) {0,0,1}), linear, 2*I);
	}
}

void stage0_loop() {
	stage_start();
	glEnable(GL_FOG);
	GLfloat clr[] = { 0.1, 0.1, 0.1, 0 };
	glFogfv(GL_FOG_COLOR, clr);
	glFogf(GL_FOG_MODE, GL_EXP);
	glFogf(GL_FOG_DENSITY, 0.005);
	
	while(!global.game_over) {
		stage0_events();
		stage_logic();
		stage_input();
		
		stage0_draw();
		stage_draw();
		
		SDL_GL_SwapBuffers();
		frame_rate();
	}
	
	glDisable(GL_FOG);
	stage_end();
}