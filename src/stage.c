/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <complex.h>

#include "stage.h"
#include "tscomplex.h"

#include <time.h>
#include "global.h"
#include "video.h"
#include "resource/bgm.h"
#include "replay.h"
#include "config.h"
#include "player.h"
#include "menu/ingamemenu.h"
#include "menu/gameovermenu.h"
#include "taisei_err.h"

StageInfo stages[] = {
	//
	// ids must be unique
	// they don't necessarily have to be human-readable or ordered
	// they're basically there just for replays, so don't change the already enstabilished ones unless you must
	//

	//
	//	Story stages
	//

//	id  loop         type         title      subtitle                       titleclr   bosstitleclr spell difficulty
	{1, stage1_loop, STAGE_STORY, "Stage 1", "Misty Lake",                  {1, 1, 1}, {1, 1, 1},   NULL, D_Any},
	{2, stage2_loop, STAGE_STORY, "Stage 2", "Walk Along the Border",       {1, 1, 1}, {1, 1, 1},   NULL, D_Any},
	{3, stage3_loop, STAGE_STORY, "Stage 3", "Through the Tunnel of Light", {0, 0, 0}, {0, 0, 0},   NULL, D_Any},
	{4, stage4_loop, STAGE_STORY, "Stage 4", "Forgotten Mansion",           {0, 0, 0}, {1, 1, 1},   NULL, D_Any},
	{5, stage5_loop, STAGE_STORY, "Stage 5", "Climbing the Tower of Babel", {1, 1, 1}, {1, 1, 1},   NULL, D_Any},
	{6, stage6_loop, STAGE_STORY, "Stage 6", "Roof of the World",           {1, 1, 1}, {1, 1, 1},   NULL, D_Any},

	//
	// Spell practice stages
	//
	// titles and subtitles for those are generated later by stage_init_array()
	//

#define S STAGE_SPELL_BIT
	// Freeze Sign ~ Perfect Freeze
	{S|  0, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+0, D_Easy},
	{S|  1, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+0, D_Normal},
	{S|  2, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+0, D_Hard},
	{S|  3, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+0, D_Lunatic},

	// Freeze Sign ~ Crystal Rain
	{S|  4, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+1, D_Easy},
	{S|  5, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+1, D_Normal},
	{S|  6, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+1, D_Hard},
	{S|  7, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+1, D_Lunatic},

	// Doom Sign ~ Icicle Fall
	{S|  8, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+2, D_Easy},
	{S|  9, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+2, D_Normal},
	{S| 10, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+2, D_Hard},
	{S| 11, stage1_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage1_spells+2, D_Lunatic},

	// Shard ~ Amulet of Harm
	{S| 12, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+0, D_Easy},
	{S| 13, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+0, D_Normal},
	{S| 14, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+0, D_Hard},
	{S| 15, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+0, D_Lunatic},

	// Lottery Sign ~ Bad Pick
	{S| 16, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+1, D_Easy},
	{S| 17, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+1, D_Normal},
	{S| 18, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+1, D_Hard},
	{S| 19, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+1, D_Lunatic},

	// Lottery Sign ~ Wheel of Fortune
	{S| 20, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+2, D_Easy},
	{S| 21, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+2, D_Normal},
	{S| 22, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+2, D_Hard},
	{S| 23, stage2_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage2_spells+2, D_Lunatic},

	// Venom Sign ~ Deadly Dance
	{S| 24, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+0, D_Easy},
	{S| 25, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+0, D_Normal},
	{S| 26, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+0, D_Hard},
	{S| 27, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+0, D_Lunatic},

	// Venom Sign ~ Acid Rain
	{S| 28, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+1, D_Hard},
	{S| 29, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+1, D_Lunatic},

	// Firefly Sign ~ Moonlight Rocket
	{S| 30, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+2, D_Easy},
	{S| 31, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+2, D_Normal},
	{S| 32, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+2, D_Hard},
	{S| 33, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+2, D_Lunatic},

	// Light Source ~ Wriggle Night Ignite
	{S| 34, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+3, D_Easy},
	{S| 35, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+3, D_Normal},
	{S| 36, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+3, D_Hard},
	{S| 37, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+3, D_Lunatic},

	// Bug Sign ~ Phosphaenus Hemipterus
	{S| 38, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+4, D_Easy},
	{S| 39, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+4, D_Normal},
	{S| 40, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+4, D_Hard},
	{S| 41, stage3_spellpractice_loop, STAGE_SPELL, NULL, NULL, {0, 0, 0}, {0, 0, 0}, stage3_spells+4, D_Lunatic},

	// Bloodless ~ Gate of Walachia
	{S| 42, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+0, D_Easy},
	{S| 43, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+0, D_Normal},
	{S| 44, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+0, D_Hard},
	{S| 45, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+0, D_Lunatic},

	// Bloodless ~ Dry Fountain
	{S| 46, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+1, D_Easy},
	{S| 47, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+1, D_Normal},

	// Bloodless ~ Red Spike
	{S| 48, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+2, D_Hard},
	{S| 49, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+2, D_Lunatic},

	// Limit ~ Animate Wall
	{S| 50, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+3, D_Easy},
	{S| 51, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+3, D_Normal},

	// Summoning ~ Demon Wall
	{S| 52, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+4, D_Hard},
	{S| 53, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+4, D_Lunatic},

	// Power Sign ~ Blow the Walls
	{S| 54, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+5, D_Easy},
	{S| 55, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+5, D_Normal},
	{S| 56, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+5, D_Hard},
	{S| 57, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+5, D_Lunatic},

	// Fear Sign ~ Bloody Danmaku
	{S| 58, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+6, D_Hard},
	{S| 59, stage4_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage4_spells+6, D_Lunatic},

	// High Voltage ~ Atmospheric Discharge
	{S| 60, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+0, D_Easy},
	{S| 61, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+0, D_Normal},
	{S| 62, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+0, D_Hard},
	{S| 63, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+0, D_Lunatic},

	// Charge Sign ~ Artificial Lightning
	{S| 64, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+1, D_Easy},
	{S| 65, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+1, D_Normal},
	{S| 66, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+1, D_Hard},
	{S| 67, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+1, D_Lunatic},

	// Spark Sign ~ Natural Cathode
	{S| 68, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+2, D_Easy},
	{S| 69, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+2, D_Normal},
	{S| 70, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+2, D_Hard},
	{S| 71, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+2, D_Lunatic},

	// Current Sign ~ Induction
	{S| 72, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+3, D_Easy},
	{S| 73, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+3, D_Normal},
	{S| 74, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+3, D_Hard},
	{S| 75, stage5_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage5_spells+3, D_Lunatic},

	// Newton Sign ~ 2.5 Laws of Movement
	{S| 76, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+0, D_Easy},
	{S| 77, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+0, D_Normal},
	{S| 78, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+0, D_Hard},
	{S| 79, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+0, D_Lunatic},

	// Maxwell Sign ~ Wave Theory
	{S| 80, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+1, D_Easy},
	{S| 81, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+1, D_Normal},
	{S| 82, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+1, D_Hard},
	{S| 83, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+1, D_Lunatic},

	// Eigenstate ~ Many-World Interpretation
	{S| 84, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+2, D_Easy},
	{S| 85, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+2, D_Normal},
	{S| 86, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+2, D_Hard},
	{S| 87, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+2, D_Lunatic},

	// Ricci Sign ~ Space Time Curvature
	{S| 88, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+3, D_Easy},
	{S| 89, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+3, D_Normal},
	{S| 90, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+3, D_Hard},
	{S| 91, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+3, D_Lunatic},

	// LHC ~ Higgs Boson Uncovered
	{S| 92, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+4, D_Easy},
	{S| 93, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+4, D_Normal},
	{S| 94, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+4, D_Hard},
	{S| 95, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+4, D_Lunatic},

	// Tower of Truth ~ Theory of Everything
	{S| 96, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+5, D_Easy},
	{S| 97, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+5, D_Normal},
	{S| 98, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+5, D_Hard},
	{S| 99, stage6_spellpractice_loop, STAGE_SPELL, NULL, NULL, {1, 1, 1}, {1, 1, 1}, stage6_spells+5, D_Lunatic},
#undef S

	{0}
};

