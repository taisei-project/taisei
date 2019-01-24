/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "menu.h"
#include "options.h"
#include "stage.h"
#include "stageselect.h"
#include "common.h"

static void draw_stage_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, "Select Stage");
	draw_menu_list(m, 100, 100, NULL);
}

void create_stage_menu(MenuData *m) {
	char title[STGMENU_MAX_TITLE_LENGTH];
	Difficulty lastdiff = D_Any;

	create_menu(m);
	m->draw = draw_stage_menu;
	m->logic = animate_menu_list;
	m->flags = MF_Abortable;
	m->transition = TransMenuDark;

	for(int i = 0; stages[i].procs; ++i) {
		if(stages[i].difficulty < lastdiff || (stages[i].difficulty == D_Extra && lastdiff != D_Extra) || (stages[i].difficulty && !lastdiff)) {
			add_menu_separator(m);
		}

		snprintf(title, STGMENU_MAX_TITLE_LENGTH, "%s: %s", stages[i].title, stages[i].subtitle);
		add_menu_entry(m, title, start_game, &(stages[i]));

		lastdiff = stages[i].difficulty;
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_commonaction_close, NULL);
}
