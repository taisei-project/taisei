/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_util_pixmap_h
#define IGUARD_util_pixmap_h

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

#define PIXMAP_COMPRESSION_FORMATS(X, ...) \
	X(ETC1_RGB, RGB, __VA_ARGS__) \
	X(ETC2_RGBA, RGBA, __VA_ARGS__) \
	X(BC1_RGB, RGB, __VA_ARGS__) \
	X(BC3_RGBA, RGBA, __VA_ARGS__) \
	X(BC4_R, R, __VA_ARGS__) \
	X(BC5_RG, RG, __VA_ARGS__) \
	X(BC7_RGBA, RGBA, __VA_ARGS__) \
	X(PVRTC1_4_RGB, RGB, __VA_ARGS__) \
	X(PVRTC1_4_RGBA, RGBA, __VA_ARGS__) \
	X(ASTC_4x4_RGBA, RGBA, __VA_ARGS__) \
	X(ATC_RGB, RGB, __VA_ARGS__) \
	X(ATC_RGBA, RGBA, __VA_ARGS__) \
	X(PVRTC2_4_RGB, RGB, __VA_ARGS__) \
	X(PVRTC2_4_RGBA, RGBA, __VA_ARGS__) \
	X(ETC2_EAC_R11, R, __VA_ARGS__) \
	X(ETC2_EAC_RG11, RG, __VA_ARGS__) \
	X(FXT1_RGB, RGB, __VA_ARGS__) \

typedef enum PixmapCompression {
	#define DEFINE_PIXMAP_COMPRESSION(cformat, ...) \
		PIXMAP_COMPRESSION_##cformat,
	PIXMAP_COMPRESSION_FORMATS(DEFINE_PIXMAP_COMPRESSION,)
	#undef DEFINE_PIXMAP_COMPRESSION
} PixmapCompression;

enum {
	PIXMAP_FLOAT_BIT = 0x80,
	PIXMAP_COMPRESSED_BIT = 0x8000,

	PIXMAP_FLAG_BITS = PIXMAP_FLOAT_BIT | PIXMAP_COMPRESSED_BIT,
};

#define PIXMAP_MAKE_FORMAT(layout, depth) \
	(((layout) << 8) | (depth))

#define PIXMAP_MAKE_FLOAT_FORMAT(layout, depth) \
	(PIXMAP_MAKE_FORMAT(layout, depth) | PIXMAP_FLOAT_BIT)

#define PIXMAP_MAKE_COMPRESSED_FORMAT(layout, compression) \
	(PIXMAP_MAKE_FORMAT(layout, compression) | PIXMAP_COMPRESSED_BIT)

#define _impl_pixmap_format_is_compressed(fmt) \
	((fmt) & PIXMAP_COMPRESSED_BIT)
#define _impl_pixmap_format_is_float(fmt) \
	((fmt) & PIXMAP_FLOAT_BIT)
#define _impl_pixmap_format_layout(fmt) \
	((fmt) & ~PIXMAP_FLAG_BITS) >> 8
#define _impl_pixmap_format_depth(fmt) \
	((fmt) & ~PIXMAP_FLAG_BITS & 0xff)
#define _impl_pixmap_format_compression(fmt) \
	((fmt) & ~PIXMAP_FLAG_BITS & 0xff)
#define _impl_pixmap_format_pixel_size(fmt) \
	((_impl_pixmap_format_depth(fmt) >> 3) * _impl_pixmap_format_layout(fmt))

