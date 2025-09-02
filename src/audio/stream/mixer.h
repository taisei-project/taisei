/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stream.h"
#include "stream_pcm.h"
#include "player.h"
#include "../backend.h"

#define MIXER_NUM_BGM_CHANNELS          1
#define MIXER_NUM_SFX_MAIN_CHANNELS     28
#define MIXER_NUM_SFX_UI_CHANNELS       4
#define MIXER_NUM_SFX_CHANNELS          (MIXER_NUM_SFX_MAIN_CHANNELS + MIXER_NUM_SFX_UI_CHANNELS)

typedef struct MixerSFXImpl {
	float gain;
	AudioStreamSpec spec;
	size_t pcm_size;
	uint8_t pcm[];
} MixerSFXImpl;

typedef struct MixerBGMImpl {
	AudioStream stream;
} MixerBGMImpl;

static_assert(offsetof(MixerBGMImpl, stream) == 0, "");

typedef struct Mixer {
	StreamPlayer players[NUM_CHANGROUPS];
	StaticPCMAudioStream sfx_streams[MIXER_NUM_SFX_CHANNELS];
	AudioStreamSpec spec;
} Mixer;

const char *const *mixer_get_supported_exts(uint *out_numexts) attr_nonnull_all;
bool mixer_group_get_channels_range(AudioChannelGroup group, int *first, int *last) attr_nonnull_all;

bool mixer_init(Mixer *mx, const AudioStreamSpec *spec) attr_nonnull(1);
void mixer_shutdown(Mixer *mx) attr_nonnull(1);

bool mixer_bgm_play(Mixer *mx, MixerBGMImpl *bgm, bool loop, double position, double fadein) attr_nonnull(1, 2);
bool mixer_bgm_stop(Mixer *mx, double fadeout) attr_nonnull(1);
bool mixer_bgm_pause(Mixer *mx) attr_nonnull(1);
bool mixer_bgm_resume(Mixer *mx) attr_nonnull(1);
double mixer_bgm_seek(Mixer *mx, double pos) attr_nonnull(1);
double mixer_bgm_tell(Mixer *mx) attr_nonnull(1);
BGMStatus mixer_bgm_status(Mixer *mx) attr_nonnull(1);
MixerBGMImpl *mixer_bgm_current(Mixer *mx) attr_nonnull(1);
bool mixer_bgm_looping(Mixer *mx) attr_nonnull(1);

AudioBackendChannel mixer_sfx_play(
	Mixer *mx,
	MixerSFXImpl *sfx, AudioChannelGroup group, AudioBackendChannel flat_chan, bool loop
) attr_nonnull(1, 2);
bool mixer_chan_stop(Mixer *mx, AudioBackendChannel chan, double fadeout) attr_nonnull(1);
bool mixer_chan_unstop(Mixer *mx, AudioBackendChannel chan, double fadein) attr_nonnull(1);

bool mixer_group_pause(Mixer *mx, AudioChannelGroup group) attr_nonnull(1);
bool mixer_group_resume(Mixer *mx, AudioChannelGroup group) attr_nonnull(1);
bool mixer_group_set_volume(Mixer *mx, AudioChannelGroup group, double vol) attr_nonnull(1);
bool mixer_group_stop(Mixer *mx, AudioChannelGroup group, double fadeout) attr_nonnull(1);

MixerBGMImpl *mixerbgm_load(const char *vfspath);
void mixerbgm_unload(MixerBGMImpl *bgm) attr_nonnull(1);
const char *mixerbgm_get_title(MixerBGMImpl *bgm) attr_nonnull(1);
const char *mixerbgm_get_artist(MixerBGMImpl *bgm) attr_nonnull(1);
const char *mixerbgm_get_comment(MixerBGMImpl *bgm) attr_nonnull(1);
double mixerbgm_get_duration(MixerBGMImpl *bgm) attr_nonnull(1);
double mixerbgm_get_loop_start(MixerBGMImpl *bgm) attr_nonnull(1);

MixerSFXImpl *mixersfx_load(const char *vfspath, const AudioStreamSpec *spec);
void mixersfx_unload(MixerSFXImpl *sfx) attr_nonnull(1);
bool mixersfx_set_volume(MixerSFXImpl *sfx, double vol) attr_nonnull(1);

void mixer_notify_bgm_unload(Mixer *mx, MixerBGMImpl *bgm) attr_nonnull_all;
void mixer_notify_sfx_unload(Mixer *mx, MixerSFXImpl *sfx) attr_nonnull_all;

void mixer_process(Mixer *mx, size_t bufsize, void *buffer);
