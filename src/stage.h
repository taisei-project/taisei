/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef STAGE_H
#define STAGE_H

#include <stdbool.h>
#include "projectile.h"
#include "boss.h"
#include "progress.h"
#include "difficulty.h"

/* taisei's strange macro language.
 *
 * sorry, I guess it is bad style, but I hardcode everything and in that case
 * you'll find yourself soon in a situation where you have to spread your
 * coherent thoughts over frames using masses of redundant ifs.
 * I've just invented this thingy to keep track of my sanity.
 *
 */

#define TIMER(ptr) int *__timep = ptr; int _i = 0, _ni = 0;  _i = _ni = _i;
#define AT(t) if(*__timep == t)
#define FROM_TO(start,end,step) _ni = _ni; _i = (*__timep - (start))/(step); if(*__timep >= (start) && *__timep <= (end) && !((*__timep - (start)) % (step)))

// Like FROM_TO just with two different intervals:
// A pause interval step and an action interval dur. For dur frames something
// happens, then for step frames there is a break.
//
// Lastly, istep is the step inside the action interval. For istep = 2, only
// every second frame of dur is executed.
//
// Finally an example for step = 4, dur = 5, and istep = 2:
//
// A_A_A____A_A_A____A_A_A____...
//
// where A denotes a frame in which the body of FROM_TO_INT gets executed.
#define FROM_TO_INT(start, end, step, dur, istep) \
		_i = (*__timep - (start))/(step+dur); _ni = ((*__timep - (start)) % (step+dur))/istep; \
		if(*__timep >= (start) && *__timep <= (end) && (*__timep - (start)) % ((dur) + (step)) <= dur && !((*__timep - (start)) % (istep)))

#define GO_AT(obj, start, end, vel) if(*__timep >= (start) && *__timep <= (end)) (obj)->pos += (vel);
#define GO_TO(obj, p, f) (obj)->pos += (f)*((p) - (obj)->pos);

typedef void (*StageProc)(void);
typedef void (*ShaderRule)(int);

// two highest bits of uint16_t, WAY higher than the amount of spells in this game can ever possibly be
#define STAGE_SPELL_BIT 0x8000
#define STAGE_EXTRASPELL_BIT 0x4000

typedef enum StageType {
	STAGE_STORY = 1,
	STAGE_EXTRA,
	STAGE_SPELL,
} StageType;

typedef struct StageProcs StageProcs;

struct StageProcs {
	StageProc begin;
	StageProc preload;
	StageProc end;
	StageProc draw;
	StageProc event;
	ShaderRule *shader_rules;
	StageProcs *spellpractice_procs;
};

typedef struct StageInfo {
	uint16_t id; // must match type of ReplayStage.stage in replay.h
	StageProcs *procs;
	StageType type;
	char *title;
	char *subtitle;
	AttackInfo *spell;
	Difficulty difficulty;
	Color titleclr;
	Color bosstitleclr;

	// Do NOT access this directly!
	// Use stage_get_progress or stage_get_progress_from_info, which will lazy-initialize it and pick the correct offset.
	StageProgress *progress;
} StageInfo;

extern StageInfo *stages;

StageInfo* stage_get(uint16_t);
StageInfo* stage_get_by_spellcard(AttackInfo *spell, Difficulty diff);

StageProgress* stage_get_progress(uint16_t id, Difficulty diff, bool allocate);
StageProgress* stage_get_progress_from_info(StageInfo *stage, Difficulty diff, bool allocate);

void stage_init_array(void);
void stage_free_array(void);

void stage_loop(StageInfo *stage);
void stage_finish(int gameover);

void draw_hud(void);

void stage_pause(void);
void stage_gameover(void);

void stage_clear_hazards(bool force);

#include "stages/stage1.h"
#include "stages/stage2.h"
#include "stages/stage3.h"
#include "stages/stage4.h"
#include "stages/stage5.h"
#include "stages/stage6.h"

#endif
