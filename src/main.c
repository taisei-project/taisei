/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <locale.h>

#include "global.h"
#include "video.h"
#include "audio.h"
#include "stage.h"
#include "menu/mainmenu.h"
#include "gamepad.h"
#include "resource/bgm.h"
#include "progress.h"
#include "hashtable.h"
#include "log.h"
#include "cli.h"
#include "vfs/setup.h"
#include "version.h"
#include "credits.h"

static void taisei_shutdown(void) {
	log_info("Shutting down");

	config_save();
	progress_save();
	progress_unload();

	free_all_refs();
	free_resources(true);
	uninit_fonts();
	audio_shutdown();
	video_shutdown();
	gamepad_shutdown();
	stage_free_array();
	config_shutdown();
	vfs_shutdown();
	events_shutdown();
	time_shutdown();

	log_info("Good bye");
	SDL_Quit();

	log_shutdown();
}

static void init_log(void) {
	LogLevel lvls_console = log_parse_levels(LOG_DEFAULT_LEVELS_CONSOLE, getenv("TAISEI_LOGLVLS_CONSOLE"));
	LogLevel lvls_stdout = lvls_console & log_parse_levels(LOG_DEFAULT_LEVELS_STDOUT, getenv("TAISEI_LOGLVLS_STDOUT"));
	LogLevel lvls_stderr = lvls_console & log_parse_levels(LOG_DEFAULT_LEVELS_STDERR, getenv("TAISEI_LOGLVLS_STDERR"));
	LogLevel lvls_backtrace = log_parse_levels(LOG_DEFAULT_LEVELS_BACKTRACE, getenv("TAISEI_LOGLVLS_BACKTRACE"));

	log_init(LOG_DEFAULT_LEVELS, lvls_backtrace);
	log_add_output(lvls_stdout, SDL_RWFromFP(stdout, false));
	log_add_output(lvls_stderr, SDL_RWFromFP(stderr, false));
}

static void init_log_file(void) {
	LogLevel lvls_file = log_parse_levels(LOG_DEFAULT_LEVELS_FILE, getenv("TAISEI_LOGLVLS_FILE"));
	log_add_output(lvls_file, vfs_open("storage/log.txt", VFS_MODE_WRITE));
}

static int run_tests(void) {
	if(tsrand_test()) {
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

	if(SDL_Init(SDL_INIT_EVENTS) < 0)
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
	int replay_idx = 0;

	init_log();

	if(run_tests()) {
		return 0;
	}

	log_info("%s %s", TAISEI_VERSION_FULL, TAISEI_VERSION_BUILD_TYPE);
	stage_init_array(); // cli_args depends on this

	// commandline arguments should be parsed as early as possible
	CLIAction a;
	cli_args(argc, argv, &a); // stage_init_array goes first!

	if(a.type == CLI_Quit) {
		free_cli_action(&a);
		return 1;
	}

	if(a.type == CLI_DumpStages) {
		for(StageInfo *stg = stages; stg->procs; ++stg) {
			tsfprintf(stdout, "%x %s: %s\n", stg->id, stg->title, stg->subtitle);
		}

		free_cli_action(&a);
		return 0;
	} else if(a.type == CLI_PlayReplay) {
		if(!replay_load_syspath(&replay, a.filename, REPLAY_READ_ALL)) {
			free_cli_action(&a);
			return 1;
		}

		replay_idx = a.stageid ? replay_find_stage_idx(&replay, a.stageid) : 0;

		if(replay_idx < 0) {
			free_cli_action(&a);
			return 1;
		}
	} else if(a.type == CLI_DumpVFSTree) {
		vfs_setup(true);

		SDL_RWops *rwops = SDL_RWFromFP(stdout, false);

		if(!rwops) {
			log_fatal("SDL_RWFromFP() failed: %s", SDL_GetError());
		}

		if(!vfs_print_tree(rwops, a.filename)) {
			log_warn("VFS error: %s", vfs_get_error());
			SDL_RWclose(rwops);
			free_cli_action(&a);
			return 1;
		}

		SDL_RWclose(rwops);
		vfs_shutdown();
		free_cli_action(&a);
		return 0;
	}

	free_cli_action(&a);
	vfs_setup(false);
	init_log_file();

	config_load();

	log_lib_versions();

	init_sdl();
	time_init();
	init_global(&a);
	events_init();
	init_fonts();
	video_init();
	init_resources();
	draw_loading_screen();
	audio_init();
	load_resources();
	gamepad_init();
	progress_load();

	set_transition(TransLoader, 0, FADE_TIME*2);

	log_info("Initialization complete");

	atexit(taisei_shutdown);

	if(a.type == CLI_PlayReplay) {
		replay_play(&replay, replay_idx);
		replay_destroy(&replay);
		return 0;
	}

	if(a.type == CLI_Credits) {
		credits_loop();
		return 0;
	}

#ifdef DEBUG
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
			player_init(&global.plr);

			if(a.plrmode) {
				global.plr.mode = a.plrmode;
			}

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
