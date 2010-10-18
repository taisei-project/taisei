/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include <SDL/SDL.h>

Global global;

void init_textures() {
	load_texture(FILE_PREFIX "gfx/wasser.png", &global.textures.water);
	load_texture(FILE_PREFIX "gfx/hud.png", &global.textures.hud);
}

void init_global() {
	init_player(&global.plr, Youmu);
	
	init_textures();
	
	load_projectiles();
	global.projs = NULL;
	global.fairies = NULL;
	
	global.frames = 0;
	global.game_over = 0;
	
	global.time = 0;
	global.fps = 0;
	
	global.lasttime = 0;
}

void game_over() {
	global.game_over = 1;
	printf("Game Over!");
}

void frame_rate() {
	int t = global.lasttime + 1000/FPS - SDL_GetTicks();
	if(t > 0)
		SDL_Delay(t);
	global.lasttime = SDL_GetTicks();
}