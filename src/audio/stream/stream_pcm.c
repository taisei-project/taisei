/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stream_pcm.h"

#include "util.h"

static ssize_t astream_pcm_read(AudioStream *stream, size_t buffer_size, void *buffer) {
	PCMStreamContext *ctx = NOT_NULL(stream->opaque);

	assume(ctx->pos <= ctx->end);
	size_t read_size = ctx->end - ctx->pos;

	if(buffer_size < read_size) {
		size_t frame_size = stream->spec.frame_size;
		read_size = (buffer_size / frame_size) * frame_size;
	}

	if(read_size > 0) {
		memcpy(buffer, ctx->pos, read_size);
		ctx->pos += read_size;
	}

	return read_size;
}

static ssize_t astream_pcm_seek(AudioStream *stream, size_t pos) {
	PCMStreamContext *ctx = NOT_NULL(stream->opaque);

	uint8_t *new_pos = ctx->start + pos * stream->spec.frame_size;

	if(UNLIKELY(new_pos > ctx->end || new_pos < ctx->start)) {
		log_error("PCM seek out of range");
		return -1;
	}

	ctx->pos = new_pos;
	assert(pos == (ctx->pos - ctx->start) / stream->spec.frame_size);

	return pos;
}

static ssize_t astream_pcm_tell(AudioStream *stream) {
	PCMStreamContext *ctx = NOT_NULL(stream->opaque);
	return (ctx->pos - ctx->start) / stream->spec.frame_size;
}

static void astream_pcm_free(AudioStream *stream) {
	PCMStreamContext *ctx = NOT_NULL(stream->opaque);
	mem_free(ctx);
}

static void astream_pcm_static_close(AudioStream *stream) {
	PCMStreamContext *ctx = NOT_NULL(stream->opaque);
	stream->length = 0;
	stream->loop_start = 0;
	ctx->end = ctx->start = ctx->pos = NULL;
}

static AudioStreamProcs astream_pcm_procs = {
	.free = astream_pcm_free,
	.read = astream_pcm_read,
	.seek = astream_pcm_seek,
	.tell = astream_pcm_tell,
};

static AudioStreamProcs astream_pcm_static_procs = {
	.free = astream_pcm_static_close,
	.read = astream_pcm_read,
	.seek = astream_pcm_seek,
	.tell = astream_pcm_tell,
};

bool astream_pcm_open(AudioStream *stream, const AudioStreamSpec *spec, size_t pcm_buffer_size, void *pcm_buffer, int32_t loop_start) {
	stream->procs = &astream_pcm_procs;
	stream->opaque = ALLOC(PCMStreamContext);

	if(!astream_pcm_reopen(stream, spec, pcm_buffer_size, pcm_buffer, loop_start)) {
		mem_free(stream->opaque);
		return false;
	}

	return true;
}

bool astream_pcm_reopen(AudioStream *stream, const AudioStreamSpec *spec, size_t pcm_buffer_size, void *pcm_buffer, int32_t loop_start) {
	assert(NOT_NULL(stream->procs)->read == astream_pcm_read);
	PCMStreamContext *ctx = NOT_NULL(stream->opaque);

	if(pcm_buffer_size % spec->frame_size) {
		log_error("Partial PCM data");
		return false;
	}

	ssize_t length = pcm_buffer_size / spec->frame_size;

	if(length > INT32_MAX) {
		log_error("PCM stream is too long");
		return false;
	}

	stream->spec = *spec;
	stream->length = length;
	stream->loop_start = loop_start;

	ctx->start = pcm_buffer;
	ctx->end = ctx->start + pcm_buffer_size;
	ctx->pos = ctx->start;

	assert(ctx->end > ctx->start);

	return true;
}

void astream_pcm_static_init(StaticPCMAudioStream *stream) {
	stream->astream.procs = &astream_pcm_static_procs;
	stream->astream.opaque = &stream->ctx;
}
