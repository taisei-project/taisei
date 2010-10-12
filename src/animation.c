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

void draw_animation(int x, int y, int row, Animation *ani) {	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ani->tex.gltex);
	
	float s = (float)ani->tex.w/ani->cols/ani->tex.truew;
	float t = ((float)ani->tex.h)/ani->tex.trueh;
	
	assert(ani->speed != 0);
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
		glTexCoord2f(s*(global.frames/ani->speed % ani->cols),row/(float)ani->rows*t);
		glVertex3f(x - ani->w/2, y - ani->h/2, 0.0f);
		
		glTexCoord2f(s*(global.frames/ani->speed % ani->cols),(row+1)/(float)ani->rows*t);
		glVertex3f(x - ani->w/2, y + ani->h/2, 0.0f);	
		
		glTexCoord2f(s*(global.frames/ani->speed % ani->cols+1),(row+1)/(float)ani->rows*t);
		glVertex3f(x + ani->w/2, y + ani->h/2, 0.0f);
		
		glTexCoord2f(s*(global.frames/ani->speed % ani->cols+1),row/(float)ani->rows*t);
		glVertex3f(x + ani->w/2, y - ani->h/2, 0.0f);		
	glEnd();
	glDisable(GL_TEXTURE_2D);
	
}
	