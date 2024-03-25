/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "struct.h"

uint32_t replay_struct_stage_metadata_checksum(ReplayStage *stg, uint16_t version);

SDL_IOStream *replay_wrap_stream_compress(uint16_t version, SDL_IOStream *rw,
					  bool autoclose);
SDL_IOStream *replay_wrap_stream_decompress(uint16_t version,
					    SDL_IOStream *rw, bool autoclose);

extern uint8_t replay_magic_header[REPLAY_MAGIC_HEADER_SIZE];
