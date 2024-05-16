/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "version.h"
#include "util/systime.h"
#include "dynarray.h"

/*
 *  All stored fields in the Replay* structures are in the order in which they appear in the file.
 *  If a field is commented out, that means it *IS* stored in the file, just not used outside of the loading routine.
 *
 *  Please maintain this convention, it makes it easier to grasp the replay file structure just by looking at this header.
 */


// The struct version is a numeric designation given to the replay file format.
// Always bump it when making incompatible changes to the layout.
// If dropping support for a version, comment out its #define and remove all related code.

/* BEGIN supported struct versions */
	// Taisei v1.1 legacy format
	#define REPLAY_STRUCT_VERSION_TS101000 5

	// Taisei v1.2 revision 0: adds game version information
	#define REPLAY_STRUCT_VERSION_TS102000_REV0 6

	// Taisei v1.2 revision 1: adds flags, stageflags and continues; reduces playername size to 255 bytes
	#define REPLAY_STRUCT_VERSION_TS102000_REV1 7

	// Taisei v1.2 revision 2: adds graze points
	#define REPLAY_STRUCT_VERSION_TS102000_REV2 8

	// Taisei v1.3 revision 0: adds piv; expands points to 64bit, graze to 32bit
	#define REPLAY_STRUCT_VERSION_TS103000_REV0 9

	// Taisei v1.3 revision 1: expands life and bomb fragments to 16bit
	#define REPLAY_STRUCT_VERSION_TS103000_REV1 10

	// Taisei v1.3 revision 2: RNG changed; seed separated from start time; time expanded to 64bit
	#define REPLAY_STRUCT_VERSION_TS103000_REV2 11

	// Taisei v1.3 revision 3: add final score at the end of each stage
	#define REPLAY_STRUCT_VERSION_TS103000_REV3 12

	// Taisei v1.4 revision 0: add statistics for player
	#define REPLAY_STRUCT_VERSION_TS104000_REV0 13

	// Taisei v1.4 revision 1: switch to zstd compression, remove plr_focus, add skip_frames (for demos), rework/fix player resource usage stats
	#define REPLAY_STRUCT_VERSION_TS104000_REV1 14
/* END supported struct versions */

#define REPLAY_VERSION_COMPRESSION_BIT 0x8000
#define REPLAY_COMPRESSION_CHUNK_SIZE 4096

// What struct version to use when saving recorded replays
#define REPLAY_STRUCT_VERSION_WRITE \
	(REPLAY_STRUCT_VERSION_TS104000_REV1 | REPLAY_VERSION_COMPRESSION_BIT)

#define REPLAY_ALLOC_INITIAL 256

#define REPLAY_MAGIC_HEADER { 0x68, 0x6f, 0x6e, 0x6f, 0xe2, 0x9d, 0xa4, 0x75, 0x6d, 0x69 }
#define REPLAY_EXTENSION "tsr"
#define REPLAY_USELESS_BYTE 0x69

#define REPLAY_MAGIC_HEADER_SIZE (sizeof((uint8_t[])REPLAY_MAGIC_HEADER))

typedef struct ReplayEvent {
	/* BEGIN stored fields */

	uint32_t frame;
	uint8_t type;
	uint16_t value;

	/* END stored fields */
} ReplayEvent;

