/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "menu.h"
#include "gameovermenu.h"
#include "ingamemenu.h"
#include "stats.h"
#include "global.h"

static void continue_game(MenuData *m, void *arg) {
	log_info("The game is being continued...");
	player_event(&global.plr, &global.replay.input, &global.replay.output, EV_CONTINUE, 0);
}

static void give_up(MenuData *m, void *arg) {
	global.gameover = GAMEOVER_ABORT;
}

MenuData *create_gameover_menu(void) {
	MenuData *m = alloc_menu();

	m->draw = draw_ingame_menu;
	m->logic = update_ingame_menu;
	m->flags = MF_Transient | MF_AlwaysProcessInput;
	m->transition = TransEmpty;

	if(global.stage->type == STAGE_SPELL) {
		m->context = "Spell Failed";

		add_menu_entry(m, "Retry", restart_game, NULL)->transition = TransFadeBlack;
		add_menu_entry(m, "Give up", give_up, NULL)->transition = TransFadeBlack;
	} else {
		m->context = "Game Over";

		add_menu_entry(m, "Continue", continue_game, NULL);
		add_menu_entry(m, "Restart the Game", restart_game, NULL)->transition = TransFadeBlack;
		add_menu_entry(m, "Give up", give_up, NULL)->transition = TransFadeBlack;
	}

	set_transition(TransEmpty, 0, m->transition_out_time, NO_CALLCHAIN);
	return m;
}
