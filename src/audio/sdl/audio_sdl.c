/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "../backend.h"
#include "../stream/mixer.h"

#include "rwops/rwops_autobuf.h"
#include "config.h"
#include "util/io.h"

#define AUDIO_FREQ 48000
#define AUDIO_FORMAT SDL_AUDIO_F32
#define AUDIO_CHANNELS 2

struct SFXImpl {
	MixerSFXImpl msfx;
};

struct BGM {
	MixerBGMImpl mbgm;
};

static struct {
	Mixer *mixer;
	SDL_AudioStream *sink;
	uint8_t silence;
} audio;

// BEGIN MISC

static void SDLCALL mixer_callback(
	void *ignore, SDL_AudioStream *sink, int additional_amount, int total_amount
) {
	if(additional_amount > 0) {
		uint8_t data[additional_amount];
		memset(data, audio.silence, additional_amount);
		mixer_process(audio.mixer, additional_amount, data);
		SDL_PutAudioStreamData(sink, data, additional_amount);
	}
}

static bool init_sdl_audio(void) {
	uint num_drivers = SDL_GetNumAudioDrivers();
	void *buf;
	SDL_IOStream *out = SDL_RWAutoBuffer(&buf, 256);

	SDL_RWprintf(out, "Available audio drivers:");

	for(uint i = 0; i < num_drivers; ++i) {
		SDL_RWprintf(out, " %s", SDL_GetAudioDriver(i));
	}

	SDL_WriteU8(out, 0);
	log_info("%s", (char*)buf);
	SDL_CloseIO(out);

	if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		log_sdl_error(LOG_ERROR, "SDL_InitSubSystem");
		return false;
	}

	log_info("Using driver '%s'", SDL_GetCurrentAudioDriver());
	return true;
}

static bool init_audio_device(SDL_AudioSpec *spec) {
	SDL_AudioSpec want = {
		.channels = AUDIO_CHANNELS,
		.format = AUDIO_FORMAT,
		.freq = AUDIO_FREQ,
	};

	int want_bufsize = config_get_int(CONFIG_MIXER_CHUNKSIZE);

	char buf[32];
	snprintf(buf, sizeof(buf), "%d", want_bufsize);
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, buf);

	audio.sink = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, mixer_callback, NULL);

	if(!audio.sink) {
		log_sdl_error(LOG_ERROR, "SDL_OpenAudioDeviceStream");
		return false;
	}

	*spec = want;  // SDL will resample it for us
	audio.silence = SDL_GetSilenceValueForFormat(want.format);

	return true;
}

static bool audio_sdl_init(void) {
	if(!init_sdl_audio()) {
		return false;
	}

	SDL_AudioSpec aspec;

	if(!init_audio_device(&aspec)) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	AudioStreamSpec mspec = astream_spec(aspec.format, aspec.channels, aspec.freq);

	audio.mixer = ALLOC(typeof(*audio.mixer));

	if(!mixer_init(audio.mixer, &mspec)) {
		mixer_shutdown(audio.mixer);
		mem_free(audio.mixer);
		SDL_DestroyAudioStream(audio.sink);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio.sink));
	log_info("Audio subsystem initialized (SDL)");

	return true;
}

static bool audio_sdl_shutdown(void) {
	SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(audio.sink));
	SDL_DestroyAudioStream(audio.sink);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	mixer_shutdown(audio.mixer);
	mem_free(audio.mixer);

	log_info("Audio subsystem deinitialized (SDL)");
	return true;
}

static bool audio_sdl_output_works(void) {
	if(!audio.sink) {
		return false;
	}

	SDL_AudioDeviceID dev = SDL_GetAudioStreamDevice(audio.sink);

	if(!dev) {
		return false;
	}

	return !SDL_AudioDevicePaused(dev);
}

static const char *const *audio_sdl_get_supported_exts(uint *numexts) {
	return mixer_get_supported_exts(numexts);
}

INLINE void lock_audio(void) {
	SDL_LockAudioStream(audio.sink);
}

INLINE void unlock_audio(void) {
	SDL_UnlockAudioStream(audio.sink);
}

#define WITH_AUDIO_LOCK(...) ({ \
	lock_audio(); \
	auto _result = __VA_ARGS__; \
	unlock_audio(); \
	_result; \
})

// END MISC

// BEGIN GROUP

static bool audio_sdl_group_get_channels_range(AudioChannelGroup group, int *first, int *last) {
	return mixer_group_get_channels_range(group, first, last);
}

static bool audio_sdl_group_pause(AudioChannelGroup group) {
	return WITH_AUDIO_LOCK(mixer_group_pause(audio.mixer, group));
}

static bool audio_sdl_group_resume(AudioChannelGroup group) {
	return WITH_AUDIO_LOCK(mixer_group_resume(audio.mixer, group));
}

static bool audio_sdl_group_stop(AudioChannelGroup group, double fadeout) {
	return WITH_AUDIO_LOCK(mixer_group_stop(audio.mixer, group, fadeout));
}

static bool audio_sdl_group_set_volume(AudioChannelGroup group, double vol) {
	return WITH_AUDIO_LOCK(mixer_group_set_volume(audio.mixer, group, vol));
}

// END GROUP

// BEGIN BGM

static BGMStatus audio_sdl_bgm_status(void) {
	return WITH_AUDIO_LOCK(mixer_bgm_status(audio.mixer));
}

static bool audio_sdl_bgm_play(BGM *bgm, bool loop, double position, double fadein) {
	return WITH_AUDIO_LOCK(mixer_bgm_play(
		audio.mixer, &bgm->mbgm, loop, position, fadein
	));
}

static bool audio_sdl_bgm_stop(double fadeout) {
	return WITH_AUDIO_LOCK(mixer_bgm_stop(audio.mixer, fadeout));
}

