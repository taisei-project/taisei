/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "progress.h"

#include "stageinfo.h"
#include "util.h"
#include "version.h"
#include "rwops/rwops_zstd.h"

#include <zlib.h>

/*
	This module implements a persistent storage of a player's game progress, such as unlocked stages, high-scores etc.

	Basic outline of the progress file structure (little-endian encoding):
		[uint64 magic] [uint32 checksum] [array of commands]

	Where a command is:
		[uint8 command] [uint16 size] [array of {size} bytes, contents depend on {command}]

	This way we can easily extend the structure to store whatever we want, without breaking compatibility.
	This is forwards-compatible as well: any unrecognized command can be skipped when reading, since its size is known.

	The checksum is calculated on the whole array of commands (see progress_checksum() for implementation).
	If the checksum is incorrect, the whole progress file is deemed invalid and ignored.

	All commands are optional and the array may be empty. In that case, the checksum may be omitted as well.
*/

/*
 * Currently implemented commands:
 */
typedef enum ProgfileCommand {
	// NOTE: These values must never change to preserve compatibility with old progress files!

	// Unlocks one or more stages. Only works for stages with fixed difficulty.
	PCMD_UNLOCK_STAGES                     = 0x00,

	// Unlocks one or more stages, each on a specific difficulty
	PCMD_UNLOCK_STAGES_WITH_DIFFICULTY     = 0x01,

	// Sets the high-score (legacy, 32-bit value)
	PCMD_HISCORE                           = 0x02,

	// Sets the times played and times cleared counters for a list of stage/difficulty combinations
	PCMD_STAGE_PLAYINFO                    = 0x03,

	// Sets the the number of times an ending was achieved for a list of endings
	PCMD_ENDINGS                           = 0x04,

	// Sets the last picked difficulty, character and shot mode
	PCMD_GAME_SETTINGS                     = 0x05,

	// Sets the game version this file was last written with
	PCMD_GAME_VERSION                      = 0x06,

	// Unlocks BGMs in the music room
	PCMD_UNLOCK_BGMS                       = 0x07,

	// Unlocks cutscenes in the cutscenes menu
	PCMD_UNLOCK_CUTSCENES                  = 0x08,

	// Sets the high-score (64-bit value)
	PCMD_HISCORE_64BIT                     = 0x09,

	// Sets the high scores and per-plrmode play/clear counters for stage/difficulty combinations
	PCMD_STAGE_PLAYINFO2                   = 0x10,
} ProgfileCommand;

/*

	Now in case you wonder why I decided to do it this way instead of stuffing everything in the config file, here are a couple of reasons:

		- The config module, as of the time of writing, is messy enough and probably needs refactoring.

		- I don't want to mix user preferences with things that are not supposed to be directly edited.

		- I don't want someone to accidentally lose progress after deleting the config file because they wanted to restore the default settings.

		- I want to discourage players from editing this file should they find it. This is why it's not textual and has a checksum.
		  Of course that doesn't actually prevent one from cheating it, but if you know how to do that, you might as well grab the game's source code and make it do whatever you please.
		  As long as this can stop a l33th4XxXxX0r1998 armed with notepad.exe or a hex editor, I'm happy.

*/

#define PROGRESS_FILE_OLD "storage/progress.dat"
#define PROGRESS_FILE "storage/progress.zst"
#define PROGRESS_MAXFILESIZE (1 << 15)

GlobalProgress progress;

static uint8_t progress_magic_bytes[] = {
	0x00, 0x67, 0x74, 0x66, 0x6f, 0xe3, 0x83, 0x84
};

static uint32_t progress_checksum(uint8_t *buf, size_t num) {
	return crc32(0xB16B00B5, buf, num);
}

typedef struct UnknownCmd {
	LIST_INTERFACE(struct UnknownCmd);

	uint8_t cmd;
	uint16_t size;
	uint8_t *data;
} UnknownCmd;

#define PLAYINFO_HEADER_FIELDS \
	PLAYINFO_FIELD(uint16_t, stage) \
	PLAYINFO_FIELD(uint8_t,  diff) \

#define PLAYINFO_FIELDS \
	PLAYINFO_FIELD(uint32_t, num_played) \
	PLAYINFO_FIELD(uint32_t, num_cleared) \

#define PLAYINFO2_FIELDS_GLOBAL \
	PLAYINFO_FIELD(uint64_t, hiscore) \

#define PLAYINFO2_FIELDS_PERMODE \
	PLAYINFO_FIELD(uint32_t, num_played) \
	PLAYINFO_FIELD(uint32_t, num_cleared) \
	PLAYINFO_FIELD(uint64_t, hiscore) \

#define WRITEFUNC_uint64_t SDL_WriteU64LE
#define WRITEFUNC_uint32_t SDL_WriteU32LE
#define WRITEFUNC_uint16_t SDL_WriteU16LE
#define WRITEFUNC_uint8_t  SDL_WriteU8
#define WRITEFUNC(t) WRITEFUNC_##t

#define READFUNC_uint64_t SDL_ReadU64LE
#define READFUNC_uint32_t SDL_ReadU32LE
#define READFUNC_uint16_t SDL_ReadU16LE
#define READFUNC_uint8_t  SDL_ReadU8
#define READFUNC(t) READFUNC_##t

