/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <time.h>
#include "savereplay.h"
#include "mainmenu.h"
#include "global.h"
#include "replay.h"
#include "plrmodes.h"
#include "common.h"
#include "video.h"

static void do_save_replay(Replay *rpy) {
	char strtime[FILENAME_TIMESTAMP_MIN_BUF_SIZE], *name;
	char prepr[16], drepr[16];

	assert(rpy->numstages > 0);
	ReplayStage *stg = rpy->stages;

	filename_timestamp(strtime, sizeof(strtime), stg->init_time);
	plrmode_repr(prepr, 16, plrmode_find(stg->plr_char, stg->plr_shot), true);
	strlcpy(drepr, difficulty_name(stg->diff), 16);
	drepr[0] += 'a' - 'A';

	if(rpy->numstages > 1) {
		name = strfmt("taisei_%s_%s_%s", strtime, prepr, drepr);
	} else {
		name = strfmt("taisei_%s_stg%X_%s_%s", strtime, stg->stage, prepr, drepr);
	}

	replay_save(rpy, name);
	free(name);
}

static void save_rpy(MenuData *menu, void *a) {
	do_save_replay(&global.replay);
}

static void draw_saverpy_menu(MenuData *m) {
	PlayerCharacter *pchar = plrchar_get(progress.game_settings.character);
	draw_main_menu_bg(m, 0, 0, 0.05, pchar->menu_texture_name, "abstract_blue");
	colorfill(0, 0, 0, 0.5);

	draw_menu_selector(SCREEN_W/2 + 100 * m->drawdata[0] - 50, SCREEN_H/2, 163, 81, m->frames);

	r_mat_push();
	r_mat_translate(SCREEN_W/2, SCREEN_H/2 - 100, 0);
	text_draw("Save Replay?", &(TextParams) {
		.font = "big",
		.align = ALIGN_CENTER,
		.shader = "text_default",
		.color = RGBA(1, 1, 1, 1),
	});
	r_mat_translate(0, 100, 0);

	for(int i = 0; i < m->ecount; i++) {
		MenuEntry *e = &(m->entries[i]);
		assert(e->name != NULL);

		float a = e->drawdata * 0.1;
		Color clr;

		if(e->action == NULL) {
			clr = *RGBA_MUL_ALPHA(0.5, 0.5, 0.5, 0.5);
		} else {
			float ia = 1-a;
			clr = *RGBA_MUL_ALPHA(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, 0.7 + 0.3 * a);
		}

		text_draw(e->name, &(TextParams) {
			.font = "big",
			.align = ALIGN_CENTER,
			.pos = { -50 + 100 * i, 0 },
			.shader = "text_default",
			.color = &clr,
		});
	}

	r_mat_pop();
}

static bool savepry_input_handler(SDL_Event *event, void *arg) {
	if(event->type == MAKE_TAISEI_EVENT(TE_MENU_CURSOR_LEFT)) {
		event->type = MAKE_TAISEI_EVENT(TE_MENU_CURSOR_UP);
	} else if(event->type == MAKE_TAISEI_EVENT(TE_MENU_CURSOR_RIGHT)) {
		event->type = MAKE_TAISEI_EVENT(TE_MENU_CURSOR_DOWN);
	}

	return false;
}

static void saverpy_menu_input(MenuData *menu) {
	events_poll((EventHandler[]){
		{ .proc = savepry_input_handler, .arg = menu },
		{ .proc = menu_input_handler, .arg = menu },
		{NULL}
	}, EFLAG_MENU);
}

static void update_saverpy_menu(MenuData *m) {
	m->drawdata[0] += (m->cursor - m->drawdata[0])/10.0;

	for(int i = 0; i < m->ecount; i++) {
		MenuEntry *e = &(m->entries[i]);
		e->drawdata += 0.2 * (10*(i == m->cursor) - e->drawdata);
	}
}

static MenuData* create_saverpy_menu(void) {
	MenuData *m = alloc_menu();

	m->input = saverpy_menu_input;
	m->draw = draw_saverpy_menu;
	m->logic = update_saverpy_menu;
	m->flags = MF_Transient;

	add_menu_entry(m, "Yes", save_rpy, NULL);
	add_menu_entry(m, "No", menu_action_close, NULL);

	return m;
}

void ask_save_replay(CallChain next) {
	assert(global.replay_stage != NULL);

	switch(config_get_int(CONFIG_SAVE_RPY)) {
		case 1:
			do_save_replay(&global.replay);
			// fallthrough
		case 0:
			run_call_chain(&next, NULL);
			break;

		case 2:
			enter_menu(create_saverpy_menu(), next);
			break;
	}
}
