/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "replay.h"

#include <time.h>

#include "global.h"

static uint8_t replay_magic_header[] = REPLAY_MAGIC_HEADER;

void replay_init(Replay *rpy) {
	memset(rpy, 0, sizeof(Replay));
	log_debug("Replay at %p initialized for writing", (void*)rpy);
}

ReplayStage* replay_create_stage(Replay *rpy, StageInfo *stage, uint64_t start_time, uint64_t seed, Difficulty diff, Player *plr) {
	ReplayStage *s;

	rpy->stages = (ReplayStage*)realloc(rpy->stages, sizeof(ReplayStage) * (++rpy->numstages));
	s = rpy->stages + rpy->numstages - 1;
	memset(s, 0, sizeof(ReplayStage));

	get_system_time(&s->init_time);

	dynarray_ensure_capacity(&s->events, REPLAY_ALLOC_INITIAL);

	s->stage = stage->id;
	s->start_time = start_time;
	s->rng_seed = seed;
	s->diff = diff;

	s->plr_pos_x = floor(creal(plr->pos));
	s->plr_pos_y = floor(cimag(plr->pos));

	s->plr_points = plr->points;
	s->plr_continues_used = plr->stats.total.continues_used;
	// s->plr_focus = plr->focus;  FIXME remove and bump version
	s->plr_char = plr->mode->character->id;
	s->plr_shot = plr->mode->shot_mode;
	s->plr_lives = plr->lives;
	s->plr_life_fragments = plr->life_fragments;
	s->plr_bombs = plr->bombs;
	s->plr_bomb_fragments = plr->bomb_fragments;
	s->plr_power = plr->power + plr->power_overflow;
	s->plr_graze = plr->graze;
	s->plr_point_item_value = plr->point_item_value;
	s->plr_inputflags = plr->inputflags;

	log_debug("Created a new stage %p in replay %p", (void*)s, (void*)rpy);
	return s;
}

void replay_stage_sync_player_state(ReplayStage *stg, Player *plr) {
	plr->points = stg->plr_points;
	plr->stats.total.continues_used = stg->plr_continues_used;
	plr->mode = plrmode_find(stg->plr_char, stg->plr_shot);
	plr->pos = stg->plr_pos_x + I * stg->plr_pos_y;
	// plr->focus = stg->plr_focus;  FIXME remove and bump version
	plr->lives = stg->plr_lives;
	plr->life_fragments = stg->plr_life_fragments;
	plr->bombs = stg->plr_bombs;
	plr->bomb_fragments = stg->plr_bomb_fragments;
	plr->power = (stg->plr_power > PLR_MAX_POWER ? PLR_MAX_POWER : stg->plr_power);
	plr->power_overflow = (stg->plr_power > PLR_MAX_POWER ? stg->plr_power - PLR_MAX_POWER : 0);
	plr->graze = stg->plr_graze;
	plr->point_item_value = stg->plr_point_item_value;
	plr->inputflags = stg->plr_inputflags;

	plr->stats.total.lives_used = stg->plr_stats_total_lives_used;
	plr->stats.stage.lives_used = stg->plr_stats_stage_lives_used;
	plr->stats.total.bombs_used = stg->plr_stats_total_bombs_used;
	plr->stats.stage.bombs_used = stg->plr_stats_stage_bombs_used;
	plr->stats.stage.continues_used = stg->plr_stats_stage_continues_used;
}

static void replay_destroy_stage(ReplayStage *stage) {
	dynarray_free_data(&stage->events);
	memset(stage, 0, sizeof(ReplayStage));
}

void replay_destroy_events(Replay *rpy) {
	if(!rpy) {
		return;
	}

	if(rpy->stages) {
		for(int i = 0; i < rpy->numstages; ++i) {
			ReplayStage *stg = rpy->stages + i;
			dynarray_free_data(&stg->events);
		}
	}
}

void replay_destroy(Replay *rpy) {
	if(!rpy) {
		return;
	}

	if(rpy->stages) {
		for(int i = 0; i < rpy->numstages; ++i) {
			replay_destroy_stage(rpy->stages + i);
		}

		free(rpy->stages);
	}

	free(rpy->playername);

	memset(rpy, 0, sizeof(Replay));
}

void replay_stage_event(ReplayStage *stg, uint32_t frame, uint8_t type, uint16_t value) {
	assert(stg != NULL);

	dynarray_size_t old_capacity = stg->events.capacity;

	ReplayEvent *e = dynarray_append(&stg->events);
	e->frame = frame;
	e->type = type;
	e->value = value;

	if(stg->events.capacity > old_capacity && stg->events.capacity > UINT16_MAX) {
		log_error("Too many events in replay; saving WILL FAIL!");
	}

	if(type == EV_OVER) {
		log_debug("The replay is OVER");
	}
}

