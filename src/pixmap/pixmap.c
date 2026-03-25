/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "pixmap.h"

#include "fileformats/fileformats.h"
#include "log.h"
#include "util.h"
#include "util/io.h"
#include "vfs/public.h"

static PixmapFileFormatHandler *fileformat_handlers[] = {
	[PIXMAP_FILEFORMAT_INTERNAL] = &pixmap_fileformat_internal,
	[PIXMAP_FILEFORMAT_PNG] = &pixmap_fileformat_png,
	[PIXMAP_FILEFORMAT_WEBP] = &pixmap_fileformat_webp,
};

uint32_t pixmap_data_size(PixmapFormat format, uint32_t width, uint32_t height) {
	assert(width >= 1);
	assert(height >= 1);
	uint64_t pixel_size = PIXMAP_FORMAT_PIXEL_SIZE(format);
	assert(pixel_size >= 1);
	uint64_t s = (uint64_t)width * (uint64_t)height * pixel_size;
	assert(s <= PIXMAP_BUFFER_MAX_SIZE);
	return s;
}

void *pixmap_alloc_buffer(PixmapFormat format, uint32_t width, uint32_t height, uint32_t *out_bufsize) {
	uint32_t s = pixmap_data_size(format, width, height);

	if(out_bufsize) {
		*out_bufsize = s;
	}

	return mem_alloc(s);
}

void *pixmap_alloc_buffer_for_copy(const Pixmap *src, uint32_t *out_bufsize) {
	return pixmap_alloc_buffer(src->format, src->width, src->height, out_bufsize);
}

void *pixmap_alloc_buffer_for_conversion(const Pixmap *src, PixmapFormat format, uint32_t *out_bufsize) {
	return pixmap_alloc_buffer(format, src->width, src->height, out_bufsize);
}

static PixmapFileFormatHandler *pixmap_handler_for_filename(const char *file) {
	const char *ext = strrchr(file, '.');

	if(!ext || !*(++ext)) {
		return NULL;
	}

	for(int i = 0; i < ARRAY_SIZE(fileformat_handlers); ++i) {
		PixmapFileFormatHandler *h = NOT_NULL(fileformat_handlers[i]);
		for(const char **l_ext = h->filename_exts; *l_ext; ++l_ext) {
			if(!SDL_strcasecmp(*l_ext, ext)) {
				return h;
			}
		}
	}

	return NULL;
}

attr_returns_nonnull
static PixmapFileFormatHandler *pixmap_handler_for_fileformat(PixmapFileFormat ff) {
	uint idx = ff;
	assert(idx < PIXMAP_NUM_FILEFORMATS);
	return NOT_NULL(fileformat_handlers[idx]);
}

static PixmapFileFormatHandler *pixmap_probe_stream(SDL_IOStream *stream) {
	for(int i = 0; i < ARRAY_SIZE(fileformat_handlers); ++i) {
		PixmapFileFormatHandler *h = NOT_NULL(fileformat_handlers[i]);

		if(!h->probe) {
			continue;
		}

		bool match = h->probe(stream);
		if(SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET) < 0) {
			log_sdl_error(LOG_ERROR, "SDL_SeekIO");
			return NULL;
		}

		if(match) {
			return h;
		}
	}

	return NULL;
}

bool pixmap_load_stream(SDL_IOStream *stream, PixmapFileFormat filefmt,
			Pixmap *dst, PixmapFormat preferred_format) {
	PixmapFileFormatHandler *handler = NULL;

	if(filefmt == PIXMAP_FILEFORMAT_AUTO) {
		handler = pixmap_probe_stream(stream);
	} else {
		handler = pixmap_handler_for_fileformat(filefmt);
	}

	if(UNLIKELY(!handler)) {
		log_error("Image format not recognized");
		return false;
	}

	if(UNLIKELY(!handler->load)) {
		log_error("Can't load images in %s format", NOT_NULL(handler->name));
		return false;
	}

	return handler->load(stream, dst, preferred_format);
}

bool pixmap_load_file(const char *path, Pixmap *dst, PixmapFormat preferred_format) {
	log_debug("%s   %x", path, preferred_format);
	SDL_IOStream *stream = vfs_open(path, VFS_MODE_READ);

	if(!stream) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool result = pixmap_load_stream(stream, PIXMAP_FILEFORMAT_AUTO, dst, preferred_format);
	SDL_CloseIO(stream);
	return result;
}

static bool pixmap_save_stream_internal(SDL_IOStream *stream,
					const Pixmap *src,
					const PixmapSaveOptions *opts,
					PixmapFileFormatHandler *handler
) {
	if(UNLIKELY(!handler->save)) {
		log_error("Can't save images in %s format", NOT_NULL(handler->name));
		return false;
	}

	return handler->save(stream, src, opts);
}

