/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"
#include "texture.h"

typedef struct Sprite {
	Texture *tex;
	FloatRect tex_area;
	union {
		// NOTE: This is stored with padding pre-applied.
		// To get the area actually occupied by the image, subtract `padding.extent` from the size
		// and bias the origin by `padding.offset`.
		// You shouldn't need to worry about it in most cases, unless you're doing some low-level
		// sprite rendering or want to add virtual paddings to your own Sprite instance.
		FloatExtent extent;
		struct { float w, h; };
	};
	FloatRect padding;
} Sprite;

FloatRect sprite_denormalized_tex_coords(const Sprite *restrict spr);
IntRect sprite_denormalized_int_tex_coords(const Sprite *restrict spr);
void sprite_set_denormalized_tex_coords(Sprite *restrict spr, FloatRect tc);

void draw_sprite(float x, float y, const char *name);
void draw_sprite_p(float x, float y, Sprite *spr);

DEFINE_RESOURCE_GETTER(Sprite, res_sprite, RES_SPRITE)
DEFINE_OPTIONAL_RESOURCE_GETTER(Sprite, res_sprite_optional, RES_SPRITE)
DEFINE_DEPRECATED_RESOURCE_GETTER(Sprite, get_sprite, res_sprite)

Sprite *prefix_get_sprite(const char *name, const char *prefix);

extern ResourceHandler sprite_res_handler;

#define SPRITE_PATH_PREFIX "res/gfx/"
#define SPRITE_EXTENSION ".spr"
