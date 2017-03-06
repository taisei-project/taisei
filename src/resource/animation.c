/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "animation.h"
#include "texture.h"
#include "global.h"
#include "resource.h"
#include "list.h"

#include "taisei_err.h"

char* animation_path(const char *name) {
	// stub
	// we could scan the gfx/ directory and find the first matching animation here
	// ...or we could describe animations with simple text files instead of encoding them in texture file names
	return NULL;
}

bool check_animation_path(const char *path) {
	char *base = strjoin(TEX_PATH_PREFIX, "ani_", NULL);
	bool result = strstartswith(path, base) && strendswith(path, TEX_EXTENSION);
	free(base);
	return result;
}

char* animation_name(const char *filename) {
	char *name = resource_util_basename(ANI_PATH_PREFIX, filename);
	char *c = name, *newname = NULL;

	while(c = strchr(c, '_')) {
		newname = ++c;
	}

	newname = strdup(newname);
	free(name);
	return newname;
}

void* load_animation(const char *filename) {
	Animation *ani = malloc(sizeof(Animation));

	char *basename = resource_util_basename(ANI_PATH_PREFIX, filename);
	char name[strlen(basename) + 1];
	strcpy(name, basename);

	char *tok;
	strtok(name, "_");

#define ANIFAIL(what) { warnx("load_animation(): bad '" what "' in filename '%s'", basename); free(basename); return NULL; }

	if((tok = strtok(NULL, "_")) == NULL)
		ANIFAIL("rows")
	ani->rows = atoi(tok);
	if((tok = strtok(NULL, "_")) == NULL)
		ANIFAIL("cols")
	ani->cols = atoi(tok);
	if((tok = strtok(NULL, "_")) == NULL)
		ANIFAIL("speed")
	ani->speed = atoi(tok);

	if(ani->speed == 0) {
		warnx("load_animation(): animation speed of %s == 0. relativity?", name);
		free(basename);
		return NULL;
	}

	ani->tex = get_tex(basename);

	if(!ani->tex) {
		warnx("load_animation(): couldn't get texture '%s'", basename);
		free(basename);
		return NULL;
	}

	ani->w = ani->tex->w / ani->cols;
	ani->h = ani->tex->h / ani->rows;

	free(basename);
	return (void*)ani;

#undef ANIFAIL
}

Animation *get_ani(const char *name) {
	return get_resource(RES_ANIM, name, RESF_REQUIRED)->animation;
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

