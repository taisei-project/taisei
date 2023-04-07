/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "util/callchain.h"
#include "menu/menu.h"
#include "cutsceneview.h"
#include "common.h"
#include "options.h"
#include "global.h"
#include "video.h"
#include "cutscenes/cutscene.h"
#include "progress.h"

static void draw_cutsceneview_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, "Cutscene Viewer");
	draw_menu_list(m, 100, 100, NULL, SCREEN_H, NULL);
}

static void restart_menu_bgm(CallChainResult ccr) {
	audio_bgm_play(res_bgm("menu"), true, 0, 0);
}

static void cutscene_player(MenuData *m, void *arg) {
	CutsceneID cutscene_id = (uintptr_t)arg;
	cutscene_enter(CALLCHAIN(restart_menu_bgm, NULL), cutscene_id);
}

MenuData *create_cutsceneview_menu(void) {
	MenuData *m = alloc_menu();

	m->draw = draw_cutsceneview_menu;
	m->logic = animate_menu_list;
	m->flags = MF_Abortable;
	m->transition = TransFadeBlack;

	for(uintptr_t id = 0; id < NUM_CUTSCENE_IDS; id++) {
		if(progress_is_cutscene_unlocked(id)) {
			const char *name = cutscene_get_name(id);
			add_menu_entry(m, name, cutscene_player, (void*)id);
		} else {
			add_menu_entry(m, "(Locked)", NULL, NULL);
		}
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_action_close, NULL);

	while(!dynarray_get(&m->entries, m->cursor).action) {
		++m->cursor;
	}

	return m;
}

