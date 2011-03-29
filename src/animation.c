/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "animation.h"
#include "global.h"

#include <assert.h>

Animation *init_animation(char *filename) {
	Animation *buf = create_element((void **)&global.animations, sizeof(Animation));
	
	char *beg = strstr(filename, "gfx/") + 4;
	char *end = strrchr(filename, '.');
		
	char *name = malloc(end - beg + 1);
	memset(name, 0, end - beg + 1);
	strncpy(name, beg, end-beg);
	
	char* tok;
	strtok(name, "_");
	
	if((tok = strtok(NULL, "_")) == NULL)
		errx(-1, "init_animation():\n!- bad 'rows' in filename '%s'", name);
	buf->rows = atoi(tok);
	if((tok = strtok(NULL, "_")) == NULL)
		errx(-1, "init_animation():\n!- bad 'cols' in filename '%s'", name);
	buf->cols = atoi(tok);
	if((tok = strtok(NULL, "_")) == NULL)
		errx(-1, "init_animation():\n!- bad 'speed' in filename '%s'", name);
	buf->speed = atoi(tok);
	
	if((tok = strtok(NULL, "_")) == NULL)
		errx(-1, "init_animation():\n!- bad 'name' in filename '%s'", name);
	
	
	buf->name = malloc(strlen(tok)+1);
	memset(buf->name, 0, strlen(tok)+1);
	strcpy(buf->name, tok);
	
	buf->tex = load_texture(filename);
	buf->w = buf->tex->w/buf->cols;
	buf->h = buf->tex->h/buf->rows;
	
	printf("-- initialized animation '%s'\n", buf->name);
	return buf;
}

Animation *get_ani(char *name) {
	Animation *a;
	Animation *res = NULL;
	for(a = global.animations; a; a = a->next) {
		if(strcmp(a->name, name) == 0) {
			res = a;
		}
	}
	
	if(res == NULL)
		errx(-1,"get_ani():\n!- cannot load animation '%s'", name);
	
	return res;
}

void delete_animations() {
	delete_all_elements((void **)&global.animations);
}

void draw_animation(float x, float y, int row, char *name) {
	draw_animation_p(x, y, row, get_ani(name));
}

void draw_animation_p(float x, float y, int row, Animation *ani) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ani->tex->gltex);
	
	float s = (float)ani->tex->w/ani->cols/ani->tex->truew;
	float t = ((float)ani->tex->h)/ani->tex->trueh/(float)ani->rows;
	
	assert(ani->speed != 0);
	
	glPushMatrix();
	glTranslatef(x,y,0);
	glScalef(ani->w/2,ani->h/2, 1);
	
	glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glScalef(s,t,1);
		glTranslatef(global.frames/ani->speed % ani->cols, row, 0);
	glMatrixMode(GL_MODELVIEW);
	
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
	