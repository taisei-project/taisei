/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <stdlib.h>
#include <SDL_mixer.h>

#include "resource.h"
#include "bgm.h"
#include "audio_mixer.h"
#include "util.h"

char* bgm_path(const char *name) {
	return audio_mixer_sound_path(BGM_PATH_PREFIX, name, true);
}

bool check_bgm_path(const char *path) {
	return strstartswith(path, BGM_PATH_PREFIX) && audio_mixer_check_sound_path(path, true);
}

static Mix_Music* load_mix_music(const char *path) {
	if(!path) {
		return NULL;
	}

	SDL_RWops *rwops = vfs_open(path, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rwops) {
		log_warn("VFS error: %s", vfs_get_error());
		return NULL;
	}

	Mix_Music *music = Mix_LoadMUS_RW(rwops, true);

	if(!music) {
		log_warn("Mix_LoadMUS_RW() failed: %s", Mix_GetError());
		return NULL;
	}

	return music;
}

void* load_bgm_begin(const char *path, uint flags) {
	Music *mus = calloc(1, sizeof(Music));
	MixerInternalMusic *imus = calloc(1, sizeof(MixerInternalMusic));
	mus->impl = imus;

	if(strendswith(path, ".bgm")) {
		char *intro = NULL;
		char *loop = NULL;

		if(!parse_keyvalue_file_with_spec(path, (KVSpec[]) {
			{ "artist" }, // don’t print a warning because this field is supposed to be here
			{ "intro",      .out_str    = &intro            },
			{ "loop",       .out_str    = &loop             },
			{ "title",      .out_str    = &mus->title       },
			{ "loop_point", .out_double = &imus->loop_point },
			{ NULL }
		})) {
			log_warn("Failed to parse bgm config '%s'", path);
		} else {
			imus->intro = load_mix_music(intro);
			imus->loop = load_mix_music(loop);
		}

		free(intro);
		free(loop);
	} else {
		imus->loop = load_mix_music(path);
	}

	if(!imus->loop && !imus->intro) {
		assert(imus->intro == NULL);
		free(imus);
		free(mus->title);
		free(mus);
		mus = NULL;
		log_warn("Failed to load bgm '%s'", path);
	}

	return mus;
}

void* load_bgm_end(void *opaque, const char *path, uint flags) {
	return opaque;
}

void unload_bgm(void *vmus) {
	Music *mus = vmus;
	MixerInternalMusic *imus = mus->impl;
	Mix_FreeMusic(imus->intro);
	Mix_FreeMusic(imus->loop);
	free(mus->impl);
	free(mus->title);
	free(mus);
}
