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
#include "audio.h"
#include "stage.h"
#include "menu/mainmenu.h"
#include "paths/native.h"
#include "gamepad.h"
#include "resource/bgm.h"
#include "progress.h"
#include "hashtable.h"

void taisei_shutdown(void) {
	config_save(CONFIG_FILE);
	progress_save();
	printf("\nshutdown:\n");

	free_all_refs();
	free_resources();
	audio_shutdown();
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

	s = strfmt("%s/%s", pref, "stdout.txt");
	freopen(s, "w", stdout);
	free(s);

	s = strfmt("%s/%s", pref, "stderr.txt");
	freopen(s, "w", stderr);
	free(s);
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

	if(hashtable_test()) {
		return 1;
	}

	if(color_test()) {
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

		for(StageInfo *stg = stages; stg->procs; ++stg) {
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

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		errx(-1, "Error initializing SDL: %s", SDL_GetError());
	printf("-- SDL\n");

	init_global();

	video_init();
	printf("-- Video and OpenGL\n");

	audio_init();
	printf("-- Audio\n");

	init_resources();
	draw_loading_screen();

	load_resources();
	gamepad_init();
	stage_init_array();
	progress_load(); // stage_init_array goes first!

	set_transition(TransLoader, 0, FADE_TIME*2);

	printf("initialization complete.\n");
	atexit(taisei_shutdown);

	if(replay_path) {
		replay_play(&replay, replay_stage);
		replay_destroy(&replay);
		return 0;
	}

#ifdef DEBUG

	if(argc >= 2 && argv[1] && !strcmp(argv[1], "dumprestables")) {
		print_resource_hashtables();
		return 0;
	}

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
			stage_loop(stg);
		} while(global.game_over == GAMEOVER_RESTART);

		return 0;
	}
#endif

	MenuData menu;
	create_main_menu(&menu);
	menu_loop(&menu);

	return 0;
}
