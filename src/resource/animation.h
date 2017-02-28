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

Animation *init_animation(const char *filename);
Animation *get_ani(const char *name);
void delete_animations(void);
void draw_animation(float x, float y, int row, const char *name);
void draw_animation_p(float x, float y, int row, Animation *ani);

#endif
