/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "fileformats.h"
#include "util.h"
#include "util/pngcruft.h"

static const uint8_t png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

static bool px_png_probe(SDL_RWops *stream) {
	uint8_t magic[sizeof(png_magic)] = { 0 };
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
	png_byte bit_depth = png_get_bit_depth(png, png_info);

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

	size_t pixel_size = pixmap_format_pixel_size(pixmap->format);

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

static void px_png_save_apply_conversions(
	const Pixmap *src_pixmap, Pixmap *dst_pixmap
) {
	PixmapLayout fmt_layout = pixmap_format_layout(src_pixmap->format);
	PixmapLayout fmt_depth = pixmap_format_layout(src_pixmap->format);

	if(fmt_depth > 16) {
		fmt_depth = 16;
	} else if(fmt_depth < 16) {
		fmt_depth = 8;
	}

	if(fmt_layout == PIXMAP_LAYOUT_RG) {
		fmt_layout = PIXMAP_LAYOUT_RGB;
	}

	PixmapFormat fmt = PIXMAP_MAKE_FORMAT(fmt_layout, fmt_depth);

	if(fmt == src_pixmap->format) {
		*dst_pixmap = *src_pixmap;
	} else {
		pixmap_convert_alloc(src_pixmap, dst_pixmap, fmt);
	}
}

static bool px_png_save(
	SDL_RWops *stream, const Pixmap *src_pixmap, const PixmapSaveOptions *base_opts
) {
	if(
		pixmap_format_is_compressed(src_pixmap->format) ||
		pixmap_format_is_float(src_pixmap->format)
	) {
		log_error("Can't write %s image to PNG", pixmap_format_name(src_pixmap->format));
		return false;
	}

	const PixmapPNGSaveOptions *opts, default_opts = PIXMAP_DEFAULT_PNG_SAVE_OPTIONS;
	opts = &default_opts;

	if(
		base_opts &&
		base_opts->file_format == PIXMAP_FILEFORMAT_PNG &&
		base_opts->struct_size > sizeof(*base_opts)
	) {
		assert(base_opts->struct_size == sizeof(*opts));
		opts = UNION_CAST(const PixmapSaveOptions*, const PixmapPNGSaveOptions*, base_opts);
	}

	const char *error = NULL;
	png_structp png;
	png_infop info_ptr;

	Pixmap px = { 0 };

	png = pngutil_create_write_struct();

	if(png == NULL) {
		error = "pngutil_create_write_struct() failed";
		goto done;
	}

	info_ptr = png_create_info_struct(png);

	if(info_ptr == NULL) {
		error = "png_create_info_struct() failed";
		goto done;
	}

	if(setjmp(png_jmpbuf(png))) {
		error = "PNG error";
		goto done;
	}

	px_png_save_apply_conversions(src_pixmap, &px);

	size_t pixel_size = pixmap_format_pixel_size(px.format);

	uint png_color;
	switch(pixmap_format_layout(px.format)) {
		case PIXMAP_LAYOUT_R:    png_color = PNG_COLOR_TYPE_GRAY; break;
		case PIXMAP_LAYOUT_RGB:  png_color = PNG_COLOR_TYPE_RGB;  break;
		case PIXMAP_LAYOUT_RGBA: png_color = PNG_COLOR_TYPE_RGBA; break;
		default: UNREACHABLE;
	}

	pngutil_init_rwops_write(png, stream);

	png_set_IHDR(
		png, info_ptr,
		px.width, px.height, pixmap_format_depth(px.format),
		png_color,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);

	if(opts->zlib_compression_level >= 0) {
		png_set_compression_level(png, opts->zlib_compression_level);
	}

	png_write_info(png, info_ptr);

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	if(pixmap_format_depth(px.format) > 8) {
		png_set_swap(png);
	}
#endif

	if(px.origin == PIXMAP_ORIGIN_BOTTOMLEFT) {
		for(int row = px.height - 1; row >= 0; --row) {
			png_write_row(png, px.data.r8->values + row * px.width * pixel_size);
		}
	} else {
		for(int row = 0; row < px.height; ++row) {
			png_write_row(png, px.data.r8->values + row * px.width * pixel_size);
		}
	}

	png_write_end(png, info_ptr);

done:
	if(error) {
		log_error("%s", error);
	}

	if(px.data.untyped != src_pixmap->data.untyped) {
		free(px.data.untyped);
	}

	if(png != NULL) {
		png_destroy_write_struct(&png, info_ptr ? &info_ptr : NULL);
	}

	return !error;
}

PixmapFileFormatHandler pixmap_fileformat_png = {
	.probe = px_png_probe,
	.load = px_png_load,
	.save = px_png_save,
	.filename_exts = (const char*[]) { "png", NULL },
	.name = "PNG",
};
