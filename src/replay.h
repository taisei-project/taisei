/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_replay_h
#define IGUARD_replay_h

#include "taisei.h"

#include "stage.h"
#include "player.h"
#include "version.h"


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
/* END supported struct versions */

#define REPLAY_VERSION_COMPRESSION_BIT 0x8000
#define REPLAY_COMPRESSION_CHUNK_SIZE 4096

// What struct version to use when saving recorded replays
#define REPLAY_STRUCT_VERSION_WRITE (REPLAY_STRUCT_VERSION_TS103000_REV0 | REPLAY_VERSION_COMPRESSION_BIT)

#define REPLAY_ALLOC_INITIAL 256

#define REPLAY_MAGIC_HEADER { 0x68, 0x6f, 0x6e, 0x6f, 0xe2, 0x9d, 0xa4, 0x75, 0x6d, 0x69 }
#define REPLAY_EXTENSION "tsr"
#define REPLAY_USELESS_BYTE 0x69

#define REPLAY_WRITE_DESYNC_CHECKS

#ifdef DEBUG
	// #define REPLAY_LOAD_DEBUG
#endif

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
	uint32_t seed;  // this also happens to be the game initiation time, and we currently use this property
                    // NOTE: this might change eventually
	uint8_t diff;

	// initial player settings
	uint64_t plr_points; // NOTE: before REPLAY_STRUCT_VERSION_TS103000_REV0: uint32_t
	/* BEGIN REPLAY_STRUCT_VERSION_TS102000_REV1 and above */
	uint8_t plr_continues_used;
	/* END REPLAY_STRUCT_VERSION_TS102000_REV1 and above */
	uint8_t plr_char;
	uint8_t plr_shot;
	uint16_t plr_pos_x;
	uint16_t plr_pos_y;
	uint8_t plr_focus;
	uint16_t plr_power;
	uint8_t plr_lives;
	uint8_t plr_life_fragments;
	uint8_t plr_bombs;
	uint8_t plr_bomb_fragments;
	uint8_t plr_inputflags;
	/* BEGIN REPLAY_STRUCT_VERSION_TS102000_REV2 and above */
	uint32_t plr_graze; // NOTE: before REPLAY_STRUCT_VERSION_TS103000_REV0: uint16_t
	/* END REPLAY_STRUCT_VERSION_TS102000_REV2 and above */
	/* BEGIN REPLAY_STRUCT_VERSION_TS103000_REV0 and above */
	uint32_t plr_point_item_value;
	/* END REPLAY_STRUCT_VERSION_TS103000_REV0 and above */

	// player input
	uint16_t numevents;

	// checksum of all of the above -- 2's complement of value returned by replay_calc_stageinfo_checksum()
	// uint32_t checksum;

	/* END stored fields */

	SystemTime init_time;
	ReplayEvent *events;

	// events allocated (may be higher than numevents)
	int capacity;

	// used during playback
	int playpos;
	int fps;
	uint16_t desync_check;
	bool desynced;
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
	uint16_t numstages;

	// Array, contains {numstages} elements when not NULL
	ReplayStage *stages;

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
} Replay;

typedef enum {
	REPLAY_RECORD,
	REPLAY_PLAY
} ReplayMode;

typedef enum ReplayReadMode {
	// bitflags
	REPLAY_READ_META = 1,
	REPLAY_READ_EVENTS = 2,
	REPLAY_READ_ALL = 3, // includes the other two
} ReplayReadMode;

typedef enum ReplayGlobalFlags {
	_REPLAY_GFLAG_NULL,
	REPLAY_GFLAG_CONTINUES          = (1 << 0), // a continue was used in any stage
	REPLAY_GFLAG_CHEATS             = (1 << 1), // a cheat was used in any stage
	REPLAY_GFLAG_CLEAR              = (1 << 2), // all stages in the replay were cleared
} ReplayGlobalFlags;

typedef enum ReplayStageFlags {
	_REPLAY_SFLAG_NULL,
	REPLAY_SFLAG_CONTINUES          = (1 << 0), // a continue was used in this stage
	REPLAY_SFLAG_CHEATS             = (1 << 1), // a cheat was used in this stage
	REPLAY_SFLAG_CLEAR              = (1 << 2), // this stage was cleared
} ReplayStageFlags;

void replay_init(Replay *rpy);
ReplayStage* replay_create_stage(Replay *rpy, StageInfo *stage, uint64_t seed, Difficulty diff, Player *plr);

void replay_destroy(Replay *rpy);
void replay_destroy_events(Replay *rpy);

void replay_stage_event(ReplayStage *stg, uint32_t frame, uint8_t type, uint16_t value);
void replay_stage_check_desync(ReplayStage *stg, int time, uint16_t check, ReplayMode mode);
void replay_stage_sync_player_state(ReplayStage *stg, Player *plr);

bool replay_write(Replay *rpy, SDL_RWops *file, uint16_t version);
bool replay_read(Replay *rpy, SDL_RWops *file, ReplayReadMode mode, const char *source);

bool replay_save(Replay *rpy, const char *name);
bool replay_load(Replay *rpy, const char *name, ReplayReadMode mode);
bool replay_load_syspath(Replay *rpy, const char *path, ReplayReadMode mode);

void replay_copy(Replay *dst, Replay *src, bool steal_events);

void replay_play(Replay *rpy, int firstidx);

int replay_find_stage_idx(Replay *rpy, uint8_t stageid);

#endif // IGUARD_replay_h
