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

typedef struct Replay {
	// initial game settings
	int stage;
	int seed;
	int diff;
	int points;
	
	// initial player settings
	Character plr_char;
	ShotMode plr_shot;
	complex plr_pos;
	float plr_power;
	int plr_lifes;
	int plr_bombs;
	
	ReplayEvent *events;
	int ecount;
	int capacity;
	
	// The fields below should not be stored
	int active;
} Replay;

enum {
	REPLAY_RECORD,
	REPLAY_PLAY
};

void replay_init(Replay *rpy, StageInfo *stage, int seed, Player *plr);
void replay_destroy(Replay *rpy);
void replay_event(Replay *rpy, int type, int key);

#define REPLAY_ALLOC_INITIAL 100
#define REPLAY_ALLOC_ADDITIONAL 100

#endif
