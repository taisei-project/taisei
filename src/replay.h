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

#define REPLAY_ALLOC_INITIAL 1000
#define REPLAY_ALLOC_ADDITIONAL 500
#define REPLAY_EXTENSION "tsr"
#define REPLAY_STRUCT_VERSION 1
#define REPLAY_MAX_NAME_LENGTH 128

#ifdef DEBUG
	#define REPLAY_WRITE_DESYNC_CHECKS
#endif

typedef struct ReplayEvent {
	uint32_t frame;
	uint8_t type;
	uint16_t value;
} ReplayEvent;

typedef struct ReplayStage {
	// initial game settings
	uint32_t stage;
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
	
	// events
	ReplayEvent *events;
	uint32_t ecount;
	
	// The fields below should not be stored
	int capacity;
	int playpos;
} ReplayStage;

typedef struct Replay {
	// metadata
	// uint16_t version;
	char *playername;
	
	// stages (NOTE FOR FUTURE: stages do not represent stage runs in particular, they can and should be used to store stuff like spell practice runs, too)
	ReplayStage *stages;
	uint32_t stgcount;
	
	// The fields below should not be stored
	int active;
	ReplayStage *current;
	int currentidx;
	uint16_t desync_check;
} Replay;

enum {
	REPLAY_RECORD,
	REPLAY_PLAY
};

void replay_init(Replay *rpy);
ReplayStage* replay_init_stage(Replay *rpy, StageInfo *stage, uint64_t seed, Player *plr);
void replay_destroy(Replay *rpy);
void replay_destroy_stage(ReplayStage *stage);
ReplayStage* replay_select(Replay *rpy, int stage);
void replay_event(Replay *rpy, uint8_t type, int16_t value);

int replay_write(Replay *rpy, SDL_RWops *file);
int replay_read(Replay *rpy, SDL_RWops *file);

char* replay_getpath(char *name, int ext);	// must be freed
int replay_save(Replay *rpy, char *name);
int replay_load(Replay *rpy, char *name);
void replay_copy(Replay *dst, Replay *src);

#endif

void replay_check_desync(Replay *rpy, int time, uint16_t check);
