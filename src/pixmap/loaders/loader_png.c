/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "loaders.h"
#include "util.h"
#include "util/pngcruft.h"

static uchar png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

static bool px_png_probe(SDL_RWops *stream) {
	uchar magic[sizeof(png_magic)] = { 0 };
	SDL_RWread(stream, magic, sizeof(magic), 1);
	return !memcmp(magic, png_magic, sizeof(magic));
}

static inline PixmapLayout clrtype_to_layout(png_byte color_type) {
	switch(color_type) {
		case PNG_COLOR_TYPE_RGB:       return PIXMAP_LAYOUT_RGB;
		case PNG_COLOR_TYPE_RGB_ALPHA: return PIXMAP_LAYOUT_RGBA;
		case PNG_COLOR_TYPE_GRAY:      return PIXMAP_LAYOUT_R;
	}

	UNREACHABLE;
}

static bool px_png_load(SDL_RWops *stream, Pixmap *pixmap, PixmapFormat preferred_format) {
	png_structp png = NULL;
	png_infop png_info = NULL;
	const char *volatile error = NULL;

	pixmap->data.untyped = NULL;

	if(!(png = pngutil_create_read_struct())) {
		error = "png_create_read_struct() failed";
		goto done;
	}

	if(!(png_info = png_create_info_struct(png))) {
		error = "png_create_info_struct() failed";
		goto done;
	}

	if(setjmp(png_jmpbuf(png))) {
		error = "PNG error";
		goto done;
	}

	pngutil_init_rwops_read(png, stream);
	png_read_info(png, png_info);

	png_byte color_type = png_get_color_type(png, png_info);
	png_byte bit_depth = png_get_bit_depth(png, png_info);

	png_set_alpha_mode(png, PNG_ALPHA_PNG, PNG_DEFAULT_sRGB);

	/*
	 * Expand palette into RGB
	 * Expand grayscale to full 8 bits
	 * Expand transparency to full RGBA
	 */
	png_set_expand(png);

	// avoid unnecessary back-and-forth conversion
	bool keep_gray = (
		color_type == PNG_COLOR_TYPE_GRAY &&
		PIXMAP_FORMAT_LAYOUT(preferred_format) == PIXMAP_LAYOUT_R
	);

	if(!keep_gray) {
		png_set_gray_to_rgb(png);
	}

	if(PIXMAP_FORMAT_DEPTH(preferred_format) == 16) {
		png_set_expand_16(png);
	}

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	png_set_swap(png);
#endif

	int num_passes = png_set_interlace_handling(png);
	png_read_update_info(png, png_info);

	attr_unused png_byte channels = png_get_channels(png, png_info);
	color_type = png_get_color_type(png, png_info);
	bit_depth = png_get_bit_depth(png, png_info);

	assert(
		(color_type == PNG_COLOR_TYPE_RGB        && channels == 3) ||
		(color_type == PNG_COLOR_TYPE_RGB_ALPHA  && channels == 4) ||
		(color_type == PNG_COLOR_TYPE_GRAY       && channels == 1)
	);
	assert(bit_depth == 8 || bit_depth == 16);

	pixmap->width = png_get_image_width(png, png_info);
	pixmap->height = png_get_image_height(png, png_info);
	pixmap->format = PIXMAP_MAKE_FORMAT(
		clrtype_to_layout(color_type),
		bit_depth
	);

	// NOTE: We read the image upside down here to avoid needing to flip it for
	// the GL backend. This is just a slight optimization, not a hard dependency.
	pixmap->origin = PIXMAP_ORIGIN_BOTTOMLEFT;

	size_t pixel_size = PIXMAP_FORMAT_PIXEL_SIZE(pixmap->format);

	if(pixmap->height > PIXMAP_BUFFER_MAX_SIZE / (pixmap->width * pixel_size)) {
		error = "The image is too large";
		goto done;
	}

	png_bytep buffer = pixmap->data.untyped = pixmap_alloc_buffer_for_copy(pixmap, &pixmap->data_size);

	for(int pass = 0; pass < num_passes; ++pass) {
		for(int row = pixmap->height - 1; row >= 0; --row) {
			png_read_row(png, buffer + row * pixmap->width * pixel_size, NULL);
		}
	}

done:
	if(png != NULL) {
		png_destroy_read_struct(
			&png,
			png_info ? &png_info : NULL,
			NULL
		);
	}

	if(error) {
		log_error("Failed to load image: %s", error);
		free(pixmap->data.untyped);
		pixmap->data.untyped = NULL;
		return false;
	}

	return true;
}

PixmapLoader pixmap_loader_png = {
	.probe = px_png_probe,
	.load = px_png_load,
	.filename_exts = (const char*[]){ "png", NULL },
};
