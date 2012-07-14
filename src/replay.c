/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "replay.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"

void replay_init(Replay *rpy, StageInfo *stage, int seed, Player *plr) {
	memset(rpy, 0, sizeof(Replay));
	
	rpy->capacity 	= REPLAY_ALLOC_INITIAL;
	rpy->events 	= (ReplayEvent*)malloc(sizeof(ReplayEvent) * rpy->capacity);
	
	rpy->stage		= stage->id;
	rpy->seed		= seed;
	rpy->diff		= global.diff;
	rpy->points		= global.points;
	
	rpy->plr_pos	= plr->pos;
	rpy->plr_char	= plr->cha;
	rpy->plr_shot	= plr->shot;
	rpy->plr_lifes	= plr->lifes;
	rpy->plr_bombs	= plr->bombs;
	rpy->plr_power	= plr->power;
	
	rpy->active		= True;
	
	printf("Replay initialized with capacity of %d\n", rpy->capacity);
}

void replay_destroy(Replay *rpy) {
	if(rpy->events)
		free(rpy->events);
	memset(rpy, 0, sizeof(Replay));
	printf("Replay destroyed.\n");
}

void replay_event(Replay *rpy, int type, int key) {
	if(!rpy->active)
		return;
	
	printf("[%d] Replay event #%d: %d, %d\n", global.frames, rpy->ecount, type, key);
	
	if(type == EV_OVER)
		printf("The replay is OVER\n");
	
	ReplayEvent *e = &(rpy->events[rpy->ecount]);
	e->frame = global.frames;
	e->type = (char)type;
	e->key = (char)key;
	rpy->ecount++;
	
	if(rpy->ecount >= rpy->capacity) {
		printf("Replay reached it's capacity of %d, reallocating\n", rpy->capacity);
		rpy->capacity += REPLAY_ALLOC_ADDITIONAL;
		rpy->events = (ReplayEvent*)realloc(rpy->events, sizeof(ReplayEvent) * rpy->capacity);
		printf("The new capacity is %d\n", rpy->capacity);
	}
}