#define PLAYINFO_FIELD(t,f) + sizeof(t)

INLINE size_t playinfo_header_size(void) {
	return PLAYINFO_HEADER_FIELDS;
}

INLINE size_t playinfo_size(uint num_entries) {
	return num_entries * (PLAYINFO_HEADER_FIELDS PLAYINFO_FIELDS);
}

INLINE size_t playinfo2_size(uint num_entries) {
	return num_entries * (
			PLAYINFO_HEADER_FIELDS PLAYINFO2_FIELDS_GLOBAL
			+ NUM_PLAYER_MODES * (PLAYINFO2_FIELDS_PERMODE)
		);
}

#undef PLAYINFO_FIELD

typedef struct PlayInfoHeader {
	#define PLAYINFO_FIELD(t,f) \
		t f;
	PLAYINFO_HEADER_FIELDS
	#undef PLAYINFO_FIELD
} PlayInfoHeader;

static size_t playinfo_header_read(SDL_IOStream *vfile, PlayInfoHeader *h) {
	#define PLAYINFO_FIELD(t,f) \
		READFUNC(t)(vfile, &h->f);
	PLAYINFO_HEADER_FIELDS
	#undef PLAYINFO_FIELD
	return playinfo_header_size();
}

static size_t playinfo_header_write(SDL_IOStream *vfile,
				    const PlayInfoHeader *h) {
	#define PLAYINFO_FIELD(t,f) \
		WRITEFUNC(t)(vfile, h->f);
	PLAYINFO_HEADER_FIELDS
	#undef PLAYINFO_FIELD
	return playinfo_header_size();
}

static size_t playinfo_data_read(SDL_IOStream *vfile, StageProgress *p) {
	#define PLAYINFO_FIELD(t,f) \
		READFUNC(t)(vfile, &p->global.f);
	PLAYINFO_FIELDS
	#undef PLAYINFO_FIELD
	return playinfo_size(1) - playinfo_header_size();
}

static size_t playinfo_data_write(SDL_IOStream *vfile, const StageProgress *p) {
	#define PLAYINFO_FIELD(t,f) \
		WRITEFUNC(t)(vfile, p->global.f);
	PLAYINFO_FIELDS
	#undef PLAYINFO_FIELD
	return playinfo_size(1) - playinfo_header_size();
}

static size_t playinfo2_data_read(SDL_IOStream *vfile, StageProgress *p) {
	#define PLAYINFO_FIELD(t,f) \
		READFUNC(t)(vfile, &p->global.f);
	PLAYINFO2_FIELDS_GLOBAL
	#undef PLAYINFO_FIELD

	static_assert(ARRAY_SIZE(p->per_plrmode) == NUM_CHARACTERS);
	static_assert(ARRAY_SIZE(p->per_plrmode[0]) == NUM_SHOT_MODES_PER_CHARACTER);

	for(int chr = 0; chr < ARRAY_SIZE(p->per_plrmode); ++chr) {
		for(int mod = 0; mod < ARRAY_SIZE(p->per_plrmode[chr]); ++mod) {
			StageProgressContents *c = &p->per_plrmode[chr][mod];
			#define PLAYINFO_FIELD(t,f) \
				READFUNC(t)(vfile, &c->f);
			PLAYINFO2_FIELDS_PERMODE
			#undef PLAYINFO_FIELD
		}
	}

	return playinfo2_size(1) - playinfo_header_size();
}

static size_t playinfo2_data_write(SDL_IOStream *vfile,
				   const StageProgress *p) {
	#define PLAYINFO_FIELD(t,f) \
		WRITEFUNC(t)(vfile, p->global.f);
	PLAYINFO2_FIELDS_GLOBAL
	#undef PLAYINFO_FIELD

	static_assert(ARRAY_SIZE(p->per_plrmode) == NUM_CHARACTERS);
	static_assert(ARRAY_SIZE(p->per_plrmode[0]) == NUM_SHOT_MODES_PER_CHARACTER);

	for(int chr = 0; chr < ARRAY_SIZE(p->per_plrmode); ++chr) {
		for(int mod = 0; mod < ARRAY_SIZE(p->per_plrmode[chr]); ++mod) {
			const StageProgressContents *c = &p->per_plrmode[chr][mod];
			#define PLAYINFO_FIELD(t,f) \
				WRITEFUNC(t)(vfile, c->f);
			PLAYINFO2_FIELDS_PERMODE
			#undef PLAYINFO_FIELD
		}
	}

	return playinfo2_size(1) - playinfo_header_size();
}

static bool progress_read_verify_cmd_size(SDL_IOStream *vfile, uint8_t cmd,
					  uint16_t cmdsize,
					  uint16_t expectsize) {
	if(cmdsize == expectsize) {
		return true;
	}

	log_warn("Command %x with bad size %u ignored", cmd, cmdsize);

	if(SDL_SeekIO(vfile, cmdsize, SDL_IO_SEEK_CUR) < 0) {
		log_sdl_error(LOG_WARN, "SDL_SeekIO");
	}

	return false;
}

