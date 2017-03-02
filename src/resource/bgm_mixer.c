/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <stdlib.h>
#include <SDL_mixer.h>

#include "resource.h"
#include "sfx.h"

char* audio_mixer_sound_path(const char *name);
bool audio_mixer_check_sound_path(const char *path, bool isbgm);

char* music_path(const char *name) {
	return audio_mixer_sound_path(name);
}

bool check_music_path(const char *path) {
	return audio_mixer_check_sound_path(path, true);
}

void* load_music(const char *path) {
	Mix_Music *music = Mix_LoadMUS(path);

	if(!music) {
		warnx("load_music(): Mix_LoadMUS() failed: %s", Mix_GetError());
		return NULL;
	}

	Music *mus = calloc(1, sizeof(Music));
	mus->impl = music;

	return music;
}

void unload_music(void *vmus) {
	Sound *mus = vmus;
	Mix_FreeMusic((Mix_Music*)mus->impl);
	free(mus);
}
