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
	if(!config_get_int(CONFIG_NO_AUDIO) || !config_get_int(CONFIG_NO_MUSIC) || !mixer_loaded) return;

	Mix_CloseAudio();
	Mix_Quit();
	mixer_loaded = 0;
	printf("-- Unloaded SDL2_mixer\n");
}

int init_mixer_if_needed(void) {
	// mixer will not be loaded if there are no audio AND no music enabled
	if((config_get_int(CONFIG_NO_AUDIO) && config_get_int(CONFIG_NO_MUSIC)) || mixer_loaded) return 1;

	int formats_mask = Mix_Init(MIX_INIT_OGG | MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3);
	if(!formats_mask)
	{
		Mix_Quit(); // Try to shutdown mixer if it was partly initialized
		config_set_int(CONFIG_NO_AUDIO, 1);
		config_set_int(CONFIG_NO_MUSIC, 1);
		return 0;
	}

	printf("-- SDL2_mixer\n\tSupported formats:%s%s%s%s\n",
		(formats_mask & MIX_INIT_OGG  ? " OGG": ""),
		(formats_mask & MIX_INIT_FLAC ? " FLAC": ""),
		(formats_mask & MIX_INIT_MOD  ? " MOD": ""),
		(formats_mask & MIX_INIT_MP3  ? " MP3": "")
	);

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2,
		config_get_int(CONFIG_MIXER_CHUNKSIZE)) == -1)
	{
		warnx("Mix_OpenAudio(): %s.\n", Mix_GetError());
	}

	set_sfx_volume(config_get_float(CONFIG_SFX_VOLUME));
	set_bgm_volume(config_get_float(CONFIG_BGM_VOLUME));

	mixer_loaded = 1;
	return 1;
}

static void sfx_cfg_noaudio_callback(ConfigIndex idx, ConfigValue v) {
	config_set_int(idx, v.i);

	if(v.i) {
		shutdown_sfx();
		return;
	}

	if(!init_sfx()) {
		config_set_int(idx, true);
		return;
	}

	load_resources();
	set_sfx_volume(config_get_float(CONFIG_SFX_VOLUME));
}

static void sfx_cfg_volume_callback(ConfigIndex idx, ConfigValue v) {
	set_sfx_volume(config_set_float(idx, v.f));
}

int init_sfx(void)
{
	static bool callbacks_set_up = false;

	if(!callbacks_set_up) {
		config_set_callback(CONFIG_NO_AUDIO, sfx_cfg_noaudio_callback);
		config_set_callback(CONFIG_SFX_VOLUME, sfx_cfg_volume_callback);
		callbacks_set_up = true;
	}

	if (config_get_int(CONFIG_NO_AUDIO)) return 1;
	if (!init_mixer_if_needed()) return 0;

	int channels = Mix_AllocateChannels(SNDCHAN_COUNT);
	if (!channels)
	{
		warnx("init_sfx(): unable to allocate any channels.\n");
		config_set_int(CONFIG_NO_AUDIO, 1);
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
	unload_mixer_if_needed();
}

Sound *load_sound_or_bgm(char *filename, Sound **dest, sound_type_t type) {
	Mix_Chunk *sound = NULL;
	Mix_Music *music = NULL;

	switch(type)
	{
		case ST_SOUND:
			sound = Mix_LoadWAV(filename);
			if (!sound) {
				warnx("load_sound_or_bgm():\n!- cannot load sound from '%s': %s", filename, Mix_GetError());
				return NULL;
			}
			break;
		case ST_MUSIC:
			music = Mix_LoadMUS(filename);
			if (!music) {
				warnx("load_sound_or_bgm():\n!- cannot load BGM from '%s': %s", filename, Mix_GetError());
				return NULL;
			}
			break;
		default:
			errx(-1,"load_sound_or_bgm():\n!- incorrect sound type specified");
	}

	Sound *snd = create_element((void **)dest, sizeof(Sound));

	snd->type = type;
	if (sound)
		snd->sound = sound;
	else
		snd->music = music;
	snd->lastplayframe = 0;

	char *beg = strrchr(filename, '/'); // TODO: check portability of '/'
	char *end = strrchr(filename, '.');
	if (!beg || !end)
		errx(-1,"load_sound_or_bgm():\n!- incorrect filename format");

	++beg; // skip '/' between last path element and file name
	int sz = end - beg + 1;
	snd->name = malloc(sz);
	if (!snd->name)
		errx(-1,"load_sound_or_bgm():\n!- failed to allocate memory for sound name (is it empty?)");
	strlcpy(snd->name, beg, sz);

	printf("-- loaded '%s' as %s '%s'\n", filename, ((type==ST_SOUND) ? "SFX" : "BGM"), snd->name);

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
	if(config_get_int(CONFIG_NO_AUDIO) || global.frameskip) return;

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
	if(config_get_int(CONFIG_NO_AUDIO)) return;
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
