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
	srand(time(0));
	
	load_resources();
	printf("- fonts:\n");
	init_fonts();
	
	if(!tconfig.intval[NO_SHADER]) {
		printf("init_fbo():\n");
		init_fbo(&global.fbg);
		init_fbo(&global.fsec);
		printf("-- finished\n");
	}
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

inline double frand() {
	return rand()/(double)RAND_MAX;
}

extern SDL_Surface *display;

void toggle_fullscreen()
{
	int newflags = display->flags;
	newflags ^= SDL_FULLSCREEN;
	
	SDL_FreeSurface(display);
	if((display = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, newflags)) == NULL)
		errx(-1, "Error opening screen: %s", SDL_GetError());
}

void global_input()
{
	Uint8 *keys = SDL_GetKeyState(NULL);
	
	// Catch the fullscreen hotkey (Alt+Enter)
	if((keys[SDLK_LALT] || keys[SDLK_RALT]) && keys[SDLK_RETURN])
	{
		if(!global.fullscreenhotkey_state)
		{
			toggle_fullscreen();
			global.fullscreenhotkey_state = 1;
		}
	}
	else global.fullscreenhotkey_state = 0;
}
