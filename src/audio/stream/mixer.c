/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "mixer.h"

#include "util.h"
#include "../backend.h"

// BEGIN UTIL

#define GPLR(mx, g) ({ \
	uint _idx = g; \
	assume(_idx < ARRAY_SIZE(mx->players)); \
	mx->players + _idx; \
})

#define IS_SFX_CHANNEL(chan) ((uint)(chan) < MIXER_NUM_SFX_CHANNELS)

static void flatchan_to_groupchan(
	AudioBackendChannel fchan, AudioChannelGroup *out_group, int *out_chan
) {
	assert(fchan >= 0);
	assert(fchan < MIXER_NUM_SFX_CHANNELS);

	if(fchan >= MIXER_NUM_SFX_MAIN_CHANNELS) {
		*out_group = CHANGROUP_SFX_UI;
		*out_chan = fchan - MIXER_NUM_SFX_MAIN_CHANNELS;
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
			return chan + MIXER_NUM_SFX_MAIN_CHANNELS;

		default:
			UNREACHABLE;
	}
}

// END UTIL

// BEGIN STATIC QUERIES

const char *const *mixer_get_supported_exts(uint *out_numexts) {
	// TODO figure this out dynamically
	static const char *const exts[] = { ".opus" };
	*out_numexts = ARRAY_SIZE(exts);
	return exts;
}

bool mixer_group_get_channels_range(AudioChannelGroup group, int *first, int *last) {
	switch(group) {
		case CHANGROUP_SFX_GAME:
			*first = 0;
			*last = MIXER_NUM_SFX_MAIN_CHANNELS - 1;
			return true;

		case CHANGROUP_SFX_UI:
			*first = MIXER_NUM_SFX_MAIN_CHANNELS;
			*last = MIXER_NUM_SFX_MAIN_CHANNELS + MIXER_NUM_SFX_UI_CHANNELS - 1;
			return true;

		case CHANGROUP_BGM:
			*first = MIXER_NUM_SFX_CHANNELS;
			*last = MIXER_NUM_SFX_CHANNELS;
			return true;

		default:
			UNREACHABLE;
	}
}

// END STATIC QUERIES

// BEGIN INIT/SHUTDOWN

bool mixer_init(Mixer *mx, const AudioStreamSpec *spec) {
	memset(mx, 0, sizeof(*mx));

	if(!splayer_init(mx->players + CHANGROUP_BGM, MIXER_NUM_BGM_CHANNELS, spec)) {
		log_error("splayer_init() failed");
		return false;
	}

	if(!splayer_init(mx->players + CHANGROUP_SFX_GAME, MIXER_NUM_SFX_MAIN_CHANNELS, spec)) {
		log_error("splayer_init() failed");
		return false;
	}

	if(!splayer_init(mx->players + CHANGROUP_SFX_UI, MIXER_NUM_SFX_UI_CHANNELS, spec)) {
		log_error("splayer_init() failed");
		return false;
	}

	for(int i = 0; i < ARRAY_SIZE(mx->sfx_streams); ++i) {
		astream_pcm_static_init(mx->sfx_streams + i);
	}

	mx->spec = *spec;
	return true;
}

void mixer_shutdown(Mixer *mx) {
	for(int i = 0; i < ARRAY_SIZE(mx->players); ++i) {
		StreamPlayer *plr = mx->players + i;

		if(plr->channels) {
			splayer_shutdown(plr);
			memset(plr, 0, sizeof(*plr));
		}
	}
}

// END INIT/SHUTDOWN

// BEGIN BGM

bool mixer_bgm_play(Mixer *mx, MixerBGMImpl *bgm, bool loop, double position, double fadein) {
	return splayer_play(mx->players + CHANGROUP_BGM, 0, &bgm->stream, loop, 1, position, fadein);
}

bool mixer_bgm_stop(Mixer *mx, double fadeout) {
	StreamPlayer *plr = mx->players + CHANGROUP_BGM;

	if(splayer_util_bgmstatus(plr, 0) == BGM_STOPPED) {
		log_warn("BGM is already stopped");
		return false;
	}

	if(fadeout > 0) {
		splayer_fadeout(plr, 0, fadeout);
	} else {
		splayer_halt(plr, 0);
	}

	return true;
}

