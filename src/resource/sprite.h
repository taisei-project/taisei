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

typedef struct SpriteMargin {
	float top, bottom, left, right;
} SpriteMargin;

typedef struct Sprite {
	Texture *tex;
	FloatRect tex_area;
	union {
		FloatExtent extent;
		struct { float w, h; };
	};
	SpriteMargin padding;
} Sprite;

INLINE float sprite_padded_width(const Sprite *restrict spr) {
	return spr->extent.w + spr->padding.left + spr->padding.right;
}

INLINE float sprite_padded_height(const Sprite *restrict spr) {
	return spr->extent.h + spr->padding.top + spr->padding.bottom;
}

INLINE FloatExtent sprite_padded_extent(const Sprite *restrict spr) {
	FloatExtent e;
	e.w = sprite_padded_width(spr);
	e.h = sprite_padded_height(spr);
	return e;
}

INLINE float sprite_padded_offset_x(const Sprite *restrict spr) {
	return (spr->padding.left - spr->padding.right) * 0.5;
}

INLINE float sprite_padded_offset_y(const Sprite *restrict spr) {
	return (spr->padding.top - spr->padding.bottom) * 0.5;
}

INLINE FloatOffset sprite_padded_offset(const Sprite *restrict spr) {
	FloatOffset o;
	o.x = sprite_padded_offset_x(spr);
	o.y = sprite_padded_offset_y(spr);
	return o;
}

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
