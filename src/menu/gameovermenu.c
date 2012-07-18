/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "menu.h"
#include "gameovermenu.h"
#include "global.h"

void continue_game(void *arg)
{
	printf("The game is being continued...\n");
	
	if(global.replaymode == REPLAY_RECORD)	// actually... I'd be strange if REPLAY_PLAY ever got there
		replay_destroy(&global.replay);		// 19:39:29 [@  laochailan] no. no fame for continue users >:D
		
	global.plr.lifes = PLR_START_LIVES;
	global.plr.continues += 1;
	global.plr.focus = 0;
	global.plr.fire = 0;
	
	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);
}

void give_up(void *arg) {
	global.game_over = GAMEOVER_ABORT;
}

MenuData *create_gameover_menu() {
	MenuData *m = malloc(sizeof(MenuData));
	create_menu(m);
	
	char s[64];
	snprintf(s, sizeof(s), "Continue (%i)", MAX_CONTINUES - global.plr.continues);
	add_menu_entry(m, s, continue_game, NULL);
	add_menu_entry(m, "Give up", give_up, NULL);
	
	return m;
}
