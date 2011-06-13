/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <err.h>

#include "global.h"
#include "stages/stage0.h"
#include "menu/mainmenu.h"

SDL_Surface *display;

void init_gl() {
	glClearColor(0, 0, 0, 0);
	
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	
	glViewport(0, 0, SCREEN_W, SCREEN_H);
		
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

void shutdown() {
	delete_textures();
	delete_animations();
	delete_sounds();
	delete_shaders();
	
	alutExit();
	SDL_FreeSurface(display);
	SDL_Quit();
}

int main(int argc, char** argv) {
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		errx(-1, "Error initializing SDL: %s", SDL_GetError());
		
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);
	if((display = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, SDL_OPENGL)) == NULL)
		errx(-1, "Error opening screen: %s", SDL_GetError());
	
	SDL_WM_SetCaption("TaiseiProject", NULL); 
	
	init_gl();
		
	if(!alutInit(&argc, argv))
		errx(-1, "Error initializing audio: %s", alutGetErrorString(alutGetError()));
	init_global();
	
	init_player(&global.plr, Youmu, YoumuOpposite);
	
	MenuData menu;
	create_main_menu(&menu);
	
// 	stage0_loop();
	main_menu_loop(&menu);
	
	shutdown();
	
	return 1;
}