/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"

typedef enum {
	LS_OFF,
	LS_LOOPING,
	LS_FADEOUT,
} LoopState;

typedef struct Sound {
	int lastplayframe;
	LoopState islooping;
	void *impl;
} Sound;

char* sound_path(const char *name);
bool check_sound_path(const char *path);
void* load_sound_begin(const char *path, uint flags);
void* load_sound_end(void *opaque, const char *path, uint flags);
void unload_sound(void *snd);

extern ResourceHandler sfx_res_handler;

#define SFX_PATH_PREFIX "res/sfx/"