static bool progress_read_magic(SDL_IOStream *file) {
	uint8_t buffer[sizeof(progress_magic_bytes)];
	size_t read = SDL_ReadIO(file, buffer, sizeof(buffer));

	if(read == sizeof(buffer)) {
		if(memcmp(buffer, progress_magic_bytes, sizeof(buffer))) {
			log_fatal("Invalid magic header");
			return false;
		} else {
			return true;
		}
	}

	switch(SDL_GetIOStatus(file)) {
		case SDL_IO_STATUS_EOF:
			log_error("Premature EOF");
			return false;
		default:
			log_sdl_error(LOG_ERROR, "SDL_ReadIO");
			return false;
	}

	UNREACHABLE;
}

static void progress_read(SDL_IOStream *file) {
	if(!progress_read_magic(file)) {
		return;
	}

	uint32_t checksum_fromfile;
	// no byteswapping here
	SDL_ReadIO(file, &checksum_fromfile, 4);

	const size_t chunk_size = (1 << 12);
	DYNAMIC_ARRAY(uint8_t) buf = {};

	for(;;) {
		dynarray_ensure_capacity(&buf, buf.num_elements + chunk_size);
		size_t read = SDL_ReadIO(file, buf.data + buf.num_elements, chunk_size);

		switch(SDL_GetIOStatus(file)) {
			case SDL_IO_STATUS_ERROR:
			case SDL_IO_STATUS_WRITEONLY:
			case SDL_IO_STATUS_NOT_READY:
				log_sdl_error(LOG_ERROR, "SDL_ReadIO");
				dynarray_free_data(&buf);
				return;
			default: break;
		}

		buf.num_elements += read;

		if(buf.num_elements > PROGRESS_MAXFILESIZE) {
			log_error("Progress file is too large (>= %i; max is %i)", buf.num_elements, PROGRESS_MAXFILESIZE);
			dynarray_free_data(&buf);
			return;
		}

		if(read == 0) {
			break;
		}
	}

	SDL_IOStream *vfile = SDL_IOFromMem(buf.data, buf.num_elements);
	uint32_t checksum = progress_checksum(buf.data, buf.num_elements);

	if(checksum != checksum_fromfile) {
		log_error("Bad checksum: %x != %x", checksum, checksum_fromfile);
		SDL_CloseIO(vfile);
		dynarray_free_data(&buf);
		return;
	}

	TaiseiVersion version_info = { 0 };

	// TODO: handle premature EOFs better

	while(SDL_TellIO(vfile) < buf.num_elements) {
		uint8_t cmd_u8 = 0xff;
		SDL_ReadU8(vfile, &cmd_u8);
		ProgfileCommand cmd = (int8_t)cmd_u8;
		uint16_t cur = 0;
		uint16_t cmdsize = 0;
		SDL_ReadU16LE(vfile, &cmdsize);

		log_debug("at %i: %i (%i)", cur, cmd, cmdsize);

		switch(cmd) {
			case PCMD_UNLOCK_STAGES:
				while(cur < cmdsize) {
					uint16_t id = 0xffff;
					SDL_ReadU16LE(vfile, &id);
					StageProgress *p = stageinfo_get_progress_by_id(id, D_Any, true);
					if(p) {
						p->unlocked = true;
					}
					cur += 2;
				}
				break;

			case PCMD_UNLOCK_STAGES_WITH_DIFFICULTY:
				while(cur < cmdsize) {
					uint16_t stg = 0xffff;
					uint8_t dflags = 0xff;

					SDL_ReadU16LE(vfile, &stg);
					SDL_ReadU8(vfile, &dflags);

					StageInfo *info = stageinfo_get_by_id(stg);

					for(uint diff = D_Easy; diff <= D_Lunatic && info != NULL; ++diff) {
						if(dflags & (1 << (diff - D_Easy))) {
							StageProgress *p = stageinfo_get_progress(info, diff, true);
							if(p) {
								p->unlocked = true;
							}
						}
					}

					cur += 3;
				}
				break;

			case PCMD_HISCORE: {
				uint32_t score32 = 0;
				SDL_ReadU32LE(vfile, &score32);
				progress.hiscore = score32;
				break;
			}

			case PCMD_HISCORE_64BIT:
				SDL_ReadU64LE(vfile, &progress.hiscore);
				break;

			case PCMD_STAGE_PLAYINFO:
				while(cur < cmdsize) {
					PlayInfoHeader h = {};
					cur += playinfo_header_read(vfile, &h);
					StageProgress *p = stageinfo_get_progress_by_id(h.stage, h.diff, true);
					StageProgress discard;

					if(!p) {
						log_warn("Invalid stage %X ignored", h.stage);
						p = &discard;
					}

					cur += playinfo_data_read(vfile, p);
				}
				break;

			case PCMD_STAGE_PLAYINFO2:
				while(cur < cmdsize) {
					PlayInfoHeader h = {};
					cur += playinfo_header_read(vfile, &h);
					StageProgress *p = stageinfo_get_progress_by_id(h.stage, h.diff, true);
					StageProgress discard;

					if(!p) {
						log_warn("Invalid stage %X ignored", h.stage);
						p = &discard;
					}

					cur += playinfo2_data_read(vfile, p);
				}
				break;

			case PCMD_ENDINGS:
				while(cur < cmdsize) {
					uint8_t ending = 0xff;
					uint32_t num_achieved = 0;

					SDL_ReadU8(vfile, &ending); cur += sizeof(uint8_t);
					SDL_ReadU32LE(vfile, &num_achieved); cur += sizeof(uint32_t);

					if(ending < NUM_ENDINGS) {
						progress.achieved_endings[ending] = num_achieved;
					} else {
						log_warn("Invalid ending %u ignored", ending);
					}
				}
				break;

			case PCMD_GAME_SETTINGS:
				if(progress_read_verify_cmd_size(vfile, cmd, cmdsize, sizeof(uint8_t) * 3)) {
					SDL_ReadU8(vfile, &progress.game_settings.difficulty);
					SDL_ReadU8(vfile, &progress.game_settings.character);
					SDL_ReadU8(vfile, &progress.game_settings.shotmode);
				}
				break;

			case PCMD_GAME_VERSION:
				if(progress_read_verify_cmd_size(vfile, cmd, cmdsize, TAISEI_VERSION_SIZE)) {
					if(version_info.major > 0) {
						log_warn("Multiple version information entries in progress file");
					}

					attr_unused size_t read = taisei_version_read(vfile, &version_info);
					assert(read == TAISEI_VERSION_SIZE);
					char *vstr = taisei_version_tostring(&version_info);
					log_info("Progress file from Taisei v%s", vstr);
					mem_free(vstr);
				}
				break;

			case PCMD_UNLOCK_BGMS:
				if(progress_read_verify_cmd_size(vfile, cmd, cmdsize, sizeof(uint64_t))) {
					uint64_t flags = 0;
					SDL_ReadU64LE(vfile, &flags);
					progress.unlocked_bgms |= flags;
				}
				break;

			case PCMD_UNLOCK_CUTSCENES:
				if(progress_read_verify_cmd_size(vfile, cmd, cmdsize, sizeof(uint64_t))) {
					uint64_t flags = 0;
					SDL_ReadU64LE(vfile, &flags);
					progress.unlocked_cutscenes |= flags;
				}
				break;

			default:
				log_warn("Unknown command %x (%u bytes). Will preserve as-is and not interpret", cmd, cmdsize);

				auto c = ALLOC(UnknownCmd, {
					.cmd = cmd,
					.size = cmdsize,
					.data = ALLOC_ARRAY(cmdsize, uint8_t)
				});
				SDL_ReadIO(vfile, c->data, c->size);
				list_append(&progress.unknown, c);

				break;
		}
	}

	if(version_info.major == 0) {
		log_warn("No version information in progress file, it's probably just old (Taisei v1.1, or an early pre-v1.2 development build)");
	} else {
		TaiseiVersion current_version;
		TAISEI_VERSION_GET_CURRENT(&current_version);
		int cmp = taisei_version_compare(&current_version, &version_info, VCMP_TWEAK);

		if(cmp != 0) {
			char *v_prog = taisei_version_tostring(&version_info);
			char *v_game = taisei_version_tostring(&current_version);

			if(cmp > 0) {
				log_info("Progress file will be automatically upgraded from v%s to v%s upon saving", v_prog, v_game);
			} else {
				log_warn("Progress file will be automatically downgraded from v%s to v%s upon saving", v_prog, v_game);
			}

			mem_free(v_prog);
			mem_free(v_game);
		}
	}

	dynarray_free_data(&buf);
	SDL_CloseIO(vfile);
}

