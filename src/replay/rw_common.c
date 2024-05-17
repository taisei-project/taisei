/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rw_common.h"

#include "log.h"
#include "rwops/rwops_zlib.h"
#include "rwops/rwops_zstd.h"

uint8_t replay_magic_header[REPLAY_MAGIC_HEADER_SIZE] = REPLAY_MAGIC_HEADER;

uint32_t replay_struct_stage_metadata_checksum(ReplayStage *stg, uint16_t version) {
	uint32_t cs = 0;

	cs += stg->stage;
	cs += stg->rng_seed;
	cs += stg->diff;

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		cs += stg->skip_frames;
	}

	cs += stg->plr_points;
	cs += stg->plr_char;
	cs += stg->plr_shot;
	cs += stg->plr_pos_x;
	cs += stg->plr_pos_y;
	cs += stg->plr_power;
	cs += stg->plr_lives;
	cs += stg->plr_life_fragments;
	cs += stg->plr_bombs;
	cs += stg->plr_bomb_fragments;
	cs += stg->plr_inputflags;

	if(!stg->num_events && stg->events.num_elements) {
		cs += (uint16_t)stg->events.num_elements;
	} else {
		cs += stg->num_events;
	}

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		cs += stg->plr_total_lives_used;
		cs += stg->plr_total_continues_used;
	}

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		cs += stg->plr_total_continues_used;
		cs += stg->flags;
	}

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV2) {
		cs += stg->plr_graze;
	}

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV0) {
		cs += stg->plr_point_item_value;
	}

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV3) {
		cs += stg->plr_points_final;
	}

	log_debug("%08x", cs);
	return cs;
}

SDL_RWops *replay_wrap_stream_compress(uint16_t version, SDL_RWops *rw, bool autoclose) {
	if((version & ~REPLAY_VERSION_COMPRESSION_BIT) >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		return SDL_RWWrapZstdWriter(rw, 22, autoclose);
	} else {
		return SDL_RWWrapZlibWriter(
			rw, RW_DEFLATE_LEVEL_DEFAULT, REPLAY_COMPRESSION_CHUNK_SIZE, autoclose);
	}
}

SDL_RWops *replay_wrap_stream_decompress(uint16_t version, SDL_RWops *rw, bool autoclose) {
	if((version & ~REPLAY_VERSION_COMPRESSION_BIT) >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		return SDL_RWWrapZstdReader(rw, autoclose);
	} else {
		return SDL_RWWrapZlibReader(rw, REPLAY_COMPRESSION_CHUNK_SIZE, autoclose);
	}
}
