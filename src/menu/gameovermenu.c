
#include "menu.h"
#include "gameovermenu.h"
#include "global.h"

void continue_game(void *arg)
{
	printf("The game is being continued...\n");
	
	global.plr.lifes = PLR_START_LIVES;
	global.plr.continues += 1;
	
	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);
}

void give_up(void *arg) {
	global.game_over = GAMEOVER_ABORT;
}

MenuData *create_gameover_menu() {
	MenuData *m = malloc(sizeof(MenuData));
	create_menu(m);
	
	if(global.plr.continues)
	{
		char s[64];
		snprintf(s, sizeof(s), "Continue (%i)", global.plr.continues);
		add_menu_entry(m, s, continue_game, NULL);
	}
	else
		add_menu_entry(m, "Continue", continue_game, NULL);
	
	add_menu_entry(m, "Give up", give_up, NULL);
	
	return m;
}
