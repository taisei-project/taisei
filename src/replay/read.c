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

#include "player.h"
#include "rwops/rwops_segment.h"

#ifdef REPLAY_LOAD_DEBUG
	#define PRINTPROP(prop, fmt) \
		log_debug(#prop " = %" # fmt " [%"PRIi64" / %"PRIi64"]", prop, SDL_RWtell(file), filesize)
#else
	#define PRINTPROP(prop,fmt) (void)(prop)
#endif

#define CHECKPROP(prop, fmt) \
	do { \
		PRINTPROP(prop,fmt); \
		if(filesize > 0 && SDL_RWtell(file) == filesize) { \
			log_error("%s: Premature EOF", source); \
			return false; \
		} \
	} while(0)

static void replay_read_string(SDL_RWops *file, char **ptr, uint16_t version) {
	size_t len;

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		len = SDL_ReadU8(file);
	} else {
		len = SDL_ReadLE16(file);
	}

	*ptr = mem_alloc(len + 1);
	SDL_RWread(file, *ptr, 1, len);
}

static bool replay_read_header(Replay *rpy, SDL_RWops *file, int64_t filesize, size_t *ofs, const char *source) {
	uint8_t header[sizeof(replay_magic_header)];
	(*ofs) += sizeof(header);

	SDL_RWread(file, header, sizeof(header), 1);

	if(memcmp(header, replay_magic_header, sizeof(header))) {
		log_error("%s: Incorrect header", source);
		return false;
	}

	CHECKPROP(rpy->version = SDL_ReadLE16(file), u);
	(*ofs) += 2;

	uint16_t base_version = (rpy->version & ~REPLAY_VERSION_COMPRESSION_BIT);
	bool compression = (rpy->version & REPLAY_VERSION_COMPRESSION_BIT);
	bool gamev_assumed = false;

	switch(base_version) {
		case REPLAY_STRUCT_VERSION_TS101000: {
			// legacy format with no versioning, assume v1.1
			TAISEI_VERSION_SET(&rpy->game_version, 1, 1, 0, 0);
			gamev_assumed = true;
			break;
		}

		case REPLAY_STRUCT_VERSION_TS102000_REV0:
		case REPLAY_STRUCT_VERSION_TS102000_REV1:
		case REPLAY_STRUCT_VERSION_TS102000_REV2:
		case REPLAY_STRUCT_VERSION_TS103000_REV0:
		case REPLAY_STRUCT_VERSION_TS103000_REV1:
		case REPLAY_STRUCT_VERSION_TS103000_REV2:
		case REPLAY_STRUCT_VERSION_TS103000_REV3:
		case REPLAY_STRUCT_VERSION_TS104000_REV0:
		case REPLAY_STRUCT_VERSION_TS104000_REV1:
		{
			if(taisei_version_read(file, &rpy->game_version) != TAISEI_VERSION_SIZE) {
				log_error("%s: Failed to read game version", source);
				return false;
			}

			(*ofs) += TAISEI_VERSION_SIZE;
			break;
		}

		default: {
			log_error("%s: Unknown struct version %u", source, base_version);
			return false;
		}
	}

	char *gamev = taisei_version_tostring(&rpy->game_version);
	log_info("Struct version %u (%scompressed), game version %s%s",
		base_version, compression ? "" : "un", gamev, gamev_assumed ? " (assumed)" : "");
	mem_free(gamev);

	if(compression) {
		CHECKPROP(rpy->fileoffset = SDL_ReadLE32(file), u);
		(*ofs) += 4;
	}

	return true;
}

