/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "texture_loader.h"

// #define BASISU_DEBUG

#ifdef BASISU_DEBUG
	#undef BASISU_DEBUG
	#define BASISU_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#undef BASISU_DEBUG
	#define BASISU_DEBUG(...) ((void)0)
#endif

char *texture_loader_basisu_try_path(const char *basename);
bool texture_loader_basisu_check_path(const char *path);
void texture_loader_basisu(TextureLoadData *ld);
