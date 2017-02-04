/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <sys/stat.h>
#include "taisei_err.h"

#include "global.h"
#include "video.h"
#include "stage.h"
#include "menu/mainmenu.h"
#include "paths/native.h"
#include "taiseigl.h"
#include "gamepad.h"
#include "resource/bgm.h"

void init_gl(void) {
	load_gl_functions();
	check_gl_extensions();
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	init_quadvbo();
	
	glClear(GL_COLOR_BUFFER_BIT);
}


void taisei_shutdown(void) {
	config_save(CONFIG_FILE);
	printf("\nshutdown:\n");
	
	if (!tconfig.intval[NO_AUDIO]) shutdown_sfx();
	if (!tconfig.intval[NO_MUSIC]) shutdown_bgm();
	free_resources();
	video_shutdown();
	gamepad_shutdown();
	
	SDL_Quit();
	printf("-- Good Bye.\n");
}

#ifdef __MINGW32__
	#define MKDIR(p) mkdir(p)
#else
	#define MKDIR(p) mkdir(p, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

int main(int argc, char** argv) {
	if(tsrand_test())
		return 0;
	
	init_paths();

	printf("Content path: %s\n", get_prefix());
	printf("Userdata path: %s\n", get_config_path());

	MKDIR(get_config_path());
	MKDIR(get_screenshots_path());
	MKDIR(get_replays_path());

	config_load(CONFIG_FILE);

	printf("initialize:\n");
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		errx(-1, "Error initializing SDL: %s", SDL_GetError());
	printf("-- SDL_Init\n");
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	video_init();
	printf("-- SDL viewport\n");
	
	init_gl();
	printf("-- GL\n");
	
	draw_loading_screen();
	
	// Order DOES matter: init_global, then sfx/bgm, then load_resources.
	init_global();
	gamepad_init();
	init_sfx(&argc, argv);
	init_bgm(&argc, argv);
	load_resources();
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
		
		StageInfo* stg = stage_get(atoi(argv[1]));
		
		if(stg) {
			printf("** Entering %s.\n", stg->title);
			
			do {
				global.game_over = 0;
				init_player(&global.plr);
				stg->loop();
			} while(global.game_over == GAMEOVER_RESTART);
			
			return 0;
		}
		
		printf("** Invalid stage number. Quitting stage skip mode.\n");
	}
#endif
	
	MenuData menu;
	create_main_menu(&menu);
	printf("-- menu\n");	
	set_sfx_volume(tconfig.fltval[SFX_VOLUME]);
	set_bgm_volume(tconfig.fltval[BGM_VOLUME]);
	start_bgm("bgm_menu");
	main_menu_loop(&menu);
	
	return 0;
}