static bool replay_read_meta(Replay *rpy, SDL_RWops *file, int64_t filesize, const char *source) {
	uint16_t version = rpy->version & ~REPLAY_VERSION_COMPRESSION_BIT;

	rpy->playername = NULL;

	replay_read_string(file, &rpy->playername, version);
	PRINTPROP(rpy->playername, s);

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		CHECKPROP(rpy->flags = SDL_ReadLE32(file), u);
	}

	uint64_t numstages;
	CHECKPROP(numstages = SDL_ReadLE16(file), u);

	if(!numstages) {
		log_error("%s: No stages in replay", source);
		goto error;
	}

	dynarray_ensure_capacity(&rpy->stages, numstages);

	for(int i = 0; i < numstages; ++i) {
		ReplayStage *stg = dynarray_append(&rpy->stages);
		*stg = (ReplayStage) { };

		if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
			CHECKPROP(stg->flags = SDL_ReadLE32(file), u);
		}

		CHECKPROP(stg->stage = SDL_ReadLE16(file), u);

		if(version >= REPLAY_STRUCT_VERSION_TS103000_REV2) {
			CHECKPROP(stg->start_time = SDL_ReadLE64(file), u);
			CHECKPROP(stg->rng_seed = SDL_ReadLE64(file), u);
		} else {
			stg->rng_seed = SDL_ReadLE32(file);
			stg->start_time = stg->rng_seed;
		}

		CHECKPROP(stg->diff = SDL_ReadU8(file), u);

		if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
			CHECKPROP(stg->skip_frames = SDL_ReadLE16(file), u);
		}

		if(version >= REPLAY_STRUCT_VERSION_TS103000_REV0) {
			CHECKPROP(stg->plr_points = SDL_ReadLE64(file), zu);
		} else {
			CHECKPROP(stg->plr_points = SDL_ReadLE32(file), zu);
		}

		if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
			CHECKPROP(stg->plr_total_lives_used = SDL_ReadU8(file), u);
			CHECKPROP(stg->plr_total_bombs_used = SDL_ReadU8(file), u);
		}

		if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
			CHECKPROP(stg->plr_total_continues_used = SDL_ReadU8(file), u);
		}

		CHECKPROP(stg->plr_char = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_shot = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_pos_x = SDL_ReadLE16(file), u);
		CHECKPROP(stg->plr_pos_y = SDL_ReadLE16(file), u);

		if(version <= REPLAY_STRUCT_VERSION_TS104000_REV0) {
			// NOTE: old plr_focus field, ignored
			SDL_ReadU8(file);
		}

		CHECKPROP(stg->plr_power = SDL_ReadLE16(file), u);
		CHECKPROP(stg->plr_lives = SDL_ReadU8(file), u);

		if(version >= REPLAY_STRUCT_VERSION_TS103000_REV1) {
			CHECKPROP(stg->plr_life_fragments = SDL_ReadLE16(file), u);
		} else {
			CHECKPROP(stg->plr_life_fragments = SDL_ReadU8(file), u);
		}

		CHECKPROP(stg->plr_bombs = SDL_ReadU8(file), u);

		if(version >= REPLAY_STRUCT_VERSION_TS103000_REV1) {
			CHECKPROP(stg->plr_bomb_fragments = SDL_ReadLE16(file), u);
		} else {
			CHECKPROP(stg->plr_bomb_fragments = SDL_ReadU8(file), u);
		}

		CHECKPROP(stg->plr_inputflags = SDL_ReadU8(file), u);

		if(version >= REPLAY_STRUCT_VERSION_TS103000_REV0) {
			CHECKPROP(stg->plr_graze = SDL_ReadLE32(file), u);
			CHECKPROP(stg->plr_point_item_value = SDL_ReadLE32(file), u);
		} else if(version >= REPLAY_STRUCT_VERSION_TS102000_REV2) {
			CHECKPROP(stg->plr_graze = SDL_ReadLE16(file), u);
			stg->plr_point_item_value = PLR_START_PIV;
		}

		if(version >= REPLAY_STRUCT_VERSION_TS103000_REV3) {
			CHECKPROP(stg->plr_points_final = SDL_ReadLE64(file), zu);
		}

		if(version == REPLAY_STRUCT_VERSION_TS104000_REV0) {
			// NOTE: These fields were always bugged, so we ignore them
			uint8_t junk[5];
			SDL_RWread(file, junk, sizeof(junk), 1);
		}

		if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
			CHECKPROP(stg->plr_stage_lives_used_final = SDL_ReadU8(file), u);
			CHECKPROP(stg->plr_stage_bombs_used_final = SDL_ReadU8(file), u);
			CHECKPROP(stg->plr_stage_continues_used_final = SDL_ReadU8(file), u);
		}

		CHECKPROP(stg->num_events = SDL_ReadLE16(file), u);

		if(replay_struct_stage_metadata_checksum(stg, version) + SDL_ReadLE32(file)) {
			log_error("%s: Stageinfo is corrupt", source);
			goto error;
		}
	}

	return true;

