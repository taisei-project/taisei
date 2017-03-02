/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <SDL_mixer.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "audio.h"
#include "global.h"
#include "list.h"
#include "taisei_err.h"

#define AUDIO_FREQ 44100
#define AUDIO_FORMAT MIX_DEFAULT_FORMAT
#define AUDIO_CHANNELS 100

static bool mixer_loaded = false;

const char *mixer_audio_exts[] = { ".wav", ".ogg", ".mp3", ".mod", ".xm", ".s3m",
		".669", ".it", ".med", ".mid", ".flac", ".aiff", ".voc",
		NULL };

void audio_backend_init(void) {
	if(mixer_loaded) {
		return;
	}

	if(SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		warnx("audio_backend_init(): SDL_InitSubSystem() failed: %s.\n", SDL_GetError());
		return;
	}

	if(!Mix_Init(MIX_INIT_OGG | MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3)) {
		Mix_Quit(); // Try to shutdown mixer if it was partly initialized
		return;
	}

	if(Mix_OpenAudio(AUDIO_FREQ, AUDIO_FORMAT, 2, config_get_int(CONFIG_MIXER_CHUNKSIZE)) == -1) {
		warnx("audio_backend_init(): Mix_OpenAudio() failed: %s.\n", Mix_GetError());
		Mix_Quit();
		return;
	}

	int channels = Mix_AllocateChannels(AUDIO_CHANNELS);
	if(!channels) {
		warnx("audio_backend_init(): unable to allocate any channels.\n");
		Mix_CloseAudio();
		Mix_Quit();
		return;
	}

	if(channels < AUDIO_CHANNELS) {
		warnx("audio_backend_init(): allocated only %d of %d channels.\n", channels, AUDIO_CHANNELS);
	}

	mixer_loaded = true;

	audio_backend_set_sfx_volume(config_get_float(CONFIG_SFX_VOLUME));
	audio_backend_set_bgm_volume(config_get_float(CONFIG_BGM_VOLUME));

	int frequency = 0;
	uint16_t format = 0;

	Mix_QuerySpec(&frequency, &format, &channels);

	if(frequency != AUDIO_FREQ || format != AUDIO_FORMAT) {
		warnx(	"audio_backend_init(): mixer spec doesn't match our request, "
				"requested (freq=%i, fmt=%u), got (freq=%i, fmt=%u). "
				"Sound may be distorted.",
				AUDIO_FREQ, AUDIO_FORMAT, frequency, format);
	}

	printf("audio_backend_init(): audio subsystem initialized (SDL2_Mixer)\n");
}

void audio_backend_shutdown(void) {
	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	mixer_loaded = false;

	printf("audio_backend_shutdown(): audio subsystem uninitialized (SDL2_Mixer)\n");
}

bool audio_backend_initialized(void) {
	return mixer_loaded;
}

void audio_backend_set_sfx_volume(float gain) {
	if(mixer_loaded)
		Mix_Volume(-1, gain * MIX_MAX_VOLUME);
}

void audio_backend_set_bgm_volume(float gain) {
	if(mixer_loaded)
		Mix_VolumeMusic(gain * MIX_MAX_VOLUME);
}

char* audio_mixer_sound_path(const char *name) {
	for(const char **ext = mixer_audio_exts; *ext; ++ext) {
		char *p = strjoin(get_prefix(), SFX_PATH_PREFIX, name, *ext, NULL);
		struct stat statbuf;

		if(!stat(p, &statbuf)) {
			return p;
		}

		free(p);
	}

	return NULL;
}

bool audio_mixer_check_sound_path(const char *path, bool isbgm) {
	if(strstartswith(resource_util_filename(path), "bgm_") == isbgm) {
		return strendswith_any(path, mixer_audio_exts);
	}

	return false;
}

void audio_backend_music_stop(void) {
	if(mixer_loaded)
		Mix_HaltMusic();
}

bool audio_backend_music_is_paused(void) {
	return mixer_loaded && Mix_PausedMusic();
}

bool audio_backend_music_is_playing(void) {
	return mixer_loaded && Mix_PlayingMusic();
}

void audio_backend_music_resume(void) {
	if(mixer_loaded)
		Mix_ResumeMusic();
}

void audio_backend_music_pause(void) {
	if(mixer_loaded)
		Mix_PauseMusic();
}

bool audio_backend_music_play(void *impl) {
	if(!mixer_loaded)
		return false;

	bool result = (Mix_PlayMusic((Mix_Music*)impl, -1) != -1);

	if(!result) {
		warnx("audio_backend_music_play(): Mix_PlayMusic() failed: %s", Mix_GetError());
	}

	return result;
}

bool audio_backend_sound_play(void *impl) {
	if(!mixer_loaded)
		return false;

	bool result = (Mix_PlayChannel(-1, (Mix_Chunk*)impl, 0) != -1);

	if(!result) {
		warnx("audio_backend_sound_play(): Mix_PlayChannel() failed: %s", Mix_GetError());
	}

	return result;
}
