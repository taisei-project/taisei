/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <stdlib.h>
#include <SDL_mixer.h>

#include "resource.h"
#include "sfx.h"

char* audio_mixer_sound_path(const char *prefix, const char *name);
bool audio_mixer_check_sound_path(const char *path, bool isbgm);

char* music_path(const char *name) {
	return audio_mixer_sound_path(BGM_PATH_PREFIX, name);
}

bool check_music_path(const char *path) {
	return strstartswith(path, BGM_PATH_PREFIX) && audio_mixer_check_sound_path(path, true);
}

void* load_music_begin(const char *path, unsigned int flags) {
	SDL_RWops *rwops = vfs_open(path, VFS_MODE_READ);

	if(!rwops) {
		log_warn("VFS error: %s", vfs_get_error());
		return NULL;
	}

	Mix_Music *music = Mix_LoadMUS_RW(rwops, true);

	if(!music) {
		log_warn("Mix_LoadMUS() failed: %s", Mix_GetError());
		return NULL;
	}

	Music *mus = calloc(1, sizeof(Music));
	mus->impl = music;

	return mus;
}

void* load_music_end(void *opaque, const char *path, unsigned int flags) {
	return opaque;
}

void unload_music(void *vmus) {
	Music *mus = vmus;
	Mix_FreeMusic((Mix_Music*)mus->impl);
	free(mus);
}
