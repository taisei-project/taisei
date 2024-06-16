/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
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
	char *ext = strrchr(file, '.');

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

static PixmapFileFormatHandler *pixmap_probe_stream(SDL_RWops *stream) {
	for(int i = 0; i < ARRAY_SIZE(fileformat_handlers); ++i) {
		PixmapFileFormatHandler *h = NOT_NULL(fileformat_handlers[i]);

		if(!h->probe) {
			continue;
		}

		bool match = h->probe(stream);
		if(SDL_RWseek(stream, 0, RW_SEEK_SET) < 0) {
			log_sdl_error(LOG_ERROR, "SDL_RWseek");
			return NULL;
		}

		if(match) {
			return h;
		}
	}

	return NULL;
}

bool pixmap_load_stream(SDL_RWops *stream, PixmapFileFormat filefmt, Pixmap *dst, PixmapFormat preferred_format) {
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
	SDL_RWops *stream = vfs_open(path, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!stream) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool result = pixmap_load_stream(stream, PIXMAP_FILEFORMAT_AUTO, dst, preferred_format);
	SDL_RWclose(stream);
	return result;
}

static bool pixmap_save_stream_internal(
	SDL_RWops *stream, const Pixmap *src, const PixmapSaveOptions *opts,
	PixmapFileFormatHandler *handler
) {
	if(UNLIKELY(!handler->save)) {
		log_error("Can't save images in %s format", NOT_NULL(handler->name));
		return false;
	}

	return handler->save(stream, src, opts);
}

bool pixmap_save_stream(SDL_RWops *stream, const Pixmap *src, const PixmapSaveOptions *opts) {
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

	SDL_RWops *stream = vfs_open(path, VFS_MODE_WRITE);

	if(UNLIKELY(!stream)) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool result = pixmap_save_stream_internal(stream, src, opts, h);
	SDL_RWclose(stream);
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
		return strdup(base_path);
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
