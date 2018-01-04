/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdbool.h>

typedef struct Music {
    char *title;
    void *impl;
} Music;

char* bgm_path(const char *name);
bool check_bgm_path(const char *path);
void* load_bgm_begin(const char *path, unsigned int flags);
void* load_bgm_end(void *opaque, const char *path, unsigned int flags);
void unload_bgm(void *snd);

#define BGM_PATH_PREFIX "res/bgm/"
