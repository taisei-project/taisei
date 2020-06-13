/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <SDL_mixer.h>

#include "../backend.h"
#include "../stream/stream.h"
#include "../stream/player.h"
#include "global.h"
#include "list.h"

#define B (_a_backend.funcs)

#define AUDIO_FREQ 48000
#define AUDIO_FORMAT AUDIO_F32SYS
#define AUDIO_CHANNELS 32
#define UI_CHANNELS 4
#define UI_CHANNEL_GROUP 0
#define MAIN_CHANNEL_GROUP 1

#if AUDIO_FORMAT == AUDIO_S16SYS
typedef int16_t sample_t;
#define OPUS_READ_SAMPLES_STEREO op_read_stereo
#elif AUDIO_FORMAT == AUDIO_F32SYS
typedef float sample_t;
#define OPUS_READ_SAMPLES_STEREO op_read_float_stereo
#else
#error Audio format not recognized
#endif

struct SFXImpl {
	Mix_Chunk *ch;

	// I needed to add this for supporting loop sounds since Mixer doesn’t remember
	// what channel a sound is playing on.  - laochailan
	int loopchan; // channel the sound may be looping on. -1 if not looping
	int playchan; // channel the sound was last played on (looping does NOT set this). -1 if never played
};

struct BGM {
	AudioStream stream;
};

static_assert(offsetof(BGM, stream) == 0, "");

static struct {
	StreamPlayer bgm_player;

	uint32_t play_counter;
	uint32_t chan_play_ids[AUDIO_CHANNELS];

	struct {
		uchar first;
		uchar num;
	} groups[2];

	AudioStreamSpec spec;
} mixer;

#define IS_VALID_CHANNEL(chan) ((uint)(chan) < AUDIO_CHANNELS)

// BEGIN MISC

static void SDLCALL audio_sdl2mixer_music_hook(void *player, uint8_t *stream, int len) {
	// TODO: find out whether SPLAYER_SILENCE_PAD is necessary

	// NOTE: In the past there was a chance, on some especially braindead Windows systems,
	// that the audio device opened by SDL_mixer would not have the requested sample format.
	// Our StreamPlayer currently only supports floating point samples, i.e. AUDIO_F32SYS.
	// Hopefully recent enough SDL and SDL_mixer versions should be able to deal with this
	// transparently with an internal conversion. If that happens not to be the case, one
	// solution would be to pipe the StreamPlayer output through another SDL_AudioStream to
	// convert the samples into whatever format the audio device expects. Though I'm more
	// inclined to just tell people to use a less shitty OS if this is still a problem.
	// But patches welcome.

	splayer_process(player, len, stream, SPLAYER_SILENCE_PAD);
}

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
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	if(!(Mix_Init(MIX_INIT_OPUS) & MIX_INIT_OPUS)) {
		log_error("Mix_Init() failed: %s", Mix_GetError());
		goto fail;
	}

	log_info("Using driver '%s'", SDL_GetCurrentAudioDriver());

	int channels = Mix_AllocateChannels(AUDIO_CHANNELS);

	if(!channels) {
		log_error("Unable to allocate any channels");
		goto fail;
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

	mixer.groups[UI_CHANNEL_GROUP].first = 0;
	mixer.groups[UI_CHANNEL_GROUP].num = ui_channels;
	mixer.groups[MAIN_CHANNEL_GROUP].first = ui_channels;
	mixer.groups[MAIN_CHANNEL_GROUP].num = main_channels;

	B.sfx_set_global_volume(config_get_float(CONFIG_SFX_VOLUME));
	B.bgm_set_global_volume(config_get_float(CONFIG_BGM_VOLUME));

	int frequency = 0;
	uint16_t format = 0;

	Mix_QuerySpec(&frequency, &format, &channels);
	mixer.spec.channels = channels;
	mixer.spec.sample_format = format;
	mixer.spec.sample_rate = frequency;

	if(frequency != AUDIO_FREQ || format != AUDIO_FORMAT) {
		log_warn(
			"Mixer spec doesn't match our request, "
			"requested (freq=%i, fmt=%u), got (freq=%i, fmt=%u). "
			"Sound may be distorted.",
			AUDIO_FREQ, AUDIO_FORMAT, frequency, format
		);
	}

	if(!splayer_init(&mixer.bgm_player, &mixer.spec)) {
		log_error("splayer_init() failed");
		goto fail;
	}

	Mix_HookMusic(audio_sdl2mixer_music_hook, &mixer.bgm_player);

	log_info("Audio subsystem initialized (SDL2_Mixer)");

	SDL_version v;
	const SDL_version *lv;

	SDL_MIXER_VERSION(&v);
	log_info("Compiled against SDL_mixer %u.%u.%u", v.major, v.minor, v.patch);

	lv = Mix_Linked_Version();
	log_info("Using SDL_mixer %u.%u.%u", lv->major, lv->minor, lv->patch);

	return true;

fail:
	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	return false;
}

