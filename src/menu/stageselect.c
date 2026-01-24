/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stageselect.h"

#include "common.h"
#include "menu.h"
#include "options.h"

#include "video.h"
#include "stageinfo.h"
#include "i18n/i18n.h"

static void draw_stage_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, "Select Stage");
	draw_menu_list(m, 100, 100, NULL, SCREEN_H, NULL);
}

MenuData* create_stage_menu(void) {
	char title[STGMENU_MAX_TITLE_LENGTH];
	Difficulty lastdiff = D_Any;
	uint16_t lastgroup = 0;

	MenuData *m = alloc_menu();

	m->draw = draw_stage_menu;
	m->logic = animate_menu_list;
	m->flags = MF_Abortable;
	m->transition = TransFadeBlack;

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);
		Difficulty diff = stg->difficulty;
		uint16_t group = stg->id & ~0xff;

		if(i && (lastgroup != group || diff < lastdiff)) {
			add_menu_separator(m);
		}

		// TODO FIXME THIS IS WRONG
		// Persisting concatenated translated strings here for now
		// However, this is a cheat debug menu so nobody cares

		char stgtitle[STAGE_MAX_TITLE_SIZE];
		stagetitle_format_localized(&stg->title, sizeof(stgtitle), stgtitle);
		snprintf(title, STGMENU_MAX_TITLE_LENGTH, "%s: %s ~ %s", stgtitle, _(stg->subtitle), difficulty_name(stg->difficulty));
		add_menu_entry(m, title, start_game, stg);

		lastdiff = diff;
		lastgroup = group;
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_action_close, NULL);

	return m;
}