void stage_init_array(void) {
	int spellnum = 0;

	for(int i = 0; stages[i].loop; ++i) {
		// we will allocate this later on demand
		stages[i].progress = NULL;

		if(stages[i].type == STAGE_SPELL) {
			char *s, title[10], *postfix = difficulty_name(stages[i].difficulty);

			snprintf(title, sizeof(title), "Spell %d", ++spellnum);
			stralloc(&stages[i].title, title);

			stages[i].subtitle = s = malloc(strlen(postfix) + strlen(stages[i].spell->name) + 4);
			strcpy(s, stages[i].spell->name);
			strcat(s, " ~ ");
			strcat(s, postfix);
		}

#ifdef DEBUG
		if(stages[i].type == STAGE_SPELL && !(stages[i].id & STAGE_SPELL_BIT)) {
			errx(-1, "Spell stage has an ID without the spell bit set: %u", stages[i].id);
		}

		for(int j = 0; stages[j].loop; ++j) {
			if(i != j && stages[i].id == stages[j].id) {
				errx(-1, "Duplicate ID in stages array: %u", stages[i].id);
			}
		}
#endif
	}
}

void stage_free_array(void) {
	for(StageInfo *stg = stages; stg->loop; ++stg) {
		if(stg->type == STAGE_SPELL) {
			free(stg->subtitle);
		}

		free(stg->progress);
	}
}