typedef void (*cmd_preparefunc_t)(size_t*, void**);
typedef void (*cmd_writefunc_t)(SDL_IOStream*, void**);

typedef struct cmd_writer_t {
	cmd_preparefunc_t prepare;
	cmd_writefunc_t write;
	void *data;
} cmd_writer_t;

#define CMD_HEADER_SIZE 3

//
//  PCMD_UNLOCK_STAGES
//

static void progress_prepare_cmd_unlock_stages(size_t *bufsize, void **arg) {
	int num_unlocked = 0;

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);
		StageProgress *p = stageinfo_get_progress(stg, D_Any, false);

		if(p && p->unlocked) {
			++num_unlocked;
		}
	}

	if(num_unlocked) {
		*bufsize += CMD_HEADER_SIZE;
		*bufsize += num_unlocked * 2;
	}

	*arg = (void*)(intptr_t)num_unlocked;
}

static void progress_write_cmd_unlock_stages(SDL_IOStream *vfile, void **arg) {
	int num_unlocked = (intptr_t)*arg;

	if(!num_unlocked) {
		return;
	}

	SDL_WriteU8(vfile, PCMD_UNLOCK_STAGES);
	SDL_WriteU16LE(vfile, num_unlocked * 2);

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);
		StageProgress *p = stageinfo_get_progress(stg, D_Any, false);

		if(p && p->unlocked) {
			SDL_WriteU16LE(vfile, stg->id);
		}
	}
}

//
//  PCMD_UNLOCK_STAGES_WITH_DIFFICULTY
//

