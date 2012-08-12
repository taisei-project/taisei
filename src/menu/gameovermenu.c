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
	global.game_over = (MAX_CONTINUES - global.plr.continues)? GAMEOVER_ABORT : GAMEOVER_DEFEAT;
}

MenuData *create_gameover_menu(void) {
	MenuData *m = malloc(sizeof(MenuData));
	create_menu(m);
	
	char s[64];
	int c = MAX_CONTINUES - global.plr.continues;
	snprintf(s, sizeof(s), "Continue (%i)", c);
	add_menu_entry(m, s, c? continue_game : NULL, NULL);
	add_menu_entry(m, c? "Give up" : "Return to Title", give_up, NULL);
	
	if(!c)
		m->cursor = 1;
	
	return m;
}
