/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "pixmap.h"
#include "util.h"
#include "pixmap_loaders/loaders.h"

// NOTE: this is pretty stupid and not at all optimized, patches welcome

#define _CONV_FUNCNAME	convert_u8_to_u8
#define _CONV_IN_MAX	UINT8_MAX
#define _CONV_IN_TYPE	uint8_t
#define _CONV_OUT_MAX	UINT8_MAX
#define _CONV_OUT_TYPE	uint8_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u8_to_u16
#define _CONV_IN_MAX	UINT8_MAX
#define _CONV_IN_TYPE	uint8_t
#define _CONV_OUT_MAX	UINT16_MAX
#define _CONV_OUT_TYPE	uint16_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u8_to_u32
#define _CONV_IN_MAX	UINT8_MAX
#define _CONV_IN_TYPE	uint8_t
#define _CONV_OUT_MAX	UINT32_MAX
#define _CONV_OUT_TYPE	uint32_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u8_to_f32
#define _CONV_IN_MAX	UINT8_MAX
#define _CONV_IN_TYPE	uint8_t
#define _CONV_OUT_MAX	1.0f
#define _CONV_OUT_TYPE	float
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u16_to_u8
#define _CONV_IN_MAX	UINT16_MAX
#define _CONV_IN_TYPE	uint16_t
#define _CONV_OUT_MAX	UINT8_MAX
#define _CONV_OUT_TYPE	uint8_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u16_to_u16
#define _CONV_IN_MAX	UINT16_MAX
#define _CONV_IN_TYPE	uint16_t
#define _CONV_OUT_MAX	UINT16_MAX
#define _CONV_OUT_TYPE	uint16_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u16_to_u32
#define _CONV_IN_MAX	UINT16_MAX
#define _CONV_IN_TYPE	uint16_t
#define _CONV_OUT_MAX	UINT32_MAX
#define _CONV_OUT_TYPE	uint32_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u16_to_f32
#define _CONV_IN_MAX	UINT16_MAX
#define _CONV_IN_TYPE	uint16_t
#define _CONV_OUT_MAX	1.0f
#define _CONV_OUT_TYPE	float
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u32_to_u8
#define _CONV_IN_MAX	UINT32_MAX
#define _CONV_IN_TYPE	uint32_t
#define _CONV_OUT_MAX	UINT8_MAX
#define _CONV_OUT_TYPE	uint8_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u32_to_u16
#define _CONV_IN_MAX	UINT32_MAX
#define _CONV_IN_TYPE	uint32_t
#define _CONV_OUT_MAX	UINT16_MAX
#define _CONV_OUT_TYPE	uint16_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u32_to_u32
#define _CONV_IN_MAX	UINT32_MAX
#define _CONV_IN_TYPE	uint32_t
#define _CONV_OUT_MAX	UINT32_MAX
#define _CONV_OUT_TYPE	uint32_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_u32_to_f32
#define _CONV_IN_MAX	UINT32_MAX
#define _CONV_IN_TYPE	uint32_t
#define _CONV_OUT_MAX	1.0f
#define _CONV_OUT_TYPE	float
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_f32_to_u8
#define _CONV_IN_MAX	1.0f
#define _CONV_IN_TYPE	float
#define _CONV_OUT_MAX	UINT8_MAX
#define _CONV_OUT_TYPE	uint8_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_f32_to_u16
#define _CONV_IN_MAX	1.0f
#define _CONV_IN_TYPE	float
#define _CONV_OUT_MAX	UINT16_MAX
#define _CONV_OUT_TYPE	uint16_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_f32_to_u32
#define _CONV_IN_MAX	1.0f
#define _CONV_IN_TYPE	float
#define _CONV_OUT_MAX	UINT32_MAX
#define _CONV_OUT_TYPE	uint32_t
#include "pixmap_conversion.inc.h"

#define _CONV_FUNCNAME	convert_f32_to_f32
#define _CONV_IN_MAX	1.0f
#define _CONV_IN_TYPE	float
#define _CONV_OUT_MAX	1.0f
#define _CONV_OUT_TYPE	float
#include "pixmap_conversion.inc.h"

