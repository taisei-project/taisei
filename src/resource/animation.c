/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "animation.h"
#include "texture.h"
#include "global.h"
#include "resource.h"
#include "list.h"

char* animation_path(const char *name) {
	return strjoin(ANI_PATH_PREFIX, name, ANI_EXTENSION, NULL);
}

bool check_animation_path(const char *path) {
	return strendswith(path, ANI_EXTENSION);
}

typedef struct AnimationLoadData {
	Animation *ani;
	char *basename;
} AnimationLoadData;

void* load_animation_begin(const char *filename, unsigned int flags) {
	char *basename = resource_util_basename(ANI_PATH_PREFIX, filename);
	char name[strlen(basename) + 1];
	strcpy(name, basename);

	Hashtable *ht = parse_keyvalue_file(filename, 5);

	if(!ht) {
		log_warn("parse_keyvalue_file() failed");
		free(basename);
		return NULL;
	}

	Animation *ani = malloc(sizeof(Animation));
	ani->rows = atoi((char*)hashtable_get_string(ht, "rows"));
	ani->cols = atoi((char*)hashtable_get_string(ht, "cols"));
	ani->speed = atoi((char*)hashtable_get_string(ht, "speed"));
	hashtable_foreach(ht, hashtable_iter_free_data, NULL);
	hashtable_free(ht);

#define ANIFAIL(what) { log_warn("Bad '" what "' in animation '%s'", basename); free(ani); free(basename); return NULL; }
	if(ani->rows < 1) ANIFAIL("rows")
	if(ani->cols < 1) ANIFAIL("cols")
#undef ANIFAIL

	if(ani->speed == 0) {
		log_warn("Animation speed of %s == 0. relativity?", name);
		free(basename);
		free(ani);
		return NULL;
	}

	AnimationLoadData *data = malloc(sizeof(AnimationLoadData));
	data->ani = ani;
	data->basename = basename;
	return data;
}

void* load_animation_end(void *opaque, const char *filename, unsigned int flags) {
	AnimationLoadData *data = opaque;

	if(!data) {
		return NULL;
	}

	Animation *ani = data->ani;
	ani->tex = get_resource(RES_TEXTURE, data->basename, flags)->texture;

	if(!ani->tex) {
		log_warn("Couldn't get texture '%s'", data->basename);
		free(data->basename);
		free(data);
		return NULL;
	}

	ani->w = ani->tex->w / ani->cols;
	ani->h = ani->tex->h / ani->rows;

	free(data->basename);
	free(data);

	return ani;
}

Animation *get_ani(const char *name) {
	return get_resource(RES_ANIM, name, RESF_DEFAULT)->animation;
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

