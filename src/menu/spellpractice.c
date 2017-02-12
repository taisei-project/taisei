/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "spellpractice.h"
#include "common.h"
#include "options.h"
#include "global.h"

void create_spell_menu(MenuData *m) {
	char title[128];
	Difficulty lastdiff = D_Any;

	create_menu(m);
	m->flags = MF_Abortable;

	for(StageInfo *stg = stages; stg->loop; ++stg) {
		if(stg->type != STAGE_SPELL) {
			continue;
		}

		if(stg->difficulty < lastdiff) {
			add_menu_separator(m);
		}

		StageProgress *p = stage_get_progress_from_info(stg, D_Any, false);
		if(p && p->unlocked) {
			snprintf(title, sizeof(title), "%s: %s", stg->title, stg->subtitle);
			add_menu_entry(m, title, start_game, stg);
		} else {
			snprintf(title, sizeof(title), "%s: ???????", stg->title);
			add_menu_entry(m, title, NULL, NULL);
		}

		lastdiff = stg->difficulty;
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_commonaction_close, NULL);

	while(!m->entries[m->cursor].action) {
		++m->cursor;
	}
}

void draw_spell_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, "Spell Practice");
	animate_menu_list(m);
	draw_menu_list(m, 100, 100, NULL);
}

int spell_menu_loop(MenuData *m) {
	return menu_loop(m, NULL, draw_spell_menu, NULL);
}
