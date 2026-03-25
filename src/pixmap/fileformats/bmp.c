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

static const uint8_t bmp_magic[] = { 'B', 'M' };

static bool px_bmp_probe(SDL_IOStream *stream) {
	uint8_t magic[sizeof(bmp_magic)] = {};
	SDL_ReadIO(stream, magic, sizeof(magic));
	return !memcmp(magic, bmp_magic, sizeof(magic));
}

static bool px_bmp_load(
	SDL_IOStream *stream, Pixmap *pixmap, PixmapFormat preferred_format
) {
	auto surf = SDL_LoadBMP_IO(stream, false);

	if(!surf) {
		log_sdl_error(LOG_ERROR, "SDL_LoadBMP_IO");
		return false;
	}

	bool ok = pixmap_from_sdl_surface(surf, pixmap);

	if(!ok) {
		log_error("%s: Failed to convert SDL surface into a pixmap", iostream_get_name(stream));
	}

	SDL_DestroySurface(surf);
	return ok;
}

static bool px_bmp_save(
	SDL_IOStream *stream, const Pixmap *src_pixmap, const PixmapSaveOptions *base_opts
) {
	auto surf = pixmap_to_sdl_surface(src_pixmap);
	bool ok = SDL_SaveBMP_IO(surf, stream, false);

	if(!ok) {
		log_sdl_error(LOG_ERROR, "SDL_SaveBMP_IO");
	}

	SDL_DestroySurface(surf);
	return ok;
}

PixmapFileFormatHandler pixmap_fileformat_bmp = {
	.probe = px_bmp_probe,
	.load = px_bmp_load,
	.save = px_bmp_save,
	.filename_exts = (const char*[]) { "bmp", NULL },
	.name = "BMP",
};