typedef enum PixmapFormat {
	PIXMAP_FORMAT_R8      = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_R,     8),
	PIXMAP_FORMAT_R16     = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_R,    16),
	PIXMAP_FORMAT_R32     = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_R,    32),

	PIXMAP_FORMAT_R16F    = PIXMAP_MAKE_FLOAT_FORMAT(PIXMAP_LAYOUT_R,    16),
	PIXMAP_FORMAT_R32F    = PIXMAP_MAKE_FLOAT_FORMAT(PIXMAP_LAYOUT_R,    32),

	PIXMAP_FORMAT_RG8     = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RG,    8),
	PIXMAP_FORMAT_RG16    = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RG,   16),
	PIXMAP_FORMAT_RG32    = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RG,   32),

	PIXMAP_FORMAT_RG16F   = PIXMAP_MAKE_FLOAT_FORMAT(PIXMAP_LAYOUT_RG,   16),
	PIXMAP_FORMAT_RG32F   = PIXMAP_MAKE_FLOAT_FORMAT(PIXMAP_LAYOUT_RG,   32),

	PIXMAP_FORMAT_RGB8    = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGB,   8),
	PIXMAP_FORMAT_RGB16   = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGB,  16),
	PIXMAP_FORMAT_RGB32   = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGB,  32),

	PIXMAP_FORMAT_RGB16F  = PIXMAP_MAKE_FLOAT_FORMAT(PIXMAP_LAYOUT_RGB,  16),
	PIXMAP_FORMAT_RGB32F  = PIXMAP_MAKE_FLOAT_FORMAT(PIXMAP_LAYOUT_RGB,  32),

	PIXMAP_FORMAT_RGBA8   = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGBA,  8),
	PIXMAP_FORMAT_RGBA16  = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGBA, 16),
	PIXMAP_FORMAT_RGBA32  = PIXMAP_MAKE_FORMAT(PIXMAP_LAYOUT_RGBA, 32),

	PIXMAP_FORMAT_RGBA16F = PIXMAP_MAKE_FLOAT_FORMAT(PIXMAP_LAYOUT_RGBA, 16),
	PIXMAP_FORMAT_RGBA32F = PIXMAP_MAKE_FLOAT_FORMAT(PIXMAP_LAYOUT_RGBA, 32),

	#define DEFINE_PIXMAP_COMPRESSED_FORMAT(cformat, layout, ...) \
		PIXMAP_FORMAT_##cformat = PIXMAP_MAKE_COMPRESSED_FORMAT(PIXMAP_LAYOUT_##layout, PIXMAP_COMPRESSION_##cformat),
	PIXMAP_COMPRESSION_FORMATS(DEFINE_PIXMAP_COMPRESSED_FORMAT,)
	#undef DEFINE_PIXMAP_COMPRESSION
} PixmapFormat;

// sanity check
static_assert_nomsg(_impl_pixmap_format_layout(PIXMAP_FORMAT_ETC2_EAC_RG11) == PIXMAP_LAYOUT_RG);
static_assert_nomsg(_impl_pixmap_format_compression(PIXMAP_FORMAT_ASTC_4x4_RGBA) == PIXMAP_COMPRESSION_ASTC_4x4_RGBA);
static_assert_nomsg(_impl_pixmap_format_depth(PIXMAP_FORMAT_RGB16F) == 16);
static_assert_nomsg(_impl_pixmap_format_depth(PIXMAP_FORMAT_RGB8) == 8);
static_assert_nomsg(_impl_pixmap_format_layout(PIXMAP_FORMAT_RGB8) == PIXMAP_LAYOUT_RGB);

INLINE bool pixmap_format_is_compressed(PixmapFormat fmt) {
	return _impl_pixmap_format_is_compressed(fmt);
}

INLINE bool pixmap_format_is_float(PixmapFormat fmt) {
	assert(!pixmap_format_is_compressed(fmt));
	return _impl_pixmap_format_is_float(fmt);
}

INLINE PixmapLayout pixmap_format_layout(PixmapFormat fmt) {
	return _impl_pixmap_format_layout(fmt);
}

INLINE uint pixmap_format_depth(PixmapFormat fmt) {
	assert(!pixmap_format_is_compressed(fmt));
	return _impl_pixmap_format_depth(fmt);
}

INLINE PixmapCompression pixmap_format_compression(PixmapFormat fmt) {
	assert(pixmap_format_is_compressed(fmt));
	return _impl_pixmap_format_compression(fmt);
}

INLINE uint pixmap_format_pixel_size(PixmapFormat fmt) {
	assert(!pixmap_format_is_compressed(fmt));
	return _impl_pixmap_format_pixel_size(fmt);
}

// TODO deprecate?
#define PIXMAP_FORMAT_LAYOUT(format) (pixmap_format_layout(format))
#define PIXMAP_FORMAT_DEPTH(format) (pixmap_format_depth(format))
#define PIXMAP_FORMAT_PIXEL_SIZE(format) (pixmap_format_pixel_size(format))
#define PIXMAP_FORMAT_IS_FLOAT(format) (pixmap_format_is_float(format))
#define PIXMAP_FORMAT_IS_COMPRESSED(format) (pixmap_format_is_compressed(format))