bool mixer_bgm_pause(Mixer *mx) {
	return splayer_pause(GPLR(mx, CHANGROUP_BGM), 0);
}

bool mixer_bgm_resume(Mixer *mx) {
	return splayer_resume(GPLR(mx, CHANGROUP_BGM), 0);
}

double mixer_bgm_seek(Mixer *mx, double pos) {
	return splayer_seek(GPLR(mx, CHANGROUP_BGM), 0, pos);
}

double mixer_bgm_tell(Mixer *mx) {
	return splayer_tell(GPLR(mx, CHANGROUP_BGM), 0);
}

BGMStatus mixer_bgm_status(Mixer *mx) {
	return splayer_util_bgmstatus(GPLR(mx, CHANGROUP_BGM), 0);
}

MixerBGMImpl *mixer_bgm_current(Mixer *mx) {
	StreamPlayer *plr = GPLR(mx, CHANGROUP_BGM);

	if(splayer_util_bgmstatus(plr, 0) != BGM_STOPPED) {
		return UNION_CAST(AudioStream*, MixerBGMImpl*, plr->channels[0].stream);
	}

	return NULL;
}

bool mixer_bgm_looping(Mixer *mx) {
	StreamPlayer *plr = GPLR(mx, CHANGROUP_BGM);

	if(splayer_util_bgmstatus(plr, 0) == BGM_STOPPED) {
		log_warn("BGM is stopped");
		return false;
	}

	return splayer_is_looping(plr, 0);
}

// END BGM

// BEGIN BGM OBJECT

const char *mixerbgm_get_title(MixerBGMImpl *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_TITLE);
}

const char *mixerbgm_get_artist(MixerBGMImpl *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_ARTIST);
}

const char *mixerbgm_get_comment(MixerBGMImpl *bgm) {
	return astream_get_meta_tag(&bgm->stream, STREAM_META_COMMENT);
}

double mixerbgm_get_duration(MixerBGMImpl *bgm) {
	return astream_util_offset_to_time(&bgm->stream, bgm->stream.length);
}

double mixerbgm_get_loop_start(MixerBGMImpl *bgm) {
	return astream_util_offset_to_time(&bgm->stream, bgm->stream.loop_start);
}

// END BGM OBJECT

// BEGIN SFX

AudioBackendChannel mixer_sfx_play(
	Mixer *mx,
	MixerSFXImpl *sfx, AudioChannelGroup group, AudioBackendChannel flat_chan, bool loop
) {
	int chan;
	StreamPlayer *plr;

	if(flat_chan != AUDIO_BACKEND_CHANNEL_INVALID) {
		assume(IS_SFX_CHANNEL(flat_chan));
		assume(group == CHANGROUP_ANY);
		flatchan_to_groupchan(flat_chan, &group, &chan);
		plr = GPLR(mx, group);
	} else {
		plr = GPLR(mx, group);
		chan = splayer_pick_channel(plr);
		flat_chan = groupchan_to_flatchan(group, chan);
	}

	assume((uint)flat_chan < ARRAY_SIZE(mx->sfx_streams));
	StaticPCMAudioStream *stream = mx->sfx_streams + flat_chan;

	if(
		astream_pcm_reopen(&stream->astream, &mx->spec, sfx->pcm_size, sfx->pcm, 0) &&
		splayer_play(plr, chan, &stream->astream, loop, sfx->gain, 0, 0)
	) {
		return flat_chan;
	}

	return AUDIO_BACKEND_CHANNEL_INVALID;
}

bool mixer_chan_stop(Mixer *mx, AudioBackendChannel chan, double fadeout) {
	AudioChannelGroup g;
	int gch;
	flatchan_to_groupchan(chan, &g, &gch);
	StreamPlayer *plr = GPLR(mx, g);

	if(fadeout) {
		return splayer_fadeout(plr, gch, fadeout);
	} else {
		splayer_halt(plr, gch);
		return true;
	}
}

