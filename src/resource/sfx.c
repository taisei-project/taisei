/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "resource.h"

#include "audio/audio.h"
#include "sfx.h"
#include "sfxbgm_common.h"

static char *sound_path(const char *name) {
	return sfxbgm_make_path(SFX_PATH_PREFIX, name, false);
}

static bool check_sound_path(const char *path) {
	return sfxbgm_check_path(SFX_PATH_PREFIX, path, false);
}

static void load_sound(ResourceLoadState *st) {
	SFX *sfx = audio_sfx_load(st->name, st->path);

	if(UNLIKELY(!sfx)) {
		res_load_failed(st);
		return;
	}

	res_load_finished(st, sfx);
}

static void unload_sound(void *vsnd) {
	audio_sfx_destroy(vsnd);
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
