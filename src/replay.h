/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef REPLAY_H
#define REPLAY_H

#include "stage.h"
#include "player.h"


/*
 *	All stored fields in the Replay* structures are in the order in which they appear in the file.
 *	If a field is commented out, that means it *IS* stored in the file, just not used outside of the loading routine.
 *
 *	Please maintain this convention, it makes it easier to grasp the replay file structure just by looking at this header.
 */

// -{ ALWAYS UPDATE THESE WHEN YOU MAKE CHANGES TO THE FILE/STRUCT LAYOUT!

// Lets us fail early on incompatible versions and garbage data
#define REPLAY_STRUCT_VERSION 4

// -}

#define REPLAY_ALLOC_INITIAL 256

#define REPLAY_MAGIC_HEADER { 0x68, 0x6f, 0x6e, 0x6f, 0xe2, 0x9d, 0xa4, 0x75, 0x6d, 0x69 }
#define REPLAY_EXTENSION "tsr"
#define REPLAY_USELESS_BYTE 0x69

#ifdef DEBUG
	#define REPLAY_WRITE_DESYNC_CHECKS
	#define REPLAY_LOAD_GARBAGE_TEST
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

	// initial game settings
	uint16_t stage;
	uint32_t seed;	// this also happens to be the game initiation time - and we use this property, don't break it please
	uint8_t diff;
	uint32_t points;
	
	// initial player settings
	uint8_t plr_char;
	uint8_t plr_shot;
	uint16_t plr_pos_x;
	uint16_t plr_pos_y;
	uint8_t plr_focus;
	uint8_t plr_fire;
	uint16_t plr_power;
	uint8_t plr_lifes;
	uint8_t plr_bombs;
	uint8_t plr_moveflags;

	// player input
	uint16_t numevents;

	// checksum of all of the above -- 2's complement of value returned by replay_calc_stageinfo_checksum()
	// uint32_t checksum;

	/* END stored fields */

	ReplayEvent *events;
	int capacity;
	int playpos;
} ReplayStage;

typedef struct Replay {
	/* BEGIN stored fields */

	// each byte must be equal to the corresponding byte in replay_magic_header.
	// uint8_t[sizeof(replay_magic_header)];

	// must be equal to REPLAY_STRUCT_VERSION
	// uint16_t version;

	// How many bytes is {playername} long. The string is not null terminated in the file, but is null terminated after it's been loaded into this struct
	// uint16_t playername_size;

	char *playername;
	uint16_t numstages;

	// ReplayStage stages[{numstages}];

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

	int active;
	ReplayStage *stages;
	ReplayStage *current;
	int currentidx;
	uint16_t desync_check;
	size_t fileoffset;
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

void replay_init(Replay *rpy);
ReplayStage* replay_init_stage(Replay *rpy, StageInfo *stage, uint64_t seed, Player *plr);
void replay_destroy(Replay *rpy);
void replay_destroy_events(Replay *rpy);
ReplayStage* replay_select(Replay *rpy, int stage);
void replay_event(Replay *rpy, uint8_t type, int16_t value);

int replay_write(Replay *rpy, SDL_RWops *file);
int replay_read(Replay *rpy, SDL_RWops *file, ReplayReadMode mode);

char* replay_getpath(char *name, int ext);	// must be freed
int replay_save(Replay *rpy, char *name);
int replay_load(Replay *rpy, char *name, ReplayReadMode mode);
void replay_copy(Replay *dst, Replay *src, int steal_events);

#endif

void replay_check_desync(Replay *rpy, int time, uint16_t check);

int replay_test(void);