typedef struct ReplayStage {
	/* BEGIN stored fields */

	/* BEGIN REPLAY_STRUCT_VERSION_TS102000_REV1 and above */

	// Bitfield, various parameters that apply to this specific stage
	uint32_t flags;

	/* END REPLAY_STRUCT_VERSION_TS102000_REV1 and above */

	// initial game settings
	uint16_t stage; // must match type of StageInfo.id in stage.h
	/* BEGIN REPLAY_STRUCT_VERSION_TS103000_REV2 and above */
	uint64_t start_time;
	/* END REPLAY_STRUCT_VERSION_TS103000_REV2 and above */
	uint64_t rng_seed; // NOTE: before REPLAY_STRUCT_VERSION_TS103000_REV2: uint32_t, also specifies start_time
	uint8_t diff;
	/* BEGIN REPLAY_STRUCT_VERSION_TS104000_REV1 and above */
	uint16_t skip_frames;
	/* END REPLAY_STRUCT_VERSION_TS104000_REV1 and above */

	// initial player settings
	uint64_t plr_points; // NOTE: before REPLAY_STRUCT_VERSION_TS103000_REV0: uint32_t
	/* BEGIN REPLAY_STRUCT_VERSION_TS104000_REV1 and above */
	uint8_t plr_total_lives_used;
	uint8_t plr_total_bombs_used;
	/* END REPLAY_STRUCT_VERSION_TS104000_REV1 and above */
	/* BEGIN REPLAY_STRUCT_VERSION_TS102000_REV1 and above */
	uint8_t plr_total_continues_used;
	/* END REPLAY_STRUCT_VERSION_TS102000_REV1 and above */
	uint8_t plr_char;
	uint8_t plr_shot;
	uint16_t plr_pos_x;
	uint16_t plr_pos_y;
	/* BEGIN REPLAY_STRUCT_VERSION_TS104000_REV0 and below */
	// NOTE: Not used anymore
	// uint8_t plr_focus;
	/* END REPLAY_STRUCT_VERSION_TS104000_REV0 and below */
	uint16_t plr_power;
	uint8_t plr_lives;
	uint16_t plr_life_fragments; // NOTE: before REPLAY_STRUCT_VERSION_TS103000_REV1: uint8_t
	uint8_t plr_bombs;
	uint16_t plr_bomb_fragments; // NOTE: before REPLAY_STRUCT_VERSION_TS103000_REV1: uint8_t
	uint8_t plr_inputflags;
	/* BEGIN REPLAY_STRUCT_VERSION_TS102000_REV2 and above */
	uint32_t plr_graze; // NOTE: before REPLAY_STRUCT_VERSION_TS103000_REV0: uint16_t
	/* END REPLAY_STRUCT_VERSION_TS102000_REV2 and above */
	/* BEGIN REPLAY_STRUCT_VERSION_TS103000_REV0 and above */
	uint32_t plr_point_item_value;
	/* END REPLAY_STRUCT_VERSION_TS103000_REV0 and above */

	/* BEGIN REPLAY_STRUCT_VERSION_TS102000_REV3 and above */
	// player score at the end of the stage
	uint64_t plr_points_final;
	/* END REPLAY_STRUCT_VERSION_TS102000_REV3 and above */

	/* BEGIN REPLAY_STRUCT_VERSION_TS104000_REV0 only */
	// NOTE: These never worked correctly and were always 0
	// uint8_t plr_stats_total_lives_used;
	// uint8_t plr_stats_stage_lives_used;
	// uint8_t plr_stats_total_bombs_used;
	// uint8_t plr_stats_stage_bombs_used;
	// uint8_t plr_stats_stage_continues_used;
	/* END REPLAY_STRUCT_VERSION_TS104000_REV0 only */

	/* BEGIN REPLAY_STRUCT_VERSION_TS104000_REV1 and above */
	// Resources used in this stage by the end of it
	uint8_t plr_stage_lives_used_final;
	uint8_t plr_stage_bombs_used_final;
	uint8_t plr_stage_continues_used_final;
	/* END REPLAY_STRUCT_VERSION_TS104000_REV1 and above */

	// player input
	// NOTE: only used to read the number of events from file.
	// Actual numbers of events stored/allocated are tracked by the .events dynamic array.
	uint16_t num_events;

	// checksum of all of the above -- 2's complement of value returned by replay_calc_stageinfo_checksum()
	// uint32_t checksum;

	/* END stored fields */

	SystemTime init_time;
	DYNAMIC_ARRAY(ReplayEvent) events;
} ReplayStage;

typedef struct Replay {
	/* BEGIN stored fields */

	// each byte must be equal to the corresponding byte in replay_magic_header.
	// uint8_t[sizeof(replay_magic_header)];

	// must be equal to REPLAY_STRUCT_VERSION
	uint16_t version;

	/* BEGIN REPLAY_STRUCT_VERSION_TS102000_REV0 and above */

	// Game version this replay was recorded on
	TaiseiVersion game_version;

	/* END REPLAY_STRUCT_VERSION_TS102000_REV0 and above */

	// Where the events begin
	// NOTE: this is not present in uncompressed replays!
	uint32_t fileoffset;

	// How many bytes is {playername} long. The string is not null terminated in the file, but is null terminated after it's been loaded into this struct
	// REPLAY_STRUCT_VERSION_TS102000_REV1 and above:
	//      uint8_t playername_size;
	// REPLAY_STRUCT_VERSION_TS102000_REV0 and below:
	//      uint16_t playername_size;

	// The player UTF8-encoded player name
	char *playername;

	/* BEGIN REPLAY_STRUCT_VERSION_TS102000_REV1 and above */

	// Bitfield, various parameters that apply to the whole replay
	uint32_t flags;

	/* END REPLAY_STRUCT_VERSION_TS102000_REV1 and above */

	// Number of stages in this replay
	// uint16_t numstages;

	// Array, contains {numstages} elements when not NULL
	// ReplayStage stages[];

	// ALL input events from ALL of the stages
	// This is actually loaded into separate sub-arrays for every stage, see ReplayStage.events
	//
	// All input events are stored at the very end of the replay so that we can save some time and memory
	// by only loading them when necessary without seeking around the file too much.
	//
	// ReplayStage input_events[];

	// at least one trailing byte, value doesn't matter
	// uint8_t useless;

	/* END stored fields */

	DYNAMIC_ARRAY(ReplayStage) stages;
} Replay;

#define REPLAY_GFLAGS \
	REPLAY_GFLAG(CONTINUES,   0) /* a continue was used in any stage */ \
	REPLAY_GFLAG(CHEATS,      1) /* a cheat was used in any stage */ \
	REPLAY_GFLAG(CLEAR,       2) /* all stages in the replay were cleared */ \

#define REPLAY_SFLAGS \
	REPLAY_SFLAG(CONTINUES,   0) /* a continue was used in any stage */ \
	REPLAY_SFLAG(CHEATS,      1) /* a cheat was used in any stage */ \
	REPLAY_SFLAG(CLEAR,       2) /* all stages in the replay were cleared */ \

typedef enum ReplayGlobalFlags {
	#define REPLAY_GFLAG(name, bit) \
		REPLAY_GFLAG_##name = (1 << bit),
	REPLAY_GFLAGS
	#undef REPLAY_GFLAG
} ReplayGlobalFlags;

typedef enum ReplayStageFlags {
	#define REPLAY_SFLAG(name, bit) \
		REPLAY_SFLAG_##name = (1 << bit),
	REPLAY_SFLAGS
	#undef REPLAY_SFLAG
} ReplayStageFlags;
