/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stream.h"
#include "stream_opus.h"

#include "log.h"
#include "util/miscmath.h"

#define PROCS(stream) (*NOT_NULL((stream)->procs))
#define PROC(stream, proc) (NOT_NULL(PROCS(stream).proc))

AudioStreamSpec astream_spec(SDL_AudioFormat sample_format, uint channels, uint sample_rate) {
	AudioStreamSpec s;
	s.sample_format = sample_format;
	s.channels = channels;
	s.sample_rate = sample_rate;
	s.frame_size = channels * (SDL_AUDIO_BITSIZE(sample_format) / CHAR_BIT);
	assert(s.frame_size > 0);
	return s;
}

bool astream_spec_equals(const AudioStreamSpec *s1, const AudioStreamSpec *s2) {
	return
		s1->channels == s2->channels &&
		s1->sample_format == s2->sample_format &&
		s1->sample_rate == s2->sample_rate;
}

SDL_AudioStream *astream_create_sdl_stream(AudioStream *source, const AudioStreamSpec *dest_spec) {
	return SDL_CreateAudioStream(&(SDL_AudioSpec) {
		.channels = source->spec.channels,
		.format = source->spec.sample_format,
		.freq = source->spec.sample_rate,
	}, &(SDL_AudioSpec) {
		.channels = dest_spec->channels,
		.format = dest_spec->sample_format,
		.freq = dest_spec->sample_rate,
	});
}

bool astream_open(AudioStream *stream, SDL_IOStream *rwops, const char *filename) {
	bool ok = false;

	ok = astream_opus_open(stream, rwops);

	if(!ok) {
		log_error("%s: failed to open audio stream", filename);
	}

	return ok;
}

void astream_close(AudioStream *stream) {
	if(PROCS(stream).free) {
		PROC(stream, free)(stream);
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
		ssize_t read = PROC(stream, read)(stream, bufsize, buffer);

		if(UNLIKELY(read == 0)) {
			ssize_t loop_start = stream->loop_start;
			if(loop_start < 0) {
				loop_start = 0;
			}

			if(UNLIKELY(astream_seek(stream, loop_start) < 0)) {
				return -1;
			}

			return PROC(stream, read)(stream, bufsize, buffer);
		}

		return read;
	}

	return PROC(stream, read)(stream, bufsize, buffer);
}

ssize_t astream_read_into_sdl_stream(AudioStream *stream, SDL_AudioStream *sdlstream, size_t bufsize, void *buffer, AudioStreamReadFlags flags) {
	char *buf = buffer;
	ssize_t read_size = astream_read(stream, bufsize, buf, flags);

	if(LIKELY(read_size > 0)) {
		if(UNLIKELY(!SDL_PutAudioStreamData(sdlstream, buf, read_size))) {
			log_sdl_error(LOG_ERROR, "SDL_PutAudioStreamData");
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

	if(LIKELY(PROCS(stream).seek)) {
		return PROC(stream, seek)(stream, pos);
	}

	log_error("Not implemented");
	return -1;
}

ssize_t astream_tell(AudioStream *stream) {
	if(LIKELY(PROCS(stream).tell)) {
		return PROC(stream, tell)(stream);
	}

	log_error("Not implemented");
	return -1;
}

const char *astream_get_meta_tag(AudioStream *stream, AudioStreamMetaTag tag) {
	if(LIKELY(PROCS(stream).meta)) {
		return PROC(stream, meta)(stream, tag);
	}

	return NULL;
}

ssize_t astream_util_time_to_offset(AudioStream *stream, double t) {
	return t * stream->spec.sample_rate;
}

double astream_util_offset_to_time(AudioStream *stream, ssize_t ofs) {
	return ofs / (double)stream->spec.sample_rate;
}

ssize_t astream_util_resampled_buffer_size(const AudioStream *src, const AudioStreamSpec *spec) {
	uint64_t size = src->length * spec->frame_size;

	if(size > INT32_MAX) {
		log_error("Source stream is too large");
		return -1;
	}

	if(spec->sample_rate != src->spec.sample_rate) {
		size = uceildiv64(size * spec->sample_rate, src->spec.sample_rate);

		if(size > INT32_MAX) {
			log_error("Resampled stream is too large");
			return -1;
		}

		size = (size / spec->frame_size) * spec->frame_size;
	}

	return size;
}

bool astream_crystalize(AudioStream *src, const AudioStreamSpec *spec, size_t buffer_size, void *buffer) {
	assert(buffer_size >= astream_util_resampled_buffer_size(src, spec));

	SDL_AudioStream *pipe = NULL;
	SDL_IOStream *rw = SDL_IOFromMem(buffer, buffer_size);

	if(UNLIKELY(!rw)) {
		log_sdl_error(LOG_ERROR, "SDL_IOFromMem");
		goto fail;
	}

	pipe = astream_create_sdl_stream(src, spec);

	if(UNLIKELY(!pipe)) {
		log_sdl_error(LOG_ERROR, "SDL_CreateAudioStream");
		goto fail;
	}

	for(;;) {
		char staging[1 << 14];

		ssize_t read = SDL_GetAudioStreamData(pipe, staging, sizeof(staging));

		if(UNLIKELY(read < 0)) {
			log_sdl_error(LOG_ERROR, "SDL_GetAudioStreamData");
			goto fail;
		}

		if(read > 0) {
			if(UNLIKELY(SDL_WriteIO(rw, staging, read) < read)) {
				log_sdl_error(LOG_ERROR, "SDL_WriteIO");
				goto fail;
			}
		}

		read = astream_read_into_sdl_stream(src, pipe, sizeof(staging), staging, 0);

		if(UNLIKELY(read < 0)) {
			goto fail;
		}

		if(read == 0) {
			SDL_FlushAudioStream(pipe);
			if(SDL_GetAudioStreamAvailable(pipe) == 0) {
				break;
			}
		}
	}

	SDL_DestroyAudioStream(pipe);
	SDL_CloseIO(rw);
	return true;

fail:
	if(rw) {
		SDL_CloseIO(rw);
	}
	SDL_DestroyAudioStream(pipe);
	return false;
}
