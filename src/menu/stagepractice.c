/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stagepractice.h"
#include "common.h"
#include "options.h"
#include "global.h"

void draw_stgpract_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, "Stage Practice");
	draw_menu_list(m, 100, 100, NULL);
}

void create_stgpract_menu(MenuData *m, Difficulty diff) {
	char title[128];

	create_menu(m);
	m->draw = draw_stgpract_menu;
	m->logic = animate_menu_list;
	m->flags = MF_Abortable;
	m->transition = TransMenuDark;

	for(StageInfo *stg = stages; stg->procs; ++stg) {
		if(stg->type != STAGE_STORY) {
			break;
		}

		StageProgress *p = stage_get_progress_from_info(stg, diff, false);

		if(p && p->unlocked) {
			snprintf(title, sizeof(title), "%s: %s", stg->title, stg->subtitle);
			add_menu_entry(m, title, start_game_no_difficulty_menu, stg);
		} else {
			snprintf(title, sizeof(title), "%s: ???????", stg->title);
			add_menu_entry(m, title, NULL, NULL);
		}
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_commonaction_close, NULL);

	while(!m->entries[m->cursor].action) {
		++m->cursor;
	}
}