bool mixer_chan_unstop(Mixer *mx, AudioBackendChannel chan, double fadein) {
	AudioChannelGroup g;
	int gch;
	flatchan_to_groupchan(chan, &g, &gch);
	StreamPlayer *plr = GPLR(mx, g);
	bool ok = false;

	if(plr->channels[gch].stream) {
		ok = true;
		if(plr->channels[gch].fade.gain < 1) {
			if(!splayer_fadein(plr, gch, fadein)) {
				ok = false;
			}
		}
	}

	return ok;
}

// END SFX

// BEGIN SFX OBJECT

bool mixersfx_set_volume(MixerSFXImpl *sfx, double vol) {
	sfx->gain = vol;
	return true;
}

// END SFX OBJECT

// BEGIN GROUPS

bool mixer_group_pause(Mixer *mx, AudioChannelGroup group) {
	return splayer_global_pause(GPLR(mx, group));
}

bool mixer_group_resume(Mixer *mx, AudioChannelGroup group) {
	return splayer_global_resume(GPLR(mx, group));
}

bool mixer_group_set_volume(Mixer *mx, AudioChannelGroup group, double vol) {
	GPLR(mx, group)->gain = vol;
	return true;
}

bool mixer_group_stop(Mixer *mx, AudioChannelGroup group, double fadeout) {
	StreamPlayer *plr = GPLR(mx, group);

	bool ok = true;

	for(int i = 0; i < plr->num_channels; ++i) {
		if(fadeout > 0) {
			if(!splayer_fadeout(plr, i, fadeout)) {
				ok = false;
			}
		} else {
			splayer_halt(plr, i);
		}
	}

	return ok;
}

// END GROUPS

// BEGIN LOAD

MixerBGMImpl *mixerbgm_load(const char *vfspath) {
	SDL_RWops *rw = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	auto bgm = ALLOC(MixerBGMImpl);

	if(!astream_open(&bgm->stream, rw, vfspath)) {
		SDL_RWclose(rw);
		mem_free(bgm);
		return NULL;
	}

	log_debug("Loaded stream from %s", vfspath);
	return bgm;
}

void mixerbgm_unload(MixerBGMImpl *bgm) {
	astream_close(&bgm->stream);
	mem_free(bgm);
}

void mixer_notify_bgm_unload(Mixer *mx, MixerBGMImpl *bgm) {
	StreamPlayer *plr = GPLR(mx, CHANGROUP_BGM);

	if(plr->channels[0].stream == &bgm->stream) {
		splayer_halt(plr, 0);
	}
}

MixerSFXImpl *mixersfx_load(const char *vfspath, const AudioStreamSpec *spec) {
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

	ssize_t pcm_size = astream_util_resampled_buffer_size(&stream, spec);

	if(pcm_size < 0) {
		astream_close(&stream);
		return NULL;
	}

	assert(pcm_size <= INT32_MAX);

	auto isnd = ALLOC_FLEX(MixerSFXImpl, pcm_size);

	bool ok = astream_crystalize(&stream, spec, pcm_size, &isnd->pcm);
	astream_close(&stream);

	if(!ok) {
		mem_free(isnd);
		return NULL;
	}

	log_debug("Loaded SFX from %s", vfspath);

	isnd->pcm_size = pcm_size;
	return isnd;
}

void mixersfx_unload(MixerSFXImpl *sfx) {
	mem_free(sfx);
}

void mixer_notify_sfx_unload(Mixer *mx, MixerSFXImpl *sfx) {
	for(int i = 0; i < ARRAY_SIZE(mx->sfx_streams); ++i) {
		StaticPCMAudioStream *s = mx->sfx_streams + i;

		if(s->ctx.start == sfx->pcm) {
			astream_close(&s->astream);
		}
	}
}

// END LOAD

// BEGIN PROCESS

void mixer_process(Mixer *mx, size_t bufsize, void *buffer) {
	for(int i = 0; i < ARRAY_SIZE(mx->players); ++i) {
		StreamPlayer *plr = mx->players + i;
		splayer_process(plr, bufsize, buffer);
	}
}

// END PROCESS