// NOTE: This returns the stage BY ID, not by the array index!
StageInfo* stage_get(uint16_t n) {
	for(StageInfo *stg = stages; stg->loop; ++stg)
		if(stg->id == n)
			return stg;
	return NULL;
}

StageInfo* stage_get_by_spellcard(AttackInfo *spell, Difficulty diff) {
	if(!spell)
		return NULL;

	for(StageInfo *stg = stages; stg->loop; ++stg)
		if(stg->spell == spell && stg->difficulty == diff)
			return stg;

	return NULL;
}

StageProgress* stage_get_progress_from_info(StageInfo *stage, Difficulty diff, bool allocate) {
	// D_Any stages will have a separate StageProgress for every selectable difficulty.
	// Stages with a fixed difficulty setting (spellpractice, extra stage...) obviously get just one and the diff parameter is ignored.

	// This stuff must stay around until progress_save(), which happens on shutdown.
	// So do NOT try to free any pointers this function returns, that will fuck everything up.

	if(!stage) {
		return NULL;
	}

	bool fixed_diff = (stage->difficulty != D_Any);

	if(!fixed_diff && (diff < D_Easy || diff > D_Lunatic)) {
		return NULL;
	}

	if(!stage->progress) {
		if(!allocate) {
			return NULL;
		}

		size_t allocsize = sizeof(StageProgress) * (fixed_diff ? 1 : NUM_SELECTABLE_DIFFICULTIES);
		stage->progress = malloc(allocsize);
		memset(stage->progress, 0, allocsize);
#ifdef DEBUG
		printf("stage_get_progress_from_info(): allocated %lu bytes for stage %u (%s: %s)\n", allocsize, stage->id, stage->title, stage->subtitle);
#endif
	}

	return stage->progress + (fixed_diff ? 0 : diff - D_Easy);
}

StageProgress* stage_get_progress(uint16_t id, Difficulty diff, bool allocate) {
	return stage_get_progress_from_info(stage_get(id), diff, allocate);
}

void stage_start(void) {
	global.timer = 0;
	global.frames = 0;
	global.stageuiframes = 0;
	global.game_over = 0;
	global.nostagebg = false;
	global.shake_view = 0;

	global.fps.stagebg_fps = global.fps.show_fps = FPS;
	global.fps.fpstime = SDL_GetTicks();

	prepare_player_for_next_stage(&global.plr);
}

void stage_pause(void) {
	MenuData menu;
	stop_bgm();
	create_ingame_menu(&menu);
	menu_loop(&menu);
	continue_bgm();
}

