/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "animation.h"
#include "texture.h"
#include "resource.h"
#include "list.h"

ResourceHandler animation_res_handler = {
	.type = RES_ANIM,
	.typename = "animation",
	.subdir = ANI_PATH_PREFIX,

	.procs = {
		.find = animation_path,
		.check = check_animation_path,
		.begin_load = load_animation_begin,
		.end_load = load_animation_end,
		.unload = unload_animation,
	},
};

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

	Animation *ani = calloc(1, sizeof(Animation));

	if(!parse_keyvalue_file_with_spec(filename, (KVSpec[]){
		{ "rows",  .out_int = &ani->rows },
		{ "cols",  .out_int = &ani->cols },
		{ "speed", .out_int = &ani->speed },
		{ NULL },
	})) {
		free(ani);
		free(basename);
		return NULL;
	}

#define ANIFAIL(what) { log_warn("Bad '" what "' value in animation '%s'", basename); free(ani); free(basename); return NULL; }
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
	ani->frames = calloc(ani->rows * ani->cols, sizeof(Sprite*));

	char buf[strlen(data->basename) + sizeof(".frame0000")];

	for(int i = 0; i < ani->rows * ani->cols; ++i) {
		snprintf(buf, sizeof(buf), "%s.frame%04d", data->basename, i);
		Resource *res = get_resource(RES_SPRITE, buf, flags);

		if(res == NULL) {
			free(ani->frames);
			free(ani);
			ani = NULL;
			break;
		}

		ani->frames[i] = res->data;
	}

	free(data->basename);
	free(data);

	return ani;
}

void unload_animation(void *vani) {
	Animation *ani = vani;
	free(ani->frames);
	free(ani);
}

Animation *get_ani(const char *name) {
	return get_resource(RES_ANIM, name, RESF_DEFAULT)->data;
}

static Sprite* get_animation_frame(Animation *ani, int col, int row) {
	int idx = row * ani->cols + col;

	assert(idx >= 0);
	assert(idx < ani->rows * ani->cols);

	return ani->frames[idx];
}

void draw_animation(float x, float y, int col, int row, const char *name) {
	draw_animation_p(x, y, col, row, get_ani(name));
}

void draw_animation_p(float x, float y, int col, int row, Animation *ani) {
	draw_sprite_p(x, y, get_animation_frame(ani, col, row));
}
