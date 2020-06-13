/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "stream.h"
#include "stream_opus.h"
#include "util.h"

bool astream_spec_equals(const AudioStreamSpec *s1, const AudioStreamSpec *s2) {
	return
		s1->channels == s2->channels &&
		s1->sample_format == s2->sample_format &&
		s1->sample_rate == s2->sample_rate;
}

SDL_AudioStream *astream_create_sdl_stream(AudioStream *source, AudioStreamSpec dest_spec) {
	return SDL_NewAudioStream(
		source->spec.sample_format,
		source->spec.channels,
		source->spec.sample_rate,
		dest_spec.sample_format,
		dest_spec.channels,
		dest_spec.sample_rate
	);
}

bool astream_open(AudioStream *stream, SDL_RWops *rwops, const char *filename) {
	bool ok = false;

	ok = astream_opus_open(stream, rwops);

	if(!ok) {
		log_error("%s: failed to open audio stream", filename);
	}

	return ok;
}

void astream_close(AudioStream *stream) {
	if(stream->procs.free) {
		stream->procs.free(stream);
	}
}

ssize_t astream_read(AudioStream *stream, size_t bufsize, void *buffer, AudioStreamReadFlags flags) {
	if(flags & ASTREAM_READ_MAX_FILL) {
		flags &= ~ASTREAM_READ_MAX_FILL;
		char *cbuf = buffer;
		char *end = cbuf + bufsize;
		ssize_t read;

		while(end - cbuf > 0 && (read = astream_read(stream, end - cbuf, cbuf, flags)) > 0) {
			cbuf += read;
		}

		return cbuf - (char*)buffer;
	}

	if(flags & ASTREAM_READ_LOOP) {
		ssize_t read = NOT_NULL(stream->procs.read)(stream, bufsize, buffer);

		if(UNLIKELY(read == 0)) {
			ssize_t loop_start = stream->loop_start;
			if(loop_start < 0) {
				loop_start = 0;
			}

			if(UNLIKELY(astream_seek(stream, loop_start) < 0)) {
				return -1;
			}

			return NOT_NULL(stream->procs.read)(stream, bufsize, buffer);
		}

		return read;
	}

	return NOT_NULL(stream->procs.read)(stream, bufsize, buffer);
}

ssize_t astream_read_into_sdl_stream(AudioStream *stream, SDL_AudioStream *sdlstream, size_t bufsize, void *buffer, AudioStreamReadFlags flags) {
	char *buf = buffer;
	ssize_t read_size = astream_read(stream, bufsize, buf, flags);

	if(LIKELY(read_size > 0)) {
		if(UNLIKELY(SDL_AudioStreamPut(sdlstream, buf, read_size) < 0)) {
			log_sdl_error(LOG_ERROR, "SDL_AudioStreamPut");
			return -1;
		}
	}

	return read_size;
}

ssize_t astream_seek(AudioStream *stream, size_t pos) {
	if(stream->length >= 0 && pos >= stream->length) {
		log_error("Position %zu out of range", pos);
		return -1;
	}

	if(LIKELY(stream->procs.seek)) {
		return stream->procs.seek(stream, pos);
	}

	log_error("Not implemented");
	return -1;
}

ssize_t astream_tell(AudioStream *stream) {
	if(LIKELY(stream->procs.tell)) {
		return stream->procs.tell(stream);
	}

	log_error("Not implemented");
	return -1;
}

const char *astream_get_meta_tag(AudioStream *stream, AudioStreamMetaTag tag) {
	if(LIKELY(stream->procs.meta)) {
		return stream->procs.meta(stream, tag);
	}

	return NULL;
}

ssize_t astream_util_time_to_offset(AudioStream *stream, double t) {
	return t * stream->spec.sample_rate;
}

double astream_util_offset_to_time(AudioStream *stream, ssize_t ofs) {
	return ofs / (double)stream->spec.sample_rate;
}
