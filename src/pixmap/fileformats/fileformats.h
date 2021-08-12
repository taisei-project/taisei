/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>

#include "../pixmap.h"

typedef struct PixmapFileFormatHandler {
	bool (*probe)(SDL_RWops *stream);
	bool (*load)(SDL_RWops *stream, Pixmap *pixmap, PixmapFormat preferred_format);
	bool (*save)(SDL_RWops *stream, const Pixmap *pixmap, const PixmapSaveOptions *opts);
	const char **filename_exts;
	const char *name;
} PixmapFileFormatHandler;

extern PixmapFileFormatHandler pixmap_fileformat_internal;
extern PixmapFileFormatHandler pixmap_fileformat_png;
extern PixmapFileFormatHandler pixmap_fileformat_webp;
