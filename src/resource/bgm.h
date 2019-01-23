/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_resource_bgm_h
#define IGUARD_resource_bgm_h

#include "taisei.h"

#include "resource.h"

typedef struct Music {
	char *title;
	void *impl;
} Music;

char* bgm_path(const char *name);
bool check_bgm_path(const char *path);
void* load_bgm_begin(const char *path, uint flags);
void* load_bgm_end(void *opaque, const char *path, uint flags);
void unload_bgm(void *snd);

extern ResourceHandler bgm_res_handler;

#define BGM_PATH_PREFIX "res/bgm/"

#endif // IGUARD_resource_bgm_h
