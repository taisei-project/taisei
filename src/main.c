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
#include "cli.h"

void taisei_shutdown(void) {
	log_info("Shutting down");

	config_save(CONFIG_FILE);
	progress_save();
	progress_unload();

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

int main(int argc, char **argv) {
	setlocale(LC_ALL, "C");

	Replay replay = {0};

	init_paths();
	init_log();

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
	stage_init_array();

	CLIAction a;
	cli_args(argc, argv, &a); // stage_init_array goes first!
	if(a.type == CLI_Quit)
		return 1;

	if(a.type == CLI_DumpStages) {
		for(StageInfo *stg = stages; stg->procs; ++stg) {
			tsfprintf(stdout, "%x %s: %s\n", stg->id, stg->title, stg->subtitle);
		}
		return 0;
	} else if(a.type == CLI_PlayReplay) {
		if(!replay_load(&replay, a.filename, REPLAY_READ_ALL | REPLAY_READ_RAWPATH)) {
			return 1;
		}
	}

	init_sdl();
	init_global();
	video_init();
	init_resources();
	draw_loading_screen();
	audio_init();
	load_resources();
	gamepad_init();
	progress_load(); // stage_init_array goes first!

	set_transition(TransLoader, 0, FADE_TIME*2);

	log_info("Initialization complete");

	atexit(taisei_shutdown);

	if(a.type == CLI_PlayReplay) {
		replay_play(&replay, a.stageid);
		replay_destroy(&replay);
		return 0;
	}

#ifdef DEBUG
	if(a.type == CLI_DumpResTables) {
		print_resource_hashtables();
		return 0;
	}

	log_warn("Compiled with DEBUG flag!");

	if(a.type == CLI_SelectStage) {
		log_info("Entering stage skip mode: Stage %x", a.stageid);
		StageInfo* stg = stage_get(a.stageid);
		assert(stg); // properly checked before this

		global.diff = stg->difficulty;

		if(a.diff) {
			global.diff = a.diff;
			log_info("Setting difficulty to %s", difficulty_name(global.diff));
		} else if(!global.diff) {
			global.diff = D_Easy;
		}

		log_info("Entering %s", stg->title);

		do {
			global.game_over = 0;
			init_player(&global.plr);
			global.plr.cha = a.plrcha;
			global.plr.shot = a.plrshot;
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
