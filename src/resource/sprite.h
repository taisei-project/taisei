/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_sprite_h
#define IGUARD_resource_sprite_h

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

char* sprite_path(const char *name);
void* load_sprite_begin(const char *path, uint flags);
void* load_sprite_end(void *opaque, const char *path, uint flags);
bool check_sprite_path(const char *path);

void draw_sprite(float x, float y, const char *name);
void draw_sprite_p(float x, float y, Sprite *spr);
void draw_sprite_batched(float x, float y, const char *name) attr_deprecated("Use r_draw_sprite instead");
void draw_sprite_batched_p(float x, float y, Sprite *spr) attr_deprecated("Use r_draw_sprite instead");
void draw_sprite_ex(float x, float y, float scale_x, float scale_y, bool batched, Sprite *spr) attr_deprecated("Use r_draw_sprite instead");

Sprite* get_sprite(const char *name);
Sprite* prefix_get_sprite(const char *name, const char *prefix);

extern ResourceHandler sprite_res_handler;

#define SPRITE_PATH_PREFIX "res/gfx/"
#define SPRITE_EXTENSION ".spr"

#endif // IGUARD_resource_sprite_h
