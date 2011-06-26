/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include <SDL/SDL.h>
#include <time.h>

Global global;

void init_global() {
	memset(&global, 0, sizeof(global));	
	
	alGenSources(SNDSRC_COUNT, global.sndsrc);
	
	srand(time(0));
		
	load_resources();
	init_fonts();
	init_fbo(&global.fbg);
	init_fbo(&global.fsec);
	
	parse_config(CONFIG_FILE);
}

void game_over() {
	global.game_over = GAMEOVER_DEFEAT;
	printf("Game Over!\n");
}

void frame_rate(int *lasttime) {
	int t = *lasttime + 1000/FPS - SDL_GetTicks();
	if(t > 0)
		SDL_Delay(t);
	
	*lasttime = SDL_GetTicks();
}

void calc_fps(FPSCounter *fps) {
	if(SDL_GetTicks() > fps->fpstime+1000) {
		fps->show_fps = fps->fps;
		fps->fps = 0;
		fps->fpstime = SDL_GetTicks();
	} else {
		fps->fps++;
	}
}

void set_ortho() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_W, SCREEN_H, 0, -100, 100);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
}