void stage_gameover(void) {
	MenuData menu;
	save_bgm();
	start_bgm("bgm_gameover");
	create_gameover_menu(&menu);
	menu_loop(&menu);
	restore_bgm();
}

void stage_input_event(EventType type, int key, void *arg) {
	switch(type) {
		case E_PlrKeyDown:
			if(key == KEY_HAHAIWIN) {
#ifdef DEBUG
				global.game_over = GAMEOVER_WIN;
#endif
				break;
			}

			if(global.dialog && (key == KEY_SHOT || key == KEY_BOMB)) {
				page_dialog(&global.dialog);
				replay_stage_event(global.replay_stage, global.frames, EV_PRESS, key);
			} else {
#ifndef DEBUG // no cheating for peasants
				if(global.replaymode == REPLAY_RECORD && key == KEY_IDDQD)
					break;
#endif

				player_event(&global.plr, EV_PRESS, key);
				replay_stage_event(global.replay_stage, global.frames, EV_PRESS, key);

				if(key == KEY_SKIP && global.dialog) {
					global.dialog->skip = true;
				}
			}
			break;

		case E_PlrKeyUp:
			player_event(&global.plr, EV_RELEASE, key);
			replay_stage_event(global.replay_stage, global.frames, EV_RELEASE, key);

			if(key == KEY_SKIP && global.dialog)
				global.dialog->skip = false;
			break;

		case E_Pause:
			stage_pause();
			break;

		case E_PlrAxisLR:
			player_event(&global.plr, EV_AXIS_LR, key);
			replay_stage_event(global.replay_stage, global.frames, EV_AXIS_LR, key);
			break;

		case E_PlrAxisUD:
			player_event(&global.plr, EV_AXIS_UD, key);
			replay_stage_event(global.replay_stage, global.frames, EV_AXIS_UD, key);
			break;

		default: break;
	}
}

void stage_replay_event(EventType type, int state, void *arg) {
	if(type == E_Pause)
		stage_pause();
}

void replay_input(void) {
	ReplayStage *s = global.replay_stage;
	int i;

	handle_events(stage_replay_event, EF_Game, NULL);

	for(i = s->playpos; i < s->numevents; ++i) {
		ReplayEvent *e = &(s->events[i]);

		if(e->frame != global.frames)
			break;

		switch(e->type) {
			case EV_OVER:
				global.game_over = GAMEOVER_DEFEAT;
				break;

			default:
				if(global.dialog && e->type == EV_PRESS && (e->value == KEY_SHOT || e->value == KEY_BOMB))
					page_dialog(&global.dialog);
				else if(global.dialog && (e->type == EV_PRESS || e->type == EV_RELEASE) && e->value == KEY_SKIP)
					global.dialog->skip = (e->type == EV_PRESS);
				else if(e->type == EV_CHECK_DESYNC)
					s->desync_check = e->value;
				else
					player_event(&global.plr, e->type, (int16_t)e->value);
				break;
		}
	}

	s->playpos = i;
	player_applymovement(&global.plr);
}

void stage_input(void) {
	handle_events(stage_input_event, EF_Game, NULL);

	// workaround
	if(global.dialog && global.dialog->skip && !gamekeypressed(KEY_SKIP)) {
		global.dialog->skip = false;
		replay_stage_event(global.replay_stage, global.frames, EV_RELEASE, KEY_SKIP);
	}

	player_applymovement(&global.plr);
	player_input_workaround(&global.plr);
}

