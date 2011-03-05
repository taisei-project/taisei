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
	if(!((SDL_GetTicks()-f->birthtime) % 300)) {
		float angle = atan((float)(global.plr.y-f->y)/(global.plr.x-f->x))*180/M_PI+90;
		if(global.plr.x < f->x) angle += 180;
		
		create_projectile(&_projs.rice, f->x, f->y, 180, ((Color){0,0,1}), simple, 2);
	}
	f->moving = 1;
	f->dir = f->v < 0;
	
	f->x += f->v*DTe;
	
	f->y = sin((SDL_GetTicks()-f->birthtime)/160.0f)*20*DTe+f->sy;
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
	glPopMatrix();
	
	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	
	glPopMatrix();
}

void stage0_events() {
	if(!(global .frames % 100)) {
// 		int i;
// 		for(i = 0; i < VIEWPORT_W/15; i++)
// 			create_projectile(&_projs.ball, i*VIEWPORT_W/15, 0, 180, ((Color) {0,0,1}), simple, 2);
		create_fairy(0, 100, 1, 180, 2, simpleFairy);
		create_fairy(VIEWPORT_W-1, 10, -1, 180, 3, simpleFairy);
		create_fairy(VIEWPORT_W-1, 200, -1, 180, 3, simpleFairy);
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
		
		frame_rate();
	}
	
	glDisable(GL_FOG);
	stage_end();
}