/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <https://github.com/nexAkari>
 */

#include "replayover.h"
#include "global.h"

static void rewatch(void *arg) {
	global.game_over = GAMEOVER_REWATCH;
}

static void backtotitle(void *arg) {
	((MenuData*)arg)->quit = 2;
}

MenuData* create_replayover_menu() {
	MenuData *m = malloc(sizeof(MenuData));
	create_menu(m);
	add_menu_entry(m, "Rewatch", rewatch, NULL);
	add_menu_entry(m, "Return to Title", backtotitle, m);
	m->selected = 1;
	
	return m;
}