#define DEPTH_FLOAT_BIT (1 << 10)

typedef void (*convfunc_t)(
	size_t in_elements,
	size_t out_elements,
	size_t num_pixels,
	void *vbuf_in,
	void *vbuf_out,
	void *default_pixel
);

#define CONV(in, in_depth, out, out_depth) { convert_##in##_to_##out, in_depth, out_depth }

struct conversion_def {
	convfunc_t func;
	uint depth_in;
	uint depth_out;
};

struct conversion_def conversion_table[] = {
	CONV(u8,   8,   u8,   8),
	CONV(u8,   8,   u16, 16),
	CONV(u8,   8,   u32, 32),
	CONV(u8,   8,   f32, 32 | DEPTH_FLOAT_BIT),

	CONV(u16, 16,   u8,   8),
	CONV(u16, 16,   u16, 16),
	CONV(u16, 16,   u32, 32),
	CONV(u16, 16,   f32, 32 | DEPTH_FLOAT_BIT),

	CONV(u32, 32,   u8,   8),
	CONV(u32, 16,   u16, 16),
	CONV(u32, 32,   u32, 32),
	CONV(u32, 32,   f32, 32 | DEPTH_FLOAT_BIT),

	CONV(f32, 32 | DEPTH_FLOAT_BIT, u8,   8),
	CONV(f32, 16 | DEPTH_FLOAT_BIT, u16, 16),
	CONV(f32, 32 | DEPTH_FLOAT_BIT, u32, 32),
	CONV(f32, 32 | DEPTH_FLOAT_BIT, f32, 32 | DEPTH_FLOAT_BIT),

	{ 0 }
};

static struct conversion_def* find_conversion(uint depth_in, uint depth_out) {
	for(struct conversion_def *cv = conversion_table; cv->func; ++cv) {
		if(cv->depth_in == depth_in && cv->depth_out == depth_out) {
			return cv;
		}
	}

	log_fatal("Pixmap conversion for %upbc -> %upbc undefined, please add", depth_in, depth_out);
}

static void *default_pixel(uint depth) {
	static uint8_t  default_u8[]  = { 0, 0, 0, UINT8_MAX  };
	static uint16_t default_u16[] = { 0, 0, 0, UINT16_MAX };
	static uint32_t default_u32[] = { 0, 0, 0, UINT32_MAX };
	static float    default_f32[] = { 0, 0, 0, 1.0f       };

	switch(depth) {
		case 8:  return default_u8;
		case 16: return default_u16;
		case 32: return default_u32;
		case 32 | DEPTH_FLOAT_BIT: return default_f32;
		default: UNREACHABLE;
	}
}

void *pixmap_alloc_buffer(PixmapFormat format, size_t width, size_t height) {
	assert(width >= 1);
	assert(height >= 1);
	size_t pixel_size = PIXMAP_FORMAT_PIXEL_SIZE(format);
	assert(pixel_size >= 1);
	return calloc(width * height, pixel_size);
}

void *pixmap_alloc_buffer_for_copy(const Pixmap *src) {
	return pixmap_alloc_buffer(src->format, src->width, src->height);
}

void *pixmap_alloc_buffer_for_conversion(const Pixmap *src, PixmapFormat format) {
	return pixmap_alloc_buffer(format, src->width, src->height);
}

static void pixmap_copy_meta(const Pixmap *src, Pixmap *dst) {
	dst->format = src->format;
	dst->width = src->width;
	dst->height = src->height;
	dst->origin = src->origin;
}

