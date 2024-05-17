/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "media.h"

#include "charprofile.h"
#include "common.h"
#include "cutsceneview.h"
#include "musicroom.h"

#include "options.h"
#include "video.h"

static void menu_action_enter_musicroom(MenuData *menu, void *arg) {
	enter_menu(create_musicroom_menu(), NO_CALLCHAIN);
}

static void menu_action_enter_cutsceneview(MenuData *menu, void *arg) {
	enter_menu(create_cutsceneview_menu(), NO_CALLCHAIN);
}

static void menu_action_enter_charprofileview(MenuData *menu, void *arg) {
	enter_menu(create_charprofile_menu(), NO_CALLCHAIN);
}

static void draw_media_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, "Media Room");
	draw_menu_list(m, 100, 100, NULL, SCREEN_H, NULL);
}

static void end_media_menu(MenuData *m) {
	res_group_release(m->context);
	mem_free(m->context);
}

MenuData *create_media_menu(void) {
	auto rg = ALLOC(ResourceGroup);
	res_group_init(rg);
	preload_charprofile_menu(rg);

	MenuData *m = alloc_menu();
	m->draw = draw_media_menu;
	m->logic = animate_menu_list;
	m->flags = MF_Abortable;
	m->transition = TransFadeBlack;
	m->end = end_media_menu;
	m->context = rg;

	add_menu_entry(m, "Character Profiles", menu_action_enter_charprofileview, NULL);
	add_menu_entry(m, "Music Room", menu_action_enter_musicroom, NULL);
	add_menu_entry(m, "Replay Cutscenes", menu_action_enter_cutsceneview, NULL);
	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_action_close, NULL);

	while(!dynarray_get(&m->entries, m->cursor).action) {
		++m->cursor;
	}

	return m;
}

void preload_media_menu(ResourceGroup *rg) {
	preload_charprofile_menu(rg);
}
