/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef ANIMATION_H
#define ANIMATION_H

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
void* load_animation(const char *filename, unsigned int flags);

Animation *get_ani(const char *name);

void draw_animation(float x, float y, int row, const char *name);
void draw_animation_p(float x, float y, int row, Animation *ani);

#define ANI_PATH_PREFIX TEX_PATH_PREFIX
#define ANI_EXTENSION ".ani"

#endif
