/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "menu.h"
#include "options.h"
#include "stageselect.h"
#include "replayview.h"
#include "spellpractice.h"
#include "stagepractice.h"
#include "difficultyselect.h"
#include "media.h"
#include "global.h"
#include "submenus.h"

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
	enter_menu(create_difficulty_menu(), CALLCHAIN(stgpract_do_choose_stage, NULL));
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