static void progress_prepare_cmd_unlock_stages_with_difficulties(size_t *bufsize, void **arg) {
	int num_unlocked = 0;

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);

		if(stg->difficulty) {
			continue;
		}

		bool unlocked = false;

		for(Difficulty diff = D_Easy; diff <= D_Lunatic; ++diff) {
			StageProgress *p = stageinfo_get_progress(stg, diff, false);

			if(p && p->unlocked) {
				unlocked = true;
			}
		}

		if(unlocked) {
			++num_unlocked;
		}
	}

	if(num_unlocked) {
		*bufsize += CMD_HEADER_SIZE;
		*bufsize += num_unlocked * 3;
	}

	*arg = (void*)(intptr_t)num_unlocked;
}

static void progress_write_cmd_unlock_stages_with_difficulties(SDL_IOStream *vfile,
							       void **arg) {
	int num_unlocked = (intptr_t)*arg;

	if(!num_unlocked) {
		return;
	}

	SDL_WriteU8(vfile, PCMD_UNLOCK_STAGES_WITH_DIFFICULTY);
	SDL_WriteU16LE(vfile, num_unlocked * 3);

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);

		if(stg->difficulty) {
			continue;
		}

		uint8_t dflags = 0;

		for(Difficulty diff = D_Easy; diff <= D_Lunatic; ++diff) {
			StageProgress *p = stageinfo_get_progress(stg, diff, false);
			if(p && p->unlocked) {
				dflags |= (uint)pow(2, diff - D_Easy);
			}
		}

		if(dflags) {
			SDL_WriteU16LE(vfile, stg->id);
			SDL_WriteU8(vfile, dflags);
		}
	}
}

//
//  PCMD_HISCORE / PCMD_HISCORE_64BIT
//

static void progress_prepare_cmd_hiscore(size_t *bufsize, void **arg) {
	*bufsize += CMD_HEADER_SIZE + sizeof(uint32_t);
	*bufsize += CMD_HEADER_SIZE + sizeof(uint64_t);
}

static void progress_write_cmd_hiscore(SDL_IOStream *vfile, void **arg) {
	// Legacy 32-bit command for compatibility
	// NOTE: must be written BEFORE the 64-bit command

	SDL_WriteU8(vfile, PCMD_HISCORE);
	SDL_WriteU16LE(vfile, sizeof(uint32_t));
	SDL_WriteU32LE(vfile, progress.hiscore & 0xFFFFFFFFu);

	SDL_WriteU8(vfile, PCMD_HISCORE_64BIT);
	SDL_WriteU16LE(vfile, sizeof(uint64_t));
	SDL_WriteU64LE(vfile, progress.hiscore);
}

//
//  PCMD_STAGE_PLAYINFO / PCMD_STAGE_PLAYINFO2
//

struct cmd_stage_playinfo_data_elem {
	PlayInfoHeader head;
	StageProgress *prog;
};

struct cmd_stage_playinfo_data {
	DYNAMIC_ARRAY(struct cmd_stage_playinfo_data_elem) elems;
};

static void progress_prepare_cmd_stage_playinfo(size_t *bufsize, void **arg) {
	auto data = ALLOC(struct cmd_stage_playinfo_data);

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);
		Difficulty d_first, d_last;

		if(stg->difficulty == D_Any) {
			d_first = D_Easy;
			d_last = D_Lunatic;
		} else {
			d_first = d_last = D_Any;
		}

		for(Difficulty d = d_first; d <= d_last; ++d) {
			StageProgress *p = stageinfo_get_progress(stg, d, false);

			if(p && (p->global.num_played || p->global.num_cleared)) {
				dynarray_append(&data->elems, {
					.head.stage = stg->id,
					.head.diff = d,
					.prog = p,
				});
			}
		}
	}

	*arg = data;

	if(data->elems.num_elements) {
		*bufsize += CMD_HEADER_SIZE + playinfo_size(data->elems.num_elements);
		*bufsize += CMD_HEADER_SIZE + playinfo2_size(data->elems.num_elements);
	}
}

static void progress_write_cmd_stage_playinfo(SDL_IOStream *vfile, void **arg) {
	struct cmd_stage_playinfo_data *data = *arg;

	if(!data->elems.num_elements) {
		goto cleanup;
	}

	SDL_WriteU8(vfile, PCMD_STAGE_PLAYINFO);
	SDL_WriteU16LE(vfile, playinfo_size(data->elems.num_elements));

	dynarray_foreach_elem(&data->elems, auto e, {
		playinfo_header_write(vfile, &e->head);
		playinfo_data_write(vfile, e->prog);
	});

	SDL_WriteU8(vfile, PCMD_STAGE_PLAYINFO2);
	SDL_WriteU16LE(vfile, playinfo2_size(data->elems.num_elements));

	dynarray_foreach_elem(&data->elems, auto e, {
		playinfo_header_write(vfile, &e->head);
		playinfo2_data_write(vfile, e->prog);
	});

cleanup:
	dynarray_free_data(&data->elems);
	mem_free(data);
}

//
//  PCMD_ENDINGS
//

static void progress_prepare_cmd_endings(size_t *bufsize, void **arg) {
	int n = 0;
	*arg = 0;

	for(int i = 0; i < NUM_ENDINGS; ++i) {
		if(progress.achieved_endings[i]) {
			++n;
		}
	}

	if(n) {
		uint16_t sz = n * (sizeof(uint8_t) + sizeof(uint32_t));
		*arg = (void*)(uintptr_t)sz;
		*bufsize += CMD_HEADER_SIZE + sz;
	}
}

