/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "replay.h"
#include "rw_common.h"

#include "player.h"
#include "rwops/rwops_segment.h"

typedef struct ReplayReadContext {
	Replay *replay;
	SDL_IOStream *stream;
	const char *filename;
	int64_t filesize;
	uint16_t version;
	ReplayReadMode mode;
} ReplayReadContext;

#ifdef REPLAY_LOAD_DEBUG
	#define PRINTPROP(prop, fmt, file) \
		log_debug(#prop " = %" # fmt " [%"PRIi64"]", prop, SDL_TellIO(file))
#else
	#define PRINTPROP(prop, fmt, file) (void)(prop)
#endif

#define RETURN_ERROR if(!(ctx->mode & REPLAY_READ_IGNORE_ERRORS)) return false

#define CHECKPROP(_prop, _readfunc, _stream, _fmt) \
	do { \
		bool _read_ok = (_readfunc)((_stream), &(_prop)); \
		if(!_read_ok) { \
			log_error("%s: Error reading " #_prop ": %s", ctx->filename, SDL_GetError()); \
			RETURN_ERROR; \
		} \
		PRINTPROP(_prop, _fmt, _stream); \
	} while(0)

static bool replay_read_string(ReplayReadContext *ctx, char **ptr) {
	size_t len = 0;
	bool ok;

	if(ctx->version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		uint8_t tmp;
		ok = SDL_ReadU8(ctx->stream, &tmp);
		len = tmp;
	} else {
		uint16_t tmp;
		ok = SDL_ReadU16LE(ctx->stream, &tmp);
		len = tmp;
	}

	if(!ok) {
		log_error("%s: Error reading string length: %s", ctx->filename, SDL_GetError());
		RETURN_ERROR;
	}

	*ptr = mem_alloc(len + 1);
	len = SDL_ReadIO(ctx->stream, *ptr, len);

	if(SDL_GetIOStatus(ctx->stream) != SDL_IO_STATUS_READY) {
		log_error("%s: Error reading string: %s", ctx->filename, SDL_GetError());

		if(!(ctx->mode & REPLAY_READ_IGNORE_ERRORS)) {
			mem_free(*ptr);
			*ptr = NULL;
			return false;
		}
	}

	return true;
}

static bool replay_read_header(
	ReplayReadContext *ctx,
	size_t *ofs
) {
	auto rpy = ctx->replay;
	auto file = ctx->stream;
	auto source = ctx->filename;

	uint8_t header[sizeof(replay_magic_header)];
	(*ofs) += sizeof(header);

	SDL_ReadIO(file, header, sizeof(header));

	if(memcmp(header, replay_magic_header, sizeof(header))) {
		log_error("%s: Incorrect header", source);
		RETURN_ERROR;
	}

	CHECKPROP(rpy->version, SDL_ReadU16LE, file, u);
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
				RETURN_ERROR;
			}

			(*ofs) += TAISEI_VERSION_SIZE;
			break;
		}

		default: {
			log_error("%s: Unknown struct version %u", source, base_version);
			RETURN_ERROR;
		}
	}

	char *gamev = taisei_version_tostring(&rpy->game_version);
	log_info("Struct version %u (%scompressed), game version %s%s",
		base_version, compression ? "" : "un", gamev, gamev_assumed ? " (assumed)" : "");
	mem_free(gamev);

	if(compression) {
		CHECKPROP(rpy->fileoffset, SDL_ReadU32LE, file, u);
		(*ofs) += 4;
	}

	return true;
}

