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
#include "paths/native.h"

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
	
	if(type == EV_OVER) {
		printf("The replay is OVER\n");
		
		char p[1337];
		snprintf(p, 1337, "%s/test.%s", get_config_path(), REPLAY_EXTENSION);
		FILE *fp = fopen(p, "wb");
		replay_write(rpy, fp);
		fflush(fp);
		fclose(fp);
	}
}

#define WRITE(type, value, count) if(fwrite(value, sizeof(type), count, file) != (count)) { \
	printf("replay_write(): fwrite failed at (%s)(%s)\n", #type, #value); \
	return False; \
}

int replay_write(Replay *rpy, FILE* file) {
	char *head = "Taisei replay file";
	short headlen = strlen(head);
	short magic = REPLAY_MAGICNUMBER;
	char replays = 1;
	
	// replay header
	WRITE(short, 		&magic,				1)
	WRITE(short,		&headlen,			1)
	WRITE(char,			head,				headlen)
	
	// amount of replays (stages) stored in this file (may be used later)
	WRITE(char,			&replays,			1)
	
	// initial game settigs
	WRITE(int,			&rpy->stage,		1)
	WRITE(int,			&rpy->seed,			1)
	WRITE(int,			&rpy->stage,		1)
	WRITE(int,			&rpy->diff,			1)
	WRITE(int,			&rpy->points,		1)
	
	// initial player settings
	WRITE(Character,	&rpy->plr_char, 	1)
	WRITE(ShotMode,		&rpy->plr_shot, 	1)
	WRITE(complex,		&rpy->plr_pos,		1)
	WRITE(float,		&rpy->plr_power,	1)
	WRITE(int,			&rpy->plr_lifes,	1)
	WRITE(int,			&rpy->plr_bombs, 	1)
	
	// events
	WRITE(int,			&rpy->ecount,		1)
	WRITE(ReplayEvent,	rpy->events,		rpy->ecount)
	
	return True;
}

#define READ(type, ptr, count) if(fread(ptr, sizeof(type), count, file) != (count)) { \
	printf("replay_read(): fread failed at (%s)(%s)\n", #type, #ptr); \
	return False; \
} else printf("replay_read(): (%s)(%s)\n", #type, #ptr);

int replay_read(Replay *rpy, FILE* file) {
	char *head;
	short headlen;
	short magic;
	char replays;
	
	printf("replay_read()\n");
	
	// replay header
	READ(short,			&magic,				1)
	
	if(magic != REPLAY_MAGICNUMBER) {
		printf("replay_read(): invalid magic number (got %d, expected %d).\n", magic, REPLAY_MAGICNUMBER);
		return False;
	}
	
	READ(short,			&headlen,			1)
	head = (char*)malloc(headlen+1);
	READ(char,			head,				headlen)
	printf("replay_read(): %s\n", head);
	
	// amount of replays (stages) stored in this file (may be used later)
	READ(char,			&replays,			1)
	
	// initial game settigs
	READ(int,			&rpy->stage,		1)
	READ(int,			&rpy->seed,			1)
	READ(int,			&rpy->stage,		1)
	READ(int,			&rpy->diff,			1)
	READ(int,			&rpy->points,		1)
	
	// initial player settings
	READ(Character,		&rpy->plr_char, 	1)
	READ(ShotMode,		&rpy->plr_shot, 	1)
	READ(complex,		&rpy->plr_pos,		1)
	READ(float,			&rpy->plr_power,	1)
	READ(int,			&rpy->plr_lifes,	1)
	READ(int,			&rpy->plr_bombs, 	1)
	
	// events
	READ(int,			&rpy->ecount,		1)
	rpy->capacity 		= rpy->ecount;
	rpy->events 		= (ReplayEvent*)malloc(sizeof(ReplayEvent) * rpy->capacity);
	READ(ReplayEvent,	rpy->events,		rpy->ecount)
	
	return True;
}

#undef WRITE
#undef READ