static void progress_write_cmd_endings(SDL_IOStream *vfile, void **arg) {
	uint16_t sz = (uintptr_t)*arg;

	if(!sz) {
		return;
	}

	SDL_WriteU8(vfile, PCMD_ENDINGS);
	SDL_WriteU16LE(vfile, sz);

	for(int i = 0; i < NUM_ENDINGS; ++i) {
		if(progress.achieved_endings[i]) {
			SDL_WriteU8(vfile, i);
			SDL_WriteU32LE(vfile, progress.achieved_endings[i]);
		}
	}
}

//
//  PCMD_GAME_SETTINGS
//

static void progress_prepare_cmd_game_settings(size_t *bufsize, void **arg) {
	*bufsize += CMD_HEADER_SIZE + sizeof(uint8_t) * 3;
}

static void progress_write_cmd_game_settings(SDL_IOStream *vfile, void **arg) {
	SDL_WriteU8(vfile, PCMD_GAME_SETTINGS);
	SDL_WriteU16LE(vfile, sizeof(uint8_t) * 3);
	SDL_WriteU8(vfile, progress.game_settings.difficulty);
	SDL_WriteU8(vfile, progress.game_settings.character);
	SDL_WriteU8(vfile, progress.game_settings.shotmode);
}

//
//  PCMD_GAME_VERSION
//

static void progress_prepare_cmd_game_version(size_t *bufsize, void **arg) {
	*bufsize += CMD_HEADER_SIZE + TAISEI_VERSION_SIZE;
}

static void progress_write_cmd_game_version(SDL_IOStream *vfile, void **arg) {
	SDL_WriteU8(vfile, PCMD_GAME_VERSION);
	SDL_WriteU16LE(vfile, TAISEI_VERSION_SIZE);

	TaiseiVersion v;
	TAISEI_VERSION_GET_CURRENT(&v);

	// the buffer consistency check should fail later if there are any errors here
	taisei_version_write(vfile, &v);
}

//
//  PCMD_UNLOCK_BGMS
//

static void progress_prepare_cmd_unlock_bgms(size_t *bufsize, void **arg) {
	*bufsize += CMD_HEADER_SIZE + sizeof(uint64_t);
}

static void progress_write_cmd_unlock_bgms(SDL_IOStream *vfile, void **arg) {
	SDL_WriteU8(vfile, PCMD_UNLOCK_BGMS);
	SDL_WriteU16LE(vfile, sizeof(uint64_t));
	SDL_WriteU64LE(vfile, progress.unlocked_bgms);
}

//
//  PCMD_UNLOCK_CUTSCENES
//

static void progress_prepare_cmd_unlock_cutscenes(size_t *bufsize, void **arg) {
	*bufsize += CMD_HEADER_SIZE + sizeof(uint64_t);
}

static void progress_write_cmd_unlock_cutscenes(SDL_IOStream *vfile,
						void **arg) {
	SDL_WriteU8(vfile, PCMD_UNLOCK_CUTSCENES);
	SDL_WriteU16LE(vfile, sizeof(uint64_t));
	SDL_WriteU64LE(vfile, progress.unlocked_cutscenes);
}

//
//  Copy unhandled commands from the original file
//

static void progress_prepare_cmd_unknown(size_t *bufsize, void **arg) {
	for(UnknownCmd *c = progress.unknown; c; c = c->next) {
		*bufsize += CMD_HEADER_SIZE + c->size;
	}
}

static void progress_write_cmd_unknown(SDL_IOStream *vfile, void **arg) {
	for(UnknownCmd *c = progress.unknown; c; c = c->next) {
		SDL_WriteU8(vfile, c->cmd);
		SDL_WriteU16LE(vfile, c->size);
		SDL_WriteIO(vfile, c->data, c->size);
	}
}

//
//  Test
//

attr_unused static void progress_prepare_cmd_test(size_t *bufsize, void **arg) {
	*bufsize += CMD_HEADER_SIZE + sizeof(uint32_t);
}

attr_unused static void progress_write_cmd_test(SDL_IOStream *vfile, void **arg) {
	SDL_WriteU8(vfile, 0x7f);
	SDL_WriteU16LE(vfile, sizeof(uint32_t));
	SDL_WriteU32LE(vfile, 0xdeadbeef);
}

