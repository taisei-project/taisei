/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stream.h"
#include "list.h"

typedef struct StreamPlayerChannel StreamPlayerChannel;
typedef struct StreamPlayer StreamPlayer;

struct StreamPlayerChannel {
	LIST_INTERFACE(StreamPlayerChannel);
	AudioStream *stream;
	SDL_AudioStream *pipe;
	AudioStreamSpec src_spec;
	float gain;
	struct {
		float target;
		float gain;
		float step;
		int num_steps;
	} fade;
	bool looping;
	bool paused;
};

struct StreamPlayer {
	StreamPlayerChannel *channels;
	LIST_ANCHOR(StreamPlayerChannel) channel_history;
	AudioStreamSpec dst_spec;
	float gain;
	int num_channels;
	bool paused;
};

bool splayer_init(StreamPlayer *plr, int num_channels, const AudioStreamSpec *dst_spec) attr_nonnull_all;
void splayer_shutdown(StreamPlayer *plr) attr_nonnull_all;
void splayer_process(StreamPlayer *plr, size_t bufsize, void *buffer) attr_nonnull_all;
bool splayer_play(StreamPlayer *plr, int chan, AudioStream *stream, bool loop, float gain, double position, double fadein) attr_nonnull_all;
bool splayer_pause(StreamPlayer *plr, int chan) attr_nonnull_all;
bool splayer_resume(StreamPlayer *plr, int chan) attr_nonnull_all;
void splayer_halt(StreamPlayer *plr, int chan) attr_nonnull_all;
bool splayer_fadeout(StreamPlayer *plr, int chan, double fadeout) attr_nonnull_all;
bool splayer_fadein(StreamPlayer *plr, int chan, double fadetime) attr_nonnull_all;
double splayer_seek(StreamPlayer *plr, int chan, double position) attr_nonnull_all;
double splayer_tell(StreamPlayer *plr, int chan) attr_nonnull_all;
bool splayer_is_looping(StreamPlayer *plr, int chan) attr_nonnull_all;
bool splayer_global_pause(StreamPlayer *plr) attr_nonnull_all;
bool splayer_global_resume(StreamPlayer *plr) attr_nonnull_all;
int splayer_pick_channel(StreamPlayer *plr) attr_nonnull_all;

#include "audio/audio.h"

BGMStatus splayer_util_bgmstatus(StreamPlayer *plr, int chan) attr_nonnull_all;
