/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
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
#include "vfs/public.h"

#define STATIC_RESOURCE_PREFIX PREFIX "/share/taisei/"

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
	config_uninit();
	vfs_uninit();

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

static void get_core_paths(char **res, char **storage) {
#ifdef RELATIVE
	*res = SDL_GetBasePath();
	strappend(res, "data/");
#else
	*res = strdup(STATIC_RESOURCE_PREFIX);
#endif

	*storage = SDL_GetPrefPath("", "taisei");
}

static void init_vfs(bool silent) {
	char *res_path, *storage_path;
	get_core_paths(&res_path, &storage_path);

	if(!silent) {
		log_info("Resource path: %s", res_path);
		log_info("Storage path: %s", storage_path);
	}

	char *p = NULL;

	struct mpoint_t {
		const char *dest;	const char *syspath;							bool mkdir;
	} mpts[] = {
		{"storage",			storage_path,									true},
		{"res",				res_path,										false},
		{"res",				p = strfmt("%s/resources", storage_path),		true},
		{NULL}
	};

	vfs_init();
	vfs_create_union_mountpoint("res");

	for(struct mpoint_t *mp = mpts; mp->dest; ++mp) {
		if(!vfs_mount_syspath(mp->dest, mp->syspath, mp->mkdir)) {
			log_fatal("Failed to mount '%s': %s", mp->dest, vfs_get_error());
		}
	}

	vfs_mkdir_required("storage/replays");
	vfs_mkdir_required("storage/screenshots");

	free(p);
	free(res_path);
	free(storage_path);
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

	stage_init_array(); // cli_args depends on this

	// commandline arguments should be parsed as early as possible
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
		if(!replay_load_syspath(&replay, a.filename, REPLAY_READ_ALL)) {
			return 1;
		}

		replay_idx = a.stageid ? replay_find_stage_idx(&replay, a.stageid) : 0;
		if(replay_idx < 0) {
			return 1;
		}
	} else if(a.type == CLI_DumpVFSTree) {
		init_vfs(true);

		SDL_RWops *rwops = SDL_RWFromFP(stdout, false);

		if(!rwops) {
			log_fatal("SDL_RWFromFP() failed: %s", SDL_GetError());
		}

		if(!vfs_print_tree(rwops, a.filename)) {
			log_warn("VFS error: %s", vfs_get_error());
			SDL_RWclose(rwops);
			return 1;
		}

		SDL_RWclose(rwops);
		return 0;
	}

	free_cli_action(&a);
	init_vfs(false);
	init_log_file();

	config_load();

	log_lib_versions();

	init_sdl();
	init_global(&a);
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
