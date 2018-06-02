/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <SDL_mixer.h>

#include "audio.h"
#include "audio_mixer.h"
#include "global.h"
#include "list.h"

#define AUDIO_FREQ 44100
#define AUDIO_FORMAT MIX_DEFAULT_FORMAT
#define AUDIO_CHANNELS 100
#define UI_CHANNELS 4
#define UI_CHANNEL_GROUP 0
#define MAIN_CHANNEL_GROUP 1

static bool mixer_loaded = false;

static struct {
	uchar first;
	uchar num;
} groups[2];

static const char *mixer_audio_exts[] = { ".ogg", ".wav", NULL };

void audio_backend_init(void) {
	if(mixer_loaded) {
		return;
	}

	if(SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		log_warn("SDL_InitSubSystem() failed: %s", SDL_GetError());
		return;
	}

	if(Mix_OpenAudio(AUDIO_FREQ, AUDIO_FORMAT, 2, config_get_int(CONFIG_MIXER_CHUNKSIZE)) == -1) {
		log_warn("Mix_OpenAudio() failed: %s", Mix_GetError());
		Mix_Quit();
		return;
	}

	if(!(Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG)) {
		log_warn("Mix_Init() failed: %s", Mix_GetError());
		Mix_Quit(); // Try to shutdown mixer if it was partly initialized
		return;
	}

	int channels = Mix_AllocateChannels(AUDIO_CHANNELS);

	if(!channels) {
		log_warn("Unable to allocate any channels");
		Mix_CloseAudio();
		Mix_Quit();
		return;
	}

	if(channels < AUDIO_CHANNELS) {
		log_warn("Allocated only %d out of %d channels", channels, AUDIO_CHANNELS);
	}

	int wanted_ui_channels = UI_CHANNELS;

	if(wanted_ui_channels > channels / 2) {
		wanted_ui_channels = channels / 2;
		log_warn("Will not reserve more than %i channels for UI sounds (wanted %i channels)", wanted_ui_channels, UI_CHANNELS);
	}

	int ui_channels;

	if((ui_channels = Mix_GroupChannels(0, wanted_ui_channels - 1, UI_CHANNEL_GROUP)) != wanted_ui_channels) {
		log_warn("Assigned only %d out of %d channels to the UI group", ui_channels, wanted_ui_channels);
	}

	int wanted_main_channels = channels - ui_channels, main_channels;

	if((main_channels = Mix_GroupChannels(ui_channels, ui_channels + wanted_main_channels - 1, MAIN_CHANNEL_GROUP)) != wanted_main_channels) {
		log_warn("Assigned only %d out of %d channels to the main group", main_channels, wanted_main_channels);
	}

	int unused_channels = channels - ui_channels - main_channels;

	if(unused_channels) {
		log_warn("%i channels not used", unused_channels);
	}

	groups[UI_CHANNEL_GROUP].first = 0;
	groups[UI_CHANNEL_GROUP].num = ui_channels;
	groups[MAIN_CHANNEL_GROUP].first = ui_channels;
	groups[MAIN_CHANNEL_GROUP].num = main_channels;

	mixer_loaded = true;

	audio_backend_set_sfx_volume(config_get_float(CONFIG_SFX_VOLUME));
	audio_backend_set_bgm_volume(config_get_float(CONFIG_BGM_VOLUME));

	int frequency = 0;
	uint16_t format = 0;

	Mix_QuerySpec(&frequency, &format, &channels);

	if(frequency != AUDIO_FREQ || format != AUDIO_FORMAT) {
		log_warn(
			"Mixer spec doesn't match our request, "
			"requested (freq=%i, fmt=%u), got (freq=%i, fmt=%u). "
			"Sound may be distorted.",
			AUDIO_FREQ, AUDIO_FORMAT, frequency, format
		);
	}

	log_info("Audio subsystem initialized (SDL2_Mixer)");

	SDL_version v;
	const SDL_version *lv;

	SDL_MIXER_VERSION(&v);
	log_info("Compiled against SDL_mixer %u.%u.%u", v.major, v.minor, v.patch);

	lv = Mix_Linked_Version();
	log_info("Using SDL_mixer %u.%u.%u", lv->major, lv->minor, lv->patch);
}

