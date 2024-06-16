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

typedef struct PCMStreamContext {
	uint8_t *start;
	uint8_t *end;
	uint8_t *pos;
} PCMStreamContext;

typedef struct StaticPCMAudioStream {
	AudioStream astream;
	PCMStreamContext ctx;
} StaticPCMAudioStream;

bool astream_pcm_open(AudioStream *stream, const AudioStreamSpec *spec, size_t pcm_buffer_size, void *pcm_buffer, int32_t loop_start)
	attr_nonnull_all;

bool astream_pcm_reopen(AudioStream *stream, const AudioStreamSpec *spec, size_t pcm_buffer_size, void *pcm_buffer, int32_t loop_start)
	attr_nonnull_all;

void astream_pcm_static_init(StaticPCMAudioStream *stream)
	attr_nonnull_all;
