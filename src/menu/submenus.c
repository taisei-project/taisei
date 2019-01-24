/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "menu.h"
#include "options.h"
#include "stageselect.h"
#include "replayview.h"
#include "spellpractice.h"
#include "stagepractice.h"
#include "difficultyselect.h"
#include "global.h"
#include "submenus.h"

void enter_options(MenuData *menu, void *arg) {
	MenuData m;
	create_options_menu(&m);
	menu_loop(&m);
}

void enter_stagemenu(MenuData *menu, void *arg) {
	MenuData m;
	create_stage_menu(&m);
	menu_loop(&m);
}

void enter_replayview(MenuData *menu, void *arg) {
	MenuData m;
	create_replayview_menu(&m);
	menu_loop(&m);
}

void enter_spellpractice(MenuData *menu, void *arg) {
	MenuData m;
	create_spell_menu(&m);
	menu_loop(&m);
}

void enter_stagepractice(MenuData *menu, void *arg) {
	MenuData m;

	do {
		create_difficulty_menu(&m);

		if(menu_loop(&m) < 0) {
			return;
		}

		global.diff = progress.game_settings.difficulty;
		create_stgpract_menu(&m, global.diff);
		menu_loop(&m);
	} while(m.selected < 0 || m.selected == m.ecount - 1);
}
