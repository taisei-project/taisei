/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "dialog.h"
#include "stageinfo.h"
#include "resource/resource.h"

typedef struct StageClearBonus {
	uint64_t base;
	uint64_t lives;
	uint64_t voltage;
	uint64_t graze;
	uint64_t total;

	struct {
		uint64_t base;
		real diff_multiplier;
		uint64_t diff_bonus;
	} all_clear;
} StageClearBonus;

void stage_enter(StageInfo *stage, ResourceGroup *rg, CallChain next);
void stage_finish(int gameover);
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

void stage_shake_view(float strength);
float stage_get_view_shake_strength(void);

void stage_load_quicksave(void);

CoSched *stage_get_sched(void);

bool stage_is_demo_mode(void);

#ifdef DEBUG
#define HAVE_SKIP_MODE
#endif

#ifdef HAVE_SKIP_MODE
	void _stage_bookmark(const char *name);
	#define STAGE_BOOKMARK(name) _stage_bookmark(#name)
	DECLARE_EXTERN_TASK(stage_bookmark, { const char *name; });
	#define STAGE_BOOKMARK_DELAYED(delay, name) INVOKE_TASK_DELAYED(delay, stage_bookmark, #name)
	bool stage_is_skip_mode(void);
#else
	#define STAGE_BOOKMARK(name) ((void)0)
	#define STAGE_BOOKMARK_DELAYED(delay, name) ((void)0)
	INLINE bool stage_is_skip_mode(void) { return false; }
#endif
