/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "../backend.h"
#include "../stream/stream.h"
#include "../stream/stream_pcm.h"
#include "../stream/player.h"

#include "util.h"
#include "rwops/rwops_autobuf.h"
#include "config.h"

#define AUDIO_FREQ 48000
#define AUDIO_FORMAT AUDIO_F32SYS
#define AUDIO_CHANNELS 2

#define CHAN_BGM 0

#define NUM_BGM_CHANNELS 1
#define NUM_SFX_MAIN_CHANNELS 28
#define NUM_SFX_UI_CHANNELS 4
#define NUM_SFX_CHANNELS (NUM_SFX_MAIN_CHANNELS + NUM_SFX_UI_CHANNELS)

struct SFXImpl {
	size_t pcm_size;
	AudioStreamSpec spec;
	float gain;

	// TODO: maybe implement this tracking in the frontend instead.
	// It's needed for things like sfx_stop_loop and sfx_play_or_restart to work, which are hacky at best.
// 	struct {
// 		SFXPlayID last_loop_id;
// 		SFXPlayID last_play_id;
// 	} per_group[NUM_SFX_GROUPS];

	uint8_t pcm[];
};

struct BGM {
	AudioStream stream;
};

static_assert(offsetof(BGM, stream) == 0, "");

static struct {
	StreamPlayer players[NUM_CHANGROUPS];

	StaticPCMAudioStream sfx_streams[NUM_SFX_CHANNELS];
	uint32_t chan_play_ids[NUM_SFX_CHANNELS];
	uint32_t play_counter;

	AudioStreamSpec spec;
	SDL_AudioDeviceID audio_device;
	uint8_t silence;
} mixer;

#define IS_VALID_CHANNEL(chan) ((uint)(chan) < NUM_SFX_CHANNELS)

// BEGIN MISC

static void SDLCALL mixer_callback(void *ignore, uint8_t *stream, int len) {
	memset(stream, mixer.silence, len);

	for(int i = 0; i < ARRAY_SIZE(mixer.players); ++i) {
		StreamPlayer *plr = mixer.players + i;
		splayer_process(plr, len, stream);
	}
}

static bool init_sdl_audio(void) {
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

	if(SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		log_sdl_error(LOG_ERROR, "SDL_InitSubSystem");
		return false;
	}

	log_info("Using driver '%s'", SDL_GetCurrentAudioDriver());
	return true;
}

static bool init_audio_device(void) {
	SDL_AudioSpec want = { 0 }, have;
	want.callback = mixer_callback;
	want.channels = AUDIO_CHANNELS;
	want.format = AUDIO_FORMAT;
	want.freq = AUDIO_FREQ;
	want.samples = config_get_int(CONFIG_MIXER_CHUNKSIZE);

	// NOTE: StreamPlayer expects stereo float32 samples.
	uint allow_changes = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE;

	#ifdef SDL_AUDIO_ALLOW_SAMPLES_CHANGE
		allow_changes |= SDL_AUDIO_ALLOW_SAMPLES_CHANGE;
	#endif

	mixer.audio_device = SDL_OpenAudioDevice(NULL, false, &want, &have, allow_changes);

	if(mixer.audio_device == 0) {
		log_sdl_error(LOG_ERROR, "SDL_OpenAudioDevice");
		return false;
	}

	if(have.freq != want.freq || have.format != want.format) {
		log_warn(
			"Audio device spec doesn't match our request, "
			"requested (freq=%i, fmt=0x%x), got (freq=%i, fmt=0x%x). "
			"Sound may be distorted.",
		   want.freq, want.format, have.freq, have.format
		);
	}

	mixer.spec = astream_spec(have.format, have.channels, have.freq);
	mixer.silence = have.silence;

	return true;
}

static void shutdown_players(void) {
	for(int i = 0; i < ARRAY_SIZE(mixer.players); ++i) {
		StreamPlayer *plr = mixer.players + i;

		if(plr->channels) {
			splayer_shutdown(plr);
			memset(plr, 0, sizeof(*plr));
		}
	}
}

static bool init_players(void) {
	if(!splayer_init(&mixer.players[CHANGROUP_BGM], NUM_BGM_CHANNELS, &mixer.spec)) {
		log_error("splayer_init() failed");
		return false;
	}

	if(!splayer_init(&mixer.players[CHANGROUP_SFX_GAME], NUM_SFX_MAIN_CHANNELS, &mixer.spec)) {
		log_error("splayer_init() failed");
		return false;
	}

	if(!splayer_init(&mixer.players[CHANGROUP_SFX_UI], NUM_SFX_UI_CHANNELS, &mixer.spec)) {
		log_error("splayer_init() failed");
		return false;
	}

	return true;
}

