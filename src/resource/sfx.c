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

static char *sound_path(const char *name) {
	return sfxbgm_make_path(RES_PATHPREFIX_SOUND, name, false);
}

static bool check_sound_path(const char *path) {
	return sfxbgm_check_path(RES_PATHPREFIX_SOUND, path, false);
}

static void *load_sound_begin(ResourceLoadInfo li) {
	SoundImpl *sound = _a_backend.funcs.sound_load(li.path);

	if(!sound) {
		return NULL;
	}

	_a_backend.funcs.sound_set_volume(sound, get_default_sfx_volume(li.name) / 128.0);

	Sound *snd = calloc(1, sizeof(Sound));
	snd->impl = sound;

	return snd;
}

static void unload_sound(void *vsnd) {
	Sound *snd = vsnd;
	_a_backend.funcs.sound_unload(snd->impl);
	free(snd);
}

ResourceHandler sfx_res_handler = {
	.type = RES_SOUND,

	.procs = {
		.find = sound_path,
		.check = check_sound_path,
		.begin_load = load_sound_begin,
		.unload = unload_sound,
	},
};
