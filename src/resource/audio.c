/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "audio.h"
#include "resource.h"
#include "global.h"
#include "list.h"
#include "taisei_err.h"

#ifdef OGG_SUPPORT_ENABLED
#include <vorbis/vorbisfile.h>
#include "ogg.h"
#endif

int alut_loaded = 0;

int warn_alut_error(const char *when) {
	ALenum error = alutGetError();
	if (error == ALUT_ERROR_NO_ERROR) return 0;
	warnx("ALUT error %d while %s: %s", error, when, alutGetErrorString(error));
	return 1;
}

void unload_alut_if_needed() {
	// ALUT will be unloaded only if there are no audio AND no music enabled
	if(!tconfig.intval[NO_AUDIO] || !tconfig.intval[NO_MUSIC] || !alut_loaded) return;

	warn_alut_error("preparing to shutdown");
	alutExit();
	warn_alut_error("shutting down");
	alut_loaded = 0;
	printf("-- Unloaded ALUT\n");
}

int init_alut_if_needed(int *argc, char *argv[]) {
	// ALUT will not be loaded if there are no audio AND no music enabled
	if((tconfig.intval[NO_AUDIO] && tconfig.intval[NO_MUSIC]) || alut_loaded) return 1;

	if(!alutInit(argc, argv))
	{
		warn_alut_error("initializing");
		alutExit(); // Try to shutdown ALUT if it was partly initialized
		warn_alut_error("shutting down");
		tconfig.intval[NO_AUDIO] = 1;
		tconfig.intval[NO_MUSIC] = 1;
		return 0;
	}

	printf("-- ALUT\n");

	alut_loaded = 1;
	return 1;
}

int init_sfx(int *argc, char *argv[])
{
	if (tconfig.intval[NO_AUDIO]) return 1;
	if (!init_alut_if_needed(argc, argv)) return 0;

	alGenSources(SNDSRC_COUNT, resources.sndsrc);
	if(warn_alut_error("creating sfx sources"))
	{
		tconfig.intval[NO_AUDIO] = 1;
		unload_alut_if_needed();
		return 0;
	}
	return 1;
}

void shutdown_sfx(void)
{
	alDeleteSources(SNDSRC_COUNT, resources.sndsrc);
	warn_alut_error("deleting sfx sources");
	if(resources.state & RS_SfxLoaded)
	{
		printf("-- freeing sounds\n");
		delete_sounds();
		resources.state &= ~RS_SfxLoaded;
	}
	tconfig.intval[NO_AUDIO] = 1;
	unload_alut_if_needed();
}

Sound *load_sound_or_bgm(char *filename, Sound **dest, const char *res_directory, const char *type) {
	ALuint sound = 0;

#ifdef OGG_SUPPORT_ENABLED
	// Try to load ogg file
	if(strcmp(type, "ogg") == 0)
	{
		ALenum format;
		char *buffer;
		ALsizei size;
		ALsizei freq;

		int ogg_err;
		if((ogg_err = load_ogg(filename, &format, &buffer, &size, &freq)) != 0)
			errx(-1,"load_sound_or_bgm():\n!- cannot load '%s' through load_ogg: error %d", filename, ogg_err);

		alGenBuffers(1, &sound);
		warn_alut_error("generating audio buffer");

		alBufferData(sound, format, buffer, size, freq);
		warn_alut_error("filling buffer with data");

		free(buffer);
		if(!sound)
			errx(-1,"load_sound_or_bgm():\n!- cannot forward loaded '%s' to alut: %s", filename, alutGetErrorString(alutGetError()));
	}
#endif

	// Fallback to standard ALUT wrapper
	if(!sound)
	{
		sound = alutCreateBufferFromFile(filename);
		warn_alut_error("creating buffer from .wav file");
	}
	if(!sound)
		errx(-1,"load_sound_or_bgm():\n!- cannot load '%s' through alut: %s", filename, alutGetErrorString(alutGetError()));

	Sound *snd = create_element((void **)dest, sizeof(Sound));

	snd->alsnd = sound;
	snd->lastplayframe = 0;

	// res_directory should have trailing slash
	char *beg = strstr(filename, res_directory) + strlen(res_directory);
	char *end = strrchr(filename, '.');

	snd->name = malloc(end - beg + 1);
	memset(snd->name, 0, end-beg + 1);
	strncpy(snd->name, beg, end-beg);

	printf("-- loaded '%s' as '%s', type %s\n", filename, snd->name, type);

	return snd;
}

Sound *load_sound(char *filename, const char *type) {
	return load_sound_or_bgm(filename, &resources.sounds, "sfx/", type);
}

Sound *get_snd(Sound *source, char *name) {
	Sound *s, *res = NULL;
	for(s = source; s; s = s->next) {
		if(strcmp(s->name, name) == 0)
			res = s;
	}

	if(res == NULL)
		warnx("get_snd():\n!- cannot find sound '%s'", name);

	return res;
}

void play_sound_p(char *name, int unconditional)
{
	if(tconfig.intval[NO_AUDIO] || global.frameskip) return;

	Sound *snd = get_snd(resources.sounds, name);
	if (snd == NULL) return;

	if (!unconditional)
	{
		if (snd->lastplayframe == global.frames) return;
		snd->lastplayframe = global.frames;
	}

	ALuint i,res = -1;
	ALint play;
	for(i = 0; i < SNDSRC_COUNT; i++) {
		alGetSourcei(resources.sndsrc[i],AL_SOURCE_STATE,&play);
		warn_alut_error("checking state of sfx source");
		if(play != AL_PLAYING) {
			res = i;
			break;
		}
	}

	if(res != -1) {
		alSourcei(resources.sndsrc[res],AL_BUFFER, snd->alsnd);
		warn_alut_error("changing buffer of sfx source");
		alSourcePlay(resources.sndsrc[res]);
		warn_alut_error("starting playback of sfx source");
	} else {
		warnx("play_sound_p():\n!- not enough sources");
	}
}

void set_sfx_volume(float gain)
{
	if(tconfig.intval[NO_AUDIO]) return;
	printf("SFX volume: %f\n", gain);
	int i;
	for(i = 0; i < SNDSRC_COUNT; i++) {
		alSourcef(resources.sndsrc[i],AL_GAIN, gain);
		warn_alut_error("changing gain of sfx source");
	}
}

void delete_sound(void **snds, void *snd) {
	free(((Sound *)snd)->name);
	alDeleteBuffers(1, &((Sound *)snd)->alsnd);
	warn_alut_error("deleting source buffer");
	delete_element(snds, snd);
}

void delete_sounds(void) {
	delete_all_elements((void **)&resources.sounds, delete_sound);
}