static void init_sfx_streams(void) {
	for(int i = 0; i < ARRAY_SIZE(mixer.sfx_streams); ++i) {
		astream_pcm_static_init(mixer.sfx_streams + i);
	}
}

static bool audio_sdl_init(void) {
	if(!init_sdl_audio()) {
		return false;
	}

	if(!init_audio_device()) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	if(!init_players()) {
		shutdown_players();
		SDL_CloseAudioDevice(mixer.audio_device);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	init_sfx_streams();

	SDL_PauseAudioDevice(mixer.audio_device, false);
	log_info("Audio subsystem initialized (SDL)");

	return true;
}

static bool audio_sdl_shutdown(void) {
	SDL_PauseAudioDevice(mixer.audio_device, true);
	shutdown_players();
	SDL_CloseAudioDevice(mixer.audio_device);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	log_info("Audio subsystem deinitialized (SDL)");
	return true;
}

static bool audio_sdl_output_works(void) {
	return SDL_GetAudioDeviceStatus(mixer.audio_device) == SDL_AUDIO_PLAYING;
}

static const char *const *audio_sdl_get_supported_exts(uint *numexts) {
	// TODO figure this out dynamically
	static const char *const exts[] = { ".opus" };
	*numexts = ARRAY_SIZE(exts);
	return exts;
}

INLINE void lock_audio(void) {
	SDL_LockAudioDevice(mixer.audio_device);
}

INLINE void unlock_audio(void) {
	SDL_UnlockAudioDevice(mixer.audio_device);
}

// END MISC

// BEGIN GROUP

static bool audio_sdl_group_get_channels_range(AudioChannelGroup group, int *first, int *last) {
	switch(group) {
		case CHANGROUP_SFX_GAME:
			*first = 0;
			*last = NUM_SFX_MAIN_CHANNELS - 1;
			return true;

		case CHANGROUP_SFX_UI:
			*first = NUM_SFX_MAIN_CHANNELS;
			*last = NUM_SFX_MAIN_CHANNELS + NUM_SFX_UI_CHANNELS - 1;
			return true;

		case CHANGROUP_BGM:
			*first = NUM_SFX_CHANNELS;
			*last = NUM_SFX_CHANNELS;
			return true;

		default:
			UNREACHABLE;
	}
}

static bool audio_sdl_group_pause(AudioChannelGroup group) {
	StreamPlayer *plr = &mixer.players[group];

	lock_audio();
	bool result = splayer_global_pause(plr);
	unlock_audio();

	return result;
}

static bool audio_sdl_group_resume(AudioChannelGroup group) {
	StreamPlayer *plr = &mixer.players[group];

	lock_audio();
	bool result = splayer_global_resume(plr);
	unlock_audio();

	return result;
}

static bool audio_sdl_group_stop(AudioChannelGroup group, double fadeout) {
	StreamPlayer *plr = &mixer.players[group];

	lock_audio();

	for(int i = 0; i < plr->num_channels; ++i) {
		if(fadeout > 0) {
			splayer_fadeout(plr, i, fadeout);
		} else {
			splayer_halt(plr, i);
		}
	}

	unlock_audio();

	return true;
}

static bool audio_sdl_group_set_volume(AudioChannelGroup group, double vol) {
	lock_audio();
	mixer.players[group].gain = vol;
	unlock_audio();
	return true;
}

static void flatchan_to_groupchan(
	AudioBackendChannel fchan, AudioChannelGroup *out_group, int *out_chan
) {
	assert(fchan >= 0);
	assert(fchan < NUM_SFX_CHANNELS);

	if(fchan >= NUM_SFX_MAIN_CHANNELS) {
		*out_group = CHANGROUP_SFX_UI;
		*out_chan = fchan - NUM_SFX_MAIN_CHANNELS;
	} else {
		*out_group = CHANGROUP_SFX_GAME;
		*out_chan = fchan;
	}
}

static int groupchan_to_flatchan(int group, int chan) {
	switch(group) {
		case CHANGROUP_SFX_GAME:
			return chan;

		case CHANGROUP_SFX_UI:
			return chan + NUM_SFX_MAIN_CHANNELS;

		default:
			UNREACHABLE;
	}
}

// END GROUP

// BEGIN BGM

static BGMStatus audio_sdl_bgm_status(void) {
	lock_audio();
	BGMStatus status = splayer_util_bgmstatus(&mixer.players[CHANGROUP_BGM], CHAN_BGM);
	unlock_audio();
	return status;
}

static bool audio_sdl_bgm_play(BGM *bgm, bool loop, double position, double fadein) {
	lock_audio();
	bool status = splayer_play(&mixer.players[CHANGROUP_BGM], CHAN_BGM, &bgm->stream, loop, 1, position, fadein);
	unlock_audio();
	return status;
}

static bool audio_sdl_bgm_stop(double fadeout) {
	lock_audio();

	if(splayer_util_bgmstatus(&mixer.players[CHANGROUP_BGM], CHAN_BGM) == BGM_STOPPED) {
		unlock_audio();
		log_warn("BGM is already stopped");
		return false;
	}

	if(fadeout > 0) {
		splayer_fadeout(&mixer.players[CHANGROUP_BGM], CHAN_BGM, fadeout);
	} else {
		splayer_halt(&mixer.players[CHANGROUP_BGM], CHAN_BGM);
	}

	unlock_audio();
	return true;
}

static bool audio_sdl_bgm_looping(void) {
	lock_audio();

	if(splayer_util_bgmstatus(&mixer.players[CHANGROUP_BGM], CHAN_BGM) == BGM_STOPPED) {
		log_warn("BGM is stopped");
		return false;
	}

	bool looping = splayer_is_looping(&mixer.players[CHANGROUP_BGM], CHAN_BGM);
	unlock_audio();

	return looping;
}

static bool audio_sdl_bgm_pause(void) {
	lock_audio();
	bool status = splayer_pause(&mixer.players[CHANGROUP_BGM], CHAN_BGM);
	unlock_audio();
	return status;
}

static bool audio_sdl_bgm_resume(void) {
	lock_audio();
	bool status = splayer_resume(&mixer.players[CHANGROUP_BGM], CHAN_BGM);
	unlock_audio();
	return status;
}

static double audio_sdl_bgm_seek(double pos) {
	lock_audio();
	pos = splayer_seek(&mixer.players[CHANGROUP_BGM], CHAN_BGM, pos);
	unlock_audio();
	return pos;
}

static double audio_sdl_bgm_tell(void) {
	lock_audio();
	double pos = splayer_tell(&mixer.players[CHANGROUP_BGM], CHAN_BGM);
	unlock_audio();
	return pos;
}

static BGM *audio_sdl_bgm_current(void) {
	BGM *bgm = NULL;
	lock_audio();

	if(splayer_util_bgmstatus(&mixer.players[CHANGROUP_BGM], CHAN_BGM) != BGM_STOPPED) {
		bgm = UNION_CAST(AudioStream*, BGM*, mixer.players[CHANGROUP_BGM].channels[CHAN_BGM].stream);
	}

	unlock_audio();
	return bgm;
}

// END BGM

// BEGIN BGM OBJECT

static const char *audio_sdl_obj_bgm_get_title(BGM *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_TITLE);
}

