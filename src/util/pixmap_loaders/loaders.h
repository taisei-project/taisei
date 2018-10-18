/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>
#include "../pixmap.h"

typedef struct PixmapLoader {
	bool (*probe)(SDL_RWops *stream);
	bool (*load)(SDL_RWops *stream, Pixmap *pixmap);
	const char **filename_exts;
} PixmapLoader;

extern PixmapLoader pixmap_loader_png;
extern PixmapLoader pixmap_loader_webp;
