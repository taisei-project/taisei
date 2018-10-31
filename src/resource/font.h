/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "sprite.h"
#include "hashtable.h"
#include "resource.h"
#include "renderer/api.h"

typedef enum {
	ALIGN_LEFT = 0, // must be 0
	ALIGN_CENTER,
	ALIGN_RIGHT,
} Alignment;

typedef ulong charcode_t;
typedef struct Font Font;

typedef struct FontMetrics {
	int ascent;
	int descent;
	int max_glyph_height;
	int lineskip;
	double scale;
} FontMetrics;

typedef struct GlyphMetrics {
	int bearing_x;
	int bearing_y;
	int width;
	int height;
	int advance;
} GlyphMetrics;

// TODO: maybe move this into util/geometry.h
typedef struct BBox {
	struct {
		int min;
		int max;
	} x;

	struct {
		int min;
		int max;
	} y;
} BBox;

typedef int (*GlyphDrawCallback)(Font *font, charcode_t charcode, SpriteParams *spr_params, void *userdata);

typedef struct TextParams {
	const char *font;
	Font *font_ptr;
	const char *shader;
	ShaderProgram *shader_ptr;
	struct {
		GlyphDrawCallback func;
		void *userdata;
	} glyph_callback;
	struct { double x, y; } pos;
	const Color *color;
	const ShaderCustomParams *shader_params;
	Texture *aux_textures[R_NUM_SPRITE_AUX_TEXTURES];
	BlendMode blend;
	Alignment align;
} TextParams;

Font* get_font(const char *font)
	attr_nonnull(1);

ShaderProgram* text_get_default_shader(void)
	attr_returns_nonnull;

const FontMetrics* font_get_metrics(Font *font)
	attr_nonnull(1) attr_returns_nonnull;

double font_get_lineskip(Font *font)
	attr_nonnull(1);

const GlyphMetrics* font_get_char_metrics(Font *font, charcode_t c)
	attr_nonnull(1);

double text_draw(const char *text, const TextParams *params)
	attr_nonnull(1, 2);

double text_draw_wrapped(const char *text, double max_width, const TextParams *params)
	attr_nonnull(1, 3);

void text_render(const char *text, Font *font, Sprite *out_sprite, BBox *out_bbox)
	attr_nonnull(1, 2, 3, 4);

void text_shorten(Font *font, char *text, double width)
	attr_nonnull(1, 2);

void text_wrap(Font *font, const char *src, double width, char *buf, size_t bufsize)
	attr_nonnull(1, 2, 4);

void text_bbox(Font *font, const char *text, uint maxlines, BBox *bbox)
	attr_nonnull(1, 2, 4);

int text_width_raw(Font *font, const char *text, uint maxlines)
	attr_nonnull(1, 2);

double text_width(Font *font, const char *text, uint maxlines)
	attr_nonnull(1, 2);

int text_height_raw(Font *font, const char *text, uint maxlines)
	attr_nonnull(1, 2);

double text_height(Font *font, const char *text, uint maxlines)
	attr_nonnull(1, 2);

extern ResourceHandler font_res_handler;

#define FONT_PATH_PREFIX "res/fonts/"
#define FONT_EXTENSION ".font"
