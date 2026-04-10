/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "fileformats.h"

#include "log.h"
#include "rwops/rwops_util.h"

static const uint8_t png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

static bool px_png_probe(SDL_IOStream *stream) {
	uint8_t magic[sizeof(png_magic)] = {};
	SDL_ReadIO(stream, magic, sizeof(magic));
	return !memcmp(magic, png_magic, sizeof(magic));
}

static bool px_png_load(
	SDL_IOStream *stream, Pixmap *pixmap, PixmapFormat preferred_format
) {
	auto surf = SDL_LoadPNG_IO(stream, false);

	if(!surf) {
		log_sdl_error(LOG_ERROR, "SDL_LoadPNG_IO");
		return false;
	}

	bool ok = pixmap_from_sdl_surface(surf, pixmap);

	if(!ok) {
		log_error("%s: Failed to convert SDL surface into a pixmap", iostream_get_name(stream));
	}

	SDL_DestroySurface(surf);
	return ok;
}

static bool px_png_save(
	SDL_IOStream *stream, const Pixmap *src_pixmap, const PixmapSaveOptions *base_opts
) {
	auto surf = pixmap_to_sdl_surface(src_pixmap);
	bool ok = SDL_SavePNG_IO(surf, stream, false);

	if(!ok) {
		log_sdl_error(LOG_ERROR, "SDL_SavePNG_IO");
	}

	SDL_DestroySurface(surf);
	return ok;
}

PixmapFileFormatHandler pixmap_fileformat_png = {
	.probe = px_png_probe,
	.load = px_png_load,
	.save = px_png_save,
	.filename_exts = (const char*[]) { "png", NULL },
	.name = "PNG",
};
