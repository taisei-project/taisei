/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "cutscenes/cutscene.h"
#include "endings.h"

typedef struct StageProgress {
	// Keep this struct small if you can
	// See stage_get_progress_from_info() in stage.c for more information

	uint32_t num_played;
	uint32_t num_cleared;
	uchar unlocked : 1;
} StageProgress;

typedef enum ProgressBGMID {
	// Do not reorder! Append at the end only, reserve space if removing.

	PBGM_MENU,
	PBGM_STAGE1,
	PBGM_STAGE1_BOSS,
	PBGM_STAGE2,
	PBGM_STAGE2_BOSS,
	PBGM_STAGE3,
	PBGM_STAGE3_BOSS,
	PBGM_STAGE4,
	PBGM_STAGE4_BOSS,
	PBGM_STAGE5,
	PBGM_STAGE5_BOSS,
	PBGM_STAGE6,
	PBGM_STAGE6_BOSS1,
	PBGM_STAGE6_BOSS2,
	PBGM_STAGE6_BOSS3,
	PBGM_ENDING,
	PBGM_CREDITS,
	PBGM_BONUS0,  // old Iku theme
	PBGM_BONUS1,  // Scuttle theme
	PBGM_GAMEOVER,
	PBGM_INTRO,
} ProgressBGMID;

typedef struct GlobalProgress {
	uint32_t hiscore;
	uint32_t achieved_endings[NUM_ENDINGS];
	uint64_t unlocked_bgms;
	uint64_t unlocked_cutscenes;
	struct UnknownCmd *unknown;

	struct {
		uint8_t difficulty;
		uint8_t character;
		uint8_t shotmode;
	} game_settings;
} GlobalProgress;

extern GlobalProgress progress;

void progress_load(void);
void progress_save(void);
void progress_unload(void);
void progress_unlock_all(void);

uint32_t progress_times_any_ending_achieved(void);
uint32_t progress_times_any_good_ending_achieved(void);

bool progress_is_bgm_unlocked(const char *name);
bool progress_is_bgm_id_unlocked(ProgressBGMID id);
void progress_unlock_bgm(const char *name);

void progress_track_ending(EndingID id);
bool progress_is_cutscene_unlocked(CutsceneID id);
void progress_unlock_cutscene(CutsceneID id);