static void replay_write_string(SDL_RWops *file, char *str, uint16_t version) {
	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		SDL_WriteU8(file, strlen(str));
	} else {
		SDL_WriteLE16(file, strlen(str));
	}

	SDL_RWwrite(file, str, 1, strlen(str));
}

static bool replay_write_events(Replay *rpy, SDL_RWops *file) {
	for(int stgidx = 0; stgidx < rpy->numstages; ++stgidx) {
		dynarray_foreach_elem(&rpy->stages[stgidx].events, ReplayEvent *evt, {
			SDL_WriteLE32(file, evt->frame);
			SDL_WriteU8(file, evt->type);
			SDL_WriteLE16(file, evt->value);
		});
	}

	return true;
}

static uint32_t replay_calc_stageinfo_checksum(ReplayStage *stg, uint16_t version) {
	uint32_t cs = 0;

	cs += stg->stage;
	cs += stg->rng_seed;
	cs += stg->diff;
	cs += stg->plr_points;
	cs += stg->plr_char;
	cs += stg->plr_shot;
	cs += stg->plr_pos_x;
	cs += stg->plr_pos_y;
	cs += stg->plr_focus;  // FIXME remove and bump version
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

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		cs += stg->plr_continues_used;
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

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV0) {
		cs += stg->plr_stats_total_lives_used;
		cs += stg->plr_stats_stage_lives_used;
		cs += stg->plr_stats_total_bombs_used;
		cs += stg->plr_stats_stage_bombs_used;
		cs += stg->plr_stats_stage_continues_used;
	}

	log_debug("%08x", cs);
	return cs;
}

static bool replay_write_stage(ReplayStage *stg, SDL_RWops *file, uint16_t version) {
	assert(version >= REPLAY_STRUCT_VERSION_TS103000_REV2);

	SDL_WriteLE32(file, stg->flags);
	SDL_WriteLE16(file, stg->stage);
	SDL_WriteLE64(file, stg->start_time);
	SDL_WriteLE64(file, stg->rng_seed);
	SDL_WriteU8(file, stg->diff);
	SDL_WriteLE64(file, stg->plr_points);
	SDL_WriteU8(file, stg->plr_continues_used);
	SDL_WriteU8(file, stg->plr_char);
	SDL_WriteU8(file, stg->plr_shot);
	SDL_WriteLE16(file, stg->plr_pos_x);
	SDL_WriteLE16(file, stg->plr_pos_y);
	SDL_WriteU8(file, stg->plr_focus);  // FIXME remove and bump version
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

	if(version >= REPLAY_STRUCT_VERSION_TS104000_REV0) {
		SDL_WriteU8(file, stg->plr_stats_total_lives_used);
		SDL_WriteU8(file, stg->plr_stats_stage_lives_used);
		SDL_WriteU8(file, stg->plr_stats_total_bombs_used);
		SDL_WriteU8(file, stg->plr_stats_stage_bombs_used);
		SDL_WriteU8(file, stg->plr_stats_stage_continues_used);
	}

	if(stg->events.num_elements > UINT16_MAX) {
		log_error("Too many events in replay, cannot write this");
		return false;
	}

	SDL_WriteLE16(file, stg->events.num_elements);
	SDL_WriteLE32(file, 1 + ~replay_calc_stageinfo_checksum(stg, version));

	return true;
}

