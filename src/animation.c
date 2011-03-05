/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "animation.h"
#include "global.h"

#include <assert.h>

void init_animation(Animation *buf, int rows, int cols, int speed, const char *filename) {
	buf->rows = rows;
	buf->cols = cols;
	
	buf->speed = speed;
	
	load_texture(filename,&buf->tex);
	buf->w = buf->tex.w/cols;
	buf->h = buf->tex.h/rows;
}

void draw_animation(int x, int y, int row, const Animation *ani) { // matrices are cool
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ani->tex.gltex);
	
	float s = (float)ani->tex.w/ani->cols/ani->tex.truew;
	float t = ((float)ani->tex.h)/ani->tex.trueh/(float)ani->rows;
	
	assert(ani->speed != 0);
	
	glPushMatrix();
	glTranslatef(x,y,0);
	glScalef(ani->w/2,ani->h/2, 1);
	
	glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glScalef(s,t,1);
		glTranslatef(global.frames/ani->speed % ani->cols, row, 0);
	glMatrixMode(GL_MODELVIEW);
	
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex3f(-1, -1, 0);
		glTexCoord2f(0,1); glVertex3f(-1, 1, 0);	
		glTexCoord2f(1,1); glVertex3f(1, 1, 0);
		glTexCoord2f(1,0); glVertex3f(1, -1, 0);		
	glEnd();
	
	glMatrixMode(GL_TEXTURE);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}
	