/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "replay.h"
#include "rw_common.h"

#include "rwops/rwops_autobuf.h"

attr_nonnull_all
static void replay_write_string(SDL_RWops *file, char *str, uint16_t version) {
	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		SDL_WriteU8(file, strlen(str));
	} else {
		SDL_WriteLE16(file, strlen(str));
	}

	SDL_RWwrite(file, str, 1, strlen(str));
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

static bool replay_write_stage(ReplayStage *stg, SDL_RWops *file, uint16_t version) {
	assert(version >= REPLAY_STRUCT_VERSION_TS103000_REV2);

	SDL_WriteLE32(file, stg->flags);
	SDL_WriteLE16(file, stg->stage);
	SDL_WriteLE64(file, stg->start_time);
	SDL_WriteLE64(file, stg->rng_seed);
	SDL_WriteU8(file, stg->diff);

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		SDL_WriteLE16(file, stg->skip_frames);
	}

	SDL_WriteLE64(file, stg->plr_points);

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		SDL_WriteU8(file, stg->plr_total_lives_used);
		SDL_WriteU8(file, stg->plr_total_bombs_used);
	}

	SDL_WriteU8(file, stg->plr_total_continues_used);
	SDL_WriteU8(file, stg->plr_char);
	SDL_WriteU8(file, stg->plr_shot);
	SDL_WriteLE16(file, stg->plr_pos_x);
	SDL_WriteLE16(file, stg->plr_pos_y);

	if(version <= REPLAY_STRUCT_VERSION_TS104000_REV0) {
		// NOTE: old plr_focus field
		SDL_WriteU8(file, 0);
	}

	SDL_WriteLE16(file, stg->plr_power);
	SDL_WriteU8(file, stg->plr_lives);
	SDL_WriteLE16(file, stg->plr_life_fragments);
	SDL_WriteU8(file, stg->plr_bombs);
	SDL_WriteLE16(file, stg->plr_bomb_fragments);
	SDL_WriteU8(file, stg->plr_inputflags);
	SDL_WriteLE32(file, stg->plr_graze);
	SDL_WriteLE32(file, stg->plr_point_item_value);

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV3) {
		SDL_WriteLE64(file, stg->plr_points_final);
	}

	if(version == REPLAY_STRUCT_VERSION_TS104000_REV0) {
		// NOTE: These fields were always bugged; older taisei only wrote zeroes here.
		uint8_t buf[5] = { 0 };
		SDL_RWwrite(file, buf, sizeof(buf), 1);
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

	SDL_WriteLE16(file, stg->events.num_elements);
	SDL_WriteLE32(file, 1 + ~replay_struct_stage_metadata_checksum(stg, version));

	return true;
}

static void replay_write_stage_events(ReplayStage *stg, SDL_RWops *file) {
	dynarray_foreach_elem(&stg->events, ReplayEvent *evt, {
		SDL_WriteLE32(file, evt->frame);
		SDL_WriteU8(file, evt->type);
		SDL_WriteLE16(file, evt->value);
	});
}

static bool replay_write_events(Replay *rpy, SDL_RWops *file) {
	dynarray_foreach_elem(&rpy->stages, ReplayStage *stg, {
		replay_write_stage_events(stg, file);
	});

	return true;
}

bool replay_write(Replay *rpy, SDL_RWops *file, uint16_t version) {
	assert(version >= REPLAY_STRUCT_VERSION_TS103000_REV2);

	uint16_t base_version = (version & ~REPLAY_VERSION_COMPRESSION_BIT);
	bool compression = (version & REPLAY_VERSION_COMPRESSION_BIT);

	SDL_RWwrite(file, replay_magic_header, sizeof(replay_magic_header), 1);
	SDL_WriteLE16(file, version);

	TaiseiVersion v;
	TAISEI_VERSION_GET_CURRENT(&v);

	if(taisei_version_write(file, &v) != TAISEI_VERSION_SIZE) {
		log_error("Failed to write game version: %s", SDL_GetError());
		return false;
	}

	void *buf;
	SDL_RWops *abuf = NULL;
	SDL_RWops *vfile = file;

	if(compression) {
		abuf = SDL_RWAutoBuffer(&buf, 64);
		vfile = replay_wrap_stream_compress(version, abuf, false);
	}

	replay_write_string(vfile, rpy->playername, base_version);
	fix_flags(rpy);

	SDL_WriteLE32(vfile, rpy->flags);
	SDL_WriteLE16(vfile, rpy->stages.num_elements);

	dynarray_foreach_elem(&rpy->stages, ReplayStage *stg, {
		if(!replay_write_stage(stg, vfile, base_version)) {
			if(compression) {
				SDL_RWclose(vfile);
				SDL_RWclose(abuf);
			}

			return false;
		}
	});

	if(compression) {
		SDL_RWclose(vfile);
		SDL_WriteLE32(file, SDL_RWtell(file) + SDL_RWtell(abuf) + 4);
		SDL_RWwrite(file, buf, SDL_RWtell(abuf), 1);
		SDL_RWclose(abuf);
		vfile = replay_wrap_stream_compress(version, file, false);
	}

	bool events_ok = replay_write_events(rpy, vfile);

	if(compression) {
		SDL_RWclose(vfile);
	}

	if(!events_ok) {
		return false;
	}

	// useless byte to simplify the premature EOF check, can be anything
	SDL_WriteU8(file, REPLAY_USELESS_BYTE);

	return true;
}
