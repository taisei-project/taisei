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
	global.projs = NULL;
	global.fairies = NULL;
	global.poweritems = NULL;
		
	global.textures = NULL;
	global.animations = NULL;
	global.sounds = NULL;
	
	global.boss = NULL;
	
	global.points = 0;	
	global.frames = 0;
	global.game_over = 0;
	
	global.fpstime = 0;
	global.fps = 0;
	
	global.lasttime = 0;
	
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