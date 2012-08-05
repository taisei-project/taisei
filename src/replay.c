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
	if(rpy->playername)
		free(rpy->playername);
	memset(rpy, 0, sizeof(Replay));
	printf("Replay destroyed.\n");
}

void replay_event(Replay *rpy, int type, int key) {
	if(!rpy->active)
		return;
	
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
	
	if(type == EV_OVER)
		printf("The replay is OVER\n");
}

void replay_write_separator(FILE *file) {
	fputs(":", file);
}

void replay_write_string(FILE *file, char *str) {
	fputs(str, file);
	replay_write_separator(file);
}

void replay_write_int(FILE *file, int val) {
	fprintf(file, "%d", val);
	replay_write_separator(file);
}

void replay_write_double(FILE *file, double val) {
	fprintf(file, "%f", val);
	replay_write_separator(file);
}

void replay_write_complex(FILE *file, complex val) {
	replay_write_double(file, creal(val));
	replay_write_double(file, cimag(val));
}

int replay_write(Replay *rpy, FILE *file) {
	int i;
	
	// header
	replay_write_int(file, REPLAY_MAGICNUMBER);
	replay_write_string(file, tconfig.strval[PLAYERNAME]);
	replay_write_string(file, "This file is not for your eyes, move on");
	replay_write_int(file, 1);
	
	// initial game settings
	replay_write_int(file, rpy->stage);
	replay_write_int(file, rpy->seed);
	replay_write_int(file, rpy->diff);
	replay_write_int(file, rpy->points);
	
	// initial player settings
	replay_write_int(file, rpy->plr_char);
	replay_write_int(file, rpy->plr_shot);
	replay_write_complex(file, rpy->plr_pos);
	replay_write_double(file, rpy->plr_power);
	replay_write_int(file, rpy->plr_lifes);
	replay_write_int(file, rpy->plr_bombs);
	
	// events
	replay_write_int(file, rpy->ecount);
	
	for(i = 0; i < rpy->ecount; ++i) {
		ReplayEvent *e = &(rpy->events[i]);
		replay_write_int(file, e->frame);
		replay_write_int(file, e->type);
		replay_write_int(file, e->key);
	}
	
	return True;
}

// read order
enum {
	// header
	RPY_H_MAGIC = 0,
	RPY_H_META1,
	RPY_H_META2,
	RPY_H_REPLAYCOUNT,
	
	// initial game settings
	RPY_G_STAGE,
	RPY_G_SEED,
	RPY_G_DIFF,
	RPY_G_PTS,
	
	// initial player settings
	RPY_P_CHAR,
	RPY_P_SHOT,
	RPY_P_POSREAL,
	RPY_P_POSIMAG,
	RPY_P_POWER,
	RPY_P_LIFES,
	RPY_P_BOMBS,
	
	// events
	RPY_E_COUNT,
	RPY_E_FRAME,
	RPY_E_TYPE,
	RPY_E_KEY
};

#define INTOF(s) ((int)strtol(s, NULL, 10))
#define FLOATOF(s) ((float)strtod(s, NULL))

