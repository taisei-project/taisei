/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include <SDL/SDL.h>
#include "font.h"

Global global;

void init_global() {
	memset(&global, 0, sizeof(global));	
	
	alGenSources(SNDSRC_COUNT, global.sndsrc);
	
	load_resources();
	init_fonts();
}

void game_over() {
	global.game_over = 1;
	printf("Game Over!\n");
}

void frame_rate() {
	int t = global.lasttime + 1000/FPS - SDL_GetTicks();
	if(t > 0)
		SDL_Delay(t);
	
	global.lasttime = SDL_GetTicks();
}