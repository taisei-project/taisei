/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef BGM_H
#define BGM_H

#include <stdbool.h>

typedef struct Music {
    // can put some meta properties in here
    void *impl;
} Music;

char* music_path(const char *name);
bool check_music_path(const char *path);
void* load_music(const char *path);
void unload_music(void *snd);

#define BGM_PATH_PREFIX "bgm/"

#endif