static bool audio_sdl2mixer_shutdown(void) {
	Mix_HookMusic(NULL, NULL);
	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	splayer_shutdown(&mixer.bgm_player);

	log_info("Audio subsystem deinitialized (SDL2_Mixer)");
	return true;
}

static bool audio_sdl2mixer_output_works(void) {
	return true;
}

static inline int gain_to_mixvol(double gain) {
	return iclamp(gain * SDL_MIX_MAXVOLUME, 0, SDL_MIX_MAXVOLUME);
}

static const char *const *audio_sdl2mixer_get_supported_bgm_exts(uint *numexts) {
	// TODO figure this out dynamically
	static const char *const exts[] = { ".opus" };
	*numexts = ARRAY_SIZE(exts);
	return exts;
}

static const char *const *audio_sdl2mixer_get_supported_sfx_exts(uint *numexts) {
	// TODO figure this out dynamically
	static const char *const exts[] = { ".opus", ".wav" };
	*numexts = ARRAY_SIZE(exts);
	return exts;
}

// END MISC

// BEGIN BGM

static BGMStatus audio_sdl2mixer_bgm_status(void) {
	return splayer_util_bgmstatus(&mixer.bgm_player);
}

static bool audio_sdl2mixer_bgm_set_global_volume(double gain) {
	splayer_lock(&mixer.bgm_player);
	mixer.bgm_player.gain = gain;
	splayer_unlock(&mixer.bgm_player);
	return true;
}

static bool audio_sdl2mixer_bgm_play(BGM *bgm, bool loop, double position, double fadein) {
	return splayer_play(&mixer.bgm_player, &bgm->stream, loop, position, fadein);
}

static bool audio_sdl2mixer_bgm_stop(double fadeout) {
	if(splayer_util_bgmstatus(&mixer.bgm_player) == BGM_STOPPED) {
		log_warn("BGM is already stopped");
		return false;
	}

	if(fadeout > 0) {
		splayer_fadeout(&mixer.bgm_player, fadeout);
	} else {
		splayer_halt(&mixer.bgm_player);
	}

	return true;
}

static bool audio_sdl2mixer_bgm_looping(void) {
	splayer_lock(&mixer.bgm_player);

	if(splayer_util_bgmstatus(&mixer.bgm_player) == BGM_STOPPED) {
		log_warn("BGM is stopped");
		return false;
	}

	bool looping = mixer.bgm_player.looping;
	splayer_unlock(&mixer.bgm_player);

	return looping;
}

static bool audio_sdl2mixer_bgm_pause(void) {
	return splayer_pause(&mixer.bgm_player);
}

static bool audio_sdl2mixer_bgm_resume(void) {
	return splayer_resume(&mixer.bgm_player);
}

static double audio_sdl2mixer_bgm_seek(double pos) {
	return splayer_seek(&mixer.bgm_player, pos);
}

static double audio_sdl2mixer_bgm_tell(void) {
	return splayer_tell(&mixer.bgm_player);
}

static BGM *audio_sdl2mixer_bgm_current(void) {
	BGM *bgm;
	splayer_lock(&mixer.bgm_player);
	bgm = UNION_CAST(AudioStream*, BGM*, mixer.bgm_player.stream);
	splayer_unlock(&mixer.bgm_player);
	return bgm;
}

static bool audio_sdl2mixer_bgm_set_loop_point(BGM *bgm, double pos) {
	return false;
}

// END BGM