static void progress_write(SDL_IOStream *file) {
	size_t bufsize = 0;
	SDL_WriteIO(file, progress_magic_bytes,
		    sizeof(progress_magic_bytes));

	cmd_writer_t cmdtable[] = {
		{progress_prepare_cmd_game_version, progress_write_cmd_game_version, NULL},
		{progress_prepare_cmd_unlock_stages, progress_write_cmd_unlock_stages, NULL},
		{progress_prepare_cmd_unlock_stages_with_difficulties, progress_write_cmd_unlock_stages_with_difficulties, NULL},
		{progress_prepare_cmd_hiscore, progress_write_cmd_hiscore, NULL},
		{progress_prepare_cmd_stage_playinfo, progress_write_cmd_stage_playinfo, NULL},
		{progress_prepare_cmd_endings, progress_write_cmd_endings, NULL},
		{progress_prepare_cmd_game_settings, progress_write_cmd_game_settings, NULL},
		// {progress_prepare_cmd_test, progress_write_cmd_test, NULL},
		{progress_prepare_cmd_unlock_bgms, progress_write_cmd_unlock_bgms, NULL},
		{progress_prepare_cmd_unlock_cutscenes, progress_write_cmd_unlock_cutscenes, NULL},
		{progress_prepare_cmd_unknown, progress_write_cmd_unknown, NULL},
		{NULL}
	};

	for(cmd_writer_t *w = cmdtable; w->prepare; ++w) {
		attr_unused size_t oldsize = bufsize;
		w->prepare(&bufsize, &w->data);
		log_debug("prepare %i: %i", (int)(w - cmdtable), (int)(bufsize - oldsize));
	}

	if(!bufsize) {
		return;
	}

	auto buf = ALLOC_ARRAY(bufsize, uint8_t);
	memset(buf, 0x7f, bufsize);
	SDL_IOStream *vfile = SDL_IOFromMem(buf, bufsize);

	for(cmd_writer_t *w = cmdtable; w->prepare; ++w) {
		attr_unused size_t oldpos = SDL_TellIO(vfile);
		w->write(vfile, &w->data);
		log_debug("write %i: %i", (int)(w - cmdtable),
			  (int)(SDL_TellIO(vfile) - oldpos));
	}

	if(SDL_TellIO(vfile) != bufsize) {
		mem_free(buf);
		log_fatal("Buffer is inconsistent");
		return;
	}

	uint32_t cs = progress_checksum(buf, bufsize);
	// no byteswapping here
	SDL_WriteIO(file, &cs, 4);

	if(!SDL_WriteIO(file, buf, bufsize)) {
		log_error("SDL_WriteIO() failed: %s", SDL_GetError());
		return;
	}

	mem_free(buf);
	SDL_CloseIO(vfile);
}

void progress_unlock_all(void) {
	size_t num_stages = stageinfo_get_num_stages();

	for(size_t i = 0; i < num_stages; ++i) {
		StageInfo *stg = NOT_NULL(stageinfo_get_by_index(i));

		for(Difficulty diff = D_Any; diff <= D_Lunatic; ++diff) {
			StageProgress *p = stageinfo_get_progress(stg, diff, true);
			if(p) {
				p->unlocked = true;
			}
		}
	}

	progress.unlocked_bgms = UINT64_MAX;
	progress.unlocked_cutscenes = UINT64_MAX;

	for(int i = 0; i < NUM_ENDINGS; ++i) {
		progress.achieved_endings[i] = max(1, progress.achieved_endings[i]);
	}
}

static void fix_ending_cutscene(EndingID ending, CutsceneID cutscene) {
	if(progress.achieved_endings[ending] > 0) {
		progress_unlock_cutscene(cutscene);
	}
}

static SDL_IOStream *progress_open_file_read(void) {
	SDL_IOStream *file = vfs_open(PROGRESS_FILE, VFS_MODE_READ);

	if(file) {
		return SDL_RWWrapZstdReader(file, true);
	}

	log_error("VFS error: %s; trying legacy path", vfs_get_error());

	file = vfs_open(PROGRESS_FILE_OLD, VFS_MODE_READ);

	if(file) {
		return file;
	}

	log_error("VFS error: %s", vfs_get_error());
	return NULL;
}

static SDL_IOStream *progress_open_file_write(void) {
	SDL_IOStream *file = vfs_open(PROGRESS_FILE, VFS_MODE_WRITE);

	if(file) {
		return SDL_RWWrapZstdWriter(file, 20, true);
	}

	log_error("VFS error: %s", vfs_get_error());
	return NULL;
}

void progress_load(void) {
	memset(&progress, 0, sizeof(GlobalProgress));

#ifdef PROGRESS_UNLOCK_ALL
	progress_unlock_all();
	progress_save();
#endif

	SDL_IOStream *file = progress_open_file_read();

	if(!file) {
		return;
	}

	progress_read(file);
	SDL_CloseIO(file);

	// Fixup old saves
	fix_ending_cutscene(ENDING_BAD_REIMU,   CUTSCENE_ID_REIMU_BAD_END);
	fix_ending_cutscene(ENDING_GOOD_REIMU,  CUTSCENE_ID_REIMU_GOOD_END);
	fix_ending_cutscene(ENDING_BAD_MARISA,  CUTSCENE_ID_MARISA_BAD_END);
	fix_ending_cutscene(ENDING_GOOD_MARISA, CUTSCENE_ID_MARISA_GOOD_END);
	fix_ending_cutscene(ENDING_BAD_YOUMU,   CUTSCENE_ID_YOUMU_BAD_END);
	fix_ending_cutscene(ENDING_GOOD_YOUMU,  CUTSCENE_ID_YOUMU_GOOD_END);
}

void progress_save(void) {
	SDL_IOStream *file = progress_open_file_write();

	progress_write(file);
	SDL_CloseIO(file);
}

