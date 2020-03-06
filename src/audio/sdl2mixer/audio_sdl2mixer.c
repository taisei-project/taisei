/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <SDL_mixer.h>
#include <opusfile.h>

#include "../backend.h"
#include "global.h"
#include "list.h"
#include "util/opuscruft.h"

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


struct SoundImpl {
	Mix_Chunk *ch;

	// I needed to add this for supporting loop sounds since Mixer doesn’t remember
	// what channel a sound is playing on.  - laochailan
	int loopchan; // channel the sound may be looping on. -1 if not looping
	int playchan; // channel the sound was last played on (looping does NOT set this). -1 if never played
};

struct MusicImpl {
	Mix_Music *loop;
	double loop_point;
};

static struct {
	Mix_Music *next_loop;
	double next_loop_point;
	uint32_t play_counter;
	uint32_t chan_play_ids[AUDIO_CHANNELS];

	struct {
		uchar first;
		uchar num;
	} groups[2];
} mixer;

#define IS_VALID_CHANNEL(chan) ((uint)(chan) < AUDIO_CHANNELS)

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

	if(!(Mix_Init(MIX_INIT_OPUS) & MIX_INIT_OPUS)) {
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

	mixer.groups[UI_CHANNEL_GROUP].first = 0;
	mixer.groups[UI_CHANNEL_GROUP].num = ui_channels;
	mixer.groups[MAIN_CHANNEL_GROUP].first = ui_channels;
	mixer.groups[MAIN_CHANNEL_GROUP].num = main_channels;

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

	log_debug("%s stopped playing", mixer.next_loop_point ? "Loop" : "Intro");

	if(mixer.next_loop) {
		if(Mix_PlayMusic(mixer.next_loop, mixer.next_loop_point ? 0 : -1) == -1) {
			log_error("Mix_PlayMusic() failed: %s", Mix_GetError());
		} else if(mixer.next_loop_point >= 0) {
			Mix_SetMusicPosition(mixer.next_loop_point);
		} else {
			mixer.next_loop = NULL;
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
	mixer.next_loop_point = imus->loop_point;

	if(mixer.next_loop_point >= 0) {
		loops = 0;
		mixer.next_loop = imus->loop;
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

	// TODO Handle looping here.
	// Unfortunately SDL2_Mixer will not tell us the total length of the file,
	// so a hack is required here.

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

INLINE SoundID make_sound_id(int chan, uint32_t play_id) {
	assert(IS_VALID_CHANNEL(chan));
	return ((uint32_t)chan + 1) | ((uint64_t)play_id << 32ull);
}

INLINE void unpack_sound_id(SoundID sid, int *chan, uint32_t *play_id) {
	int ch = (int)((uint32_t)(sid & 0xffffffff) - 1);
	assert(IS_VALID_CHANNEL(ch));
	*chan = ch;
	*play_id = sid >> 32ull;
}

INLINE int get_soundid_chan(SoundID sid) {
	int chan;
	uint32_t play_id;
	unpack_sound_id(sid, &chan, &play_id);

	if(mixer.chan_play_ids[chan] == play_id) {
		return chan;
	}

	return -1;
}

static SoundID play_sound_tracked(uint32_t chan, SoundImpl *isnd, int loops) {
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

static SoundID audio_sdl2mixer_sound_play(SoundImpl *isnd, AudioBackendSoundGroup group) {
	return play_sound_tracked(pick_channel(group, MAIN_CHANNEL_GROUP), isnd, 0);
}

static SoundID audio_sdl2mixer_sound_play_or_restart(SoundImpl *isnd, AudioBackendSoundGroup group) {
	int chan = isnd->playchan;

	if(chan < 0 || Mix_GetChunk(chan) != isnd->ch) {
		chan = pick_channel(group, MAIN_CHANNEL_GROUP);
	}

	return play_sound_tracked(chan, isnd, 0);
}

static SoundID audio_sdl2mixer_sound_loop(SoundImpl *isnd, AudioBackendSoundGroup group) {
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
		data[i]*=1.-min(1,(double)e->counter/(double)e->duration);
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

static bool audio_sdl2mixer_sound_stop_loop(SoundImpl *isnd) {
	if(isnd->loopchan < 0) {
		return false;
	}

	return fade_channel(isnd->loopchan, LOOPFADEOUT);
}

static bool audio_sdl2mixer_sound_stop_id(SoundID sid) {
	int chan = get_soundid_chan(sid);

	if(chan >= 0) {
		return fade_channel(chan, LOOPFADEOUT * 3);
	}

	return false;
}

static bool audio_sdl2mixer_sound_pause_all(AudioBackendSoundGroup group) {
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


static bool audio_sdl2mixer_sound_resume_all(AudioBackendSoundGroup group) {
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
	static const char *const exts[] = { ".opus", ".ogg", ".wav" };
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
		log_error("Mix_LoadMUS_RW() failed: %s (%s)", Mix_GetError(), vfspath);
		return NULL;
	}

	MusicImpl *imus = calloc(1, sizeof(*imus));
	imus->loop = mus;

	log_debug("Loaded stream from %s", vfspath);
	return imus;
}

static void audio_sdl2mixer_music_unload(MusicImpl *imus) {
	Mix_FreeMusic(imus->loop);
	free(imus);
}

static OggOpusFile *try_load_opus(SDL_RWops *rw) {
	uint8_t buf[128];
	SDL_RWread(rw, buf, 1, sizeof(buf));
	int error = op_test(NULL, buf, sizeof(buf));

	if(error != 0) {
		log_debug("op_test() failed: %i", error);
		goto fail;
	}

	OggOpusFile *opus = op_open_callbacks(rw, &opusutil_rwops_callbacks, buf, sizeof(buf), &error);

	if(opus == NULL) {
		log_debug("op_open_callbacks() failed: %i", error);
		goto fail;
	}

	return opus;

fail:
	SDL_RWseek(rw, 0, RW_SEEK_SET);
	return NULL;
}

static Mix_Chunk *read_opus_chunk(OggOpusFile *opus, const char *vfspath) {
	int mix_frequency = 0;
	int mix_channels;
	uint16_t mix_format = 0;
	Mix_QuerySpec(&mix_frequency, &mix_format, &mix_channels);

	SDL_AudioStream *stream = SDL_NewAudioStream(AUDIO_FORMAT, 2, 48000, mix_format, mix_channels, mix_frequency);

	for(;;) {
		sample_t read_buf[1920];
		int r = OPUS_READ_SAMPLES_STEREO(opus, read_buf, sizeof(read_buf));

		if(r == OP_HOLE) {
			continue;
		}

		if(r < 0) {
			log_error("Opus decode error %i (%s)", r, vfspath);
			SDL_FreeAudioStream(stream);
			op_free(opus);
			return NULL;
		}

		if(r == 0) {
			op_free(opus);
			SDL_AudioStreamFlush(stream);
			break;
		}

		// r is the number of samples read !!PER CHANNEL!!
		// since we tell Opusfile to downmix to stereo, exactly 2 channels are guaranteed.
		int read = r * 2;

		if(SDL_AudioStreamPut(stream, read_buf, read * sizeof(*read_buf)) < 0) {
			log_sdl_error(LOG_ERROR, "SDL_AudioStreamPut");
			SDL_FreeAudioStream(stream);
			op_free(opus);
			return NULL;
		}
	}

	int resampled_bytes = SDL_AudioStreamAvailable(stream);

	if(resampled_bytes <= 0) {
		log_error("SDL_AudioStream returned no data");
		SDL_FreeAudioStream(stream);
		return NULL;
	}

	// WARNING: it's important to use the SDL_* allocation functions here for the pcm buffer,
	// so that SDL_mixer can free it properly later.

	uint8_t *pcm = SDL_calloc(1, resampled_bytes);

	SDL_AudioStreamGet(stream, pcm, resampled_bytes);
	SDL_FreeAudioStream(stream);

	Mix_Chunk *snd = Mix_QuickLoad_RAW(pcm, resampled_bytes);

	// hack to make SDL_mixer free the buffer for us
	snd->allocated = true;

	return snd;
}

static SoundImpl *audio_sdl2mixer_sound_load(const char *vfspath) {
	SDL_RWops *rw = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	// SDL2_Mixer as of 2.0.4 is too dumb to actually try to load Opus chunks,
	// even if it was compiled with Opus support (which only works for music apparently /facepalm)
	// So we'll try to load them via opusfile manually.

	Mix_Chunk *snd = NULL;
	OggOpusFile *opus = try_load_opus(rw);

	if(opus) {
		snd = read_opus_chunk(opus, vfspath);

		if(!snd) {
			// error message printed by read_opus_chunk
			return NULL;
		}
	} else {
		snd = Mix_LoadWAV_RW(rw, true);

		if(!snd) {
			log_error("Mix_LoadWAV_RW() failed: %s (%s)", Mix_GetError(), vfspath);
			return NULL;
		}
	}

	SoundImpl *isnd = calloc(1, sizeof(*isnd));
	isnd->ch = snd;
	isnd->loopchan = -1;
	isnd->playchan = -1;

	log_debug("Loaded chunk from %s", vfspath);
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
		.sound_stop_id = audio_sdl2mixer_sound_stop_id,
		.sound_unload = audio_sdl2mixer_sound_unload,
	}
};
