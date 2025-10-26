/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "savereplay.h"

#include "common.h"
#include "mainmenu.h"

#include "config.h"
#include "difficulty.h"
#include "events.h"
#include "i18n/i18n.h"
#include "plrmodes.h"
#include "progress.h"
#include "replay/struct.h"
#include "util/graphics.h"
#include "video.h"

attr_nonnull_all
static void do_save_replay(Replay *rpy) {
	char strtime[FILENAME_TIMESTAMP_MIN_BUF_SIZE], *name;
	char prepr[16], drepr[16];

	assert(rpy->stages.num_elements > 0);
	ReplayStage *stg = dynarray_get_ptr(&rpy->stages, 0);

	filename_timestamp(strtime, sizeof(strtime), stg->init_time);
	plrmode_repr(prepr, 16, plrmode_find(stg->plr_char, stg->plr_shot), true);
	strlcpy(drepr, difficulty_name(stg->diff), 16);
	drepr[0] += 'a' - 'A';

	if(rpy->stages.num_elements > 1) {
		name = strfmt("taisei_%s_%s_%s", strtime, prepr, drepr);
	} else {
		name = strfmt("taisei_%s_stg%X_%s_%s", strtime, stg->stage, prepr, drepr);
	}

	if(rpy->playername) {
		replay_save(rpy, name);
	} else {
		rpy->playername = config_get_str(CONFIG_PLAYERNAME);
		replay_save(rpy, name);
		rpy->playername = NULL;
	}

	mem_free(name);
}

static void save_rpy(MenuData *menu, void *a) {
	do_save_replay(a);
}

static void draw_saverpy_menu(MenuData *m) {
	PlayerCharacter *pchar = plrchar_get(progress.game_settings.character);
	draw_main_menu_bg(m, 0, 0, 0.05, pchar->menu_texture_name, "stage1/cirnobg");
	colorfill(0, 0, 0, 0.5);

	draw_menu_selector(SCREEN_W/2 + 100 * m->drawdata[0] - 50, SCREEN_H/2, 163, 81, m->frames);

	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W/2, SCREEN_H/2 - 100, 0);
	text_draw(_("Save Replay?"), &(TextParams) {
		.font = "big",
		.align = ALIGN_CENTER,
		.shader = "text_default",
		.color = RGBA(1, 1, 1, 1),
	});
	r_mat_mv_translate(0, 100, 0);

	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
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
	});

	r_mat_mv_pop();
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
		{}
	}, EFLAG_MENU);
}

static void update_saverpy_menu(MenuData *m) {
	m->drawdata[0] += (m->cursor - m->drawdata[0])/10.0;

	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		e->drawdata += 0.2 * (10 * (i == m->cursor) - e->drawdata);
	});
}

static MenuData *create_saverpy_menu(Replay *rpy) {
	MenuData *m = alloc_menu();

	m->input = saverpy_menu_input;
	m->draw = draw_saverpy_menu;
	m->logic = update_saverpy_menu;
	m->flags = MF_Transient | MF_NoDemo;

	add_menu_entry(m, _("Yes"), save_rpy, rpy);
	add_menu_entry(m, _("No"), menu_action_close, NULL);

	return m;
}

void ask_save_replay(Replay *rpy, CallChain next) {
	switch(config_get_int(CONFIG_SAVE_RPY)) {
		case 1:
			do_save_replay(rpy);
			// fallthrough
		case 0:
			run_call_chain(&next, NULL);
			break;

		case 2:
			enter_menu(create_saverpy_menu(rpy), next);
			break;
	}
}
