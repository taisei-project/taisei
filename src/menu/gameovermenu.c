/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "menu.h"
#include "gameovermenu.h"
#include "ingamemenu.h"
#include "global.h"

static void continue_game(MenuData *m, void *arg) {
	log_info("The game is being continued...");

	if(global.replaymode == REPLAY_RECORD) { // actually... I'd be strange if REPLAY_PLAY ever got there
		replay_destroy(&global.replay);      // 19:39:29 [@  laochailan] no. no fame for continue users >:D
		global.replay_stage = NULL;
	}

	global.plr.lives = PLR_START_LIVES;
	global.plr.life_fragments = 0;
	global.continues += 1;

	stage_clear_hazards(false);
}

static void give_up(MenuData *m, void *arg) {
	global.game_over = (MAX_CONTINUES - global.continues)? GAMEOVER_ABORT : GAMEOVER_DEFEAT;
}

void create_gameover_menu(MenuData *m) {
	create_menu(m);
	m->draw = draw_ingame_menu;

	m->flags = MF_Transient | MF_AlwaysProcessInput;
	m->transition = TransEmpty;

	if(global.stage->type == STAGE_SPELL) {
		m->context = "Spell Failed";

		add_menu_entry(m, "Retry", restart_game, NULL)->transition = TransFadeBlack;
		add_menu_entry(m, "Give up", give_up, NULL)->transition = TransFadeBlack;
	} else {
		m->context = "Game Over";

		char s[64];
		int c = MAX_CONTINUES - global.continues;
		snprintf(s, sizeof(s), "Continue (%i)", c);
		add_menu_entry(m, s, c? continue_game : NULL, NULL);
		add_menu_entry(m, "Restart the Game", restart_game, NULL)->transition = TransFadeBlack;
		add_menu_entry(m, c? "Give up" : "Return to Title", give_up, NULL)->transition = TransFadeBlack;

		if(!c)
			m->cursor = 1;
	}

	set_transition(TransEmpty, 0, m->transition_out_time);
}
