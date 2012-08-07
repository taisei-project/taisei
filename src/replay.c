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

void replay_init(Replay *rpy) {
	memset(rpy, 0, sizeof(Replay));
	rpy->active = True;
	
	printf("replay_init(): replay initialized for writting\n");
}

ReplayStage* replay_init_stage(Replay *rpy, StageInfo *stage, int seed, Player *plr) {
	ReplayStage *s;
	
	rpy->stages = (ReplayStage*)realloc(rpy->stages, sizeof(ReplayStage) * (++rpy->stgcount));
	s = &(rpy->stages[rpy->stgcount-1]);
	memset(s, 0, sizeof(ReplayStage));
	
	s->capacity 	= REPLAY_ALLOC_INITIAL;
	s->events 		= (ReplayEvent*)malloc(sizeof(ReplayEvent) * s->capacity);
	
	s->stage		= stage->id;
	s->seed			= seed;
	s->diff			= global.diff;
	s->points		= global.points;
	
	s->plr_pos		= plr->pos;
	s->plr_focus	= plr->focus;
	s->plr_fire		= plr->fire;
	s->plr_char		= plr->cha;
	s->plr_shot		= plr->shot;
	s->plr_lifes	= plr->lifes;
	s->plr_bombs	= plr->bombs;
	s->plr_power	= plr->power;
	
	printf("replay_init_stage(): created a new stage for writting\n");
	replay_select(rpy, rpy->stgcount-1);
	return s;
}

void replay_destroy_stage(ReplayStage *stage) {
	if(stage->events)
		free(stage->events);
	memset(stage, 0, sizeof(ReplayStage));
}

void replay_destroy(Replay *rpy) {
	if(rpy->stages) {
		int i; for(i = 0; i < rpy->stgcount; ++i)
			replay_destroy_stage(&(rpy->stages[i]));
		free(rpy->stages);
	}
	
	if(rpy->playername)
		free(rpy->playername);
	
	memset(rpy, 0, sizeof(Replay));
	printf("Replay destroyed.\n");
}

ReplayStage* replay_select(Replay *rpy, int stage) {
	if(stage < 0 || stage >= rpy->stgcount)
		return NULL;
	
	rpy->current = &(rpy->stages[stage]);
	rpy->currentidx = stage;
	return rpy->current;
}

