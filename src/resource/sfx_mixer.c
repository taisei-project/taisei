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
#include "sfx.h"
#include "audio_mixer.h"
#include "util.h"
#include "audio.h"

char* sound_path(const char *name) {
	return audio_mixer_sound_path(SFX_PATH_PREFIX, name, false);
}

bool check_sound_path(const char *path) {
	return strstartswith(path, SFX_PATH_PREFIX) && audio_mixer_check_sound_path(path, false);
}

void* load_sound_begin(const char *path, uint flags) {
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

	assert(strstartswith(path, SFX_PATH_PREFIX));
	char resname[strlen(path) - sizeof(SFX_PATH_PREFIX) + 2];
	strcpy(resname, path + sizeof(SFX_PATH_PREFIX) - 1);
	char *dot = strrchr(resname, '.');
	assert(dot != NULL);
	*dot = 0;

	Mix_VolumeChunk(sound, get_default_sfx_volume(resname));
	log_debug("%s volume: %i", resname, Mix_VolumeChunk(sound, -1));

	MixerInternalSound *isnd = calloc(1, sizeof(MixerInternalSound));
	isnd->ch = sound;
	isnd->loopchan = -1;
	isnd->playchan = -1;

	Sound *snd = calloc(1, sizeof(Sound));
	snd->impl = isnd;

	return snd;
}

void* load_sound_end(void *opaque, const char *path, uint flags) {
	return opaque;
}

void unload_sound(void *vsnd) {
	Sound *snd = vsnd;
	Mix_FreeChunk(((MixerInternalSound *)snd->impl)->ch);
	free(snd->impl);
	free(snd);
}