static void* delete_unknown_cmd(List **dest, List *elem, void *arg) {
	UnknownCmd *cmd = (UnknownCmd*)elem;
	mem_free(cmd->data);
	mem_free(list_unlink(dest, elem));
	return NULL;
}

void progress_unload(void) {
	list_foreach(&progress.unknown, delete_unknown_cmd, NULL);
}

void progress_track_ending(EndingID id) {

	log_debug("Ending ID tracked: #%i", id);
	assert((uint)id < NUM_ENDINGS);
	progress.achieved_endings[id]++;
}

uint32_t progress_times_any_ending_achieved(void) {
	uint x = 0;

	for(uint i = 0; i < NUM_ENDINGS; ++i) {
		x += progress.achieved_endings[i];
	}

	return x;
}

uint32_t progress_times_any_good_ending_achieved(void) {
	uint x = 0;

	#define ENDING(e) x += progress.achieved_endings[e];
	GOOD_ENDINGS
	#undef ENDING

	return x;
}

static ProgressBGMID progress_bgm_id(const char *bgm) {
	static const char* map[] = {
		[PBGM_MENU]         = "menu",
		[PBGM_STAGE1]       = "stage1",
		[PBGM_STAGE1_BOSS]  = "stage1boss",
		[PBGM_STAGE2]       = "stage2",
		[PBGM_STAGE2_BOSS]  = "stage2boss",
		[PBGM_STAGE3]       = "stage3",
		[PBGM_STAGE3_BOSS]  = "stage3boss",
		[PBGM_STAGE4]       = "stage4",
		[PBGM_STAGE4_BOSS]  = "stage4boss",
		[PBGM_STAGE5]       = "stage5",
		[PBGM_STAGE5_BOSS]  = "stage5boss",
		[PBGM_STAGE6]       = "stage6",
		[PBGM_STAGE6_BOSS1] = "stage6boss_phase1",
		[PBGM_STAGE6_BOSS2] = "stage6boss_phase2",
		[PBGM_STAGE6_BOSS3] = "stage6boss_phase3",
		[PBGM_ENDING]       = "ending",
		[PBGM_CREDITS]      = "credits",
		[PBGM_BONUS0]       = "bonus0",
		[PBGM_BONUS1]       = "scuttle",
		[PBGM_GAMEOVER]     = "gameover",
		[PBGM_INTRO]        = "intro",
	};

	for(int i = 0; i < ARRAY_SIZE(map); ++i) {
		if(!strcmp(map[i], bgm)) {
			return i;
		}
	}

	UNREACHABLE;
}

static inline uint64_t progress_bgm_bit(ProgressBGMID id) {
	return (UINT64_C(1) << id);
}

bool progress_is_bgm_unlocked(const char *name) {
	return progress.unlocked_bgms & progress_bgm_bit(progress_bgm_id(name));
}

bool progress_is_bgm_id_unlocked(ProgressBGMID id) {
	return progress.unlocked_bgms & progress_bgm_bit(id);
}

void progress_unlock_bgm(const char *name) {
	progress.unlocked_bgms |= progress_bgm_bit(progress_bgm_id(name));
}

static inline uint64_t progress_cutscene_bit(CutsceneID id) {
	return (UINT64_C(1) << id);
}

bool progress_is_cutscene_unlocked(CutsceneID id) {
	return progress.unlocked_cutscenes & progress_cutscene_bit(id);
}

void progress_unlock_cutscene(CutsceneID id) {
	progress.unlocked_cutscenes |= progress_cutscene_bit(id);
}

attr_returns_nonnull attr_nonnull_all
static StageProgressContents *stage_plrmode_data(StageProgress *p, PlayerMode *pm) {
	uint ichar = pm->character->id;
	uint imode = pm->shot_mode;

	assert(ichar < ARRAY_SIZE(p->per_plrmode));
	assert(imode < ARRAY_SIZE(p->per_plrmode[ichar]));

	return &p->per_plrmode[ichar][imode];
}

void progress_register_stage_played(StageProgress *p, PlayerMode *pm) {
	p->unlocked = true;
	p->global.num_played += 1;
	auto c = stage_plrmode_data(p, pm);
	c->num_played += 1;

	char tmp[64];
	plrmode_repr(tmp, sizeof(tmp), pm, false);

	log_debug("Stage played %i times total; %i as %s",
		p->global.num_played,
		c->num_played,
		tmp
	);
}

void progress_register_stage_cleared(StageProgress *p, PlayerMode *pm) {
	p->unlocked = true;
	p->global.num_cleared += 1;
	auto c = stage_plrmode_data(p, pm);
	c->num_cleared += 1;

	char tmp[64];
	plrmode_repr(tmp, sizeof(tmp), pm, false);

	log_debug("Stage cleared %i times total; %i as %s",
		p->global.num_cleared,
		c->num_cleared,
		tmp
	);
}

void progress_register_hiscore(StageProgress *p, PlayerMode *pm, uint64_t score) {
	if(score > progress.hiscore) {
		progress.hiscore = score;
	}

	if(score > p->global.hiscore) {
		p->global.hiscore = score;
	}

	auto c = stage_plrmode_data(p, pm);

	if(score > c->hiscore) {
		c->hiscore = score;
	}
}
