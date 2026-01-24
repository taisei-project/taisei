/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"
#include "difficulty.h"
#include "progress.h"
#include "renderer/api.h"

typedef void (*StageProc)(void);
typedef void (*StagePreloadProc)(ResourceGroup *rg);
typedef bool (*ShaderRule)(Framebuffer*); // true = drawn to color buffer

// two highest bits of uint16_t, WAY higher than the amount of spells in this game can ever possibly be
#define STAGE_SPELL_BIT 0x8000
#define STAGE_EXTRASPELL_BIT 0x4000

#define STAGE_MAX_TITLE_SIZE 64

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

typedef enum StageTitleType {
	STAGE_TITLE_CUSTOM = 0,
	STAGE_TITLE_STORY,
	STAGE_TITLE_SPELL,
} StageTitleType;

typedef struct StageTitle {
	StageTitleType type;
	union {
		const char *title;
		int numeral;
	};
} StageTitle;

typedef struct StageInfo {
	StageProcs *procs;
	StageTitle title;
	const char *subtitle;
	AttackInfo *spell;
	Difficulty difficulty;
	StageType type;
	uint16_t id; // must match type of ReplayStage.stage in replay.h
} StageInfo;

size_t stageinfo_get_num_stages(void);

StageInfo *stageinfo_get_by_index(size_t index);
StageInfo *stageinfo_get_by_id(uint16_t id);
StageInfo *stageinfo_get_by_spellcard(AttackInfo *spell, Difficulty diff);

StageProgress *stageinfo_get_progress(StageInfo *stageinfo, Difficulty diff, bool allocate);
StageProgress *stageinfo_get_progress_by_id(uint16_t id, Difficulty diff, bool allocate);

int stagetitle_format_localized(StageTitle *title, size_t buf_size, char buf[buf_size]);

void stageinfo_init(void);
void stageinfo_shutdown(void);
void stageinfo_reload(void);
