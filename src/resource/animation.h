/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"
#include "sprite.h"

struct Animation;

typedef struct Animation {
	// i'm emulating lao's fixed rows/cols table system,
	// even though animations could have arbitrarily sized framegroups now.
	// i just don't want to rewrite the whole animation system from scratch.
	// be my guest if you want to try and do this the right way.

	int rows;
	int cols;
	int speed;

	Sprite **frames;
} Animation;

char* animation_path(const char *name);
bool check_animation_path(const char *path);
void* load_animation_begin(const char *filename, uint flags);
void* load_animation_end(void *opaque, const char *filename, uint flags);
void unload_animation(void *vani);

Animation *get_ani(const char *name);

void draw_animation(float x, float y, int col, int row, const char *name);
void draw_animation_p(float x, float y, int col, int row, Animation *ani);

extern ResourceHandler animation_res_handler;

#define ANI_PATH_PREFIX TEX_PATH_PREFIX
#define ANI_EXTENSION ".ani"
