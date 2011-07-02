
#include "menu.h"
#include "gameovermenu.h"
#include "global.h"

void continue_game(void *arg)
{
	printf("The game is being continued...\n");
	
	create_item(global.plr.pos, 6-15*I, Power);
	create_item(global.plr.pos, -6-15*I, Power);
	
	global.plr.pos = VIEWPORT_W/2 + VIEWPORT_H*I;
	global.plr.recovery = -(global.frames + 150);
	
	if(global.plr.bombs < 3)
		global.plr.bombs = 3;
	
	global.plr.lifes = 2;
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
		char s[256];
		sprintf(s, "Continue (%i)", global.plr.continues);
		add_menu_entry(m, s, continue_game, NULL);
	}
	else
		add_menu_entry(m, "Continue", continue_game, NULL);
	
	add_menu_entry(m, "Give up", give_up, NULL);
	
	return m;
}