static const char *audio_sdl_obj_bgm_get_artist(BGM *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_ARTIST);
}

static const char *audio_sdl_obj_bgm_get_comment(BGM *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_COMMENT);
}

static double audio_sdl_obj_bgm_get_duration(BGM *bgm) {
	return astream_util_offset_to_time(&bgm->stream, bgm->stream.length);
}

static double audio_sdl_obj_bgm_get_loop_start(BGM *bgm) {
	return astream_util_offset_to_time(&bgm->stream, bgm->stream.loop_start);
}

// END BGM OBJECT

// BEGIN SFX

static AudioBackendChannel play_sfx_on_channel(
	SFXImpl *sfx,
	AudioChannelGroup group,
	AudioBackendChannel flat_chan,
	bool loop,
	double pos,
	double fadein
) {
	lock_audio();

	int chan;
	StreamPlayer *plr;

	if(flat_chan != AUDIO_BACKEND_CHANNEL_INVALID) {
		assume(IS_VALID_CHANNEL(flat_chan));
		assume(group == CHANGROUP_ANY);
		flatchan_to_groupchan(flat_chan, &group, &chan);
		plr = &mixer.players[group];
	} else {
		plr = &mixer.players[group];
		chan = splayer_pick_channel(plr);
		flat_chan = groupchan_to_flatchan(group, chan);
	}

	StaticPCMAudioStream *stream = &mixer.sfx_streams[flat_chan];

	bool ok = (
		astream_pcm_reopen(&stream->astream, &mixer.spec, sfx->pcm_size, sfx->pcm, 0)
		&& splayer_play(plr, chan, &stream->astream, loop, sfx->gain, pos, fadein)
	);

	unlock_audio();

	return ok ? flat_chan : AUDIO_BACKEND_CHANNEL_INVALID;
}

