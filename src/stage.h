/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stage_h
#define IGUARD_stage_h

#include "taisei.h"

#include "projectile.h"
#include "boss.h"
#include "progress.h"
#include "difficulty.h"
#include "util/graphics.h"
#include "dialog.h"
#include "coroutine.h"

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

// This is newest addition to the macro zoo! It allows you to loop a sound like
// you loop your French- I mean your danmaku code. Nothing strange going on here.
#define PLAY_FOR(name,start, end) FROM_TO(start,end,2) { play_loop(name); }

// easy to soundify versions of FROM_TO and friends. Note how I made FROM_TO_INT even more complicated!
#define FROM_TO_SND(snd,start,end,step) PLAY_FOR(snd,start,end); FROM_TO(start,end,step)
#define FROM_TO_INT_SND(snd,start,end,step,dur,istep) FROM_TO_INT(start,end,step,dur,2) { play_loop(snd); }FROM_TO_INT(start,end,step,dur,istep)

typedef void (*StageProc)(void);
typedef bool (*ShaderRule)(Framebuffer*); // true = drawn to color buffer

// two highest bits of uint16_t, WAY higher than the amount of spells in this game can ever possibly be
#define STAGE_SPELL_BIT 0x8000
#define STAGE_EXTRASPELL_BIT 0x4000

typedef enum StageType {
	STAGE_STORY = 1,
	STAGE_EXTRA,
	STAGE_SPELL,
	STAGE_SPECIAL,
} StageType;

typedef struct StageProcs StageProcs;

struct StageProcs {
	StageProc begin;
	StageProc preload;
	StageProc end;
	StageProc draw;
	StageProc event;
	StageProc update;
	ShaderRule *shader_rules;
	ShaderRule *postprocess_rules;
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

	// Do NOT access this directly!
	// Use stage_get_progress or stage_get_progress_from_info, which will lazy-initialize it and pick the correct offset.
	StageProgress *progress;
} StageInfo;

extern StageInfo *stages;

typedef struct StageClearBonus {
	uint64_t base;
	uint64_t lives;
	uint64_t voltage;
	uint64_t graze;
	uint64_t total;
	bool all_clear;
} StageClearBonus;

StageInfo* stage_get(uint16_t);
StageInfo* stage_get_by_spellcard(AttackInfo *spell, Difficulty diff);

StageProgress* stage_get_progress(uint16_t id, Difficulty diff, bool allocate);
StageProgress* stage_get_progress_from_info(StageInfo *stage, Difficulty diff, bool allocate);

void stage_init_array(void);
void stage_free_array(void);

void stage_enter(StageInfo *stage, CallChain next);
void stage_finish(int gameover);

void stage_pause(void);
void stage_gameover(void);

void stage_start_bgm(const char *bgm);

typedef enum ClearHazardsFlags {
	CLEAR_HAZARDS_BULLETS = (1 << 0),
	CLEAR_HAZARDS_LASERS = (1 << 1),
	CLEAR_HAZARDS_FORCE = (1 << 2),
	CLEAR_HAZARDS_NOW = (1 << 3),
	CLEAR_HAZARDS_SPAWN_VOLTAGE = (1 << 4),

	CLEAR_HAZARDS_ALL = CLEAR_HAZARDS_BULLETS | CLEAR_HAZARDS_LASERS,
} ClearHazardsFlags;

void stage_clear_hazards(ClearHazardsFlags flags);
void stage_clear_hazards_at(cmplx origin, double radius, ClearHazardsFlags flags);
void stage_clear_hazards_in_ellipse(Ellipse e, ClearHazardsFlags flags);
void stage_clear_hazards_predicate(bool (*predicate)(EntityInterface *ent, void *arg), void *arg, ClearHazardsFlags flags);

void stage_set_voltage_thresholds(uint easy, uint normal, uint hard, uint lunatic);

bool stage_is_cleared(void);

void stage_unlock_bgm(const char *bgm);

void stage_begin_dialog(Dialog *d) attr_nonnull_all;

#ifdef DEBUG
void _stage_bookmark(const char *name);
#define STAGE_BOOKMARK(name) _stage_bookmark(#name)
DECLARE_EXTERN_TASK(stage_bookmark, { const char *name; });
#define STAGE_BOOKMARK_DELAYED(delay, name) INVOKE_TASK_DELAYED(delay, stage_bookmark, #name)
#else
#define STAGE_BOOKMARK(name) ((void)0)
#define STAGE_BOOKMARK_DELAYED(delay, name) ((void)0)
#endif

#include "stages/stage1.h"
#include "stages/stage2.h"
#include "stages/stage3.h"
#include "stages/stage4.h"
#include "stages/stage5.h"
#include "stages/stage6.h"
#include "stages/extra.h"

#endif // IGUARD_stage_h