bool pixmap_save_stream(SDL_IOStream *stream, const Pixmap *src,
			const PixmapSaveOptions *opts) {
	PixmapFileFormat fmt = opts->file_format;
	PixmapFileFormatHandler *h = pixmap_handler_for_fileformat(fmt);
	return pixmap_save_stream_internal(stream, src, opts, h);
}

bool pixmap_save_file(const char *path, const Pixmap *src, const PixmapSaveOptions *opts) {
	PixmapFileFormatHandler *h = NULL;
	PixmapFileFormat fmt = PIXMAP_FILEFORMAT_AUTO;

	if(opts) {
		assert(opts->struct_size >= sizeof(PixmapSaveOptions));
		fmt = opts->file_format;
	}

	if(fmt == PIXMAP_FILEFORMAT_AUTO) {
		h = pixmap_handler_for_filename(path);
	} else {
		h = pixmap_handler_for_fileformat(fmt);
	}

	if(UNLIKELY(!h)) {
		h = NOT_NULL(pixmap_handler_for_fileformat(PIXMAP_FILEFORMAT_INTERNAL));
		log_warn(
			"Couldn't determine file format from filename `%s`, assuming %s",
			path, NOT_NULL(h->name)
		);
	}

	SDL_IOStream *stream = vfs_open(path, VFS_MODE_WRITE);

	if(UNLIKELY(!stream)) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool result = pixmap_save_stream_internal(stream, src, opts, h);
	SDL_CloseIO(stream);
	return result;
}

bool pixmap_check_filename(const char *path) {
	return (bool)pixmap_handler_for_filename(path);
}