void pixmap_convert(const Pixmap *src, Pixmap *dst, PixmapFormat format) {
	size_t num_pixels = src->width * src->height;
	size_t pixel_size = PIXMAP_FORMAT_PIXEL_SIZE(format);

	assert(dst->data.untyped != NULL);
	pixmap_copy_meta(src, dst);

	if(src->format == format) {
		memcpy(dst->data.untyped, src->data.untyped, num_pixels * pixel_size);
		return;
	}

	dst->format = format;

	struct conversion_def *cv = find_conversion(
		PIXMAP_FORMAT_DEPTH(src->format) | (PIXMAP_FORMAT_IS_FLOAT(src->format) * DEPTH_FLOAT_BIT),
		PIXMAP_FORMAT_DEPTH(dst->format) | (PIXMAP_FORMAT_IS_FLOAT(dst->format) * DEPTH_FLOAT_BIT)
	);

	cv->func(
		PIXMAP_FORMAT_LAYOUT(src->format),
		PIXMAP_FORMAT_LAYOUT(dst->format),
		num_pixels,
		src->data.untyped,
		dst->data.untyped,
		default_pixel(PIXMAP_FORMAT_DEPTH(dst->format) | (PIXMAP_FORMAT_IS_FLOAT(dst->format) * DEPTH_FLOAT_BIT))
	);
}

void pixmap_convert_alloc(const Pixmap *src, Pixmap *dst, PixmapFormat format) {
	dst->data.untyped = pixmap_alloc_buffer_for_conversion(src, format);
	pixmap_convert(src, dst, format);
}

void pixmap_convert_inplace_realloc(Pixmap *src, PixmapFormat format) {
	assert(src->data.untyped != NULL);

	if(src->format == format) {
		return;
	}

	Pixmap tmp;
	pixmap_copy_meta(src, &tmp);
	pixmap_convert_alloc(src, &tmp, format);

	free(src->data.untyped);
	*src = tmp;
}

void pixmap_copy(const Pixmap *src, Pixmap *dst) {
	assert(dst->data.untyped != NULL);
	pixmap_copy_meta(src, dst);
	memcpy(dst->data.untyped, src->data.untyped, pixmap_data_size(src));
}

void pixmap_copy_alloc(const Pixmap *src, Pixmap *dst) {
	dst->data.untyped = pixmap_alloc_buffer_for_copy(src);
	pixmap_copy(src, dst);
}

size_t pixmap_data_size(const Pixmap *px) {
	return px->width * px->height * PIXMAP_FORMAT_PIXEL_SIZE(px->format);
}

void pixmap_flip_y(const Pixmap *src, Pixmap *dst) {
	assert(dst->data.untyped != NULL);
	pixmap_copy_meta(src, dst);

	size_t rows = src->height;
	size_t row_length = src->width * PIXMAP_FORMAT_PIXEL_SIZE(src->format);

	char *cdst = dst->data.untyped;
	const char *csrc = src->data.untyped;

	for(size_t row = 0, irow = rows - 1; row < rows; ++row, --irow) {
		memcpy(cdst + irow * row_length, csrc + row * row_length, row_length);
	}
}

void pixmap_flip_y_alloc(const Pixmap *src, Pixmap *dst) {
	dst->data.untyped = pixmap_alloc_buffer_for_copy(src);
	pixmap_flip_y(src, dst);
}

void pixmap_flip_y_inplace(Pixmap *src) {
	size_t rows = src->height;
	size_t row_length = src->width * PIXMAP_FORMAT_PIXEL_SIZE(src->format);
	char *data = src->data.untyped;
	char swap_buffer[row_length];

	for(size_t row = 0; row < rows / 2; ++row) {
		memcpy(swap_buffer, data + row * row_length, row_length);
		memcpy(data + row * row_length, data + (rows - row - 1) * row_length, row_length);
		memcpy(data + (rows - row - 1) * row_length, swap_buffer, row_length);
	}
}

void pixmap_flip_to_origin(const Pixmap *src, Pixmap *dst, PixmapOrigin origin) {
	assert(dst->data.untyped != NULL);

	if(src->origin == origin) {
		pixmap_copy(src, dst);
	} else {
		pixmap_flip_y(src, dst);
		dst->origin = origin;
	}
}

void pixmap_flip_to_origin_alloc(const Pixmap *src, Pixmap *dst, PixmapOrigin origin) {
	dst->data.untyped = pixmap_alloc_buffer_for_copy(src);
	pixmap_flip_to_origin(src, dst, origin);
}

void pixmap_flip_to_origin_inplace(Pixmap *src, PixmapOrigin origin) {
	if(src->origin == origin) {
		return;
	}

	pixmap_flip_y_inplace(src);
	src->origin = origin;
}

