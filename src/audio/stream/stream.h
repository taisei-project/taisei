/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include <SDL_audio.h>

typedef struct AudioStream AudioStream;
typedef struct AudioStreamProcs AudioStreamProcs;
typedef struct AudioStreamSpec AudioStreamSpec;

typedef enum AudioStreamMetaTag {
	STREAM_META_TITLE,
	STREAM_META_ARTIST,
	STREAM_META_COMMENT,
} AudioStreamMetaTag;

struct AudioStreamProcs {
	ssize_t (*read)(AudioStream *s, size_t bufsize, void *buffer);
	ssize_t (*tell)(AudioStream *s);
	ssize_t (*seek)(AudioStream *s, size_t pos);
	const char *(*meta)(AudioStream *s, AudioStreamMetaTag tag);
	void (*free)(AudioStream *s);
};

struct AudioStreamSpec {
	SDL_AudioFormat sample_format;  // typedef'd to uint16_t
	uint16_t channels;
	uint32_t sample_rate;
	uint32_t frame_size;
};

struct AudioStream {
	AudioStreamProcs *procs;
	void *opaque;
	AudioStreamSpec spec;
	int32_t length;
	int32_t loop_start;
};

typedef enum AudioStreamReadFlags {
	ASTREAM_READ_MAX_FILL = (1 << 0),
	ASTREAM_READ_LOOP = (1 << 1),
} AudioStreamReadFlags;


bool astream_open(AudioStream *stream, SDL_RWops *rwops, const char *filename) attr_nonnull_all;
void astream_close(AudioStream *stream) attr_nonnull_all;
ssize_t astream_read(AudioStream *stream, size_t bufsize, void *buffer, AudioStreamReadFlags flags) attr_nonnull_all;
ssize_t astream_read_into_sdl_stream(AudioStream *stream, SDL_AudioStream *sdlstream, size_t bufsize, void *buffer, AudioStreamReadFlags flags) attr_nonnull_all;
ssize_t astream_seek(AudioStream *stream, size_t pos) attr_nonnull_all;
ssize_t astream_tell(AudioStream *stream) attr_nonnull_all;
const char *astream_get_meta_tag(AudioStream *stream, AudioStreamMetaTag tag) attr_nonnull_all;

SDL_AudioStream *astream_create_sdl_stream(AudioStream *source, const AudioStreamSpec *dest_spec);
bool astream_crystalize(AudioStream *src, const AudioStreamSpec *spec, size_t buffer_size, void *buffer);

ssize_t astream_util_time_to_offset(AudioStream *stream, double t) attr_nonnull_all;
double astream_util_offset_to_time(AudioStream *stream, ssize_t ofs) attr_nonnull_all;

AudioStreamSpec astream_spec(SDL_AudioFormat sample_format, uint channels, uint sample_rate);
bool astream_spec_equals(const AudioStreamSpec *s1, const AudioStreamSpec *s2);

ssize_t astream_util_resampled_buffer_size(const AudioStream *src, const AudioStreamSpec *spec);