char *pixmap_source_path(const char *prefix, const char *path) {
	char base_path[strlen(prefix) + strlen(path) + 1];
	strcpy(base_path, prefix);
	strcpy(base_path + strlen(prefix), path);

	if(pixmap_check_filename(base_path) && vfs_query(base_path).exists) {
		return mem_strdup(base_path);
	}

	char *dot = strrchr(base_path, '.');

	if(dot) {
		*dot = 0;
	}

	for(int i = 0; i < ARRAY_SIZE(fileformat_handlers); ++i) {
		PixmapFileFormatHandler *h = NOT_NULL(fileformat_handlers[i]);

		for(const char **l_ext = h->filename_exts; *l_ext; ++l_ext) {
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

static SDL_PixelFormat sdl_pixfmt_conversion(SDL_PixelFormat fmt) {
	if(SDL_ISPIXELFORMAT_FLOAT(fmt)) {
		if(SDL_BYTESPERPIXEL(fmt) > 8) {
			if(SDL_ISPIXELFORMAT_ALPHA(fmt)) {
				return SDL_PIXELFORMAT_RGBA128_FLOAT;
			} else {
				return SDL_PIXELFORMAT_RGB96_FLOAT;
			}
		} else {
			if(SDL_ISPIXELFORMAT_ALPHA(fmt)) {
				return SDL_PIXELFORMAT_RGBA64_FLOAT;
			} else {
				return SDL_PIXELFORMAT_RGB48_FLOAT;
			}
		}
	}

	if(SDL_ISPIXELFORMAT_10BIT(fmt) || SDL_BYTESPERPIXEL(fmt) > 4) {
		if(SDL_ISPIXELFORMAT_ALPHA(fmt)) {
			return SDL_PIXELFORMAT_RGBA64;
		} else {
			return SDL_PIXELFORMAT_RGB48;
		}
	}

	if(SDL_ISPIXELFORMAT_ALPHA(fmt)) {
		return SDL_PIXELFORMAT_RGBA32;
	} else {
		return SDL_PIXELFORMAT_RGB24;
	}
}

static PixmapFormat sdl_pixfmt_to_taisei(SDL_PixelFormat fmt) {
	switch(fmt) {
		case SDL_PIXELFORMAT_RGB24:         return PIXMAP_FORMAT_RGB8;
		case SDL_PIXELFORMAT_RGB48:         return PIXMAP_FORMAT_RGB16;
		case SDL_PIXELFORMAT_RGB48_FLOAT:   return PIXMAP_FORMAT_RGB16F;
		case SDL_PIXELFORMAT_RGB96_FLOAT:   return PIXMAP_FORMAT_RGB32F;
		case SDL_PIXELFORMAT_RGBA32:        return PIXMAP_FORMAT_RGBA8;
		case SDL_PIXELFORMAT_RGBA64:        return PIXMAP_FORMAT_RGBA16;
		case SDL_PIXELFORMAT_RGBA64_FLOAT:  return PIXMAP_FORMAT_RGBA16;
		case SDL_PIXELFORMAT_RGBA128_FLOAT: return PIXMAP_FORMAT_RGBA32;
		default: return 0;
	}
}

static SDL_PixelFormat sdl_pixfmt_from_taisei(PixmapFormat fmt) {
	switch(fmt) {
		case PIXMAP_FORMAT_RGB8:    return SDL_PIXELFORMAT_RGB24;
		case PIXMAP_FORMAT_RGB16:   return SDL_PIXELFORMAT_RGB48;
		case PIXMAP_FORMAT_RGB16F:  return SDL_PIXELFORMAT_RGB48_FLOAT;
		case PIXMAP_FORMAT_RGB32F:  return SDL_PIXELFORMAT_RGB96_FLOAT;
		case PIXMAP_FORMAT_RGBA8:   return SDL_PIXELFORMAT_RGBA32;
		case PIXMAP_FORMAT_RGBA16:  return SDL_PIXELFORMAT_RGBA64;
		case PIXMAP_FORMAT_RGBA16F: return SDL_PIXELFORMAT_RGBA64_FLOAT;
		case PIXMAP_FORMAT_RGBA32F: return SDL_PIXELFORMAT_RGBA128_FLOAT;
		default:                    return SDL_PIXELFORMAT_UNKNOWN;
	}
}

bool pixmap_from_sdl_surface(SDL_Surface *surf, Pixmap *px) {
	assert(!px->data.untyped);

	SDL_PixelFormat target_sdl_fmt = sdl_pixfmt_conversion(surf->format);
	SDL_Surface *temp_surf = NULL;
	bool ok = false;

	if(target_sdl_fmt != surf->format) {
		temp_surf = SDL_ConvertSurface(surf, target_sdl_fmt);

		if(!temp_surf) {
			log_sdl_error(LOG_ERROR, "SDL_ConvertSurface");
			return false;
		}

		surf = temp_surf;
	}

	PixmapFormat px_fmt = sdl_pixfmt_to_taisei(surf->format);

	if(!px_fmt) {
		log_error("Can't map %s to a supported pixmap format", SDL_GetPixelFormatName(surf->format));
		goto fail;
	}

	px->width = surf->w;
	px->height = surf->h;
	px->format = px_fmt;
	px->data.untyped = pixmap_alloc_buffer_for_copy(px, &px->data_size);
	px->origin = PIXMAP_ORIGIN_TOPLEFT;

	assert(surf->pitch == surf->w * PIXMAP_FORMAT_PIXEL_SIZE(px->format));
	assert(!SDL_MUSTLOCK(surf));
	memcpy(px->data.untyped, surf->pixels, px->data_size);
	ok = true;

fail:
	if(temp_surf) {
		SDL_DestroySurface(temp_surf);
	}

	return ok;
}

SDL_Surface *pixmap_to_sdl_surface(const Pixmap *px) {
	SDL_PixelFormat fmt = sdl_pixfmt_from_taisei(px->format);

	if(fmt == SDL_PIXELFORMAT_UNKNOWN) {
		log_error("Can't map %s to an SDL pixel format", pixmap_format_name(px->format));
		return NULL;
	}

	SDL_Surface *surf = NULL;
	int pitch = PIXMAP_FORMAT_PIXEL_SIZE(px->format) * px->width;

	if(px->origin == PIXMAP_ORIGIN_BOTTOMLEFT) {
		surf = SDL_CreateSurface(px->width, px->height, fmt);
		if(!surf) {
			log_sdl_error(LOG_ERROR, "SDL_CreateSurface");
		} else {
			assert(!SDL_MUSTLOCK(surf));
			assert(surf->pitch == pitch);
			Pixmap flipped = *px;
			flipped.data.untyped = surf->pixels;
			pixmap_flip_y(px, &flipped);
		}
	} else {
		surf = SDL_CreateSurfaceFrom(px->width, px->height, fmt, px->data.untyped, pitch);
		if(!surf) {
			log_sdl_error(LOG_ERROR, "SDL_CreateSurfaceFrom");
		}
	}

	return surf;
}

SwizzleMask swizzle_canonize(SwizzleMask sw_in) {
	SwizzleMask sw_out;
	sw_out.r = sw_in.r ? sw_in.r : 'r';
	sw_out.g = sw_in.g ? sw_in.g : 'g';
	sw_out.b = sw_in.b ? sw_in.b : 'b';
	sw_out.a = sw_in.a ? sw_in.a : 'a';
	assert(swizzle_is_valid(sw_out));
	return sw_out;
}

static bool swizzle_component_is_valid(char c) {
	switch(c) {
		case 'r': case 'g': case 'b': case 'a': case '0': case '1':
			return true;

		default:
			return false;
	}
}

bool swizzle_is_valid(SwizzleMask sw) {
	for(int i = 0; i < 4; ++i) {
		if(!swizzle_component_is_valid(sw.rgba[i])) {
			return false;
		}
	}

	return true;
}

bool swizzle_is_significant(SwizzleMask sw, uint num_significant_channels) {
	assert(num_significant_channels <= 4);

	for(uint i = 0; i < num_significant_channels; ++i) {
		assert(swizzle_component_is_valid(sw.rgba[i]));
		if(sw.rgba[i] != "rgba"[i]) {
			return true;
		}
	}

	return false;
}
