/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "texture.h"

struct Animation;

typedef struct Animation {
	int rows;
	int cols;

	int w;
	int h;

	int speed;

	Texture *tex;
} Animation;

char* animation_path(const char *name);
bool check_animation_path(const char *path);
void* load_animation_begin(const char *filename, unsigned int flags);
void* load_animation_end(void *opaque, const char *filename, unsigned int flags);

Animation *get_ani(const char *name);

void draw_animation(float x, float y, int col, int row, const char *name);
void draw_animation_p(float x, float y, int col, int row, Animation *ani);

#define ANI_PATH_PREFIX TEX_PATH_PREFIX
#define ANI_EXTENSION ".ani"