#define PIXMAP_PIXEL_STRUCT_R(type) union { \
	type values[1]; \
	struct { \
		type r; \
	}; \
}

#define PIXMAP_PIXEL_STRUCT_RG(type) union { \
	type values[2]; \
	struct { \
		type r; \
		type g; \
	}; \
}

#define PIXMAP_PIXEL_STRUCT_RGB(type) union { \
	type values[3]; \
	struct { \
		type r; \
		type g; \
		type b; \
	}; \
}

#define PIXMAP_PIXEL_STRUCT_RGBA(type) union { \
	type values[4]; \
	struct { \
		type r; \
		type g; \
		type b; \
		type a; \
	}; \
}

typedef PIXMAP_PIXEL_STRUCT_R(uint8_t)     PixelR8;
typedef PIXMAP_PIXEL_STRUCT_R(uint16_t)    PixelR16;
typedef PIXMAP_PIXEL_STRUCT_R(uint32_t)    PixelR32;
typedef PIXMAP_PIXEL_STRUCT_R(float)       PixelR32F;

typedef PIXMAP_PIXEL_STRUCT_RG(uint8_t)    PixelRG8;
typedef PIXMAP_PIXEL_STRUCT_RG(uint16_t)   PixelRG16;
typedef PIXMAP_PIXEL_STRUCT_RG(uint32_t)   PixelRG32;
typedef PIXMAP_PIXEL_STRUCT_RG(float)      PixelRG32F;

typedef PIXMAP_PIXEL_STRUCT_RGB(uint8_t)   PixelRGB8;
typedef PIXMAP_PIXEL_STRUCT_RGB(uint16_t)  PixelRGB16;
typedef PIXMAP_PIXEL_STRUCT_RGB(uint32_t)  PixelRGB32;
typedef PIXMAP_PIXEL_STRUCT_RGB(float)     PixelRGB32F;

typedef PIXMAP_PIXEL_STRUCT_RGBA(uint8_t)  PixelRGBA8;
typedef PIXMAP_PIXEL_STRUCT_RGBA(uint16_t) PixelRGBA16;
typedef PIXMAP_PIXEL_STRUCT_RGBA(uint32_t) PixelRGBA32;
typedef PIXMAP_PIXEL_STRUCT_RGBA(float)    PixelRGBA32F;

typedef struct Pixmap {
	union {
		void         *untyped;

		PixelR8      *r8;
		PixelR16     *r16;
		PixelR32     *r32;
		PixelR32F    *r32f;

		PixelRG8     *rg8;
		PixelRG16    *rg16;
		PixelRG32    *rg32;
		PixelRG32F   *rg32f;

		PixelRGB8    *rgb8;
		PixelRGB16   *rgb16;
		PixelRGB32   *rgb32;
		PixelRGB32F  *rgb32f;

		PixelRGBA8   *rgba8;
		PixelRGBA16  *rgba16;
		PixelRGBA32  *rgba32;
		PixelRGBA32F *rgba32f;
	} data;

	size_t width;
	size_t height;
	PixmapFormat format;
	PixmapOrigin origin;
} Pixmap;

void *pixmap_alloc_buffer(PixmapFormat format, size_t width, size_t height) attr_returns_allocated;
void *pixmap_alloc_buffer_for_copy(const Pixmap *src) attr_nonnull(1) attr_returns_allocated;
void *pixmap_alloc_buffer_for_conversion(const Pixmap *src, PixmapFormat format) attr_nonnull(1) attr_returns_allocated;

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

bool pixmap_load_file(const char *path, Pixmap *dst, PixmapFormat preferred_format) attr_nonnull(1, 2) attr_nodiscard;
bool pixmap_load_stream(SDL_RWops *stream, Pixmap *dst, PixmapFormat preferred_format) attr_nonnull(1, 2) attr_nodiscard;

bool pixmap_check_filename(const char *path);
char *pixmap_source_path(const char *prefix, const char *path) attr_nodiscard;

const char *pixmap_format_name(PixmapFormat fmt);

#endif // IGUARD_util_pixmap_h
