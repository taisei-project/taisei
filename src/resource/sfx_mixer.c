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
#include "audio_mixer.h"

char* sound_path(const char *name) {
	return audio_mixer_sound_path(SFX_PATH_PREFIX, name, true);
}

bool check_sound_path(const char *path) {
	return strstartswith(path, SFX_PATH_PREFIX) && audio_mixer_check_sound_path(path, false);
}

void* load_sound_begin(const char *path, unsigned int flags) {
	SDL_RWops *rwops = vfs_open(path, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rwops) {
		log_warn("VFS error: %s", vfs_get_error());
		return NULL;
	}

	Mix_Chunk *sound = Mix_LoadWAV_RW(rwops, true);

	if(!sound) {
		log_warn("Mix_LoadWAV_RW() failed: %s", Mix_GetError());
		return NULL;
	}

	Sound *snd = calloc(1, sizeof(Sound));
	snd->impl = calloc(1,sizeof(MixerInternalSound));
	((MixerInternalSound*)snd->impl)->ch = sound;
	((MixerInternalSound*)snd->impl)->loopchan = -1;
	return snd;
}

void* load_sound_end(void *opaque, const char *path, unsigned int flags) {
	return opaque;
}

void unload_sound(void *vsnd) {
	Sound *snd = vsnd;
	Mix_FreeChunk(((MixerInternalSound *)snd->impl)->ch);
	free(snd->impl);
	free(snd);
}
