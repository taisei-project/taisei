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
#include "progress.h"

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
	progress_save();
	printf("\nshutdown:\n");

	if(!config_get_int(CONFIG_NO_AUDIO)) shutdown_sfx();
	if(!config_get_int(CONFIG_NO_MUSIC)) shutdown_bgm();

	free_resources();
	video_shutdown();
	gamepad_shutdown();
	stage_free_array();
	config_uninit();

	SDL_Quit();
	printf("-- Good Bye.\n");
}

void init_log(void) {
#if defined(__WINDOWS__) && !defined(__WINDOWS_CONSOLE__)
	const char *pref = get_config_path();
	char *s;

	s = malloc(strlen(pref) + strlen("stdout.txt") + 2);
	strcpy(s, pref);
	strcat(s, "/stdout.txt");

	freopen(s, "w", stdout);

	strcpy(s, pref);
	strcat(s, "/stderr.txt");

	freopen(s, "w", stderr);
#endif
}

int run_tests(void) {
	if(tsrand_test()) {
		return 1;
	}

	if(replay_test()) {
		return 1;
	}

	if(zrwops_test()) {
		return 1;
	}

	return 0;
}

#ifndef __POSIX__
	#define MKDIR(p) mkdir(p)
#else
	#define MKDIR(p) mkdir(p, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

int main(int argc, char **argv) {
	if(run_tests()) {
		return 0;
	}

#ifdef DEBUG
	if(argc >= 2 && argv[1] && !strcmp(argv[1], "dumpstages")) {
		stage_init_array();

		for(StageInfo *stg = stages; stg->loop; ++stg) {
			printf("%i %s: %s\n", stg->id, stg->title, stg->subtitle);
		}

		return 0;
	}
#endif

	Replay replay = {0};
	const char *replay_path = NULL;
	int replay_stage = 0;

	if(argc >= 2 && !strcmp(argv[1], "replay")) {
		if(argc < 3) {
			fprintf(stderr, "Usage: %s replay /path/to/replay.tsr [stage num]\n", argv[0]);
			return 1;
		}

		replay_path = argv[2];

		if(argc > 3) {
			replay_stage = atoi(argv[3]);
		}

		if(!replay_load(&replay, replay_path, REPLAY_READ_ALL | REPLAY_READ_RAWPATH)) {
			return 1;
		}
	}

	init_paths();
	init_log();

	printf("Content path: %s\n", get_prefix());
	printf("Userdata path: %s\n", get_config_path());

	MKDIR(get_config_path());
	MKDIR(get_screenshots_path());
	MKDIR(get_replays_path());

	config_load(CONFIG_FILE);

	printf("initialize:\n");
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0)
		errx(-1, "Error initializing SDL: %s", SDL_GetError());
	printf("-- SDL_Init\n");

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	init_global();

	video_init();
	printf("-- SDL viewport\n");

	init_gl();
	printf("-- GL\n");

	draw_loading_screen();

	gamepad_init();
	stage_init_array();
	progress_load(); // stage_init_array goes first!

	// Order DOES matter: init_global, then sfx/bgm, then load_resources.
	init_sfx();
	init_bgm();
	load_resources();
	printf("initialization complete.\n");

	set_transition(TransLoader, 0, FADE_TIME*2);

	atexit(taisei_shutdown);

	if(replay_path) {
		replay_play(&replay, replay_stage);
		replay_destroy(&replay);
		return 0;
	}

#ifdef DEBUG
	printf("** Compiled with DEBUG flag!\n");
	if(argc >= 2 && argv[1]) {
		printf("** Entering stage skip mode: Stage %d\n", atoi(argv[1]));

		StageInfo* stg = stage_get(atoi(argv[1]));

		if(!stg) {
			errx(-1, "Invalid stage id");
		}

		global.diff = stg->difficulty;

		if(!global.diff) {
			global.diff = D_Easy;
		}

		if(argc == 3 && argv[2]) {
			printf("** Setting difficulty to %d.\n", atoi(argv[2]));
			global.diff = atoi(argv[2]);
		}

		printf("** Entering %s.\n", stg->title);

		do {
			global.game_over = 0;
			init_player(&global.plr);
			global.stage = stg;
			stg->loop();
		} while(global.game_over == GAMEOVER_RESTART);

		return 0;
	}
#endif

	MenuData menu;
	create_main_menu(&menu);
	menu_loop(&menu);

	return 0;
}
