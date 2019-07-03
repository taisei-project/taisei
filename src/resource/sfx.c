/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "resource.h"
#include "sfx.h"
#include "audio/backend.h"
#include "sfxbgm_common.h"
#include "util.h"

static char* sound_path(const char *name) {
	return sfxbgm_make_path(SFX_PATH_PREFIX, name, false);
}

static bool check_sound_path(const char *path) {
	return sfxbgm_check_path(SFX_PATH_PREFIX, path, false);
}

static void* load_sound_begin(const char *path, uint flags) {
	SoundImpl *sound = _a_backend.funcs.sound_load(path);

	if(!sound) {
		return NULL;
	}

	assert(strstartswith(path, SFX_PATH_PREFIX));
	char resname[strlen(path) - sizeof(SFX_PATH_PREFIX) + 2];
	strcpy(resname, path + sizeof(SFX_PATH_PREFIX) - 1);
	char *dot = strrchr(resname, '.');
	assert(dot != NULL);
	*dot = 0;

	_a_backend.funcs.sound_set_volume(sound, get_default_sfx_volume(resname) / 128.0);

	Sound *snd = calloc(1, sizeof(Sound));
	snd->impl = sound;

	return snd;
}

static void* load_sound_end(void *opaque, const char *path, uint flags) {
	return opaque;
}

static void unload_sound(void *vsnd) {
	Sound *snd = vsnd;
	_a_backend.funcs.sound_unload(snd->impl);
	free(snd);
}

ResourceHandler sfx_res_handler = {
    .type = RES_SFX,
    .typename = "sfx",
    .subdir = SFX_PATH_PREFIX,

    .procs = {
        .find = sound_path,
        .check = check_sound_path,
        .begin_load = load_sound_begin,
        .end_load = load_sound_end,
        .unload = unload_sound,
    },
};
