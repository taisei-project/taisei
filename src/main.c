/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <SDL.h>
#include <GL/glew.h>
#include <sys/stat.h>
#include "taisei_err.h"

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
	
	if(!tconfig.intval[NO_SHADER])
		delete_shaders();
	
	if(!tconfig.intval[NO_AUDIO]) {
		delete_sounds();
		alutExit();
	}
	
	SDL_FreeSurface(display);
	SDL_Quit();
}

int main(int argc, char** argv) {
	mkdir((const char*)get_config_path(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir((const char*)get_screenshots_path(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	
	parse_config(CONFIG_FILE);	
	
	printf("initialize:\n");
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		errx(-1, "Error initializing SDL: %s", SDL_GetError());
	printf("-- SDL_Init\n");
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	int flags = SDL_OPENGL;
	if(tconfig.intval[FULLSCREEN])
		flags |= SDL_FULLSCREEN;
	
	if((display = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, flags)) == NULL)
		errx(-1, "Error opening screen: %s", SDL_GetError());
		
	printf("-- SDL viewport\n");
		
	SDL_WM_SetCaption("TaiseiProject", NULL); 
	
	int e;
	if((e = glewInit()) != GLEW_OK)
		errx(-1, "GLEW failed: %s", glewGetErrorString(e));
	
	printf("-- GLEW\n");
	init_gl();
	
	printf("-- GL\n");
		
	if(!tconfig.intval[NO_AUDIO] && !alutInit(&argc, argv)) {
		warnx("Error initializing audio: %s", alutGetErrorString(alutGetError()));
		tconfig.intval[NO_AUDIO] = 1;
		printf("-- ALUT\n");
	}	
	
	init_global();
	printf("initialization complete.\n");
	MenuData menu;
	create_main_menu(&menu);
	printf("-- menu\n");
	
	main_menu_loop(&menu);
	
	shutdown();
	
	return 1;
}