// BEGIN BGM OBJECT

static const char *audio_sdl2mixer_obj_bgm_get_title(BGM *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_TITLE);
}

static const char *audio_sdl2mixer_obj_bgm_get_artist(BGM *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_ARTIST);
}

static const char *audio_sdl2mixer_obj_bgm_get_comment(BGM *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_COMMENT);
}

static double audio_sdl2mixer_obj_bgm_get_duration(BGM *bgm) {
	return astream_util_offset_to_time(&bgm->stream, bgm->stream.length);
}

static double audio_sdl2mixer_obj_bgm_get_loop_start(BGM *bgm) {
	return astream_util_offset_to_time(&bgm->stream, bgm->stream.loop_start);
}

// END BGM OBJECT

// BEGIN SFX

static bool audio_sdl2mixer_sfx_set_global_volume(double gain) {
	Mix_Volume(-1, gain_to_mixvol(gain));
	return true;
}

static int translate_group(AudioBackendSFXGroup group, int defmixgroup) {
	switch(group) {
		case SFXGROUP_MAIN: return MAIN_CHANNEL_GROUP;
		case SFXGROUP_UI:   return UI_CHANNEL_GROUP;
		default:            return defmixgroup;
	}
}

static int pick_channel(AudioBackendSFXGroup group, int defmixgroup) {
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

INLINE SFXPlayID make_sound_id(int chan, uint32_t play_id) {
	assert(IS_VALID_CHANNEL(chan));
	return ((uint32_t)chan + 1) | ((uint64_t)play_id << 32ull);
}

INLINE void unpack_sound_id(SFXPlayID sid, int *chan, uint32_t *play_id) {
	int ch = (int)((uint32_t)(sid & 0xffffffff) - 1);
	assert(IS_VALID_CHANNEL(ch));
	*chan = ch;
	*play_id = sid >> 32ull;
}

INLINE int get_soundid_chan(SFXPlayID sid) {
	int chan;
	uint32_t play_id;
	unpack_sound_id(sid, &chan, &play_id);

	if(mixer.chan_play_ids[chan] == play_id) {
		return chan;
	}

	return -1;
}

static SFXPlayID play_sound_tracked(uint32_t chan, SFXImpl *isnd, int loops) {
	assert(IS_VALID_CHANNEL(chan));
	int actual_chan = Mix_PlayChannel(chan, isnd->ch, loops);

	if(actual_chan < 0) {
		log_error("Mix_PlayChannel() failed: %s", Mix_GetError());
		return 0;
	}

	Mix_UnregisterAllEffects(actual_chan);

	if(loops) {
		isnd->loopchan = actual_chan;
	} else {
		isnd->playchan = chan;
	}

	assert(IS_VALID_CHANNEL(actual_chan));
	chan = (uint32_t)actual_chan;
	uint32_t play_id = ++mixer.play_counter;
	mixer.chan_play_ids[chan] = play_id;

	return make_sound_id(chan, play_id);
}

static SFXPlayID audio_sdl2mixer_sfx_play(
	SFXImpl *isnd, AudioBackendSFXGroup group) {
	return play_sound_tracked(pick_channel(group, MAIN_CHANNEL_GROUP), isnd, 0);
}

static SFXPlayID audio_sdl2mixer_sfx_play_or_restart(
	SFXImpl *isnd, AudioBackendSFXGroup group) {
	int chan = isnd->playchan;

	if(chan < 0 || Mix_GetChunk(chan) != isnd->ch) {
		chan = pick_channel(group, MAIN_CHANNEL_GROUP);
	}

	return play_sound_tracked(chan, isnd, 0);
}

static SFXPlayID audio_sdl2mixer_sfx_loop(
	SFXImpl *isnd, AudioBackendSFXGroup group) {
	return play_sound_tracked(pick_channel(group, MAIN_CHANNEL_GROUP), isnd, -1);
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

	sample_t *data = stream;
	len /= sizeof(sample_t);

	for(int i = 0; i < len; i++) {
		e->counter++;
		data[i]*=1.-fmin(1,(double)e->counter/(double)e->duration);
	}
}

static bool fade_channel(int chan, int fadeout) {
	assert(IS_VALID_CHANNEL(chan));

	if(mixer.chan_play_ids[chan] == 0) {
		return false;
	}

	CustomFadeout *effect = calloc(1, sizeof(CustomFadeout));
	effect->counter = 0;
	effect->duration = (fadeout * AUDIO_FREQ) / 1000;
	Mix_ExpireChannel(chan, fadeout);
	Mix_RegisterEffect(chan, custom_fadeout_proc, custom_fadeout_free, effect);
	mixer.chan_play_ids[chan] = 0;

	return true;
}

static bool audio_sdl2mixer_sfx_stop_loop(SFXImpl *isnd) {
	if(isnd->loopchan < 0) {
		return false;
	}

	return fade_channel(isnd->loopchan, LOOPFADEOUT);
}

static bool audio_sdl2mixer_sfx_stop_id(SFXPlayID sid) {
	int chan = get_soundid_chan(sid);

	if(chan >= 0) {
		return fade_channel(chan, LOOPFADEOUT * 3);
	}

	return false;
}

static bool audio_sdl2mixer_sfx_pause_all(AudioBackendSFXGroup group) {
	int mixgroup = translate_group(group, -1);

	if(mixgroup == -1) {
		Mix_Pause(-1);
	} else {
		// why is there no Mix_PauseGroup?

		for(int i = mixer.groups[mixgroup].first; i < mixer.groups[mixgroup].first + mixer.groups[mixgroup].num; ++i) {
			Mix_Pause(i);
		}
	}

	return true;
}


static bool audio_sdl2mixer_sfx_resume_all(AudioBackendSFXGroup group) {
	int mixgroup = translate_group(group, -1);

	if(mixgroup == -1) {
		Mix_Resume(-1);
	} else {
		// why is there no Mix_ResumeGroup?

		for(int i = mixer.groups[mixgroup].first; i < mixer.groups[mixgroup].first + mixer.groups[mixgroup].num; ++i) {
			Mix_Resume(i);
		}
	}

	return true;
}

static bool audio_sdl2mixer_sfx_stop_all(AudioBackendSFXGroup group) {
	int mixgroup = translate_group(group, -1);

	if(mixgroup == -1) {
		Mix_HaltChannel(-1);
	} else {
		Mix_HaltGroup(mixgroup);
	}

	return true;
}

static bool audio_sdl2mixer_sfx_set_volume(SFXImpl *isnd, double gain) {
	Mix_VolumeChunk(isnd->ch, gain_to_mixvol(gain));
	return true;
}

// END SFX

// BEGIN LOAD

static BGM *audio_sdl2mixer_bgm_load(const char *vfspath) {
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

static void audio_sdl2mixer_bgm_unload(BGM *bgm) {
	splayer_lock(&mixer.bgm_player);
	if(mixer.bgm_player.stream == &bgm->stream) {
		splayer_halt(&mixer.bgm_player);
	}
	splayer_unlock(&mixer.bgm_player);

	astream_close(&bgm->stream);
	free(bgm);
}

static Mix_Chunk *read_chunk(AudioStream *astream, const char *vfspath) {
	SDL_AudioStream *pipe = astream_create_sdl_stream(astream, mixer.spec);

	ssize_t read;
	do {
		char buf[1 << 13];
		read = astream_read_into_sdl_stream(astream, pipe, sizeof(buf), buf, 0);
	} while(read > 0);

	astream_close(astream);

	if(read < 0) {
		SDL_FreeAudioStream(pipe);
		return NULL;
	}

	SDL_AudioStreamFlush(pipe);
	int resampled_bytes = SDL_AudioStreamAvailable(pipe);

	if(resampled_bytes <= 0) {
		log_error("SDL_AudioStream returned no data");
		SDL_FreeAudioStream(pipe);
		return NULL;
	}

	// WARNING: it's important to use the SDL_* allocation functions here for the pcm buffer,
	// so that SDL_mixer can free it properly later.

	uint8_t *pcm = SDL_calloc(1, resampled_bytes);

	SDL_AudioStreamGet(pipe, pcm, resampled_bytes);
	SDL_FreeAudioStream(pipe);

	Mix_Chunk *snd = Mix_QuickLoad_RAW(pcm, resampled_bytes);

	// hack to make SDL_mixer free the buffer for us
	snd->allocated = true;

	return snd;
}

static SFXImpl *audio_sdl2mixer_sfx_load(const char *vfspath) {
	SDL_RWops *rw = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	// SDL2_Mixer as of 2.0.4 is too dumb to actually try to load Opus chunks,
	// even if it was compiled with Opus support (which only works for music apparently /facepalm).
	// So we'll load them via our custom streaming API.

	Mix_Chunk *snd = NULL;
	AudioStream stream;

	if(astream_open(&stream, rw, vfspath)) {
		snd = read_chunk(&stream, vfspath);

		if(!snd) {
			// error message printed by read_chunk
			return NULL;
		}
	} else {
		snd = Mix_LoadWAV_RW(rw, true);

		if(!snd) {
			log_error("Mix_LoadWAV_RW() failed: %s (%s)", Mix_GetError(), vfspath);
			return NULL;
		}
	}

	SFXImpl *isnd = calloc(1, sizeof(*isnd));
	isnd->ch = snd;
	isnd->loopchan = -1;
	isnd->playchan = -1;

	log_debug("Loaded chunk from %s", vfspath);
	return isnd;
}

static void audio_sdl2mixer_sfx_unload(SFXImpl *isnd) {
	Mix_FreeChunk(isnd->ch);
	free(isnd);
}

// END LOAD

AudioBackend _a_backend_sdl2mixer = {
	.name = "sdl2mixer",
	.funcs = {
		.bgm_current = audio_sdl2mixer_bgm_current,
		.bgm_load = audio_sdl2mixer_bgm_load,
		.bgm_looping = audio_sdl2mixer_bgm_looping,
		.bgm_pause = audio_sdl2mixer_bgm_pause,
		.bgm_play = audio_sdl2mixer_bgm_play,
		.bgm_resume = audio_sdl2mixer_bgm_resume,
		.bgm_seek = audio_sdl2mixer_bgm_seek,
		.bgm_set_global_volume = audio_sdl2mixer_bgm_set_global_volume,
		.bgm_set_loop_point = audio_sdl2mixer_bgm_set_loop_point,
		.bgm_status = audio_sdl2mixer_bgm_status,
		.bgm_stop = audio_sdl2mixer_bgm_stop,
		.bgm_tell = audio_sdl2mixer_bgm_tell,
		.bgm_unload = audio_sdl2mixer_bgm_unload,
		.get_supported_bgm_exts = audio_sdl2mixer_get_supported_bgm_exts,
		.get_supported_sfx_exts = audio_sdl2mixer_get_supported_sfx_exts,
		.init = audio_sdl2mixer_init,
		.output_works = audio_sdl2mixer_output_works,
		.sfx_load = audio_sdl2mixer_sfx_load,
		.sfx_loop = audio_sdl2mixer_sfx_loop,
		.sfx_pause_all = audio_sdl2mixer_sfx_pause_all,
		.sfx_play = audio_sdl2mixer_sfx_play,
		.sfx_play_or_restart = audio_sdl2mixer_sfx_play_or_restart,
		.sfx_resume_all = audio_sdl2mixer_sfx_resume_all,
		.sfx_set_global_volume = audio_sdl2mixer_sfx_set_global_volume,
		.sfx_set_volume = audio_sdl2mixer_sfx_set_volume,
		.sfx_stop_all = audio_sdl2mixer_sfx_stop_all,
		.sfx_stop_id = audio_sdl2mixer_sfx_stop_id,
		.sfx_stop_loop = audio_sdl2mixer_sfx_stop_loop,
		.sfx_unload = audio_sdl2mixer_sfx_unload,
		.shutdown = audio_sdl2mixer_shutdown,

		.object = {
			.bgm = {
				.get_title = audio_sdl2mixer_obj_bgm_get_title,
				.get_artist = audio_sdl2mixer_obj_bgm_get_artist,
				.get_comment = audio_sdl2mixer_obj_bgm_get_comment,
				.get_duration = audio_sdl2mixer_obj_bgm_get_duration,
				.get_loop_start = audio_sdl2mixer_obj_bgm_get_loop_start,
			},
		},
	}
};
