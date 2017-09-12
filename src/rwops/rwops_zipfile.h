/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef RWZIP_H
#define RWZIP_H

#include <SDL.h>
#include <zip.h>
#include <stdbool.h>

SDL_RWops* SDL_RWFromZipFile(zip_file_t *zipfile, bool autoclose);

#endif
