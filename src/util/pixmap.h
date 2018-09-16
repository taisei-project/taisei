/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>

typedef enum PixmapLayout {
	PIXMAP_LAYOUT_R = 1,
	PIXMAP_LAYOUT_RG,
	PIXMAP_LAYOUT_RGB,
	PIXMAP_LAYOUT_RGBA,
} PixmapLayout;

typedef enum PixmapOrigin {
	PIXMAP_ORIGIN_TOPLEFT,
	PIXMAP_ORIGIN_BOTTOMLEFT,
} PixmapOrigin;

#define PIXMAP_MAKE_FORMAT(layout, depth) (((depth) >> 3) | ((layout) << 8))
#define PIXMAP_FORMAT_LAYOUT(format) ((format) >> 8)
#define PIXMAP_FORMAT_DEPTH(format) (((format) & 0xff) << 3)
#define PIXMAP_FORMAT_PIXEL_SIZE(format) ((PIXMAP_FORMAT_DEPTH(format) >> 3) * PIXMAP_FORMAT_LAYOUT(format))

typedef enum PixmapFormat {
	PIXMAP_FORMAT_R8     = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_R,     8),
	PIXMAP_FORMAT_R16    = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_R,    16),
	PIXMAP_FORMAT_R32    = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_R,    32),

	PIXMAP_FORMAT_RG8    = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RG,    8),
	PIXMAP_FORMAT_RG16   = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RG,   16),
	PIXMAP_FORMAT_RG32   = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RG,   32),

	PIXMAP_FORMAT_RGB8   = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGB,   8),
	PIXMAP_FORMAT_RGB16  = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGB,  16),
	PIXMAP_FORMAT_RGB32  = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGB,  32),

	PIXMAP_FORMAT_RGBA8  = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGBA,  8),
	PIXMAP_FORMAT_RGBA16 = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGBA, 16),
	PIXMAP_FORMAT_RGBA32 = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGBA, 32),
} PixmapFormat;

#define PIXMAP_PIXEL_STRUCT_R(depth) union { \
	uint##depth##_t values[1]; \
	struct { \
		uint##depth##_t r; \
	}; \
}

#define PIXMAP_PIXEL_STRUCT_RG(depth) union { \
	uint##depth##_t values[2]; \
	struct { \
		uint##depth##_t r; \
		uint##depth##_t g; \
	}; \
}

#define PIXMAP_PIXEL_STRUCT_RGB(depth) union { \
	uint##depth##_t values[3]; \
	struct { \
		uint##depth##_t r; \
		uint##depth##_t g; \
		uint##depth##_t b; \
	}; \
}

#define PIXMAP_PIXEL_STRUCT_RGBA(depth) union { \
	uint##depth##_t values[4]; \
	struct { \
		uint##depth##_t r; \
		uint##depth##_t g; \
		uint##depth##_t b; \
		uint##depth##_t a; \
	}; \
}

typedef PIXMAP_PIXEL_STRUCT_R(8)     PixelR8;
typedef PIXMAP_PIXEL_STRUCT_R(16)    PixelR16;
typedef PIXMAP_PIXEL_STRUCT_R(32)    PixelR32;

typedef PIXMAP_PIXEL_STRUCT_RG(8)    PixelRG8;
typedef PIXMAP_PIXEL_STRUCT_RG(16)   PixelRG16;
typedef PIXMAP_PIXEL_STRUCT_RG(32)   PixelRG32;

typedef PIXMAP_PIXEL_STRUCT_RGB(8)   PixelRGB8;
typedef PIXMAP_PIXEL_STRUCT_RGB(16)  PixelRGB16;
typedef PIXMAP_PIXEL_STRUCT_RGB(32)  PixelRGB32;

typedef PIXMAP_PIXEL_STRUCT_RGBA(8)  PixelRGBA8;
typedef PIXMAP_PIXEL_STRUCT_RGBA(16) PixelRGBA16;
typedef PIXMAP_PIXEL_STRUCT_RGBA(32) PixelRGBA32;

typedef struct Pixmap {
	union {
		void        *untyped;

		PixelR8     *r8;
		PixelR16    *r16;
		PixelR32    *r32;

		PixelRG8    *rg8;
		PixelRG16   *rg16;
		PixelRG32   *rg32;

		PixelRGB8   *rgb8;
		PixelRGB16  *rgb16;
		PixelRGB32  *rgb32;

		PixelRGBA8  *rgba8;
		PixelRGBA16 *rgba16;
		PixelRGBA32 *rgba32;
	} data;

	size_t width;
	size_t height;
	PixmapFormat format;
	PixmapOrigin origin;
} Pixmap;

void* pixmap_alloc_buffer(PixmapFormat format, size_t width, size_t height) attr_nodiscard;
void* pixmap_alloc_buffer_for_copy(const Pixmap *src) attr_nonnull(1) attr_nodiscard;
void* pixmap_alloc_buffer_for_conversion(const Pixmap *src, PixmapFormat format) attr_nonnull(1) attr_nodiscard;

void pixmap_copy(const Pixmap *src, Pixmap *dst) attr_nonnull(1, 2);
void pixmap_copy_alloc(const Pixmap *src, Pixmap *dst) attr_nonnull(1, 2);

void pixmap_convert(const Pixmap *src, Pixmap *dst, PixmapFormat format) attr_nonnull(1, 2);
void pixmap_convert_alloc(const Pixmap *src, Pixmap *dst, PixmapFormat format) attr_nonnull(1, 2);
void pixmap_convert_inplace_realloc(Pixmap *src, PixmapFormat format);

void pixmap_flip_y(const Pixmap *src, Pixmap *dst) attr_nonnull(1, 2);
void pixmap_flip_y_alloc(const Pixmap *src, Pixmap *dst) attr_nonnull(1, 2);
void pixmap_flip_y_inplace(Pixmap *src) attr_nonnull(1);

void pixmap_flip_to_origin(const Pixmap *src, Pixmap *dst, PixmapOrigin origin) attr_nonnull(1, 2);
void pixmap_flip_to_origin_alloc(const Pixmap *src, Pixmap *dst, PixmapOrigin origin) attr_nonnull(1, 2);
void pixmap_flip_to_origin_inplace(Pixmap *src, PixmapOrigin origin) attr_nonnull(1);

size_t pixmap_data_size(const Pixmap *px) attr_nonnull(1);

bool pixmap_load_file(const char *path, Pixmap *dst) attr_nonnull(1, 2) attr_nodiscard;
bool pixmap_load_stream(SDL_RWops *stream, Pixmap *dst) attr_nonnull(1, 2) attr_nodiscard;
bool pixmap_load_stream_tga(SDL_RWops *stream, Pixmap *dst) attr_nonnull(1, 2) attr_nodiscard;
