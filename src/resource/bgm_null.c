/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <stdlib.h>
#include <stdbool.h>

char* music_path(const char *name) { return NULL; }
bool check_music_path(const char *path) { return NULL; }
void* load_music_begin(const char *path, unsigned int flags) { return NULL; }
void* load_music_end(void *opaque, const char *path, unsigned int flags) { return NULL; }
void unload_music(void *vmus) { }