int replay_read(Replay *rpy, FILE *file) {
	int readstate = RPY_H_MAGIC;
	int bufidx = 0, eidx = 0;
	char buf[REPLAY_READ_MAXSTRLEN], c;
	memset(rpy, 0, sizeof(Replay));
	
	while((c = fgetc(file)) != EOF) {
		if(c == ':') {
			buf[bufidx] = 0;
			bufidx = 0;
			
			switch(readstate) {
				case RPY_H_MAGIC: {
					int magic = INTOF(buf);
					if(magic != REPLAY_MAGICNUMBER) {
						printf("replay_read(): invalid magic number: %d\n", magic);
						replay_destroy(rpy);
						return False;
					}
					
					break;
				}
				
				case RPY_H_META1:
					stralloc(&rpy->playername, buf);
					printf("replay_read(): replay META1 is: %s\n", buf);
					break;
				
				case RPY_H_META2: case RPY_H_REPLAYCOUNT:	// skip
					break;
				
				case RPY_G_STAGE:	rpy->stage		 = INTOF(buf);		break;
				case RPY_G_SEED:	rpy->seed		 = INTOF(buf);		break;
				case RPY_G_DIFF:	rpy->diff		 = INTOF(buf);		break;
				case RPY_G_PTS:		rpy->points		 = INTOF(buf);		break;
				
				case RPY_P_CHAR:	rpy->plr_char	 = INTOF(buf);		break;
				case RPY_P_SHOT:	rpy->plr_shot	 = INTOF(buf);		break;
				case RPY_P_POSREAL:	rpy->plr_pos	 = INTOF(buf);		break;
				case RPY_P_POSIMAG:	rpy->plr_pos	+= INTOF(buf) * I;	break;
				case RPY_P_POWER:	rpy->plr_power	 = FLOATOF(buf);	break;
				case RPY_P_LIFES:	rpy->plr_lifes	 = INTOF(buf);		break;
				case RPY_P_BOMBS:	rpy->plr_bombs	 = INTOF(buf);		break;
				
				case RPY_E_COUNT:
					rpy->capacity 	= rpy->ecount = INTOF(buf);
					if(rpy->capacity <= 0) {
						printf("replay_read(): insane capacity: %i\n", rpy->capacity);
						replay_destroy(rpy);
						return False;
					}
					rpy->events		= (ReplayEvent*)malloc(sizeof(ReplayEvent) * rpy->capacity);
					break;
				
				case RPY_E_FRAME:	rpy->events[eidx].frame = INTOF(buf);			break;
				case RPY_E_TYPE:	rpy->events[eidx].type  = INTOF(buf);			break;
				case RPY_E_KEY:		rpy->events[eidx].key 	= INTOF(buf); eidx++;	break;
			}
			
			if(rpy->capacity && eidx == rpy->capacity)
				return True;
			
			if(readstate == RPY_E_KEY)
				readstate = RPY_E_FRAME;
			else
				++readstate;
		} else {
			buf[bufidx++] = c;
			if(bufidx >= REPLAY_READ_MAXSTRLEN) {
				printf("replay_read(): item is too long\n");
				replay_destroy(rpy);
				return False;
			}
		}
	}
	
	return True;
}

#undef FLOATOF
#undef INTOF

char* replay_getpath(char *name, int ext) {
	char *p = (char*)malloc(strlen(get_replays_path()) + strlen(name) + strlen(REPLAY_EXTENSION) + 3);
	if(ext)
		sprintf(p, "%s/%s.%s", get_replays_path(), name, REPLAY_EXTENSION);
	else
		sprintf(p, "%s/%s", get_replays_path(), name);
	return p;
}

int replay_save(Replay *rpy, char *name) {
	char *p = replay_getpath(name, !strendswith(name, REPLAY_EXTENSION));
	printf("replay_save(): saving %s\n", p);
	
	FILE *fp = fopen(p, "w");
	
	if(!fp) {
		printf("replay_save(): fopen() failed\n");
		return False;
	}
	
	free(p);
	int result = replay_write(rpy, fp);
	fflush(fp);
	fclose(fp);
	return result;
}

int replay_load(Replay *rpy, char *name) {
	char *p = replay_getpath(name, !strendswith(name, REPLAY_EXTENSION));
	printf("replay_load(): loading %s\n", p);
	
	FILE *fp = fopen(p, "r");
	
	if(!fp) {
		printf("replay_load(): fopen() failed\n");
		return False;
	}
	
	free(p);
	int result = replay_read(rpy, fp);
	fclose(fp);
	return result;
}

void replay_copy(Replay *dst, Replay *src) {
	replay_destroy(dst);
	memcpy(dst, src, sizeof(Replay));
	dst->capacity = dst->ecount;
	dst->events = (ReplayEvent*)malloc(sizeof(ReplayEvent) * dst->capacity);
	memcpy(dst->events, src->events, dst->capacity * sizeof(ReplayEvent));
	dst->playername = (char*)malloc(strlen(src->playername)+1);
	strcpy(dst->playername, src->playername);
}
