/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "log.h"
#include "replay.h"
#include "rw_common.h"

#include "rwops/rwops_autobuf.h"

attr_nonnull_all
static void replay_write_string(SDL_IOStream *file, char *str, uint16_t version) {
	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		SDL_WriteU8(file, strlen(str));
	} else {
		SDL_WriteU16LE(file, strlen(str));
	}

	SDL_WriteIO(file, str, strlen(str));
}

static void fix_flags(Replay *rpy) {
	dynarray_foreach_elem(&rpy->stages, ReplayStage *stg, {
		if(!(stg->flags & REPLAY_SFLAG_CLEAR)) {
			rpy->flags &= ~REPLAY_GFLAG_CLEAR;
			return;
		}
	});

	rpy->flags |= REPLAY_GFLAG_CLEAR;
}

static bool replay_write_stage(ReplayStage *stg, SDL_IOStream *file,
			       uint16_t version) {
	assert(version >= REPLAY_STRUCT_VERSION_TS103000_REV2);

	SDL_WriteU32LE(file, stg->flags);
	SDL_WriteU16LE(file, stg->stage);
	SDL_WriteU64LE(file, stg->start_time);
	SDL_WriteU64LE(file, stg->rng_seed);
	SDL_WriteU8(file, stg->diff);

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		SDL_WriteU16LE(file, stg->skip_frames);
	}

	SDL_WriteU64LE(file, stg->plr_points);

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		SDL_WriteU8(file, stg->plr_total_lives_used);
		SDL_WriteU8(file, stg->plr_total_bombs_used);
	}

	SDL_WriteU8(file, stg->plr_total_continues_used);
	SDL_WriteU8(file, stg->plr_char);
	SDL_WriteU8(file, stg->plr_shot);
	SDL_WriteU16LE(file, stg->plr_pos_x);
	SDL_WriteU16LE(file, stg->plr_pos_y);

	if(version <= REPLAY_STRUCT_VERSION_TS104000_REV0) {
		// NOTE: old plr_focus field
		SDL_WriteU8(file, 0);
	}

	SDL_WriteU16LE(file, stg->plr_power);
	SDL_WriteU8(file, stg->plr_lives);
	SDL_WriteU16LE(file, stg->plr_life_fragments);
	SDL_WriteU8(file, stg->plr_bombs);
	SDL_WriteU16LE(file, stg->plr_bomb_fragments);
	SDL_WriteU8(file, stg->plr_inputflags);
	SDL_WriteU32LE(file, stg->plr_graze);
	SDL_WriteU32LE(file, stg->plr_point_item_value);

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV3) {
		SDL_WriteU64LE(file, stg->plr_points_final);
	}

	if(version == REPLAY_STRUCT_VERSION_TS104000_REV0) {
		// NOTE: These fields were always bugged; older taisei only wrote zeroes here.
		uint8_t buf[5] = {};
		SDL_WriteIO(file, buf, sizeof(buf));
	}

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		SDL_WriteU8(file, stg->plr_stage_lives_used_final);
		SDL_WriteU8(file, stg->plr_stage_bombs_used_final);
		SDL_WriteU8(file, stg->plr_stage_continues_used_final);
	}

	if(stg->events.num_elements > UINT16_MAX) {
		log_error("Too many events in replay, cannot write this");
		return false;
	}

	SDL_WriteU16LE(file, stg->events.num_elements);
	SDL_WriteU32LE(file, 1 + ~replay_struct_stage_metadata_checksum(stg, version));

	return true;
}

static void replay_write_stage_events(ReplayStage *stg, SDL_IOStream *file) {
	dynarray_foreach_elem(&stg->events, ReplayEvent *evt, {
		SDL_WriteU32LE(file, evt->frame);
		SDL_WriteU8(file, evt->type);
		SDL_WriteU16LE(file, evt->value);
	});
}

static bool replay_write_events(Replay *rpy, SDL_IOStream *file) {
	dynarray_foreach_elem(&rpy->stages, ReplayStage *stg, {
		replay_write_stage_events(stg, file);
	});

	return true;
}

bool replay_write(Replay *rpy, SDL_IOStream *file, uint16_t version) {
	assert(version >= REPLAY_STRUCT_VERSION_TS103000_REV2);

	uint16_t base_version = (version & ~REPLAY_VERSION_COMPRESSION_BIT);
	bool compression = (version & REPLAY_VERSION_COMPRESSION_BIT);

	SDL_WriteIO(file, replay_magic_header,
		    sizeof(replay_magic_header));
	SDL_WriteU16LE(file, version);

	TaiseiVersion v;
	TAISEI_VERSION_GET_CURRENT(&v);

	if(taisei_version_write(file, &v) != TAISEI_VERSION_SIZE) {
		log_error("Failed to write game version: %s", SDL_GetError());
		return false;
	}

	void *buf;
	SDL_IOStream *abuf = NULL;
	SDL_IOStream *vfile = file;

	if(compression) {
		abuf = SDL_RWAutoBuffer(&buf, 64);
		vfile = replay_wrap_stream_compress(version, abuf, false);
	}

	replay_write_string(vfile, rpy->playername, base_version);
	fix_flags(rpy);

	SDL_WriteU32LE(vfile, rpy->flags);
	SDL_WriteU16LE(vfile, rpy->stages.num_elements);

	dynarray_foreach_elem(&rpy->stages, ReplayStage *stg, {
		if(!replay_write_stage(stg, vfile, base_version)) {
			if(compression) {
				SDL_CloseIO(vfile);
				SDL_CloseIO(abuf);
			}

			return false;
		}
	});

	if(compression) {
		SDL_CloseIO(vfile);
		SDL_WriteU32LE(file, SDL_TellIO(file) + SDL_TellIO(abuf) + 4);
		SDL_WriteIO(file, buf, SDL_TellIO(abuf));
		SDL_CloseIO(abuf);
		vfile = replay_wrap_stream_compress(version, file, false);
	}

	bool events_ok = replay_write_events(rpy, vfile);

	if(compression) {
		SDL_CloseIO(vfile);
	}

	if(!events_ok) {
		return false;
	}

	// useless byte to simplify the premature EOF check, can be anything
	SDL_WriteU8(file, REPLAY_USELESS_BYTE);

	return true;
}
