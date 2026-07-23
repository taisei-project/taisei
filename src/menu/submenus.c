/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "submenus.h"

#include "difficultyselect.h"
#include "media.h"
#include "menu.h"
#include "options.h"
#include "replayview.h"
#include "spellpractice.h"
#include "stagepractice.h"
#include "stageselect.h"

#include "global.h"

static void on_leave_options(CallChainResult ccr) {
	MenuData *m = ccr.result;

	if(m && m->state == MS_Dead) {
		taisei_commit_persistent_data();
	}
}

void menu_action_enter_options(MenuData *menu, void *arg) {
	enter_menu(create_options_menu(), CALLCHAIN(on_leave_options, NULL));
}

void menu_action_enter_stagemenu(MenuData *menu, void *arg) {
	enter_menu(create_stage_menu(), NO_CALLCHAIN);
}

void menu_action_enter_replayview(MenuData *menu, void *arg) {
	enter_menu(create_replayview_menu(), NO_CALLCHAIN);
}

void menu_action_enter_spellpractice(MenuData *menu, void *arg) {
	enter_menu(create_spell_menu(), NO_CALLCHAIN);
}

static void stgpract_do_choose_stage(CallChainResult ccr);

void menu_action_enter_stagepractice(MenuData *menu, void *arg) {
	uint32_t mask = 0;
	int n = stageinfo_get_num_stages();
	for(Difficulty d = D_Easy; d <= D_Lunatic; ++d) {
		for(int i = 0; i < n; ++i) {
			StageInfo *stg = stageinfo_get_by_index(i);
			if(stg->type != STAGE_STORY) {
				break;
			}
			StageProgress *p = stageinfo_get_progress(stg, d, false);

			if(p && p->unlocked) {
				mask |= 1 << d;
				break;
			}
		}
	}

	enter_menu(create_difficulty_menu(mask), CALLCHAIN(stgpract_do_choose_stage, NULL));
}

static void stgpract_do_choose_stage(CallChainResult ccr) {
	MenuData *prev_menu = ccr.result;
	assert(prev_menu != NULL);

	if(prev_menu->selected >= 0) {
		enter_menu(create_stgpract_menu(progress.game_settings.difficulty), NO_CALLCHAIN);
	}
}

void menu_action_enter_media(MenuData *menu, void *arg) {
	enter_menu(create_media_menu(), NO_CALLCHAIN);
}
