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

int mixer_loaded = 0;

void unload_mixer_if_needed(void) {
	// mixer will be unloaded only if there are no audio AND no music enabled
	if(!tconfig.intval[NO_AUDIO] || !tconfig.intval[NO_MUSIC] || !mixer_loaded) return;

	Mix_CloseAudio();
	Mix_Quit();
	mixer_loaded = 0;
	printf("-- Unloaded SDL2_mixer\n");
}

int init_mixer_if_needed(void) {
	// mixer will not be loaded if there are no audio AND no music enabled
	if((tconfig.intval[NO_AUDIO] && tconfig.intval[NO_MUSIC]) || mixer_loaded) return 1;

	int formats_mask = Mix_Init(MIX_INIT_OGG | MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3);
	if(!formats_mask)
	{
		Mix_Quit(); // Try to shutdown mixer if it was partly initialized
		tconfig.intval[NO_AUDIO] = 1;
		tconfig.intval[NO_MUSIC] = 1;
		return 0;
	}

	printf("-- SDL2_mixer\n\tSupported formats:%s%s%s%s\n",
		(formats_mask & MIX_INIT_OGG  ? " OGG": ""),
		(formats_mask & MIX_INIT_FLAC ? " FLAC": ""),
		(formats_mask & MIX_INIT_MOD  ? " MOD": ""),
		(formats_mask & MIX_INIT_MP3  ? " MP3": "")
	);

	if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
	{
		warnx("Mix_OpenAudio(): %s.\n", Mix_GetError());
	}

	mixer_loaded = 1;
	return 1;
}

int init_sfx(void)
{
	if (tconfig.intval[NO_AUDIO]) return 1;
	if (!init_mixer_if_needed()) return 0;
	
	int channels = Mix_AllocateChannels(SNDCHAN_COUNT);
	if (!channels)
	{
		warnx("init_sfx(): unable to allocate any channels.\n");
		tconfig.intval[NO_AUDIO] = 1;
		unload_mixer_if_needed();
		return 0;
	}
	if (channels < SNDCHAN_COUNT) warnx("init_sfx(): allocated only %d of %d channels.\n", channels, SNDCHAN_COUNT);
	
	return 1;
}

void shutdown_sfx(void)
{
	if(resources.state & RS_SfxLoaded)
	{
		printf("-- freeing sounds\n");
		delete_sounds();
		resources.state &= ~RS_SfxLoaded;
	}
	tconfig.intval[NO_AUDIO] = 1;
	unload_mixer_if_needed();
}

Sound *load_sound_or_bgm(char *filename, Sound **dest, sound_type_t type) {
	Mix_Chunk *sound;
	Mix_Music *music;

	switch(type)
	{
		case ST_SOUND:
			sound = Mix_LoadWAV(filename);
			if (!sound)
				errx(-1,"load_sound_or_bgm():\n!- cannot load sound from '%s': %s", filename, Mix_GetError());
			
		case ST_MUSIC:
			music = Mix_LoadMUS(filename);
			if (!music)
				errx(-1,"load_sound_or_bgm():\n!- cannot load BGM from '%s': %s", filename, Mix_GetError());
			
		default:
			errx(-1,"load_sound_or_bgm():\n!- incorrect sound type specified");
	}

	Sound *snd = create_element((void **)dest, sizeof(Sound));

	snd->type = type;
	if (type == ST_SOUND) snd->sound = sound; else snd->music = music;
	snd->lastplayframe = 0;

	char *beg = strrchr(filename, '/'); // TODO: check portability of '/'
	char *end = strrchr(filename, '.');
	if (!beg || !end)
		errx(-1,"load_sound_or_bgm():\n!- incorrect filename format");

	++beg; // skip '/' between last path element and file name
	snd->name = malloc(end - beg + 1);
	if (!snd->name)
		errx(-1,"load_sound_or_bgm():\n!- failed to allocate memory for sound name (is it empty?)");

	memset(snd->name, 0, end-beg + 1);
	strncpy(snd->name, beg, end-beg);

	printf("-- loaded '%s' as '%s'\n", filename, snd->name);

	return snd;
}

Sound *load_sound(char *filename) {
	return load_sound_or_bgm(filename, &resources.sounds, ST_SOUND);
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

	if(Mix_PlayChannel(-1, snd->sound, 0) == -1)
		warnx("play_sound_p(): error playing sound: %s", Mix_GetError());
}

void set_sfx_volume(float gain)
{
	if(tconfig.intval[NO_AUDIO]) return;
	printf("SFX volume: %f\n", gain);
	Mix_Volume(-1, gain * MIX_MAX_VOLUME);
}

void delete_sound(void **snds, void *snd) {
	Sound *ssnd = (Sound *)snd;
	free(ssnd->name);
	if(ssnd->type == ST_MUSIC) Mix_FreeMusic(ssnd->music);
	else Mix_FreeChunk(ssnd->sound);
	delete_element(snds, snd);
}

void delete_sounds(void) {
	delete_all_elements((void **)&resources.sounds, delete_sound);
}