static bool replay_read_stage(ReplayReadContext *ctx, ReplayStage *stg) {
	auto version = ctx->version;
	auto file = ctx->stream;

	*stg = (ReplayStage) { };

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		CHECKPROP(stg->flags, SDL_ReadU32LE, file, u);
	}

	CHECKPROP(stg->stage, SDL_ReadU16LE, file, u);

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV2) {
		CHECKPROP(stg->start_time, SDL_ReadU64LE, file, zu);
		CHECKPROP(stg->rng_seed, SDL_ReadU64LE, file, zu);
	} else {
		uint32_t rng_seed_32 = 0;
		CHECKPROP(rng_seed_32, SDL_ReadU32LE, file, u);
		stg->rng_seed = rng_seed_32;
		stg->start_time = stg->rng_seed;
	}

	CHECKPROP(stg->diff, SDL_ReadU8, file, u);

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		CHECKPROP(stg->skip_frames, SDL_ReadU16LE, file, u);
	}

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV0) {
		CHECKPROP(stg->plr_points, SDL_ReadU64LE, file, zu);
	} else {
		uint32_t plr_points_32 = 0;
		CHECKPROP(plr_points_32, SDL_ReadU32LE, file, u);
		stg->plr_points = plr_points_32;
	}

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		CHECKPROP(stg->plr_total_lives_used, SDL_ReadU8, file, u);
		CHECKPROP(stg->plr_total_bombs_used, SDL_ReadU8, file, u);
	}

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		CHECKPROP(stg->plr_total_continues_used, SDL_ReadU8, file, u);
	}

	CHECKPROP(stg->plr_char, SDL_ReadU8, file, u);
	CHECKPROP(stg->plr_shot, SDL_ReadU8, file, u);
	CHECKPROP(stg->plr_pos_x, SDL_ReadU16LE, file, u);
	CHECKPROP(stg->plr_pos_y, SDL_ReadU16LE, file, u);

	if(version <= REPLAY_STRUCT_VERSION_TS104000_REV0) {
		// NOTE: old plr_focus field, ignored
		uint8_t plr_focus_ignored;
		CHECKPROP(plr_focus_ignored, SDL_ReadU8, file, u);
	}

	CHECKPROP(stg->plr_power, SDL_ReadU16LE, file, u);
	CHECKPROP(stg->plr_lives, SDL_ReadU8, file, u);

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV1) {
		CHECKPROP(stg->plr_life_fragments, SDL_ReadU16LE, file, u);
	} else {
		uint8_t plr_life_fragments_8 = 0;
		CHECKPROP(plr_life_fragments_8, SDL_ReadU8, file, u);
		stg->plr_life_fragments = plr_life_fragments_8;
	}

	CHECKPROP(stg->plr_bombs, SDL_ReadU8, file, u);

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV1) {
		CHECKPROP(stg->plr_bomb_fragments, SDL_ReadU16LE, file, u);
	} else {
		uint8_t plr_bomb_fragments_8 = 0;
		CHECKPROP(plr_bomb_fragments_8, SDL_ReadU8, file, u);
		stg->plr_bomb_fragments = plr_bomb_fragments_8;
	}

	CHECKPROP(stg->plr_inputflags, SDL_ReadU8, file, u);

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV0) {
		CHECKPROP(stg->plr_graze, SDL_ReadU32LE, file, u);
		CHECKPROP(stg->plr_point_item_value, SDL_ReadU32LE, file, u);
	} else if(version >= REPLAY_STRUCT_VERSION_TS102000_REV2) {
		uint16_t plr_graze_16 = 0;
		CHECKPROP(plr_graze_16, SDL_ReadU16LE, file, u);
		stg->plr_graze = plr_graze_16;
		stg->plr_point_item_value = PLR_START_PIV;
	}

	if(version >= REPLAY_STRUCT_VERSION_TS103000_REV3) {
		CHECKPROP(stg->plr_points_final, SDL_ReadU64LE, file, zu);
	}

	if(version == REPLAY_STRUCT_VERSION_TS104000_REV0) {
		// NOTE: These fields were always bugged, so we ignore them
		uint8_t junk[5];
		SDL_ReadIO(file, junk, sizeof(junk));
	}

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV1) {
		CHECKPROP(stg->plr_stage_lives_used_final, SDL_ReadU8, file, u);
		CHECKPROP(stg->plr_stage_bombs_used_final, SDL_ReadU8, file, u);
		CHECKPROP(stg->plr_stage_continues_used_final, SDL_ReadU8, file, u);
	}

	CHECKPROP(stg->num_events, SDL_ReadU16LE, file, u);

	uint32_t checksum = 0;
	CHECKPROP(checksum, SDL_ReadU32LE, file, x);

	if(replay_struct_stage_metadata_checksum(stg, version) + checksum) {
		log_error("%s: Stageinfo is corrupt", ctx->filename);
		RETURN_ERROR;
	}

	return true;
}

static bool _replay_read_meta(ReplayReadContext *ctx) {
	auto rpy = ctx->replay;
	auto file = ctx->stream;
	uint16_t version = rpy->version & ~REPLAY_VERSION_COMPRESSION_BIT;
	ctx->version = version;

	rpy->playername = NULL;

	if(!replay_read_string(ctx, &rpy->playername)) {
		RETURN_ERROR;
	}

	PRINTPROP(rpy->playername, s, file);

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		CHECKPROP(rpy->flags, SDL_ReadU32LE, file, u);
	}

	uint16_t numstages;
	CHECKPROP(numstages, SDL_ReadU16LE, file, u);

	if(!numstages) {
		log_error("%s: No stages in replay", ctx->filename);
		RETURN_ERROR;
	}

	dynarray_ensure_capacity(&rpy->stages, numstages);

	for(int i = 0; i < numstages; ++i) {
		if(!replay_read_stage(ctx, dynarray_append(&rpy->stages))) {
			RETURN_ERROR;
		}
	}

	return true;
}

