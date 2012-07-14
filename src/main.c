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
#include "stages/stage1.h"
#include "menu/mainmenu.h"
#include "paths/native.h"

SDL_Surface *display;

void init_gl() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	init_quadvbo();
}


void taisei_shutdown() {
	printf("\nshutdown:\n");
	if(resources.state & RS_SfxLoaded) {
		printf("-- alutExit()\n");
		alutExit();
	}
	
	printf("-- freeing textures\n");
	delete_textures();
	
	printf("-- freeing FBOs\n");
	delete_fbo(&resources.fbg[0]);
	delete_fbo(&resources.fbg[1]);
	delete_fbo(&resources.fsec);
	printf("-- freeing VBOs\n");
	delete_vbo(&_quadvbo);
	printf("-- freeing shaders\n");
	delete_shaders();
	
	SDL_FreeSurface(display);
	SDL_Quit();
	printf("-- Good Bye.\n");
}

int main(int argc, char** argv) {
#ifdef __MINGW32__
	mkdir(get_config_path());
	mkdir(get_screenshots_path());
#else
	mkdir(get_config_path(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(get_screenshots_path(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
	
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
	
	if(!tconfig.intval[NO_AUDIO] && !alutInit(&argc, argv))
	{
		warnx("Error initializing audio: %s", alutGetErrorString(alutGetError()));
		tconfig.intval[NO_AUDIO] = 1;
		printf("-- ALUT\n");
	}
	
	init_global();
	printf("initialization complete.\n");
	
	atexit(taisei_shutdown);
		
#ifdef DEBUG
	printf("** Compiled with DEBUG flag!\n");
	if(argc >= 2 && argv[1]) {
		printf("** Entering stage skip mode: Stage %d\n", atoi(argv[1]));
		
		global.diff = D_Easy;
		
		if(argc == 3 && argv[2]) {
			printf("** Setting difficulty to %d.\n", atoi(argv[2]));
			global.diff = atoi(argv[2]);
		}
		
		init_player(&global.plr);
		
		switch(atoi(argv[1])) {
		case 1:
			stage0_loop();
		case 2:
			stage1_loop();
		case 3:
			stage2_loop();
		case 4:
			stage3_loop();
			return 1;
		}
		
		printf("** Invalid stage number. Quitting stage skip mode.\n");
	}
#endif
	
	MenuData menu;
	create_main_menu(&menu);
	printf("-- menu\n");	
	main_menu_loop(&menu);
		
	return 1;
}