void audio_backend_shutdown(void) {
	mixer_loaded = false;

	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	log_info("Audio subsystem uninitialized (SDL2_Mixer)");
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

char* audio_mixer_sound_path(const char *prefix, const char *name, bool isbgm) {
	char *p = NULL;

	if(isbgm && (p = try_path(prefix, name, ".bgm"))) {
		return p;
	}

	for(const char **ext = mixer_audio_exts; *ext; ++ext) {
		if((p = try_path(prefix, name, *ext))) {
			return p;
		}
	}

	return NULL;
}

bool audio_mixer_check_sound_path(const char *path, bool isbgm) {
	if(isbgm && strendswith(path, ".bgm")) {
		return true;
	}

	return strendswith_any(path, mixer_audio_exts);
}

static Mix_Music *next_loop;
static double next_loop_point;

static void mixer_music_finished(void) {
	// XXX: there may be a race condition in here
	// probably should protect next_loop with a mutex

	log_debug("%s stopped playing", next_loop_point ? "Loop" : "Intro");

	if(next_loop) {
		if(Mix_PlayMusic(next_loop, next_loop_point ? 0 : -1) == -1) {
			log_warn("Mix_PlayMusic() failed: %s", Mix_GetError());
		} else if(next_loop_point >= 0) {
			Mix_SetMusicPosition(next_loop_point);
		} else {
			next_loop = NULL;
		}
	}
}

void audio_backend_music_stop(void) {
	if(mixer_loaded) {
		Mix_HookMusicFinished(NULL);
		Mix_HaltMusic();
	}
}

void audio_backend_music_fade(double fadetime) {
	if(mixer_loaded) {
		Mix_HookMusicFinished(NULL);
		Mix_FadeOutMusic(floor(1000 * fadetime));
	}
}

bool audio_backend_music_is_paused(void) {
	return mixer_loaded && Mix_PausedMusic();
}

bool audio_backend_music_is_playing(void) {
	return mixer_loaded && Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT;
}

void audio_backend_music_resume(void) {
	if(mixer_loaded) {
		Mix_HookMusicFinished(mixer_music_finished);
		Mix_ResumeMusic();
	}
}

void audio_backend_music_pause(void) {
	if(mixer_loaded) {
		Mix_HookMusicFinished(NULL);
		Mix_PauseMusic();
	}
}

bool audio_backend_music_play(void *impl) {
	if(!mixer_loaded)
		return false;

	MixerInternalMusic *imus = impl;
	Mix_Music *mmus;
	int loops;

	Mix_HookMusicFinished(NULL);
	Mix_HaltMusic();

	if(imus->intro) {
		next_loop = imus->loop;
		next_loop_point = next_loop ? imus->loop_point : 0;
		mmus = imus->intro;
		loops = 0;
		Mix_HookMusicFinished(mixer_music_finished);
	} else {
		mmus = imus->loop;
		next_loop_point = imus->loop_point;

		if(next_loop_point >= 0) {
			loops = 0;
			next_loop = imus->loop;
			Mix_HookMusicFinished(mixer_music_finished);
		} else {
			loops = -1;
			Mix_HookMusicFinished(NULL);
		}
	}

	bool result = (Mix_PlayMusic(mmus, loops) != -1);

	if(!result) {
		log_warn("Mix_PlayMusic() failed: %s", Mix_GetError());
	}

	return result;
}

bool audio_backend_music_set_position(double pos) {
	if(!mixer_loaded) {
		return false;
	}

	// FIXME: BGMs that have intros are not handled correctly!

	Mix_RewindMusic();

	if(Mix_SetMusicPosition(pos)) {
		log_warn("Mix_SetMusicPosition() failed: %s", Mix_GetError());
		return false;
	}

	return true;
}

static int translate_group(AudioBackendSoundGroup group, int defmixgroup) {
	switch(group) {
		case SNDGROUP_MAIN: return MAIN_CHANNEL_GROUP;
		case SNDGROUP_UI:   return UI_CHANNEL_GROUP;
		default:            return defmixgroup;
	}
}

static int pick_channel(AudioBackendSoundGroup group, int defmixgroup) {
	int mixgroup = translate_group(group, MAIN_CHANNEL_GROUP);
	int channel = -1;

	if((channel = Mix_GroupAvailable(mixgroup)) < 0) {
		// all channels busy? try to override the oldest playing sound
		if((channel = Mix_GroupOldest(mixgroup)) < 0) {
			log_warn("No suitable channel available in group %i to play the sample on", mixgroup);
		}
	}

	return channel;
}

static int audio_backend_sound_play_on_channel(int chan, MixerInternalSound *isnd) {
	chan = Mix_PlayChannel(chan, isnd->ch, 0);

	if(chan < 0) {
		log_warn("Mix_PlayChannel() failed: %s", Mix_GetError());
		return false;
	}

	isnd->playchan = chan;
	return true;
}

bool audio_backend_sound_play(void *impl, AudioBackendSoundGroup group) {
	if(!mixer_loaded)
		return false;

	MixerInternalSound *isnd = impl;
	return audio_backend_sound_play_on_channel(pick_channel(group, MAIN_CHANNEL_GROUP), isnd);
}

bool audio_backend_sound_play_or_restart(void *impl, AudioBackendSoundGroup group) {
	if(!mixer_loaded) {
		return false;
	}

	MixerInternalSound *isnd = impl;
	int chan = isnd->playchan;

	if(chan < 0 || Mix_GetChunk(chan) != isnd->ch) {
		chan = pick_channel(group, MAIN_CHANNEL_GROUP);
	}

	return audio_backend_sound_play_on_channel(chan, isnd);
}

bool audio_backend_sound_loop(void *impl, AudioBackendSoundGroup group) {
	if(!mixer_loaded)
		return false;

	MixerInternalSound *snd = (MixerInternalSound *)impl;
	snd->loopchan = Mix_PlayChannel(pick_channel(group, MAIN_CHANNEL_GROUP), snd->ch, -1);

	if(snd->loopchan == -1) {
		log_warn("Mix_PlayChannel() failed: %s", Mix_GetError());
		return false;
	}

	return true;
}

bool audio_backend_sound_stop_loop(void *impl) {
	if(!mixer_loaded)
		return false;

	MixerInternalSound *snd = (MixerInternalSound *)impl;

	if(snd->loopchan == -1) {
		return false;
	}

	Mix_FadeOutChannel(snd->loopchan,LOOPFADEOUT);

	return true;

}

bool audio_backend_sound_pause_all(AudioBackendSoundGroup group) {
	if(!mixer_loaded) {
		return false;
	}

	int mixgroup = translate_group(group, -1);

	if(mixgroup == -1) {
		Mix_Pause(-1);
	} else {
		// why is there no Mix_PauseGroup?

		for(int i = groups[mixgroup].first; i < groups[mixgroup].first + groups[mixgroup].num; ++i) {
			Mix_Pause(i);
		}
	}

	return true;
}


bool audio_backend_sound_resume_all(AudioBackendSoundGroup group) {
	if(!mixer_loaded) {
		return false;
	}

	int mixgroup = translate_group(group, -1);

	if(mixgroup == -1) {
		Mix_Resume(-1);
	} else {
		// why is there no Mix_ResumeGroup?

		for(int i = groups[mixgroup].first; i < groups[mixgroup].first + groups[mixgroup].num; ++i) {
			Mix_Resume(i);
		}
	}

	return true;
}

bool audio_backend_sound_stop_all(AudioBackendSoundGroup group) {
	if(!mixer_loaded) {
		return false;
	}

	int mixgroup = translate_group(group, -1);

	if(mixgroup == -1) {
		Mix_HaltChannel(-1);
	} else {
		Mix_HaltGroup(mixgroup);
	}

	return true;
}
