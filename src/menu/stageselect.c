/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "global.h"
#include "menu.h"
#include "options.h"
#include "stage.h"
#include "stageselect.h"
#include "common.h"

void create_stage_menu(MenuData *m) {
	char title[STGMENU_MAX_TITLE_LENGTH];
	Difficulty lastdiff = D_Any;

	create_menu(m);
	m->flags = MF_Abortable;

	for(int i = 0; stages[i].loop; ++i) {
		if(stages[i].difficulty < lastdiff || (stages[i].difficulty && !lastdiff)) {
			add_menu_separator(m);
		}

		snprintf(title, STGMENU_MAX_TITLE_LENGTH, "%s: %s", stages[i].title, stages[i].subtitle);
		add_menu_entry(m, title, start_game, &(stages[i]));

		lastdiff = stages[i].difficulty;
	}

	add_menu_entry(m, "Back", menu_commonaction_close, NULL);
}

void draw_stage_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, "Select Stage");
	animate_menu_list(m);
	draw_menu_list(m, 100, 100, NULL);
}

int stage_menu_loop(MenuData *m) {
	return menu_loop(m, NULL, draw_stage_menu, NULL);
}
