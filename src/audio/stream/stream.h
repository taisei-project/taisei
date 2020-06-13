/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_audio_stream_stream_h
#define IGUARD_audio_stream_stream_h

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
	SDL_AudioFormat sample_format;
	int sample_rate;
	int channels;
};

struct AudioStream {
	AudioStreamProcs procs;
	void *opaque;
	AudioStreamSpec spec;
	ssize_t length;
	ssize_t loop_start;
};

typedef enum AudioStreamReadFlags {
	ASTREAM_READ_MAX_FILL = (1 << 0),
	ASTREAM_READ_LOOP = (1 << 1),
} AudioStreamReadFlags;

SDL_AudioStream *astream_create_sdl_stream(AudioStream *source, AudioStreamSpec dest_spec);

bool astream_open(AudioStream *stream, SDL_RWops *rwops, const char *filename) attr_nonnull_all;
void astream_close(AudioStream *stream) attr_nonnull_all;
ssize_t astream_read(AudioStream *stream, size_t bufsize, void *buffer, AudioStreamReadFlags flags) attr_nonnull_all;
ssize_t astream_read_into_sdl_stream(AudioStream *stream, SDL_AudioStream *sdlstream, size_t bufsize, void *buffer, AudioStreamReadFlags flags) attr_nonnull_all;
ssize_t astream_seek(AudioStream *stream, size_t pos) attr_nonnull_all;
ssize_t astream_tell(AudioStream *stream) attr_nonnull_all;
const char *astream_get_meta_tag(AudioStream *stream, AudioStreamMetaTag tag) attr_nonnull_all;

ssize_t astream_util_time_to_offset(AudioStream *stream, double t) attr_nonnull_all;
double astream_util_offset_to_time(AudioStream *stream, ssize_t ofs) attr_nonnull_all;

bool astream_spec_equals(const AudioStreamSpec *s1, const AudioStreamSpec *s2);

#endif // IGUARD_audio_stream_stream_h
