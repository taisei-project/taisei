/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "pixmap.h"
#include "util.h"

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
	int swizzle[4]
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

static void pixmap_copy_meta(const Pixmap *src, Pixmap *dst) {
	dst->format = src->format;
	dst->width = src->width;
	dst->height = src->height;
	dst->origin = src->origin;
}

void pixmap_copy(const Pixmap *src, Pixmap *dst) {
	assert(dst->data.untyped != NULL);
	assert(src->data_size > 0);
	assert(dst->data_size >= src->data_size);

	if(!pixmap_format_is_compressed(src->format)) {
		assert(src->data_size == pixmap_data_size(src->format, src->width, src->height));
	}

	pixmap_copy_meta(src, dst);
	memcpy(dst->data.untyped, src->data.untyped, src->data_size);
}

void pixmap_copy_alloc(const Pixmap *src, Pixmap *dst) {
	dst->data.untyped = pixmap_alloc_buffer_for_copy(src, &dst->data_size);
	pixmap_copy(src, dst);
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
		NULL
	);
}

static int swizzle_idx(char s) {
	switch(s) {
		case 'r': return 0;
		case 'g': return 1;
		case 'b': return 2;
		case 'a': return 3;
		case '0': return 4;
		case '1': return 5;
		default: UNREACHABLE;
	}
}

void pixmap_swizzle_inplace(Pixmap *px, SwizzleMask swizzle) {
	uint channels = pixmap_format_layout(px->format);
	swizzle = swizzle_canonize(swizzle);

	if(!swizzle_is_significant(swizzle, channels)) {
		return;
	}

	uint cvt_id = pixmap_format_depth(px->format) | (pixmap_format_is_float(px->format) * DEPTH_FLOAT_BIT);
	struct conversion_def *cv = find_conversion(cvt_id, cvt_id);

	cv->func(
		channels,
		channels,
		px->width * px->height,
		px->data.untyped,
		px->data.untyped,
		(int[]) {
			swizzle_idx(swizzle.r),
			swizzle_idx(swizzle.g),
			swizzle_idx(swizzle.b),
			swizzle_idx(swizzle.a),
		}
	);
}

void pixmap_convert_alloc(const Pixmap *src, Pixmap *dst, PixmapFormat format) {
	dst->data.untyped = pixmap_alloc_buffer_for_conversion(src, format, &dst->data_size);
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

	mem_free(src->data.untyped);
	*src = tmp;
}

void pixmap_flip_y(const Pixmap *src, Pixmap *dst) {
	assert(dst->data.untyped != NULL);
	pixmap_copy_meta(src, dst);

	size_t rows = src->height;
	size_t row_length = src->width * PIXMAP_FORMAT_PIXEL_SIZE(src->format);

	if(UNLIKELY(row_length == 0)) {
		return;
	}

	char *cdst = dst->data.untyped;
	const char *csrc = src->data.untyped;

	for(size_t row = 0, irow = rows - 1; row < rows; ++row, --irow) {
		memcpy(cdst + irow * row_length, csrc + row * row_length, row_length);
	}
}

void pixmap_flip_y_alloc(const Pixmap *src, Pixmap *dst) {
	dst->data.untyped = pixmap_alloc_buffer_for_copy(src, &dst->data_size);
	pixmap_flip_y(src, dst);
}

void pixmap_flip_y_inplace(Pixmap *src) {
	size_t rows = src->height;
	size_t row_length = src->width * PIXMAP_FORMAT_PIXEL_SIZE(src->format);

	if(UNLIKELY(row_length == 0)) {
		return;
	}

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
	dst->data.untyped = pixmap_alloc_buffer_for_copy(src, &dst->data_size);
	pixmap_flip_to_origin(src, dst, origin);
}

void pixmap_flip_to_origin_inplace(Pixmap *src, PixmapOrigin origin) {
	if(src->origin == origin) {
		return;
	}

	pixmap_flip_y_inplace(src);
	src->origin = origin;
}
