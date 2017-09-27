/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include <stdbool.h>

typedef struct Music {
    // can put some meta properties in here
    void *impl;
} Music;

char* music_path(const char *name);
bool check_music_path(const char *path);
void* load_music_begin(const char *path, unsigned int flags);
void* load_music_end(void *opaque, const char *path, unsigned int flags);
void unload_music(void *snd);

#define BGM_PATH_PREFIX "res/bgm/"