void draw_hud(void) {
	draw_texture(SCREEN_W/2.0, SCREEN_H/2.0, "hud");

	char buf[16], *diff;
	int i;

	glPushMatrix();
	glTranslatef(615,0,0);

	glPushMatrix();
	glTranslatef((SCREEN_W - 615) * 0.25, 20, 0);
	glScalef(0.6, 0.6, 0);

	Color clr;
	difficulty_color(&clr, global.diff);
	glColor4f(clr.r, clr.g, clr.b, 0.7f);

	diff = difficulty_name(global.diff);
	draw_text(AL_Center, 1, 1, diff, _fonts.mainmenu);
	draw_text(AL_Center, 2, 2, diff, _fonts.mainmenu);
	glColor4f(1,1,1,1);
	draw_text(AL_Center, 0, 0, diff, _fonts.mainmenu);
	glPopMatrix();

	if(global.stage->type == STAGE_SPELL) {
		glColor4f(1, 1, 1, 0.7);
		draw_text(AL_Left, -6, 167, "N/A", _fonts.standard);
		draw_text(AL_Left, -6, 200, "N/A", _fonts.standard);
		glColor4f(1, 1, 1, 1.0);
	} else {
		float a = 1, s = 0, fadein = 1, fadeout = 1, fade = 1;
	
		if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
			fadein  = min(1, -min(0, global.frames - global.boss->current->starttime) / (float)ATTACK_START_DELAY);
			fadeout = (!!global.boss->current->finished) * (1 - (global.boss->current->endtime - global.frames) / (float)ATTACK_END_DELAY_EXTRA) / 0.74;
			fade = max(fadein, fadeout);

			s = 1 - fade;
			a = 0.5 + 0.5 * fade;
		}
	
		if(a != 1) glColor4f(a,a,a,a);
		for(i = 0; i < global.plr.lifes; i++)
			draw_texture(16*i,167, "star");

		for(i = 0; i < global.plr.bombs; i++)
			draw_texture(16*i,200, "star");
		if(a != 1) glColor4f(1,1,1,1);

		if(s) {
			float s2 = max(0, swing(s, 3));
			glPushMatrix();
			glTranslatef((SCREEN_W - 615) * 0.25 - 615 * (1 - pow(2*fadein-1, 2)), 400, 0);
			//glColor4f(1, 0.5, 0.3, 0.7 * s);
			glColor4f(0.3, 0.6, 0.7, 0.7 * s);
			glRotatef(-25 + 360 * (1-s2), 0, 0, 1);
			glScalef(s2, s2, 0);
			draw_text(AL_Center,  1,  1, "Extra Spell!", _fonts.mainmenu);
			draw_text(AL_Center, -1, -1, "Extra Spell!", _fonts.mainmenu);
			glColor4f(1, 1, 1, s);
			draw_text(AL_Center, 0, 0, "Extra Spell!", _fonts.mainmenu);
		glColor4f(1, 1, 1, 1);
			glPopMatrix();
		}
	}

	sprintf(buf, "%.2f", global.plr.power / 100.0);
	draw_text(AL_Center, 10, 236, buf, _fonts.standard);

	sprintf(buf, "%i", global.plr.graze);
	draw_text(AL_Left, -5, 270, buf, _fonts.standard);

	sprintf(buf, "%i", global.plr.points);
	draw_text(AL_Center, 13, 49, buf, _fonts.standard);

	if(global.plr.iddqd) {
		draw_text(AL_Left, -70, 475, "GOD MODE", _fonts.mainmenu);
	}

	glPopMatrix();

#ifdef DEBUG
	sprintf(buf, "%i fps / %i stgframes", global.fps.show_fps, global.timer);
#else
	sprintf(buf, "%i fps", global.fps.show_fps);
#endif
	draw_text(AL_Right, SCREEN_W, SCREEN_H-20, buf, _fonts.standard);

	if(global.boss)
		draw_texture(VIEWPORT_X+creal(global.boss->pos), 590, "boss_indicator");
}

