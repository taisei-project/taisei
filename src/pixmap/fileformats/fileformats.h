/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL.h>

#include "../pixmap.h"

typedef struct PixmapFileFormatHandler {
	bool (*probe)(SDL_IOStream *stream);
	bool (*load)(SDL_IOStream *stream, Pixmap *pixmap,
		     PixmapFormat preferred_format);
	bool (*save)(SDL_IOStream *stream, const Pixmap *pixmap,
		     const PixmapSaveOptions *opts);
	const char **filename_exts;
	const char *name;
} PixmapFileFormatHandler;

extern PixmapFileFormatHandler pixmap_fileformat_internal;
extern PixmapFileFormatHandler pixmap_fileformat_png;
extern PixmapFileFormatHandler pixmap_fileformat_webp;