void replay_event(Replay *rpy, int type, int key) {
	if(!rpy->active)
		return;
	
	ReplayStage *s = rpy->current;
	ReplayEvent *e = &(s->events[s->ecount]);
	e->frame = global.frames;
	e->type = (char)type;
	e->key = (char)key;
	s->ecount++;
	
	if(s->ecount >= s->capacity) {
		printf("Replay reached it's capacity of %d, reallocating\n", s->capacity);
		s->capacity += REPLAY_ALLOC_ADDITIONAL;
		s->events = (ReplayEvent*)realloc(s->events, sizeof(ReplayEvent) * s->capacity);
		printf("The new capacity is %d\n", s->capacity);
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
	int i, j;
	
	// header
	replay_write_int(file, REPLAY_MAGICNUMBER);
	replay_write_string(file, tconfig.strval[PLAYERNAME]);
	replay_write_string(file, "Get out of here, you nasty cheater!");
	replay_write_int(file, rpy->stgcount);
	
	for(j = 0; j < rpy->stgcount; ++j) {
		ReplayStage *stg = &(rpy->stages[j]);
		
		// initial game settings
		replay_write_int(file, stg->stage);
		replay_write_int(file, stg->seed);
		replay_write_int(file, stg->diff);
		replay_write_int(file, stg->points);
		
		// initial player settings
		replay_write_int(file, stg->plr_char);
		replay_write_int(file, stg->plr_shot);
		replay_write_complex(file, stg->plr_pos);
		replay_write_int(file, stg->plr_focus);
		replay_write_int(file, stg->plr_fire);
		replay_write_double(file, stg->plr_power);
		replay_write_int(file, stg->plr_lifes);
		replay_write_int(file, stg->plr_bombs);
		
		// events
		replay_write_int(file, stg->ecount);
		
		for(i = 0; i < stg->ecount; ++i) {
			ReplayEvent *e = &(stg->events[i]);
			replay_write_int(file, e->frame);
			replay_write_int(file, e->type);
			replay_write_int(file, e->key);
		}
	}
	
	return True;
}

// read order
enum {
	// header
	RPY_H_MAGIC = 0,
	RPY_H_META1,
	RPY_H_META2,
	RPY_H_STAGECOUNT,
	
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
	RPY_P_FOCUS,
	RPY_P_FIRE,
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
	ReplayStage *s;
	int stgnum = 0;
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
				
				case RPY_H_META2:	// skip
					break;
				
				case RPY_H_STAGECOUNT:
					rpy->stgcount = INTOF(buf);
					if(rpy->stgcount <= 0) {
						printf("replay_read(): insane stage count: %i\n", rpy->stgcount);
						replay_destroy(rpy);
						return False;
					}
					rpy->stages = (ReplayStage*)malloc(sizeof(ReplayStage) * rpy->stgcount);
					s = &(rpy->stages[0]);
					memset(s, 0, sizeof(ReplayStage));
					break;
				
				case RPY_G_STAGE:	s->stage		 = INTOF(buf);		break;
				case RPY_G_SEED:	s->seed		 	 = INTOF(buf);		break;
				case RPY_G_DIFF:	s->diff		 	 = INTOF(buf);		break;
				case RPY_G_PTS:		s->points		 = INTOF(buf);		break;
				
				case RPY_P_CHAR:	s->plr_char		 = INTOF(buf);		break;
				case RPY_P_SHOT:	s->plr_shot		 = INTOF(buf);		break;
				case RPY_P_POSREAL:	s->plr_pos		 = INTOF(buf);		break;
				case RPY_P_POSIMAG:	s->plr_pos		+= INTOF(buf) * I;	break;
				case RPY_P_FOCUS:	s->plr_focus	 = INTOF(buf);		break;
				case RPY_P_FIRE:	s->plr_fire		 = INTOF(buf);		break;
				case RPY_P_POWER:	s->plr_power	 = FLOATOF(buf);	break;
				case RPY_P_LIFES:	s->plr_lifes	 = INTOF(buf);		break;
				case RPY_P_BOMBS:	s->plr_bombs	 = INTOF(buf);		break;
				
				case RPY_E_COUNT:
					s->capacity = s->ecount = INTOF(buf);
					if(s->capacity <= 0) {
						printf("replay_read(): insane capacity in stage %i: %i\n", stgnum, s->capacity);
						replay_destroy(rpy);
						return False;
					}
					s->events = (ReplayEvent*)malloc(sizeof(ReplayEvent) * s->capacity);
					break;
				
				case RPY_E_FRAME:	s->events[eidx].frame	= INTOF(buf);			break;
				case RPY_E_TYPE:	s->events[eidx].type	= INTOF(buf);			break;
				case RPY_E_KEY:	
					s->events[eidx].key = INTOF(buf);
					eidx++;
					
					if(eidx == s->capacity) {
						if((++stgnum) >= rpy->stgcount)
							return True;
						s = &(rpy->stages[stgnum]);
						readstate = RPY_G_STAGE;
						eidx = 0;
						continue;
					}
					
					break;
			}
			
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
	
	printf("replay_read(): replay isn't properly terminated\n");
	replay_destroy(rpy);
	return False;
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
	
	free(p);
	
	if(!fp) {
		printf("replay_save(): fopen() failed\n");
		return False;
	}
		
	int result = replay_write(rpy, fp);
	fflush(fp);
	fclose(fp);
	return result;
}

int replay_load(Replay *rpy, char *name) {
	char *p = replay_getpath(name, !strendswith(name, REPLAY_EXTENSION));
	printf("replay_load(): loading %s\n", p);
	
	FILE *fp = fopen(p, "r");
	
	free(p);
	
	if(!fp) {
		printf("replay_load(): fopen() failed\n");
		return False;
	}
		
	int result = replay_read(rpy, fp);
	fclose(fp);
	return result;
}

void replay_copy(Replay *dst, Replay *src) {
	int i;
	replay_destroy(dst);
	memcpy(dst, src, sizeof(Replay));
	
	dst->playername = (char*)malloc(strlen(src->playername)+1);
	strcpy(dst->playername, src->playername);
	
	dst->stages = (ReplayStage*)malloc(sizeof(ReplayStage) * src->stgcount);
	memcpy(dst->stages, src->stages, sizeof(ReplayStage) * src->stgcount);
	
	for(i = 0; i < src->stgcount; ++i) {
		ReplayStage *s, *d;
		s = &(src->stages[i]);
		d = &(dst->stages[i]);
		
		d->capacity = s->ecount;
		d->events = (ReplayEvent*)malloc(sizeof(ReplayEvent) * d->capacity);
		memcpy(d->events, s->events, sizeof(ReplayEvent) * d->capacity);
	}
}