static void fix_flags(Replay *rpy) {
	for(int i = 0; i < rpy->numstages; ++i) {
		if(!(rpy->stages[i].flags & REPLAY_SFLAG_CLEAR)) {
			rpy->flags &= ~REPLAY_GFLAG_CLEAR;
			return;
		}
	}

	rpy->flags |= REPLAY_GFLAG_CLEAR;
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
		vfile = SDL_RWWrapZlibWriter(
			abuf, RW_DEFLATE_LEVEL_DEFAULT, REPLAY_COMPRESSION_CHUNK_SIZE, false
		);
	}

	replay_write_string(vfile, config_get_str(CONFIG_PLAYERNAME), base_version);
	fix_flags(rpy);

	SDL_WriteLE32(vfile, rpy->flags);
	SDL_WriteLE16(vfile, rpy->numstages);

	for(int i = 0; i < rpy->numstages; ++i) {
		if(!replay_write_stage(rpy->stages + i, vfile, base_version)) {
			if(compression) {
				SDL_RWclose(vfile);
				SDL_RWclose(abuf);
			}

			return false;
		}
	}

	if(compression) {
		SDL_RWclose(vfile);
		SDL_WriteLE32(file, SDL_RWtell(file) + SDL_RWtell(abuf) + 4);
		SDL_RWwrite(file, buf, SDL_RWtell(abuf), 1);
		SDL_RWclose(abuf);
		vfile = SDL_RWWrapZlibWriter(file, RW_DEFLATE_LEVEL_DEFAULT, REPLAY_COMPRESSION_CHUNK_SIZE, false);
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

#ifdef REPLAY_LOAD_DEBUG
#define PRINTPROP(prop,fmt) log_debug(#prop " = %" # fmt " [%"PRIi64" / %"PRIi64"]", prop, SDL_RWtell(file), filesize)
#else
#define PRINTPROP(prop,fmt) (void)(prop)
#endif

#define CHECKPROP(prop,fmt) PRINTPROP(prop,fmt); if(filesize > 0 && SDL_RWtell(file) == filesize) { log_error("%s: Premature EOF", source); return false; }

static void replay_read_string(SDL_RWops *file, char **ptr, uint16_t version) {
	size_t len;

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		len = SDL_ReadU8(file);
	} else {
		len = SDL_ReadLE16(file);
	}

	*ptr = malloc(len + 1);
	memset(*ptr, 0, len + 1);

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
	free(gamev);

	if(compression) {
		CHECKPROP(rpy->fileoffset = SDL_ReadLE32(file), u);
		(*ofs) += 4;
	}

	return true;
}

static bool _replay_read_meta(Replay *rpy, SDL_RWops *file, int64_t filesize, const char *source) {
	uint16_t version = rpy->version & ~REPLAY_VERSION_COMPRESSION_BIT;

	replay_read_string(file, &rpy->playername, version);
	PRINTPROP(rpy->playername, s);

	if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
		CHECKPROP(rpy->flags = SDL_ReadLE32(file), u);
	}

	CHECKPROP(rpy->numstages = SDL_ReadLE16(file), u);

	if(!rpy->numstages) {
		log_error("%s: No stages in replay", source);
		return false;
	}

	rpy->stages = malloc(sizeof(ReplayStage) * rpy->numstages);
	memset(rpy->stages, 0, sizeof(ReplayStage) * rpy->numstages);

	for(int i = 0; i < rpy->numstages; ++i) {
		ReplayStage *stg = rpy->stages + i;

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

		if(version >= REPLAY_STRUCT_VERSION_TS103000_REV0) {
			CHECKPROP(stg->plr_points = SDL_ReadLE64(file), zu);
		} else {
			CHECKPROP(stg->plr_points = SDL_ReadLE32(file), zu);
		}

		if(version >= REPLAY_STRUCT_VERSION_TS102000_REV1) {
			CHECKPROP(stg->plr_continues_used = SDL_ReadU8(file), u);
		}

		CHECKPROP(stg->plr_char = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_shot = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_pos_x = SDL_ReadLE16(file), u);
		CHECKPROP(stg->plr_pos_y = SDL_ReadLE16(file), u);
		CHECKPROP(stg->plr_focus = SDL_ReadU8(file), u);  // FIXME remove and bump version
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

		if(version >= REPLAY_STRUCT_VERSION_TS104000_REV0) {
			CHECKPROP(stg->plr_stats_total_lives_used = SDL_ReadU8(file), u);
			CHECKPROP(stg->plr_stats_stage_lives_used = SDL_ReadU8(file), u);
			CHECKPROP(stg->plr_stats_total_bombs_used = SDL_ReadU8(file), u);
			CHECKPROP(stg->plr_stats_stage_bombs_used = SDL_ReadU8(file), u);
			CHECKPROP(stg->plr_stats_stage_continues_used = SDL_ReadU8(file), u);
		}

		CHECKPROP(stg->num_events = SDL_ReadLE16(file), u);

		if(replay_calc_stageinfo_checksum(stg, version) + SDL_ReadLE32(file)) {
			log_error("%s: Stageinfo is corrupt", source);
			return false;
		}
	}

	return true;
}

static bool replay_read_meta(Replay *rpy, SDL_RWops *file, int64_t filesize, const char *source) {
	rpy->playername = NULL;
	rpy->stages = NULL;

	if(!_replay_read_meta(rpy, file, filesize, source)) {
		free(rpy->playername);
		free(rpy->stages);
		return false;
	}

	return true;
}