void stage_draw(StageInfo *info, StageRule bgdraw, ShaderRule *shaderrules, int time) {

	if(!config_get_int(CONFIG_NO_SHADER)) {
		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[0].fbo);
		glViewport(0,0,SCREEN_W,SCREEN_H);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	glTranslatef(-(VIEWPORT_X+VIEWPORT_W/2.0), -(VIEWPORT_Y+VIEWPORT_H/2.0),0);
	glEnable(GL_DEPTH_TEST);

	if(config_get_int(CONFIG_NO_STAGEBG) == 2 && global.fps.stagebg_fps < config_get_int(CONFIG_NO_STAGEBG_FPSLIMIT)
		&& !global.nostagebg) {

		printf("stage_draw(): !- Stage background has been switched off due to low frame rate. You can change that in the options.\n");
		global.nostagebg = true;
	}

	if(config_get_int(CONFIG_NO_STAGEBG) == 1)
		global.nostagebg = true;

	if(!global.nostagebg)
		bgdraw();

	glPopMatrix();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	set_ortho();

	glPushMatrix();
	glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);

	if(!config_get_int(CONFIG_NO_SHADER))
		apply_bg_shaders(shaderrules);

	if(global.boss) {
		glPushMatrix();
		glTranslatef(creal(global.boss->pos), cimag(global.boss->pos), 0);

		if(!(global.frames % 5)) {
			complex offset = (frand()-0.5)*50 + (frand()-0.5)*20.0*I;
			create_particle3c("boss_shadow", -20.0*I, rgba(0.2,0.35,0.5,0.5), EnemyFlareShrink, enemy_flare, 50, (-100.0*I-offset)/(50.0+frand()*10), add_ref(global.boss));
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		glRotatef(global.frames*4.0, 0, 0, -1);
		float f = 0.8+0.1*sin(global.frames/8.0);
		glScalef(f,f,f);
		draw_texture(0,0,"boss_circle");

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPopMatrix();
	}

	player_draw(&global.plr);

	draw_items();
	draw_projectiles(global.projs);

	draw_projectiles(global.particles);
	draw_enemies(global.enemies);
	draw_lasers(false);

	if(global.boss)
		draw_boss(global.boss);

	if(global.dialog)
		draw_dialog(global.dialog);

	draw_stage_title(info);

	if(!config_get_int(CONFIG_NO_SHADER)) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		video_set_viewport();
		glPushMatrix();
		if(global.shake_view) {
			glTranslatef(global.shake_view*sin(global.frames),global.shake_view*sin(global.frames+3),0);
			
			if(global.shake_view_fade) {
				global.shake_view -= global.shake_view_fade;
				if(global.shake_view <= 0)
					global.shake_view = global.shake_view_fade = 0;
			}
		}

		draw_fbo_viewport(&resources.fsec);

		glPopMatrix();
	}

	glPopMatrix();

	draw_hud();
	draw_transition();
}

int apply_shaderrules(ShaderRule *shaderrules, int fbonum) {
	int i;

	if(!global.nostagebg) {
		for(i = 0; shaderrules != NULL && shaderrules[i] != NULL; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[!fbonum].fbo);
			shaderrules[i](fbonum);

			fbonum = !fbonum;
		}
	}

	return fbonum;
}

void apply_bg_shaders(ShaderRule *shaderrules) {
	int fbonum = 0;

	if(global.boss && global.boss->current && global.boss->current->draw_rule) {
		if(global.frames - global.boss->current->starttime <= 0)
			fbonum = apply_shaderrules(shaderrules, fbonum);

		int t = global.frames - global.boss->current->starttime;
		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[0].fbo);
		global.boss->current->draw_rule(global.boss, t);
		
		if(global.boss->current->type == AT_ExtraSpell)
			draw_extraspell_bg(global.boss, t);

		glPushMatrix();
			glTranslatef(creal(global.boss->pos), cimag(global.boss->pos), 0);
			glRotatef(global.frames*7.0, 0, 0, -1);

			if(t < 0) {
				float f = 1.0 - t/(float)ATTACK_START_DELAY;
				glScalef(f,f,f);
			}

			draw_texture(0,0,"boss_spellcircle0");
		glPopMatrix();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	} else
		fbonum = apply_shaderrules(shaderrules, fbonum);

	glBindFramebuffer(GL_FRAMEBUFFER, resources.fsec.fbo);

	if(global.boss) { // Boss background shader
		Shader *shader = get_shader("boss_zoom");
		glUseProgram(shader->prog);

		complex fpos = VIEWPORT_H*I + conj(global.boss->pos) + (VIEWPORT_X + VIEWPORT_Y*I);
		complex pos = fpos + 15*cexp(I*global.frames/4.5);

		glUniform2f(uniloc(shader, "blur_orig"),
					creal(pos)/resources.fbg[fbonum].nw, cimag(pos)/resources.fbg[fbonum].nh);
		glUniform2f(uniloc(shader, "fix_orig"),
					creal(fpos)/resources.fbg[fbonum].nw, cimag(fpos)/resources.fbg[fbonum].nh);
		glUniform1f(uniloc(shader, "blur_rad"), 0.2+0.025*sin(global.frames/15.0));
		glUniform1f(uniloc(shader, "rad"), 0.24);
		glUniform1f(uniloc(shader, "ratio"), (float)resources.fbg[fbonum].nh/resources.fbg[fbonum].nw);
		if(global.boss->zoomcolor)
			glUniform4fv(uniloc(shader, "color"), 1, (float *)global.boss->zoomcolor);
		else
			glUniform4f(uniloc(shader, "color"), 0.1, 0.2, 0.3, 1);
	}

	draw_fbo_viewport(&resources.fbg[fbonum]);

	glUseProgram(0);

	if(global.frames - global.plr.recovery < 0) {
		float t = BOMB_RECOVERY - global.plr.recovery + global.frames;
		float fade = 1;

		if(t < BOMB_RECOVERY/6)
			fade = t/BOMB_RECOVERY*6;

		if(t > BOMB_RECOVERY/4*3)
			fade = 1-t/BOMB_RECOVERY*4 + 3;

		glPushMatrix();
		glTranslatef(-30,-30,0);
		fade_out(fade*0.6);
		glPopMatrix();
	}
}

