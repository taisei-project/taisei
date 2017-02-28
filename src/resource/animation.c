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

Animation *get_ani_quiet(char *name) {
	Animation *a, *res = NULL;
	for(a = resources.animations; a; a = a->next) {
		if(strcmp(a->name, name) == 0) {
			res = a;
		}
	}
	return res;
}

Animation *init_animation(char *filename, bool transient) {
	char *name = determine_name(filename);
	if(name == NULL)
	{
		errx(-1, "init_animation(): unable to determine name of animation '%s'.", filename);
	}
	
	Animation *buf = get_ani_quiet(name);
	if (buf != NULL)
	{
		warnx("init_animation(): trying to load already loaded animation '%s' from '%s' -> skipped.", name, filename);
		return buf;
	}
	
	buf = create_element((void **)&resources.animations, sizeof(Animation));

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

	buf->transient = transient;
	buf->name = malloc(strlen(tok)+1);
	memset(buf->name, 0, strlen(tok)+1);
	strcpy(buf->name, tok);

	buf->tex = load_texture(filename, transient);
	buf->w = buf->tex->w/buf->cols;
	buf->h = buf->tex->h/buf->rows;

	free(name);
	printf("-- initialized animation '%s'\n", buf->name);
	return buf;
}

Animation *get_ani(char *name) {
	Animation *res = get_ani_quiet(name);
	if(res == NULL)
		errx(-1,"get_ani():\n!- cannot load animation '%s'", name);
	return res;
}

void delete_animation(void **anis, void *ani) {
	free(((Animation *)ani)->name);
	delete_element(anis, ani);
}

void delete_animations(bool transient) {
	delete_all_or_transient_elements((void **)&resources.animations, delete_animation, transient);
}

void draw_animation(float x, float y, int row, char *name) {
	draw_animation_p(x, y, row, get_ani(name));
}

void draw_animation_p(float x, float y, int row, Animation *ani) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ani->tex->gltex);

	float s = (float)ani->tex->w/ani->cols/ani->tex->truew;
	float t = ((float)ani->tex->h)/ani->tex->trueh/(float)ani->rows;

	if(ani->speed == 0)
		errx(-1, "animation speed of %s == 0. relativity?", ani->name);

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
