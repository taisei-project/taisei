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
	struct Animation *next;
	struct Animation *prev;
	bool transient;

	int rows;
	int cols;

	int w,h;

	int speed;

	char *name;
	Texture *tex;
} Animation;

Animation *init_animation(char *filename, bool transient);
Animation *get_ani(char *name);
void delete_animations(bool transient);
void draw_animation(float x, float y, int row, char *name);
void draw_animation_p(float x, float y, int row, Animation *ani);

#endif