static bool replay_read_meta(ReplayReadContext *ctx) {
	if(!_replay_read_meta(ctx)) {
		mem_free(ctx->replay->playername);
		ctx->replay->playername = NULL;
		dynarray_free_data(&ctx->replay->stages);
		return false;
	}

	return true;
}

static bool _replay_read_events(ReplayReadContext *ctx) {
	dynarray_foreach_elem(&ctx->replay->stages, ReplayStage *stg, {
		if(!stg->num_events) {
			log_error("%s: No events in stage", ctx->filename);
			RETURN_ERROR;
		}

		dynarray_ensure_capacity(&stg->events, stg->num_events);

		for(int j = 0; j < stg->num_events; ++j) {
			ReplayEvent *evt = dynarray_append(&stg->events, {});

			CHECKPROP(evt->frame, SDL_ReadU32LE, ctx->stream, u);
			CHECKPROP(evt->type, SDL_ReadU8, ctx->stream, u);
			CHECKPROP(evt->value, SDL_ReadU16LE, ctx->stream, u);
		}
	});

	return true;
}

static bool replay_read_events(ReplayReadContext *ctx) {
	if(!_replay_read_events(ctx)) {
		replay_destroy_events(ctx->replay);
		return false;
	}

	return true;
}

bool replay_read(Replay *rpy, SDL_IOStream *file, ReplayReadMode mode, const char *source) {
	int64_t filesize; // must be signed

	if(!source) {
		source = "<unknown>";
	}

	assert((mode & REPLAY_READ_ALL) != 0);
	filesize = SDL_GetIOSize(file);

	if(filesize < 0) {
		log_warn("%s: SDL_GetIOSize() failed: %s", source, SDL_GetError());
	}

	ReplayReadContext _ctx = {
		.replay = rpy,
		.stream = file,
		.filename = source,
		.filesize = filesize,
		.version = rpy->version & ~REPLAY_VERSION_COMPRESSION_BIT,
		.mode = mode,
	};
	auto ctx = &_ctx;

	if(mode & REPLAY_READ_META) {
		*rpy = (typeof(*rpy)) {};

		if(filesize > 0 && filesize <= sizeof(replay_magic_header) + 2) {
			log_error("%s: Replay file is too short (%"PRIi64")", source, filesize);

			if(!(mode & REPLAY_READ_IGNORE_ERRORS)) {
				return false;
			}
		}

		size_t ofs = 0;

		if(!replay_read_header(ctx, &ofs)) {
			RETURN_ERROR;
		}

		bool compression = false;

		if(rpy->version & REPLAY_VERSION_COMPRESSION_BIT) {
			if(rpy->fileoffset < SDL_TellIO(file)) {
				log_error("%s: Invalid offset %"PRIi32"", source, rpy->fileoffset);
				RETURN_ERROR;
			}

			ctx->stream = replay_wrap_stream_decompress(
				rpy->version,
				SDL_RWWrapSegment(file, ofs, rpy->fileoffset, false),
				true
			);

			ctx->filesize = -1;
			compression = true;
		}

		if(!replay_read_meta(ctx)) {
			if(compression) {
				SDL_CloseIO(ctx->stream);
			}

			return false;
		}

		if(compression) {
			SDL_CloseIO(ctx->stream);
			ctx->stream = file;
		} else {
			rpy->fileoffset = SDL_TellIO(file);
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

			if(SDL_SeekIO(file, rpy->fileoffset, SDL_IO_SEEK_SET) < 0) {
				log_error("%s: SDL_SeekIO() failed: %s", source, SDL_GetError());
				return false;
			}
		}

		bool compression = false;

		if(rpy->version & REPLAY_VERSION_COMPRESSION_BIT) {
			ctx->stream = replay_wrap_stream_decompress(rpy->version, file, false);
			ctx->filesize = -1;
			compression = true;
		}

		if(!replay_read_events(ctx)) {
			if(compression) {
				SDL_CloseIO(ctx->stream);
			}

			return false;
		}

		if(compression) {
			SDL_CloseIO(ctx->stream);
		}

		// useless byte to simplify the premature EOF check, can be anything
		uint8_t stupid;
		SDL_ReadU8(file, &stupid);
	}

	return true;
}
