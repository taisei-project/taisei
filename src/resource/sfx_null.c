/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <stdlib.h>
#include <stdbool.h>

char* sound_path(const char *name) { return NULL; }
bool check_sound_path(const char *path) { return NULL; }
void* load_sound_begin(const char *path, unsigned int flags) { return NULL; }
void* load_sound_end(void *opaque, const char *path, unsigned int flags) { return NULL; }
void unload_sound(void *vmus) { }