void stage_logic(int time) {
	player_logic(&global.plr);

	process_enemies(&global.enemies);
	process_projectiles(&global.projs, true);
	process_items();
	process_lasers();
	process_projectiles(&global.particles, false);

	if(global.boss && !global.dialog)
		process_boss(&global.boss);

	if(global.dialog && global.dialog->skip && global.frames - global.dialog->page_time > 3)
		page_dialog(&global.dialog);

	global.frames++;
	global.stageuiframes++;

	if(!global.dialog && !global.boss)
		global.timer++;

	if(global.timer == time - FADE_TIME ||
		  (
			global.replaymode == REPLAY_PLAY &&
			global.frames == global.replay_stage->events[global.replay_stage->numevents-1].frame - FADE_TIME
		  )
		) {
		set_transition(TransFadeBlack, FADE_TIME, FADE_TIME*2);
	}

	if(time && global.timer >= time)
		global.game_over = GAMEOVER_WIN;

	// BGM handling
	if(global.dialog && global.dialog->messages[global.dialog->pos].side == BGM) {
		start_bgm(global.dialog->messages[global.dialog->pos].msg);
		page_dialog(&global.dialog);
	}
}

void stage_end(void) {
	delete_enemies(&global.enemies);
	delete_items();
	delete_lasers();

	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);

	if(global.dialog) {
		delete_dialog(global.dialog);
		global.dialog = NULL;
	}

	if(global.boss) {
		free_boss(global.boss);
		global.boss = NULL;
	}
}