static bool audio_sdl_bgm_looping(void) {
	return WITH_AUDIO_LOCK(mixer_bgm_looping(audio.mixer));
}

static bool audio_sdl_bgm_pause(void) {
	return WITH_AUDIO_LOCK(mixer_bgm_pause(audio.mixer));
}

static bool audio_sdl_bgm_resume(void) {
	return WITH_AUDIO_LOCK(mixer_bgm_resume(audio.mixer));
}

static double audio_sdl_bgm_seek(double pos) {
	return WITH_AUDIO_LOCK(mixer_bgm_seek(audio.mixer, pos));
}

static double audio_sdl_bgm_tell(void) {
	return WITH_AUDIO_LOCK(mixer_bgm_tell(audio.mixer));
}

static BGM *audio_sdl_bgm_current(void) {
	return (BGM*)WITH_AUDIO_LOCK(mixer_bgm_current(audio.mixer));
}

// END BGM

// BEGIN BGM OBJECT

static const char *audio_sdl_obj_bgm_get_title(BGM *bgm) {
	return mixerbgm_get_title(&bgm->mbgm);
}

static const char *audio_sdl_obj_bgm_get_artist(BGM *bgm) {
	return mixerbgm_get_artist(&bgm->mbgm);
}

static const char *audio_sdl_obj_bgm_get_comment(BGM *bgm) {
	return mixerbgm_get_comment(&bgm->mbgm);
}

static double audio_sdl_obj_bgm_get_duration(BGM *bgm) {
	return mixerbgm_get_duration(&bgm->mbgm);
}

static double audio_sdl_obj_bgm_get_loop_start(BGM *bgm) {
	return mixerbgm_get_loop_start(&bgm->mbgm);
}

// END BGM OBJECT

// BEGIN SFX

static AudioBackendChannel audio_sdl_sfx_play(
	SFXImpl *sfx, AudioChannelGroup group, AudioBackendChannel chan, bool loop
) {
	return WITH_AUDIO_LOCK(mixer_sfx_play(
		audio.mixer, &sfx->msfx, group, chan, loop
	));
}

static bool audio_sdl_chan_stop(AudioBackendChannel chan, double fadeout) {
	return WITH_AUDIO_LOCK(mixer_chan_stop(audio.mixer, chan, fadeout));
}

static bool audio_sdl_chan_unstop(AudioBackendChannel chan, double fadein) {
	return WITH_AUDIO_LOCK(mixer_chan_unstop(audio.mixer, chan, fadein));
}

// END SFX

// BEGIN SFX OBJECT

static bool audio_sdl_obj_sfx_set_volume(SFXImpl *sfx, double vol) {
	return mixersfx_set_volume(&sfx->msfx, vol);
}

// END SFX OBJECT

// BEGIN LOAD

static BGM *audio_sdl_bgm_load(const char *vfspath) {
	return (BGM*)mixerbgm_load(vfspath);
}

static void audio_sdl_bgm_unload(BGM *bgm) {
	WITH_AUDIO_LOCK((mixer_notify_bgm_unload(audio.mixer, &bgm->mbgm), 0));
	mixerbgm_unload(&bgm->mbgm);
}

static SFXImpl *audio_sdl_sfx_load(const char *vfspath) {
	return (SFXImpl*)mixersfx_load(vfspath, &audio.mixer->spec);
}

static void audio_sdl_sfx_unload(SFXImpl *sfx) {
	WITH_AUDIO_LOCK((mixer_notify_sfx_unload(audio.mixer, &sfx->msfx), 0));
	mixersfx_unload(&sfx->msfx);
}

// END LOAD

AudioBackend _a_backend_sdl = {
	.name = "sdl",
	.funcs = {
		.bgm_current = audio_sdl_bgm_current,
		.bgm_load = audio_sdl_bgm_load,
		.bgm_looping = audio_sdl_bgm_looping,
		.bgm_pause = audio_sdl_bgm_pause,
		.bgm_play = audio_sdl_bgm_play,
		.bgm_resume = audio_sdl_bgm_resume,
		.bgm_seek = audio_sdl_bgm_seek,
		.bgm_status = audio_sdl_bgm_status,
		.bgm_stop = audio_sdl_bgm_stop,
		.bgm_tell = audio_sdl_bgm_tell,
		.bgm_unload = audio_sdl_bgm_unload,
		.chan_stop = audio_sdl_chan_stop,
		.chan_unstop = audio_sdl_chan_unstop,
		.get_supported_bgm_exts = audio_sdl_get_supported_exts,
		.get_supported_sfx_exts = audio_sdl_get_supported_exts,
		.group_get_channels_range = audio_sdl_group_get_channels_range,
		.group_pause = audio_sdl_group_pause,
		.group_resume = audio_sdl_group_resume,
		.group_set_volume = audio_sdl_group_set_volume,
		.group_stop = audio_sdl_group_stop,
		.init = audio_sdl_init,
		.output_works = audio_sdl_output_works,
		.sfx_load = audio_sdl_sfx_load,
		.sfx_play = audio_sdl_sfx_play,
		.sfx_unload = audio_sdl_sfx_unload,
		.shutdown = audio_sdl_shutdown,

		.object = {
			.bgm = {
				.get_artist = audio_sdl_obj_bgm_get_artist,
				.get_comment = audio_sdl_obj_bgm_get_comment,
				.get_duration = audio_sdl_obj_bgm_get_duration,
				.get_loop_start = audio_sdl_obj_bgm_get_loop_start,
				.get_title = audio_sdl_obj_bgm_get_title,
			},

			.sfx = {
				.set_volume = audio_sdl_obj_sfx_set_volume,
			},
		},
	}
};
