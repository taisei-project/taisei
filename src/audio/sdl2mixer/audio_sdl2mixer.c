/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <SDL_mixer.h>

#include "../backend.h"
#include "global.h"
#include "list.h"

#define B (_a_backend.funcs)

#define AUDIO_FREQ 44100
#define AUDIO_FORMAT MIX_DEFAULT_FORMAT
#define AUDIO_CHANNELS 100
#define UI_CHANNELS 4
#define UI_CHANNEL_GROUP 0
#define MAIN_CHANNEL_GROUP 1

// I needed to add this for supporting loop sounds since Mixer doesn’t remember
// what channel a sound is playing on.  - laochailan

struct SoundImpl {
	Mix_Chunk *ch;
	int loopchan; // channel the sound may be looping on. -1 if not looping
	int playchan; // channel the sound was last played on (looping does NOT set this). -1 if never played
};

struct MusicImpl {
	Mix_Music *loop;
	double loop_point;
};

static struct {
	uchar first;
	uchar num;
} groups[2];

static Mix_Music *next_loop;
static double next_loop_point;

static bool audio_sdl2mixer_init(void) {
	if(SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		log_sdl_error(LOG_ERROR, "SDL_InitSubSystem");
		return false;
	}

	uint num_drivers = SDL_GetNumAudioDrivers();
	void *buf;
	SDL_RWops *out = SDL_RWAutoBuffer(&buf, 256);

	SDL_RWprintf(out, "Available audio drivers:");

	for(uint i = 0; i < num_drivers; ++i) {
		SDL_RWprintf(out, " %s", SDL_GetAudioDriver(i));
	}

	SDL_WriteU8(out, 0);
	log_info("%s", (char*)buf);
	SDL_RWclose(out);

	if(Mix_OpenAudio(AUDIO_FREQ, AUDIO_FORMAT, 2, config_get_int(CONFIG_MIXER_CHUNKSIZE)) == -1) {
		log_error("Mix_OpenAudio() failed: %s", Mix_GetError());
		Mix_Quit();
		return false;
	}

	if(!(Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG)) {
		log_error("Mix_Init() failed: %s", Mix_GetError());
		Mix_Quit();
		return false;
	}

	log_info("Using driver '%s'", SDL_GetCurrentAudioDriver());

	int channels = Mix_AllocateChannels(AUDIO_CHANNELS);

	if(!channels) {
		log_error("Unable to allocate any channels");
		Mix_CloseAudio();
		Mix_Quit();
		return false;
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

	B.sound_set_global_volume(config_get_float(CONFIG_SFX_VOLUME));
	B.music_set_global_volume(config_get_float(CONFIG_BGM_VOLUME));

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
	return true;
}

static bool audio_sdl2mixer_shutdown(void) {
	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	log_info("Audio subsystem deinitialized (SDL2_Mixer)");
	return true;
}

static bool audio_sdl2mixer_output_works(void) {
	return true;
}

static inline int gain_to_mixvol(double gain) {
	return iclamp(gain * SDL_MIX_MAXVOLUME, 0, SDL_MIX_MAXVOLUME);
}

static bool audio_sdl2mixer_sound_set_global_volume(double gain) {
	Mix_Volume(-1, gain_to_mixvol(gain));
	return true;
}

static bool audio_sdl2mixer_music_set_global_volume(double gain) {
	Mix_VolumeMusic(gain * gain_to_mixvol(gain));
	return true;
}

static void mixer_music_finished(void) {
	// XXX: there may be a race condition in here
	// probably should protect next_loop with a mutex

	log_debug("%s stopped playing", next_loop_point ? "Loop" : "Intro");

	if(next_loop) {
		if(Mix_PlayMusic(next_loop, next_loop_point ? 0 : -1) == -1) {
			log_error("Mix_PlayMusic() failed: %s", Mix_GetError());
		} else if(next_loop_point >= 0) {
			Mix_SetMusicPosition(next_loop_point);
		} else {
			next_loop = NULL;
		}
	}
}

static bool audio_sdl2mixer_music_stop(void) {
	Mix_HookMusicFinished(NULL);
	Mix_HaltMusic();
	return true;
}

static bool audio_sdl2mixer_music_fade(double fadetime) {
	Mix_HookMusicFinished(NULL);
	Mix_FadeOutMusic(floor(1000 * fadetime));
	return true;
}

static bool audio_sdl2mixer_music_is_paused(void) {
	return Mix_PausedMusic();
}

static bool audio_sdl2mixer_music_is_playing(void) {
	return Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT;
}

static bool audio_sdl2mixer_music_resume(void) {
	Mix_HookMusicFinished(mixer_music_finished);
	Mix_ResumeMusic();
	return true;
}

static bool audio_sdl2mixer_music_pause(void) {
	Mix_HookMusicFinished(NULL);
	Mix_PauseMusic();
	return true;
}

static bool audio_sdl2mixer_music_play(MusicImpl *imus) {
	Mix_Music *mmus;
	int loops;

	Mix_HookMusicFinished(NULL);
	Mix_HaltMusic();

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

	bool result = (Mix_PlayMusic(mmus, loops) != -1);

	if(!result) {
		log_error("Mix_PlayMusic() failed: %s", Mix_GetError());
	}

	return result;
}

static bool audio_sdl2mixer_music_set_position(double pos) {
	Mix_RewindMusic();

	if(Mix_SetMusicPosition(pos)) {
		log_error("Mix_SetMusicPosition() failed: %s", Mix_GetError());
		return false;
	}

	return true;
}

static bool audio_sdl2mixer_music_set_loop_point(MusicImpl *imus, double pos) {
	imus->loop_point = pos;
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

static int audio_sdl2mixer_sound_play_on_channel(int chan, SoundImpl *isnd) {
	chan = Mix_PlayChannel(chan, isnd->ch, 0);

	Mix_UnregisterAllEffects(chan);
	if(chan < 0) {
		log_error("Mix_PlayChannel() failed: %s", Mix_GetError());
		return false;
	}

	isnd->playchan = chan;
	return true;
}

static bool audio_sdl2mixer_sound_play(SoundImpl *isnd, AudioBackendSoundGroup group) {
	return audio_sdl2mixer_sound_play_on_channel(pick_channel(group, MAIN_CHANNEL_GROUP), isnd);
}

static bool audio_sdl2mixer_sound_play_or_restart(SoundImpl *isnd, AudioBackendSoundGroup group) {
	int chan = isnd->playchan;

	if(chan < 0 || Mix_GetChunk(chan) != isnd->ch) {
		chan = pick_channel(group, MAIN_CHANNEL_GROUP);
	}

	return audio_sdl2mixer_sound_play_on_channel(chan, isnd);
}

static bool audio_sdl2mixer_sound_loop(SoundImpl *isnd, AudioBackendSoundGroup group) {
	isnd->loopchan = Mix_PlayChannel(pick_channel(group, MAIN_CHANNEL_GROUP), isnd->ch, -1);
	Mix_UnregisterAllEffects(isnd->loopchan);

	if(isnd->loopchan == -1) {
		log_error("Mix_PlayChannel() failed: %s", Mix_GetError());
		return false;
	}

	return true;
}

// XXX: This custom fading effect circumvents https://bugzilla.libsdl.org/show_bug.cgi?id=2904
typedef struct CustomFadeout {
	int duration; // in samples
	int counter;
} CustomFadeout;

static void custom_fadeout_free(int chan, void *udata) {
	free(udata);
}

static void custom_fadeout_proc(int chan, void *stream, int len, void *udata) {
	CustomFadeout *e = udata;

	assert(AUDIO_FORMAT == AUDIO_S16SYS); // if you wanna change the format, you get to implement it here. This is the hardcoded default format in SDL_Mixer by the way

	int16_t *data = stream;
	len /= 2;
	for(int i = 0; i < len; i++) {
		e->counter++;
		data[i]*=1.-min(1,(double)e->counter/(double)e->duration);
	}
}

static bool audio_sdl2mixer_sound_stop_loop(SoundImpl *isnd) {
	if(isnd->loopchan == -1) {
		return false;
	}

	CustomFadeout *effect = calloc(1, sizeof(CustomFadeout));
	effect->counter = 0;
	effect->duration = LOOPFADEOUT*AUDIO_FREQ/1000;
	Mix_ExpireChannel(isnd->loopchan, LOOPFADEOUT);
	Mix_RegisterEffect(isnd->loopchan, custom_fadeout_proc, custom_fadeout_free, effect);

	return true;

}

static bool audio_sdl2mixer_sound_pause_all(AudioBackendSoundGroup group) {
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


static bool audio_sdl2mixer_sound_resume_all(AudioBackendSoundGroup group) {
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

static bool audio_sdl2mixer_sound_stop_all(AudioBackendSoundGroup group) {
	int mixgroup = translate_group(group, -1);

	if(mixgroup == -1) {
		Mix_HaltChannel(-1);
	} else {
		Mix_HaltGroup(mixgroup);
	}

	return true;
}

static const char *const *audio_sdl2mixer_get_supported_exts(uint *numexts) {
	static const char *const exts[] = { ".ogg", ".wav" };
	*numexts = ARRAY_SIZE(exts);
	return exts;
}

static MusicImpl *audio_sdl2mixer_music_load(const char *vfspath) {
	SDL_RWops *rw = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	Mix_Music *mus = Mix_LoadMUS_RW(rw, true);

	if(!mus) {
		log_error("Mix_LoadMUS_RW() failed: %s", Mix_GetError());
		return NULL;
	}

	MusicImpl *imus = calloc(1, sizeof(*imus));
	imus->loop = mus;

	return imus;
}

static void audio_sdl2mixer_music_unload(MusicImpl *imus) {
	Mix_FreeMusic(imus->loop);
	free(imus);
}

static SoundImpl *audio_sdl2mixer_sound_load(const char *vfspath) {
	SDL_RWops *rw = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	Mix_Chunk *snd = Mix_LoadWAV_RW(rw, true);

	if(!snd) {
		log_error("Mix_LoadWAV_RW() failed: %s", Mix_GetError());
		return NULL;
	}

	SoundImpl *isnd = calloc(1, sizeof(*isnd));
	isnd->ch = snd;
	isnd->loopchan = -1;
	isnd->playchan = -1;

	return isnd;
}

static void audio_sdl2mixer_sound_unload(SoundImpl *isnd) {
	Mix_FreeChunk(isnd->ch);
	free(isnd);
}

static bool audio_sdl2mixer_sound_set_volume(SoundImpl *isnd, double gain) {
	Mix_VolumeChunk(isnd->ch, gain_to_mixvol(gain));
	return true;
}

AudioBackend _a_backend_sdl2mixer = {
	.name = "sdl2mixer",
	.funcs = {
		.get_supported_music_exts = audio_sdl2mixer_get_supported_exts,
		.get_supported_sound_exts = audio_sdl2mixer_get_supported_exts,
		.init = audio_sdl2mixer_init,
		.music_fade = audio_sdl2mixer_music_fade,
		.music_is_paused = audio_sdl2mixer_music_is_paused,
		.music_is_playing = audio_sdl2mixer_music_is_playing,
		.music_load = audio_sdl2mixer_music_load,
		.music_pause = audio_sdl2mixer_music_pause,
		.music_play = audio_sdl2mixer_music_play,
		.music_resume = audio_sdl2mixer_music_resume,
		.music_set_global_volume = audio_sdl2mixer_music_set_global_volume,
		.music_set_loop_point = audio_sdl2mixer_music_set_loop_point,
		.music_set_position = audio_sdl2mixer_music_set_position,
		.music_stop = audio_sdl2mixer_music_stop,
		.music_unload = audio_sdl2mixer_music_unload,
		.output_works = audio_sdl2mixer_output_works,
		.shutdown = audio_sdl2mixer_shutdown,
		.sound_load = audio_sdl2mixer_sound_load,
		.sound_loop = audio_sdl2mixer_sound_loop,
		.sound_pause_all = audio_sdl2mixer_sound_pause_all,
		.sound_play = audio_sdl2mixer_sound_play,
		.sound_play_or_restart = audio_sdl2mixer_sound_play_or_restart,
		.sound_resume_all = audio_sdl2mixer_sound_resume_all,
		.sound_set_global_volume = audio_sdl2mixer_sound_set_global_volume,
		.sound_set_volume = audio_sdl2mixer_sound_set_volume,
		.sound_stop_all = audio_sdl2mixer_sound_stop_all,
		.sound_stop_loop = audio_sdl2mixer_sound_stop_loop,
		.sound_unload = audio_sdl2mixer_sound_unload,
	}
};