static AudioBackendChannel audio_sdl_sfx_play(
	SFXImpl *sfx, AudioChannelGroup group, AudioBackendChannel chan, bool loop
) {
	return play_sfx_on_channel(sfx, group, chan, loop, 0, 0);
}

static bool audio_sdl_chan_stop(AudioBackendChannel chan, double fadeout) {
	AudioChannelGroup g;
	int gch;
	flatchan_to_groupchan(chan, &g, &gch);
	StreamPlayer *plr = &mixer.players[g];

	lock_audio();

	if(fadeout) {
		splayer_fadeout(plr, gch, fadeout);
	} else {
		splayer_halt(plr, gch);
	}

	unlock_audio();

	return true;
}

static bool audio_sdl_chan_unstop(AudioBackendChannel chan, double fadein) {
	AudioChannelGroup g;
	int gch;
	flatchan_to_groupchan(chan, &g, &gch);
	StreamPlayer *plr = &mixer.players[g];
	bool ok = false;

	lock_audio();

	if(plr->channels[gch].stream) {
		if(plr->channels[gch].fade.gain < 1) {
			splayer_fadein(plr, gch, fadein);
		}
		ok = true;
	}

	unlock_audio();

	return ok;
}

// END SFX

// BEGIN SFX OBJECT

static bool audio_sdl_obj_sfx_set_volume(SFXImpl *sfx, double vol) {
	sfx->gain = vol;
	return true;
}

// END SFX OBJECT

// BEGIN LOAD

static BGM *audio_sdl_bgm_load(const char *vfspath) {
	SDL_RWops *rw = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	BGM *bgm = calloc(1, sizeof(*bgm));

	if(!astream_open(&bgm->stream, rw, vfspath)) {
		SDL_RWclose(rw);
		free(bgm);
		return NULL;
	}

	log_debug("Loaded stream from %s", vfspath);
	return bgm;
}

static void audio_sdl_bgm_unload(BGM *bgm) {
	lock_audio();
	if(mixer.players[CHANGROUP_BGM].channels[CHAN_BGM].stream == &bgm->stream) {
		splayer_halt(&mixer.players[CHANGROUP_BGM], CHAN_BGM);
	}
	unlock_audio();

	astream_close(&bgm->stream);
	free(bgm);
}

static SFXImpl *audio_sdl_sfx_load(const char *vfspath) {
	SDL_RWops *rw = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	AudioStream stream;

	if(!astream_open(&stream, rw, vfspath)) {
		return NULL;
	}

	if(stream.length < 0) {
		log_error("Indeterminate stream length");
		astream_close(&stream);
		return NULL;
	}

	if(stream.length == 0) {
		log_error("Empty stream");
		astream_close(&stream);
		return NULL;
	}

	ssize_t pcm_size = astream_util_resampled_buffer_size(&stream, &mixer.spec);

	if(pcm_size < 0) {
		astream_close(&stream);
		return NULL;
	}

	assert(pcm_size <= INT32_MAX);

	SFXImpl *isnd = calloc(1, sizeof(*isnd) + pcm_size);

	bool ok = astream_crystalize(&stream, &mixer.spec, pcm_size, &isnd->pcm);
	astream_close(&stream);

	if(!ok) {
		free(isnd);
		return NULL;
	}

	log_debug("Loaded SFX from %s", vfspath);

	isnd->pcm_size = pcm_size;
	return isnd;
}

static void audio_sdl_sfx_unload(SFXImpl *sfx) {
	lock_audio();

	for(int i = 0; i < ARRAY_SIZE(mixer.sfx_streams); ++i) {
		StaticPCMAudioStream *s = &mixer.sfx_streams[i];

		if(s->ctx.start == sfx->pcm) {
			astream_close(&s->astream);
		}
	}

	unlock_audio();

	free(sfx);
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