void stage_loop(StageRule start, StageRule end, StageRule draw, StageRule event, ShaderRule *shaderrules, int endtime, char *bgmname) {
	if(global.game_over == GAMEOVER_WIN) {
		global.game_over = 0;
	} else if(global.game_over) {
		return;
	}

	// I really want to separate all of the game state from the global struct sometime
	StageInfo *info = global.stage;

	uint32_t seed = (uint32_t)time(0);
	tsrand_switch(&global.rand_game);
	tsrand_seed_p(&global.rand_game, seed);
	stage_start();

	if(global.replaymode == REPLAY_RECORD) {
		if(config_get_int(CONFIG_SAVE_RPY)) {
			global.replay_stage = replay_create_stage(&global.replay, info, seed, global.diff, global.plr.points, &global.plr);

			// make sure our player state is consistent with what goes into the replay
			init_player(&global.plr);
			replay_stage_sync_player_state(global.replay_stage, &global.plr);
		} else {
			global.replay_stage = NULL;
		}

		printf("Random seed: %u\n", seed);

		if(info->type == STAGE_SPELL) {
			global.plr.lifes = 0;
			global.plr.bombs = 0;
		}
	} else {
		if(!global.replay_stage) {
			errx(-1, "Attemped to replay a NULL stage");
			return;
		}

		ReplayStage *stg = global.replay_stage;
		printf("REPLAY_PLAY mode: %d events, stage: \"%s\"\n", stg->numevents, stage_get(stg->stage)->title);

		tsrand_seed_p(&global.rand_game, stg->seed);
		printf("Random seed: %u\n", stg->seed);

		global.diff = stg->diff;
		init_player(&global.plr);
		replay_stage_sync_player_state(stg, &global.plr);
		stg->playpos = 0;
	}

	Enemy *e = global.plr.slaves, *tmp;
	short power = global.plr.power;
	global.plr.power = -1;

	while(e != 0) {
		tmp = e;
		e = e->next;
		delete_enemy(&global.plr.slaves, tmp);
	}

	player_set_power(&global.plr, power);

	start();
	start_bgm(bgmname);

	if(info->type == STAGE_SPELL) {
		endtime = 0;
	}

	while(global.game_over <= 0) {
		if(!global.boss && !global.dialog)
			event();

		if(!endtime && info->type == STAGE_SPELL && !global.boss) {
			endtime = global.timer + 60;
		}

		((global.replaymode == REPLAY_PLAY) ? replay_input : stage_input)();
		replay_stage_check_desync(global.replay_stage, global.frames, (tsrand() ^ global.plr.points) & 0xFFFF, global.replaymode);

		stage_logic(endtime);

		if(global.frameskip && global.frames % global.frameskip) {
			continue;
		}

		calc_fps(&global.fps);

		tsrand_lock(&global.rand_game);
		tsrand_switch(&global.rand_visual);
		stage_draw(info, draw, shaderrules, endtime);
		tsrand_unlock(&global.rand_game);
		tsrand_switch(&global.rand_game);

// 		print_state_checksum();

		SDL_GL_SwapWindow(video.window);

		if(global.replaymode == REPLAY_PLAY && gamekeypressed(KEY_SKIP)) {
			global.lasttime = SDL_GetTicks();
		} else {
			frame_rate(&global.lasttime);
		}
	}

	if(global.replaymode == REPLAY_RECORD) {
		replay_stage_event(global.replay_stage, global.frames, EV_OVER, 0);
	}

	end();
	stage_end();
	tsrand_switch(&global.rand_visual);
	global.stage = NULL;
}

void draw_title(int t, StageInfo *info, Alignment al, int x, int y, const char *text, TTF_Font *font, Color *color) {
	int i;
	float f = 0;
	if(t < 30 || t > 220)
		return;

	if((i = abs(t-135)) >= 50) {
		i -= 50;
		f = 1/35.0*i;
	}

	if(!config_get_int(CONFIG_NO_SHADER)) {
		Shader *sha = get_shader("stagetitle");
		glUseProgram(sha->prog);
		glUniform1i(uniloc(sha, "trans"), 1);
		glUniform1f(uniloc(sha, "t"), 1.0-f);
		glUniform3fv(uniloc(sha, "color"), 1, (float *)color);

		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, get_tex("titletransition")->gltex);
		glActiveTexture(GL_TEXTURE0);
	} else {
		glColor4f(info->titleclr.r,info->titleclr.g,info->titleclr.b,1.0-f);
	}

	draw_text(al, x, y, text, font);

	glColor4f(1,1,1,1);
	glUseProgram(0);
}

void draw_stage_title(StageInfo *info) {
	int t = global.stageuiframes;

	draw_title(t, info, AL_Center, VIEWPORT_W/2, VIEWPORT_H/2-40, info->title, _fonts.mainmenu, &info->titleclr);
	draw_title(t, info, AL_Center, VIEWPORT_W/2, VIEWPORT_H/2, info->subtitle, _fonts.standard, &info->titleclr);

	if ((current_bgm.title != NULL) && (current_bgm.started_at >= 0))
	{
		draw_title(t - current_bgm.started_at, info, AL_Right, VIEWPORT_W-15, VIEWPORT_H-35, current_bgm.title, _fonts.standard,
			current_bgm.isboss ? &info->bosstitleclr : &info->titleclr);
	}
}
