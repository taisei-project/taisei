/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stagepractice.h"

#include "common.h"
#include "options.h"

#include "i18n/i18n.h"
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

		// TODO FIXME THIS IS WRONG
		// Persisting concatenated translated strings here for now
		// This menu needs a custom draw function and a general overhaul

		char please_rewrite_this_whole_thing[STAGE_MAX_TITLE_SIZE];
		stagetitle_format_localized(&stg->title, sizeof(please_rewrite_this_whole_thing), please_rewrite_this_whole_thing);

		if(p && p->unlocked) {
			snprintf(title, sizeof(title), "%s: %s", please_rewrite_this_whole_thing, _(stg->subtitle));
			add_menu_entry(m, title, start_game_no_difficulty_menu, stg);
		} else {
			snprintf(title, sizeof(title), "%s: ???????", please_rewrite_this_whole_thing);
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
