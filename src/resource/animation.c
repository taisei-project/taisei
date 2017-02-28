/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "animation.h"
#include "global.h"
#include "resource.h"
#include "list.h"

#include "taisei_err.h"

Animation *init_animation(const char *filename) {
	Animation *buf = malloc(sizeof(Animation));

	char *beg = strstr(filename, "gfx/") + 4;
	char *end = strrchr(filename, '.');

	int sz = end - beg + 1;
	char name[end - beg + 1];
	strlcpy(name, beg, sz);

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

	if(buf->speed == 0)
		errx(-1, "animation speed of %s == 0. relativity?", name);

	if((tok = strtok(NULL, "_")) == NULL)
		errx(-1, "init_animation():\n!- bad 'name' in filename '%s'", name);

	char refname[strlen(tok)+1];
	memset(refname, 0, strlen(tok)+1);
	strcpy(refname, tok);

	buf->tex = load_texture(filename);
	buf->w = buf->tex->w/buf->cols;
	buf->h = buf->tex->h/buf->rows;

	hashtable_set_string(resources.animations, refname, buf);

	printf("-- initialized animation '%s'\n", name);
	return buf;
}

Animation *get_ani(const char *name) {
	Animation *res = hashtable_get_string(resources.animations, name);

	if(res == NULL)
		errx(-1,"get_ani():\n!- cannot load animation '%s'", name);

	return res;
}

void delete_animations(void) {
	resources_delete_and_unset_all(resources.animations, hashtable_iter_free_data, NULL);
}

void draw_animation(float x, float y, int row, const char *name) {
	draw_animation_p(x, y, row, get_ani(name));
}

void draw_animation_p(float x, float y, int row, Animation *ani) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ani->tex->gltex);

	float s = (float)ani->tex->w/ani->cols/ani->tex->truew;
	float t = ((float)ani->tex->h)/ani->tex->trueh/(float)ani->rows;

	glPushMatrix();
	if(x || y)
		glTranslatef(x,y,0);
	if(ani->w != 1 || ani->h != 1)
		glScalef(ani->w,ani->h, 1);

	glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glScalef(s,t,1);

		int col = global.frames/ani->speed % ani->cols;
		if(col || row)
			glTranslatef(col, row, 0);
	glMatrixMode(GL_MODELVIEW);

	draw_quad();

	glMatrixMode(GL_TEXTURE);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

