/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_audio_stream_player_h
#define IGUARD_audio_stream_player_h

#include "taisei.h"

#include "stream.h"

typedef struct StreamPlayer {
	AudioStream *stream;
	SDL_mutex *mutex;
	SDL_AudioStream *pipe;
	AudioStreamSpec src_spec;
	AudioStreamSpec dst_spec;
	float gain;
	float fade_gain;
	float fade_target;
	float fade_step;
	bool looping;
	bool paused;
} StreamPlayer;

typedef enum StreamPlayerProcessFlags {
	SPLAYER_SILENCE_PAD = (1 << 0),
} StreamPlayerProcessFlags;

bool splayer_init(StreamPlayer *plr, const AudioStreamSpec *dst_spec) attr_nonnull_all;
void splayer_lock(StreamPlayer *plr) attr_nonnull_all;
void splayer_unlock(StreamPlayer *plr) attr_nonnull_all;
void splayer_shutdown(StreamPlayer *plr) attr_nonnull_all;
size_t splayer_process(StreamPlayer *plr, size_t bufsize, void *buffer, StreamPlayerProcessFlags flags) attr_nonnull_all;
bool splayer_play(StreamPlayer *plr, AudioStream *stream, bool loop, double position, double fadein) attr_nonnull_all;
bool splayer_pause(StreamPlayer *plr) attr_nonnull_all;
bool splayer_resume(StreamPlayer *plr) attr_nonnull_all;
void splayer_halt(StreamPlayer *plr) attr_nonnull_all;
bool splayer_fadeout(StreamPlayer *plr, double fadeout) attr_nonnull_all;
double splayer_seek(StreamPlayer *plr, double position) attr_nonnull_all;
double splayer_tell(StreamPlayer *plr) attr_nonnull_all;

#include "audio/audio.h"

BGMStatus splayer_util_bgmstatus(StreamPlayer *plr) attr_nonnull_all;

#endif // IGUARD_audio_stream_player_h
