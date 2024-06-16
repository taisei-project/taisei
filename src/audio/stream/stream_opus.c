/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stream_opus.h"

#include "log.h"
#include "util.h"

#include <opusfile.h>

#define OPUS_SAMPLE_RATE 48000

static ssize_t astream_opus_read(AudioStream *s, size_t bufsize, void *buffer) {
	OggOpusFile *of = s->opaque;

	int r;
	do {
		r = op_read_float_stereo(of, buffer, bufsize / sizeof(float));
	} while(r == OP_HOLE);

	if(r < 0) {
		log_error("op_read_float_stereo() failed (error %i)", r);
		return -1;
	}

	// r == num. samples read per channel
	return r * 2 * sizeof(float);
}

static ssize_t astream_opus_tell(AudioStream *s) {
	ssize_t r = op_pcm_tell(s->opaque);

	if(r < 0) {
		log_error("op_pcm_tell() failed (error %zi)", r);
		return -1;
	}

	return r;
}

static ssize_t astream_opus_seek(AudioStream *s, size_t pos) {
	ssize_t r = op_pcm_seek(s->opaque, pos);

	if(r < 0) {
		log_error("op_pcm_seek() failed (error %zi)", r);
		return -1;
	}

	return r;
}

static void astream_opus_free(AudioStream *s) {
	op_free(s->opaque);
}

static const char *get_opus_tag(OggOpusFile *of, const char *tag) {
	const OpusTags *tags = op_tags(of, -1);

	if(UNLIKELY(tags == NULL)) {
		log_error("op_tags() returned NULL");
		return NULL;
	}

	int tag_len = strlen(tag);
	assert(tag[tag_len - 1] == '=');

	for(int i = 0; i < tags->comments; ++i) {
		const char *comment = tags->user_comments[i];

		if(strlen(comment) < tag_len) {
			continue;
		}

		if(!SDL_strncasecmp(comment, tag, tag_len)) {
			return comment + tag_len;
		}
	}

	return NULL;
}

static const char *astream_opus_meta(AudioStream *s, AudioStreamMetaTag tag) {
	OggOpusFile *of = s->opaque;

	static const char *tag_map[] = {
		[STREAM_META_ARTIST]  = "ARTIST=",
		[STREAM_META_TITLE]   = "TITLE=",
		[STREAM_META_COMMENT] = "DESCRIPTION=",
	};

	attr_unused uint idx = tag;
	assert(idx < ARRAY_SIZE(tag_map));

	return get_opus_tag(of, tag_map[tag]);
}

static AudioStreamProcs astream_opus_procs = {
	.free = astream_opus_free,
	.meta = astream_opus_meta,
	.read = astream_opus_read,
	.seek = astream_opus_seek,
	.tell = astream_opus_tell,
};

static bool astream_opus_init(AudioStream *stream, OggOpusFile *opus) {
	if(!op_seekable(opus)) {
		log_error("Opus stream is not seekable");
		return false;
	}

	ssize_t length = op_pcm_total(opus, -1);

	if(length < 0) {
		log_error("Can't determine length of Opus stream");
		return false;
	}

	if(length > INT32_MAX) {
		log_error("Opus stream is too large");
		return false;
	}

	stream->length = length;
	stream->spec = astream_spec(AUDIO_F32SYS, 2, OPUS_SAMPLE_RATE);
	stream->procs = &astream_opus_procs;
	stream->opaque = opus;

	const char *loop_tag = get_opus_tag(opus, "LOOPSTART=");

	if(loop_tag) {
		// TODO: we assume this is the sample position, but it may also be in HH:MM:SS.mmm format.
		stream->loop_start = strtoll(loop_tag, NULL, 10);
	} else {
		stream->loop_start = 0;
	}

	// LOOPEND/LOOPLENGTH not supported (TODO?)

	return true;
}

static int opus_rwops_read(void *_stream, unsigned char *_ptr, int _nbytes) {
	SDL_RWops *rw = _stream;
	return SDL_RWread(rw, _ptr, 1, _nbytes);
}

static int opus_rwops_seek(void *_stream, opus_int64 _offset, int _whence) {
	SDL_RWops *rw = _stream;
	return SDL_RWseek(rw, _offset, _whence) < 0 ? -1 : 0;
}

static opus_int64 opus_rwops_tell(void *_stream) {
	SDL_RWops *rw = _stream;
	return SDL_RWtell(rw);
}

static int opus_rwops_close(void *_stream) {
	SDL_RWops *rw = _stream;
	return SDL_RWclose(rw) < 0 ? EOF : 0;
}

bool astream_opus_open(AudioStream *stream, SDL_RWops *rw) {
	uint8_t buf[128];
	SDL_RWread(rw, buf, 1, sizeof(buf));
	int error = op_test(NULL, buf, sizeof(buf));

	if(error != 0) {
		log_debug("op_test() failed: %i", error);
		goto fail;
	}

	OggOpusFile *opus = op_open_callbacks(
		rw,
		&(OpusFileCallbacks) {
			.read = opus_rwops_read,
			.seek = opus_rwops_seek,
			.tell = opus_rwops_tell,
			.close = opus_rwops_close,
		},
		buf, sizeof(buf), &error
	);

	if(opus == NULL) {
		log_error("op_open_callbacks() failed: %i", error);
		goto fail;
	}

	return astream_opus_init(stream, opus);

fail:
	SDL_RWseek(rw, 0, RW_SEEK_SET);
	return false;
}
