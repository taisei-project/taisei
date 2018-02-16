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
#include "texture.h"

typedef struct Sprite {
	Texture *tex;
	FloatRect tex_area;
	float w;
	float h;
} Sprite;

char* sprite_path(const char *name);
void* load_sprite_begin(const char *path, unsigned int flags);
void* load_sprite_end(void *opaque, const char *path, unsigned int flags);
bool check_sprite_path(const char *path);

void draw_sprite(float x, float y, const char *name);
void draw_sprite_p(float x, float y, Sprite *spr);
void draw_sprite_unaligned(float x, float y, const char *name);
void draw_sprite_unaligned_p(float x, float y, Sprite *spr);
void draw_sprite_ex(float x, float y, float scale_x, float scale_y, bool align, Sprite *spr);

void begin_draw_sprite(float x, float y, float scale_x, float scale_y, bool align, Sprite *spr);
void end_draw_sprite(void);

Sprite* get_sprite(const char *name);
Sprite* prefix_get_sprite(const char *name, const char *prefix);

extern ResourceHandler sprite_res_handler;

#define SPRITE_PATH_PREFIX "res/gfx/"
#define SPRITE_EXTENSION ".spr"
