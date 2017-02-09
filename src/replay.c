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
#include <math.h>
#include <time.h>

#include "global.h"
#include "paths/native.h"
#include "taisei_err.h"

static uint8_t replay_magic_header[] = {
	0x68, 0x6f, 0x6e, 0x6f, 0xe2, 0x9d, 0xa4, 0x75, 0x6d, 0x69
};

void replay_init(Replay *rpy) {
	memset(rpy, 0, sizeof(Replay));
	rpy->active = True;
	
	rpy->playername = malloc(strlen(tconfig.strval[PLAYERNAME]) + 1);
	strcpy(rpy->playername, tconfig.strval[PLAYERNAME]);

	printf("replay_init(): replay initialized for writting\n");
}

ReplayStage* replay_init_stage(Replay *rpy, StageInfo *stage, uint64_t seed, Player *plr) {
	ReplayStage *s;
	
	rpy->stages = (ReplayStage*)realloc(rpy->stages, sizeof(ReplayStage) * (++rpy->stgcount));
	s = &(rpy->stages[rpy->stgcount-1]);
	memset(s, 0, sizeof(ReplayStage));
	
	s->capacity = REPLAY_ALLOC_INITIAL;
	s->events = (ReplayEvent*)malloc(sizeof(ReplayEvent) * s->capacity);
	
	s->stage = stage->id;
	s->seed	= seed;
	s->diff	= global.diff;
	s->points = global.points;
	
	s->plr_pos_x = floor(creal(plr->pos));
	s->plr_pos_y = floor(cimag(plr->pos));

	s->plr_focus = plr->focus;
	s->plr_fire	= plr->fire;
	s->plr_char	= plr->cha;
	s->plr_shot	= plr->shot;
	s->plr_lifes = plr->lifes;
	s->plr_bombs = plr->bombs;
	s->plr_power = plr->power;
	s->plr_moveflags = plr->moveflags;

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

void replay_event(Replay *rpy, uint8_t type, int16_t value) {
	if(!rpy->active)
		return;
	
	ReplayStage *s = rpy->current;
	ReplayEvent *e = &(s->events[s->ecount]);
	e->frame = global.frames;
	e->type = type;
	e->value = (uint16_t)value;
	s->ecount++;
	
	if(s->ecount >= s->capacity) {
		printf("Replay stage reached its capacity of %d, reallocating\n", s->capacity);
		s->capacity *= 2;
		s->events = (ReplayEvent*)realloc(s->events, sizeof(ReplayEvent) * s->capacity);
		printf("The new capacity is %d\n", s->capacity);
	}
	
	if(type == EV_OVER)
		printf("The replay is OVER\n");
}

void replay_write_string(SDL_RWops *file, char *str) {
	SDL_WriteLE16(file, strlen(str));
	SDL_RWwrite(file, str, 1, strlen(str));
}

int replay_write_stage_event(ReplayEvent *evt, SDL_RWops *file) {
	SDL_WriteLE32(file, evt->frame);
	SDL_WriteU8(file, evt->type);
	SDL_WriteLE16(file, evt->value);

	return True;
}

uint32_t replay_calc_stageinfo_checksum(ReplayStage *stg) {
	uint32_t cs = 0;
	cs += stg->stage;
	cs += stg->seed;
	cs += stg->diff;
	cs += stg->points;
	cs += stg->plr_char;
	cs += stg->plr_shot;
	cs += stg->plr_pos_x;
	cs += stg->plr_pos_y;
	cs += stg->plr_focus;
	cs += stg->plr_fire;
	cs += stg->plr_power;
	cs += stg->plr_lifes;
	cs += stg->plr_bombs;
	cs += stg->plr_moveflags;
	cs += stg->ecount;
	return cs;
}

int replay_write_stage(ReplayStage *stg, SDL_RWops *file) {
	int i;

	SDL_WriteLE16(file, stg->stage);
	SDL_WriteLE32(file, stg->seed);
	SDL_WriteU8(file, stg->diff);
	SDL_WriteLE32(file, stg->points);
	SDL_WriteU8(file, stg->plr_char);
	SDL_WriteU8(file, stg->plr_shot);
	SDL_WriteLE16(file, stg->plr_pos_x);
	SDL_WriteLE16(file, stg->plr_pos_y);
	SDL_WriteU8(file, stg->plr_focus);
	SDL_WriteU8(file, stg->plr_fire);
	SDL_WriteLE16(file, stg->plr_power);
	SDL_WriteU8(file, stg->plr_lifes);
	SDL_WriteU8(file, stg->plr_bombs);
	SDL_WriteU8(file, stg->plr_moveflags);
	SDL_WriteLE16(file, stg->ecount);
	SDL_WriteLE32(file, 1 + ~replay_calc_stageinfo_checksum(stg));

	for(i = 0; i < stg->ecount; ++i) {
		if(!replay_write_stage_event(&stg->events[i], file)) {
			return False;
		}
	}

	return True;
}

int replay_write(Replay *rpy, SDL_RWops *file) {
	uint8_t *u8_p;
	int i;

	for(u8_p = replay_magic_header; *u8_p; ++u8_p) {
		SDL_WriteU8(file, *u8_p);
	}

	SDL_WriteLE16(file, REPLAY_STRUCT_VERSION);
	replay_write_string(file, rpy->playername);
	SDL_WriteLE16(file, rpy->stgcount);

	for(i = 0; i < rpy->stgcount; ++i) {
		if(!replay_write_stage(&rpy->stages[i], file)) {
			return False;
		}
	}

	// useless byte to simplify the premature EOF check, can be anything
	SDL_WriteU8(file, REPLAY_USELESS_BYTE);

	return True;
}

void replay_read_string(SDL_RWops *file, char **ptr) {
	size_t len = SDL_ReadLE16(file);

	*ptr = malloc(len + 1);
	memset(*ptr, 0, len + 1);

	SDL_RWread(file, *ptr, 1, len);
}

int replay_read(Replay *rpy, SDL_RWops *file) {
	uint8_t *u8_p;
	int64_t size;
	int i, j;

#ifdef REPLAY_LOAD_GARBAGE_TEST
#define PRINTPROP(prop,fmt) printf("replay_read(): " #prop " = %" # fmt " [%li / %li]\n", prop, SDL_RWtell(file), size)
#define PRINTPROP_NOASSIGN(prop,fmt) PRINTPROP(prop,fmt)
#else
#define PRINTPROP(prop,fmt) prop
#define PRINTPROP_NOASSIGN(prop,fmt)
#endif

#define CHECKPROP(prop,fmt) PRINTPROP(prop,fmt); if(SDL_RWtell(file) == size) { warnx("replay_read(): premature EOF"); return False; }

	memset(rpy, 0, sizeof(Replay));

	size = SDL_RWseek(file, 0, RW_SEEK_END);

	if(size <= sizeof(replay_magic_header) + 2) {
		warnx("replay_read(): replay is too short (%i)", size);
		return False;
	}

	SDL_RWseek(file, 0, RW_SEEK_SET);

	printf("replay_read(): %li bytes\n", size);

	for(u8_p = replay_magic_header; *u8_p; ++u8_p) {
		if(SDL_ReadU8(file) != *u8_p) {
			warnx("replay_read(): incorrect header");
			return False;
		}
	}

	if(SDL_ReadLE16(file) != REPLAY_STRUCT_VERSION) {
		warnx("replay_read(): incorrect version");
		return False;
	}

	replay_read_string(file, &rpy->playername);
	PRINTPROP_NOASSIGN(rpy->playername, s);

	CHECKPROP(rpy->stgcount = SDL_ReadLE16(file), u);

	if(!rpy->stgcount) {
		warnx("replay_read(): no stages in replay");
		return False;
	}

	rpy->stages = malloc(sizeof(ReplayStage) * rpy->stgcount);
	memset(rpy->stages, 0, sizeof(ReplayStage) * rpy->stgcount);

	for(i = 0; i < rpy->stgcount; ++i) {
		ReplayStage *stg = &rpy->stages[i];

		CHECKPROP(stg->stage = SDL_ReadLE16(file), u);
		CHECKPROP(stg->seed = SDL_ReadLE32(file), u);
		CHECKPROP(stg->diff = SDL_ReadU8(file), u);
		CHECKPROP(stg->points = SDL_ReadLE32(file), u);
		CHECKPROP(stg->plr_char = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_shot = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_pos_x = SDL_ReadLE16(file), u);
		CHECKPROP(stg->plr_pos_y = SDL_ReadLE16(file), u);
		CHECKPROP(stg->plr_focus = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_fire = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_power = SDL_ReadLE16(file), u);
		CHECKPROP(stg->plr_lifes = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_bombs = SDL_ReadU8(file), u);
		CHECKPROP(stg->plr_moveflags = SDL_ReadU8(file), u);
		CHECKPROP(stg->ecount = SDL_ReadLE16(file), u);

		if(replay_calc_stageinfo_checksum(stg) + SDL_ReadLE32(file)) {
			warnx("replay_read(): stageinfo is corrupt");
			return False;
		}

		if(!stg->ecount) {
			warnx("replay_read(): no events in stage");
			return False;
		}

		stg->events = malloc(sizeof(ReplayEvent) * stg->ecount);
		memset(stg->events, 0, sizeof(ReplayEvent) * stg->ecount);

		for(j = 0; j < stg->ecount; ++j) {
			ReplayEvent *evt = &stg->events[j];

			CHECKPROP(evt->frame = SDL_ReadLE32(file), u);
			CHECKPROP(evt->type = SDL_ReadU8(file), u);
			CHECKPROP(evt->value = SDL_ReadLE16(file), u);
		}
	}

	// useless byte to simplify the premature EOF check, can be anything
	SDL_ReadU8(file);

	return True;
}

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
	
	SDL_RWops *file = SDL_RWFromFile(p, "wb");
	free(p);
	
	if(!file) {
		warnx("replay_save(): SDL_RWFromFile() failed: %s\n", SDL_GetError());
		return False;
	}
		
	int result = replay_write(rpy, file);
	SDL_RWclose(file);
	return result;
}

