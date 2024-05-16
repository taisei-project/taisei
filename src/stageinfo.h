/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "renderer/api.h"
#include "difficulty.h"
#include "boss.h"
#include "progress.h"

typedef void (*StageProc)(void);
typedef void (*StagePreloadProc)(ResourceGroup *rg);
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
	StagePreloadProc preload;
	StageProc end;
	StageProc draw;
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
} StageInfo;

size_t stageinfo_get_num_stages(void);

StageInfo *stageinfo_get_by_index(size_t index);
StageInfo *stageinfo_get_by_id(uint16_t id);
StageInfo *stageinfo_get_by_spellcard(AttackInfo *spell, Difficulty diff);

StageProgress *stageinfo_get_progress(StageInfo *stageinfo, Difficulty diff, bool allocate);
StageProgress *stageinfo_get_progress_by_id(uint16_t id, Difficulty diff, bool allocate);

void stageinfo_init(void);
void stageinfo_shutdown(void);
void stageinfo_reload(void);