static bool _replay_read_events(Replay *rpy, SDL_RWops *file, int64_t filesize, const char *source) {
	for(int i = 0; i < rpy->numstages; ++i) {
		ReplayStage *stg = rpy->stages + i;

		if(!stg->num_events) {
			log_error("%s: No events in stage", source);
			return false;
		}

		dynarray_ensure_capacity(&stg->events, stg->num_events);

		for(int j = 0; j < stg->num_events; ++j) {
			ReplayEvent *evt = dynarray_append(&stg->events);

			CHECKPROP(evt->frame = SDL_ReadLE32(file), u);
			CHECKPROP(evt->type = SDL_ReadU8(file), u);
			CHECKPROP(evt->value = SDL_ReadLE16(file), u);
		}
	}

	return true;
}

static bool replay_read_events(Replay *rpy, SDL_RWops *file, int64_t filesize, const char *source) {
	if(!_replay_read_events(rpy, file, filesize, source)) {
		replay_destroy_events(rpy);
		return false;
	}

	return true;
}

bool replay_read(Replay *rpy, SDL_RWops *file, ReplayReadMode mode, const char *source) {
	int64_t filesize; // must be signed
	SDL_RWops *vfile = file;

	if(!source) {
		source = "<unknown>";
	}

	if(!(mode & REPLAY_READ_ALL) ) {
		log_fatal("%s: Called with invalid read mode %x", source, mode);
	}

	mode &= REPLAY_READ_ALL;
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

			vfile = SDL_RWWrapZlibReader(
				SDL_RWWrapSegment(file, ofs, rpy->fileoffset, false),
				REPLAY_COMPRESSION_CHUNK_SIZE,
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

			for(int i = 0; i < rpy->numstages; ++i) {
				if(rpy->stages->events.data) {
					log_warn("%s: BUG: Reading events into a replay that already had events, call replay_destroy_events() if this is intended", source);
					replay_destroy_events(rpy);
					break;
				}
			}

			if(SDL_RWseek(file, rpy->fileoffset, RW_SEEK_SET) < 0) {
				log_error("%s: SDL_RWseek() failed: %s", source, SDL_GetError());
				return false;
			}
		}

		bool compression = false;

		if(rpy->version & REPLAY_VERSION_COMPRESSION_BIT) {
			vfile = SDL_RWWrapZlibReader(file, REPLAY_COMPRESSION_CHUNK_SIZE, false);
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

#undef CHECKPROP
#undef PRINTPROP

static char* replay_getpath(const char *name, bool ext) {
	return ext ?    strfmt("storage/replays/%s.%s", name, REPLAY_EXTENSION) :
					strfmt("storage/replays/%s",    name);
}

bool replay_save(Replay *rpy, const char *name) {
	char *p = replay_getpath(name, !strendswith(name, REPLAY_EXTENSION));
	char *sp = vfs_repr(p, true);
	log_info("Saving %s", sp);
	free(sp);

	SDL_RWops *file = vfs_open(p, VFS_MODE_WRITE);
	free(p);

	if(!file) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool result = replay_write(rpy, file, REPLAY_STRUCT_VERSION_WRITE);
	SDL_RWclose(file);
	vfs_sync(VFS_SYNC_STORE, NO_CALLCHAIN);
	return result;
}

static const char* replay_mode_string(ReplayReadMode mode) {
	if((mode & REPLAY_READ_ALL) == REPLAY_READ_ALL) {
		return "full";
	}

	if(mode & REPLAY_READ_META) {
		return "meta";
	}

	if(mode & REPLAY_READ_EVENTS) {
		return "events";
	}

	log_fatal("Bad mode %i", mode);
}

bool replay_load(Replay *rpy, const char *name, ReplayReadMode mode) {
	char *p = replay_getpath(name, !strendswith(name, REPLAY_EXTENSION));
	char *sp = vfs_repr(p, true);
	log_info("Loading %s (%s)", sp, replay_mode_string(mode));

	SDL_RWops *file = vfs_open(p, VFS_MODE_READ);
	free(p);

	if(!file) {
		log_error("VFS error: %s", vfs_get_error());
		free(sp);
		return false;
	}

	bool result = replay_read(rpy, file, mode, sp);

	free(sp);
	SDL_RWclose(file);
	return result;
}

bool replay_load_syspath(Replay *rpy, const char *path, ReplayReadMode mode) {
	log_info("Loading %s (%s)", path, replay_mode_string(mode));
	SDL_RWops *file;

#ifndef __WINDOWS__
	if(!strcmp(path, "-"))
		file = SDL_RWFromFP(stdin,false);
	else
		file = SDL_RWFromFile(path, "rb");
#else
	file = SDL_RWFromFile(path, "rb");
#endif

	if(!file) {
		log_error("SDL_RWFromFile() failed: %s", SDL_GetError());
		return false;
	}

	bool result = replay_read(rpy, file, mode, path);

	SDL_RWclose(file);
	return result;
}

void replay_copy(Replay *dst, Replay *src, bool steal_events) {
	int i;

	replay_destroy(dst);
	memcpy(dst, src, sizeof(Replay));

	dst->playername = strdup(src->playername);
	dst->stages = memdup(src->stages, sizeof(*src->stages) * src->numstages);

	for(i = 0; i < src->numstages; ++i) {
		ReplayStage *s, *d;
		s = src->stages + i;
		d = dst->stages + i;

		if(steal_events) {
			memset(&s->events, 0, sizeof(s->events));
		} else {
			dynarray_set_elements(&d->events, s->events.num_elements, s->events.data);
		}
	}
}

void replay_stage_check_desync(ReplayStage *stg, int time, uint16_t check, ReplayMode mode) {
	if(!stg || time % (FPS * 5)) {
		return;
	}

	if(mode == REPLAY_PLAY) {
		if(stg->desync_check && stg->desync_check != check) {
			log_warn("Frame %d: replay desync detected! 0x%04x != 0x%04x", time, stg->desync_check, check);
			stg->desynced = true;

			if(global.is_replay_verification) {
				// log_fatal("Replay verification failed");
				exit(1);
			}
		} else if(global.is_replay_verification) {
			log_info("Frame %d: 0x%04x OK", time, check);
		} else {
			log_debug("Frame %d: 0x%04x OK", time, check);
		}
	}
#ifdef REPLAY_WRITE_DESYNC_CHECKS
	else {
		// log_debug("0x%04x", check);
		replay_stage_event(stg, time, EV_CHECK_DESYNC, (int16_t)check);
	}
#endif
}

int replay_find_stage_idx(Replay *rpy, uint8_t stageid) {
	assert(rpy != NULL);
	assert(rpy->stages != NULL);

	for(int i = 0; i < rpy->numstages; ++i) {
		if(rpy->stages[i].stage == stageid) {
			return i;
		}
	}

	log_warn("Stage %X was not found in the replay", stageid);
	return -1;
}

typedef struct ReplayContext {
	CallChain cc;
	int stage_idx;
} ReplayContext;

static void replay_do_cleanup(CallChainResult ccr);
static void replay_do_play(CallChainResult ccr);
static void replay_do_post_play(CallChainResult ccr);

void replay_play(Replay *rpy, int firstidx, CallChain next) {
	if(rpy != &global.replay) {
		replay_copy(&global.replay, rpy, true);
	}

	if(firstidx >= global.replay.numstages || firstidx < 0) {
		log_error("No stage #%i in the replay", firstidx);
		replay_destroy(&global.replay);
		run_call_chain(&next, NULL);
		return;
	}

	global.replaymode = REPLAY_PLAY;

	ReplayContext *ctx = calloc(1, sizeof(*ctx));
	ctx->cc = next;
	ctx->stage_idx = firstidx;

	replay_do_play(CALLCHAIN_RESULT(ctx, NULL));
}

static void replay_do_play(CallChainResult ccr) {
	ReplayContext *ctx = ccr.ctx;
	ReplayStage *rstg = NULL;
	StageInfo *stginfo = NULL;

	while(ctx->stage_idx < global.replay.numstages) {
		rstg = global.replay_stage = global.replay.stages + ctx->stage_idx++;
		stginfo = stageinfo_get_by_id(rstg->stage);

		if(!stginfo) {
			log_warn("Invalid stage %X in replay at %i skipped.", rstg->stage, ctx->stage_idx);
			continue;
		}

		break;
	}

	if(stginfo == NULL) {
		replay_do_cleanup(ccr);
	} else {
		global.plr.mode = plrmode_find(rstg->plr_char, rstg->plr_shot);
		stage_enter(stginfo, CALLCHAIN(replay_do_post_play, ctx));
	}
}

static void replay_do_post_play(CallChainResult ccr) {
	ReplayContext *ctx = ccr.ctx;

	if(global.gameover == GAMEOVER_ABORT) {
		replay_do_cleanup(ccr);
		return;
	}

	if(global.gameover == GAMEOVER_RESTART) {
		--ctx->stage_idx;
	}

	global.gameover = 0;
	replay_do_play(ccr);
}

static void replay_do_cleanup(CallChainResult ccr) {
	ReplayContext *ctx = ccr.ctx;

	global.gameover = 0;
	global.replaymode = REPLAY_RECORD;
	replay_destroy(&global.replay);
	global.replay_stage = NULL;
	free_resources(false);

	CallChain cc = ctx->cc;
	free(ctx);
	run_call_chain(&cc, NULL);
}
