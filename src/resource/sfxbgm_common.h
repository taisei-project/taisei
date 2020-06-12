/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_sfxbgm_common_h
#define IGUARD_resource_sfxbgm_common_h

#include "taisei.h"

char *sfxbgm_make_path(const char *prefix, const char *name, bool isbgm, bool trymeta);
bool sfxbgm_check_path(const char *prefix, const char *path, bool isbgm);

#endif // IGUARD_resource_sfxbgm_common_h
