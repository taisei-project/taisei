/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_rwops_rwops_zipfile_h
#define IGUARD_rwops_rwops_zipfile_h

#include "taisei.h"

#include <SDL.h>
#include <zip.h>

SDL_RWops* SDL_RWFromZipFile(zip_file_t *zipfile, bool autoclose);

#endif // IGUARD_rwops_rwops_zipfile_h
