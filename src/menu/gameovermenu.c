/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "gameovermenu.h"

#include "global.h"
#include "ingamemenu.h"
#include "i18n/i18n.h"
#include "menu.h"
#include "stats.h"

typedef struct GameoverMenuContext {
	IngameMenuContext base;
	GameoverMenuAction *output;
} GameoverMenuContext;

static void set_action(MenuData *m, void *arg) {
	GameoverMenuContext *ctx = m->context;
	*ctx->output = (uintptr_t)arg;
}

static MenuEntry *add_action_entry(MenuData *m, const char *text, GameoverMenuAction action, bool enabled) {
	return add_menu_entry(m, text, enabled ? set_action : NULL, (void*)action);
}

static MenuEntry *add_quickload_entry(MenuData *m, const GameoverMenuParams *params) {
	if(!params->quickload_shown) {
		return NULL;
	}

	return add_action_entry(m, _("Load Quicksave"), GAMEOVERMENU_ACTION_QUICKLOAD, params->quickload_enabled);
}

static void free_gameover_menu(MenuData *m) {
	mem_free(m->context);
}

MenuData *create_gameover_menu(const GameoverMenuParams *params) {
	MenuData *m = alloc_menu();

	auto ctx = ALLOC(GameoverMenuContext, {
		.output = params->output,
	});

	*ctx->output = GAMEOVERMENU_ACTION_DEFAULT;

	m->draw = draw_ingame_menu;
	m->logic = update_ingame_menu;
	m->end = free_gameover_menu;
	m->flags = MF_Transient | MF_AlwaysProcessInput;
	m->transition = TransEmpty;
	m->context = ctx;

	add_quickload_entry(m, params);

	if(global.stage->type == STAGE_SPELL) {
		ctx->base.title = _("Spell Failed");

		add_action_entry(m, _("Restart"), GAMEOVERMENU_ACTION_RESTART, true)
			->transition = TransFadeBlack;
		add_action_entry(m, _("Give up"), GAMEOVERMENU_ACTION_QUIT, true)
			->transition = TransFadeBlack;
	} else {
		ctx->base.title = _("Game Over");

		char s[64];
		int c = MAX_CONTINUES - global.plr.stats.total.continues_used;
		bool have_continues = c > 0;

		if(have_continues) {
			snprintf(s, sizeof(s), _("Continue (%i)"), c);
		} else {
			snprintf(s, sizeof(s), _("Continue"));
		}

		add_action_entry(m, s, GAMEOVERMENU_ACTION_CONTINUE, have_continues);
		add_action_entry(m, _("Restart the Game"), GAMEOVERMENU_ACTION_RESTART, true)
			->transition = TransFadeBlack;
		add_action_entry(m, have_continues ? _("Give up") : _("Return to Title"), GAMEOVERMENU_ACTION_QUIT, true)
			->transition = TransFadeBlack;
	}

	while(!dynarray_get(&m->entries, m->cursor).action) {
		++m->cursor;
	}

	set_transition(TransEmpty, 0, m->transition_out_time, NO_CALLCHAIN);
	return m;
}
