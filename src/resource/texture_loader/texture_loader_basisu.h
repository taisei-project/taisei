/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_resource_texture_loader_texture_loader_basisu_h
#define IGUARD_resource_texture_loader_texture_loader_basisu_h

#include "taisei.h"

#include "texture_loader.h"

char *texture_loader_basisu_try_path(const char *basename);
bool texture_loader_basisu_check_path(const char *path);
void texture_loader_basisu(TextureLoadData *ld);

#endif // IGUARD_resource_texture_loader_texture_loader_basisu_h