error:
	mem_free(rpy->playername);
	rpy->playername = NULL;
	dynarray_free_data(&rpy->stages);
	return false;
}

static bool replay_read_events(Replay *rpy, SDL_RWops *file, int64_t filesize, const char *source) {
	dynarray_foreach_elem(&rpy->stages, ReplayStage *stg, {
		if(!stg->num_events) {
			log_error("%s: No events in stage", source);
			goto error;
		}

		dynarray_ensure_capacity(&stg->events, stg->num_events);

		for(int j = 0; j < stg->num_events; ++j) {
			ReplayEvent *evt = dynarray_append(&stg->events);

			CHECKPROP(evt->frame = SDL_ReadLE32(file), u);
			CHECKPROP(evt->type = SDL_ReadU8(file), u);
			CHECKPROP(evt->value = SDL_ReadLE16(file), u);
		}
	});

	return true;

error:
	replay_destroy_events(rpy);
	return false;
}

bool replay_read(Replay *rpy, SDL_RWops *file, ReplayReadMode mode, const char *source) {
	int64_t filesize; // must be signed
	SDL_RWops *vfile = file;

	if(!source) {
		source = "<unknown>";
	}

	mode &= REPLAY_READ_ALL;
	assert(mode != 0);
	filesize = SDL_RWsize(file);

	if(filesize < 0) {
		log_warn("%s: SDL_RWsize() failed: %s", source, SDL_GetError());
	}

	if(mode & REPLAY_READ_META) {
		memset(rpy, 0, sizeof(Replay));

		if(filesize > 0 && filesize <= sizeof(replay_magic_header) + 2) {
			log_error("%s: Replay file is too short (%"PRIi64")", source, filesize);
			return false;
		}

		size_t ofs = 0;

		if(!replay_read_header(rpy, file, filesize, &ofs, source)) {
			return false;
		}

		bool compression = false;

		if(rpy->version & REPLAY_VERSION_COMPRESSION_BIT) {
			if(rpy->fileoffset < SDL_RWtell(file)) {
				log_error("%s: Invalid offset %"PRIi32"", source, rpy->fileoffset);
				return false;
			}

			vfile = replay_wrap_stream_decompress(
				rpy->version,
				SDL_RWWrapSegment(file, ofs, rpy->fileoffset, false),
				true
			);

			filesize = -1;
			compression = true;
		}

		if(!replay_read_meta(rpy, vfile, filesize, source)) {
			if(compression) {
				SDL_RWclose(vfile);
			}

			return false;
		}

		if(compression) {
			SDL_RWclose(vfile);
			vfile = file;
		} else {
			rpy->fileoffset = SDL_RWtell(file);
		}
	}

	if(mode & REPLAY_READ_EVENTS) {
		if(!(mode & REPLAY_READ_META)) {
			if(!rpy->fileoffset) {
				log_fatal("%s: Tried to read events before reading metadata", source);
			}

			dynarray_foreach_elem(&rpy->stages, ReplayStage *stg, {
				if(stg->events.data) {
					log_warn("%s: BUG: Reading events into a replay that already had events, call replay_destroy_events() if this is intended", source);
					replay_destroy_events(rpy);
					break;
				}
			});

			if(SDL_RWseek(file, rpy->fileoffset, RW_SEEK_SET) < 0) {
				log_error("%s: SDL_RWseek() failed: %s", source, SDL_GetError());
				return false;
			}
		}

		bool compression = false;

		if(rpy->version & REPLAY_VERSION_COMPRESSION_BIT) {
			vfile = replay_wrap_stream_decompress(rpy->version, file, false);
			filesize = -1;
			compression = true;
		}

		if(!replay_read_events(rpy, vfile, filesize, source)) {
			if(compression) {
				SDL_RWclose(vfile);
			}

			return false;
		}

		if(compression) {
			SDL_RWclose(vfile);
		}

		// useless byte to simplify the premature EOF check, can be anything
		SDL_ReadU8(file);
	}

	return true;
}
