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
