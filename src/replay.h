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

typedef struct ReplayEvent {
	int frame;
	char type;
	char key;
} ReplayEvent;

typedef struct ReplayStage {
	// initial game settings
	int stage;
	int seed;	// this also happens to be the game initiation time - and we use this property, don't break it please
	int diff;
	int points;
	
	// initial player settings
	Character plr_char;
	ShotMode plr_shot;
	complex plr_pos;
	short plr_focus;
	short plr_fire;
	float plr_power;
	int plr_lifes;
	int plr_bombs;
	
	// events
	ReplayEvent *events;
	int ecount;
	
	// The fields below should not be stored
	int capacity;
	int playpos;
} ReplayStage;

typedef struct Replay {
	// metadata
	char *playername;
	
	// stages (NOTE FOR FUTURE: stages do not represent stage runs in particular, they can and should be used to store stuff like spell practice runs, too)
	ReplayStage *stages;
	int stgcount;
	
	// The fields below should not be stored
	int active;
	ReplayStage *current;
	int currentidx;
} Replay;

enum {
	REPLAY_RECORD,
	REPLAY_PLAY
};

void replay_init(Replay *rpy);
ReplayStage* replay_init_stage(Replay *rpy, StageInfo *stage, int seed, Player *plr);
void replay_destroy(Replay *rpy);
void replay_destroy_stage(ReplayStage *stage);
ReplayStage* replay_select(Replay *rpy, int stage);
void replay_event(Replay *rpy, int type, int key);

int replay_write(Replay *rpy, FILE *file);
int replay_read(Replay *rpy, FILE *file);

char* replay_getpath(char *name, int ext);	// must be freed
int replay_save(Replay *rpy, char *name);
int replay_load(Replay *rpy, char *name);
void replay_copy(Replay *dst, Replay *src);

#define REPLAY_ALLOC_INITIAL 100
#define REPLAY_ALLOC_ADDITIONAL 100
#define REPLAY_MAGICNUMBER 1337
#define REPLAY_EXTENSION "tsr"
#define REPLAY_READ_MAXSTRLEN 128

#endif
