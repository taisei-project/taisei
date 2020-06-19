/*
 * This software is licensed under the terms of the MIT License.
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

static char *sound_path(const char *name) {
	return sfxbgm_make_path(SFX_PATH_PREFIX, name, false);
}

static bool check_sound_path(const char *path) {
	return sfxbgm_check_path(SFX_PATH_PREFIX, path, false);
}

static void load_sound(ResourceLoadState *st) {
	SFXImpl *sound = _a_backend.funcs.sfx_load(st->path);

	if(!sound) {
		res_load_failed(st);
		return;
	}

	_a_backend.funcs.object.sfx.set_volume(sound, get_default_sfx_volume(st->name) / 128.0);

	SFX *snd = calloc(1, sizeof(SFX));
	snd->impl = sound;

	res_load_finished(st, snd);
}

static void unload_sound(void *vsnd) {
	SFX *snd = vsnd;
	_a_backend.funcs.sfx_unload(snd->impl);
	free(snd);
}

ResourceHandler sfx_res_handler = {
    .type = RES_SFX,
    .typename = "sfx",
    .subdir = SFX_PATH_PREFIX,

    .procs = {
        .find = sound_path,
        .check = check_sound_path,
        .load = load_sound,
        .unload = unload_sound,
    },
};
