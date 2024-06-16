/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stagepractice.h"

#include "common.h"
#include "options.h"

#include "stageinfo.h"
#include "video.h"

static void draw_stgpract_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, "Stage Practice");
	draw_menu_list(m, 100, 100, NULL, SCREEN_H, NULL);
}

MenuData* create_stgpract_menu(Difficulty diff) {
	char title[128];

	MenuData *m = alloc_menu();

	m->draw = draw_stgpract_menu;
	m->logic = animate_menu_list;
	m->flags = MF_Abortable;
	m->transition = TransFadeBlack;

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);

		if(stg->type != STAGE_STORY) {
			break;
		}

		StageProgress *p = stageinfo_get_progress(stg, diff, false);

		if(p && p->unlocked) {
			snprintf(title, sizeof(title), "%s: %s", stg->title, stg->subtitle);
			add_menu_entry(m, title, start_game_no_difficulty_menu, stg);
		} else {
			snprintf(title, sizeof(title), "%s: ???????", stg->title);
			add_menu_entry(m, title, NULL, NULL);
		}
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_action_close, NULL);

	while(!dynarray_get(&m->entries, m->cursor).action) {
		++m->cursor;
	}

	return m;
}
