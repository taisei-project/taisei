/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
 */

#include "stage0.h"

#include <SDL/SDL_opengl.h>
#include "../stage.h"
#include "../global.h"

void simpleFairy(Fairy *f) {
	switch(global.frames-f->birthtime) {
		int a;		
		case 50:
		case 70:
		case 90:
			for(a = 120; a <= 220; a += 10)
				create_projectile(f->x, f->y, 2, a, simple, &_projs.rice, ((Color){0,0,1}));
	}
	f->moving = 1;
	f->dir = 1;
	f->x += 1;
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
	Color c = { 1,0,0 };
	switch(global.frames) {
		case 100:
			create_fairy(200, 100, 1, 180, 100, simpleFairy);
		case 150:
		case 200:
			break;
		case 250:
			create_projectile(200, 200, 0, 220, simple, &_projs.bigball, ((Color){1,0,0}));
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
		stage_logic();
		stage_input();
		stage0_events();
		
		glClear(GL_COLOR_BUFFER_BIT);
		stage0_draw();
		stage_draw();
		
		SDL_Delay(20);
	}
	
	glDisable(GL_FOG);
	stage_end();
}