int replay_load(Replay *rpy, char *name) {
	char *p = replay_getpath(name, !strendswith(name, REPLAY_EXTENSION));
	printf("replay_load(): loading %s\n", p);
	
	SDL_RWops *file = SDL_RWFromFile(p, "rb");
	free(p);
	
	if(!file) {
		warnx("replay_save(): SDL_RWFromFile() failed: %s\n", SDL_GetError());
		return False;
	}
		
	int result = replay_read(rpy, file);

	if(!result) {
		replay_destroy(rpy);
	}

	SDL_RWclose(file);
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

void replay_check_desync(Replay *rpy, int time, uint16_t check) {
	if(time % (FPS * 5)) {
		return;
	}

	if(global.replaymode == REPLAY_RECORD) {
#ifdef REPLAY_WRITE_DESYNC_CHECKS
		printf("replay_check_desync(): %u\n", check);
		replay_event(rpy, EV_CHECK_DESYNC, (int16_t)check);
#endif
	} else {
		if(rpy->desync_check && rpy->desync_check != check) {
			warnx("replay_check_desync(): Replay desync detected! %u != %u\n", rpy->desync_check, check);
		} else {
			printf("replay_check_desync(): %u OK\n", check);
		}
	}
}

int replay_test(void) {
#ifdef REPLAY_LOAD_GARBAGE_TEST
	int sz = getenvint("TAISEI_REPLAY_LOAD_GARBAGE_TEST");
	int headsz = sizeof(replay_magic_header) + 8; // 8 = version (uint16) + strlen (uint16) + plrname "test" (char[4])

	if(sz <= 0) {
		return 0;
	}

	uint8_t *u8_p, *buf = malloc(sz + headsz);
	SDL_RWops *handle = SDL_RWFromMem(buf, sz + headsz);

	// SDL_RWwrite(handle, replay_magic_header, 1, sizeof(replay_magic_header));

	for(u8_p = replay_magic_header; *u8_p; ++u8_p) {
		SDL_WriteU8(handle, *u8_p);
	}

	SDL_WriteLE16(handle, REPLAY_STRUCT_VERSION);
	SDL_WriteLE16(handle, 4);
	SDL_WriteU8(handle, 't');
	SDL_WriteU8(handle, 'e');
	SDL_WriteU8(handle, 's');
	SDL_WriteU8(handle, 't');

	printf("replay_test(): wrote a valid replay header\n");

	RandomState rnd;
	tsrand_init(&rnd, time(0));

	for(int i = 0; i < sz; ++i) {
		SDL_WriteU8(handle, tsrand_p(&rnd) & 0xFF);
	}

	printf("replay_test(): wrote %i bytes of garbage\n", sz);

	SDL_RWseek(handle, 0, RW_SEEK_SET);

	for(int i = 0; i < headsz; ++i) {
		printf("%x ", buf[i]);
	}

	printf("\n");

	Replay rpy;

	if(replay_read(&rpy, handle)) {
		errx(-1, "Succeeded loading garbage data as a replay... that shouldn't happen");
	}

	replay_destroy(&rpy);
	free(buf);
	return 1;
#else
	return 0;
#endif
}
