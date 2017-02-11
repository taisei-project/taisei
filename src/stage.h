/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef STAGE_H
#define STAGE_H

/* taisei's strange macro language.
 *
 * sorry, I guess it is bad style, but I hardcode everything and in that case
 * you'll find yourself soon in a situation where you have to spread your
 * coherent thoughts over frames using masses of redundant ifs.
 * I've just invented this thingy to keep track of my sanity.
 *
 */

#include <stdbool.h>
#include "projectile.h"
#include "boss.h"
#include "progress.h"

typedef enum {
	D_Any = 0,
	D_Easy,
	D_Normal,
	D_Hard,
	D_Lunatic,
	D_Extra // reserved for later
} Difficulty;

#define NUM_SELECTABLE_DIFFICULTIES D_Lunatic

#define TIMER(ptr) int *__timep = ptr; int _i = 0, _ni = 0;  _i = _ni = _i;
#define AT(t) if(*__timep == t)
#define FROM_TO(start,end,step) _ni = _ni; _i = (*__timep - (start))/(step); if(*__timep >= (start) && *__timep <= (end) && !((*__timep - (start)) % (step)))
#define FROM_TO_INT(start, end, step, dur, istep) \
		_i = (*__timep - (start))/(step+dur); _ni = ((*__timep - (start)) % (step+dur))/istep; \
		if(*__timep >= (start) && *__timep <= (end) && (*__timep - (start)) % ((dur) + (step)) <= dur && !((*__timep - (start)) % (istep)))

#define GO_AT(obj, start, end, vel) if(*__timep >= (start) && *__timep <= (end)) (obj)->pos += (vel);
#define GO_TO(obj, p, f) (obj)->pos += (f)*((p) - (obj)->pos);

typedef void (*StageRule)(void);
typedef void (*ShaderRule)(int);

// highest bit of uint16_t, WAY higher than the amount of spells in this game can ever possibly be
#define STAGE_SPELL_BIT 0x8000

typedef enum StageType {
	STAGE_STORY = 1,
	STAGE_EXTRA,
	STAGE_SPELL,
} StageType;

typedef struct StageInfo {
	//
	// don't reorder these!
	//

	uint16_t id; // must match type of ReplayStage.stage in replay.h
	StageRule loop;
	StageType type;
	char *title;
	char *subtitle;
	Color titleclr;
	Color bosstitleclr;
	AttackInfo *spell;
	Difficulty difficulty;

	// Do NOT access this directly!
	// Use stage_get_progress or stage_get_progress_from_info, which will lazy-initialize it and pick the correct offset.
	StageProgress *progress;
} StageInfo;

extern StageInfo stages[];
StageInfo* stage_get(uint16_t);
StageInfo* stage_get_by_spellcard(AttackInfo *spell, Difficulty diff);
StageProgress* stage_get_progress(uint16_t id, Difficulty diff);
StageProgress* stage_get_progress_from_info(StageInfo *stage, Difficulty diff);
void stage_init_array(void);

void stage_loop(StageRule start, StageRule end, StageRule draw, StageRule event, ShaderRule *shaderrules, int endtime);

void apply_bg_shaders(ShaderRule *shaderrules);

void draw_stage_title(StageInfo *info);
void draw_hud(void);

void stage_pause(void);
void stage_gameover(void);

#include "stages/stage1.h"
#include "stages/stage2.h"
#include "stages/stage3.h"
#include "stages/stage4.h"
#include "stages/stage5.h"
#include "stages/stage6.h"

#endif