static PixmapLoader *pixmap_loaders[] = {
	&pixmap_loader_png,
	&pixmap_loader_webp,
	NULL,
};

static PixmapLoader *pixmap_loader_for_filename(const char *file) {
	char *ext = strrchr(file, '.');

	if(!ext || !*(++ext)) {
		return NULL;
	}

	for(PixmapLoader **loader = pixmap_loaders; *loader; ++loader) {
		PixmapLoader *l = *loader;
		for(const char **l_ext = l->filename_exts; *l_ext; ++l_ext) {
			if(!SDL_strcasecmp(*l_ext, ext)) {
				return l;
			}
		}
	}

	return NULL;
}

bool pixmap_load_stream(SDL_RWops *stream, Pixmap *dst, PixmapFormat preferred_format) {
	for(PixmapLoader **loader = pixmap_loaders; *loader; ++loader) {
		bool match = (*loader)->probe(stream);
		SDL_RWseek(stream, 0, RW_SEEK_SET);

		if(match) {
			return (*loader)->load(stream, dst, preferred_format);
		}
	}

	log_error("Image format not recognized");
	return false;
}

bool pixmap_load_file(const char *path, Pixmap *dst, PixmapFormat preferred_format) {
	log_debug("%s   %x", path, preferred_format);
	SDL_RWops *stream = vfs_open(path, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!stream) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool result = pixmap_load_stream(stream, dst, preferred_format);
	SDL_RWclose(stream);
	return result;
}

bool pixmap_check_filename(const char *path) {
	return (bool)pixmap_loader_for_filename(path);
}

char *pixmap_source_path(const char *prefix, const char *path) {
	char base_path[strlen(prefix) + strlen(path) + 1];
	strcpy(base_path, prefix);
	strcpy(base_path + strlen(prefix), path);

	if(pixmap_check_filename(base_path) && vfs_query(base_path).exists) {
		return strdup(base_path);
	}

	char *dot = strrchr(path, '.');

	if(dot) {
		*dot = 0;
	}

	for(PixmapLoader **loader = pixmap_loaders; *loader; ++loader) {
		PixmapLoader *l = *loader;
		for(const char **l_ext = l->filename_exts; *l_ext; ++l_ext) {
			char ext[strlen(*l_ext) + 2];
			ext[0] = '.';
			strcpy(ext + 1, *l_ext);

			char *p = try_path("", base_path, ext);

			if(p != NULL) {
				return p;
			}
		}
	}

	return NULL;
}

const char *pixmap_format_name(PixmapFormat fmt) {
	switch(fmt) {
		#define HANDLE(f) case f: return #f
		HANDLE(PIXMAP_FORMAT_R8);
		HANDLE(PIXMAP_FORMAT_R16);
		HANDLE(PIXMAP_FORMAT_R16F);
		HANDLE(PIXMAP_FORMAT_R32F);
		HANDLE(PIXMAP_FORMAT_RG8);
		HANDLE(PIXMAP_FORMAT_RG16);
		HANDLE(PIXMAP_FORMAT_RG16F);
		HANDLE(PIXMAP_FORMAT_RG32F);
		HANDLE(PIXMAP_FORMAT_RGB8);
		HANDLE(PIXMAP_FORMAT_RGB16);
		HANDLE(PIXMAP_FORMAT_RGB16F);
		HANDLE(PIXMAP_FORMAT_RGB32F);
		HANDLE(PIXMAP_FORMAT_RGBA8);
		HANDLE(PIXMAP_FORMAT_RGBA16);
		HANDLE(PIXMAP_FORMAT_RGBA16F);
		HANDLE(PIXMAP_FORMAT_RGBA32F);

		#define HANDLE_COMPRESSED(comp, layout, ...) HANDLE(PIXMAP_FORMAT_##comp);
		PIXMAP_COMPRESSION_FORMATS(HANDLE_COMPRESSED,)
		#undef HANDLE_COMPRESSED
		#undef HANDLE

		default: return NULL;
	}
}
