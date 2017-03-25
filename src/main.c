/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <sys/stat.h>
#include <errno.h>
#include <locale.h>

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
#include "log.h"

void taisei_shutdown(void) {
	log_info("Shutting down");

	config_save(CONFIG_FILE);
	progress_save();

	free_all_refs();
	free_resources(true);
	audio_shutdown();
	video_shutdown();
	gamepad_shutdown();
	stage_free_array();
	config_uninit();

	log_info("Good bye");
	SDL_Quit();

	log_shutdown();
}

void init_log(void) {
	const char *pref = get_config_path();
	char *logpath = strfmt("%s/%s", pref, "log.txt");

	LogLevel lvls_console = log_parse_levels(LOG_DEFAULT_LEVELS_CONSOLE, getenv("TAISEI_LOGLVLS_CONSOLE"));
	LogLevel lvls_stdout = lvls_console & log_parse_levels(LOG_DEFAULT_LEVELS_STDOUT, getenv("TAISEI_LOGLVLS_STDOUT"));
	LogLevel lvls_stderr = lvls_console & log_parse_levels(LOG_DEFAULT_LEVELS_STDERR, getenv("TAISEI_LOGLVLS_STDERR"));
	LogLevel lvls_file = log_parse_levels(LOG_DEFAULT_LEVELS_FILE, getenv("TAISEI_LOGLVLS_FILE"));
	LogLevel lvls_backtrace = log_parse_levels(LOG_DEFAULT_LEVELS_BACKTRACE, getenv("TAISEI_LOGLVLS_BACKTRACE"));

	log_init(LOG_DEFAULT_LEVELS, lvls_backtrace);
	log_add_output(lvls_stdout, SDL_RWFromFP(stdout, false));
	log_add_output(lvls_stderr, SDL_RWFromFP(stderr, false));
	log_add_output(lvls_file, SDL_RWFromFile(logpath, "w"));

	free(logpath);
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

static void init_sdl(void) {
	SDL_version v;

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		log_fatal("SDL_Init() failed: %s", SDL_GetError());

	log_info("SDL initialized");

	SDL_VERSION(&v);
	log_info("Compiled against SDL %u.%u.%u", v.major, v.minor, v.patch);

	SDL_GetVersion(&v);
	log_info("Using SDL %u.%u.%u", v.major, v.minor, v.patch);
}

static void log_lib_versions(void) {
	log_info("Compiled against zlib %s", ZLIB_VERSION);
	log_info("Using zlib %s", zlibVersion());
	log_info("Compiled against libpng %s", PNG_LIBPNG_VER_STRING);
	log_info("Using libpng %s", png_get_header_ver(NULL));
}

static void log_args(int argc, char **argv) {
	for(int i = 0; i < argc; ++i) {
		log_debug("argv[%i] = %s ", i, argv[i]);
	}
}

int main(int argc, char **argv) {
	setlocale(LC_ALL, "C");

#ifdef DEBUG
	if(argc >= 2 && argv[1] && !strcmp(argv[1], "dumpstages")) {
		stage_init_array();

		for(StageInfo *stg = stages; stg->procs; ++stg) {
			tsfprintf(stdout, "%i %s: %s\n", stg->id, stg->title, stg->subtitle);
		}

		return 0;
	}
#endif

	Replay replay = {0};
	const char *replay_path = NULL;
	int replay_stage = 0;

	if(argc >= 2 && !strcmp(argv[1], "replay")) {
		if(argc < 3) {
			tsfprintf(stderr, "Usage: %s replay /path/to/replay.tsr [stage num]\n", argv[0]);
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
	log_args(argc, argv);

	if(run_tests()) {
		return 0;
	}

	log_info("Content path: %s", get_prefix());
	log_info("Userdata path: %s", get_config_path());

	MKDIR(get_config_path());
	MKDIR(get_screenshots_path());
	MKDIR(get_replays_path());

	if(chdir(get_prefix())) {
		log_fatal("chdir() failed: %s", strerror(errno));
	} else {
		char cwd[1024]; // i don't care if this is not enough for you, getcwd is garbage
		getcwd(cwd, sizeof(cwd));
		log_info("Changed working directory to %s", cwd);
	}

	config_load(CONFIG_FILE);

	log_lib_versions();
	init_sdl();
	init_global();
	video_init();
	init_resources();
	draw_loading_screen();
	audio_init();
	load_resources();
	gamepad_init();
	stage_init_array();
	progress_load(); // stage_init_array goes first!

	set_transition(TransLoader, 0, FADE_TIME*2);

	log_info("Initialization complete");

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

	log_warn("Compiled with DEBUG flag!");

	if(argc >= 2 && argv[1] &&
		!strstartswith(argv[1], "-psn_") // OSX passes this when started up from an application bundle
	) {
		log_info("Entering stage skip mode: Stage %d", atoi(argv[1]));

		StageInfo* stg = stage_get(atoi(argv[1]));

		if(!stg) {
			log_fatal("Invalid stage id");
		}

		global.diff = stg->difficulty;

		if(argc == 3 && argv[2]) {
			global.diff = atoi(argv[2]);
			log_info("Setting difficulty to %s", difficulty_name(global.diff));
		} else if(!global.diff) {
			global.diff = D_Easy;
		}

		log_info("Entering %s", stg->title);

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
