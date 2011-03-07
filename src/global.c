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

void init_textures() {
	load_texture(FILE_PREFIX "gfx/wasser.png", &global.textures.water);
	load_texture(FILE_PREFIX "gfx/hud.png", &global.textures.hud);
	load_texture(FILE_PREFIX "gfx/fairy_circle.png", &global.textures.fairy_circle);
	load_texture(FILE_PREFIX "gfx/focus.png", &global.textures.focus);
	
	init_animation(&global.textures.fairy, 2, 2, 15, FILE_PREFIX "gfx/fairy.png"); 
}

void init_global() {
	init_player(&global.plr, Youmu);
	
	init_textures();
	
	load_projectiles();
	init_fonts();
	
	global.projs = NULL;
	global.fairies = NULL;
	
	global.frames = 0;
	global.game_over = 0;
	
	global.fpstime = 0;
	global.fps = 0;
	
	global.lasttime = 0;
}

void game_over() {
	global.game_over = 1;
	printf("Game Over!");
}

void frame_rate() {
	int t = global.lasttime + 1000/FPS - SDL_GetTicks();
	global.time = SDL_GetTicks();
	if(t > 0)
		SDL_Delay(t);
	
	global.lasttime = SDL_GetTicks();
}