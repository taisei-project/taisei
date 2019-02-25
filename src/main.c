/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <locale.h>
#include <png.h>

#include "global.h"
#include "video.h"
#include "audio/audio.h"
#include "stage.h"
#include "menu/mainmenu.h"
#include "menu/savereplay.h"
#include "gamepad.h"
#include "progress.h"
#include "log.h"
#include "cli.h"
#include "vfs/setup.h"
#include "version.h"
#include "credits.h"
#include "renderer/api.h"
#include "taskmanager.h"

static void taisei_shutdown(void) {
	log_info("Shutting down");

	taskmgr_global_shutdown();

	if(!global.is_replay_verification) {
		config_save();
		progress_save();
	}

	progress_unload();

	free_all_refs();
	free_resources(true);
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
	LogLevel lvls_console = log_parse_levels(LOG_DEFAULT_LEVELS_CONSOLE, env_get("TAISEI_LOGLVLS_CONSOLE", NULL));
	LogLevel lvls_stdout = lvls_console & log_parse_levels(LOG_DEFAULT_LEVELS_STDOUT, env_get("TAISEI_LOGLVLS_STDOUT", NULL));
	LogLevel lvls_stderr = lvls_console & log_parse_levels(LOG_DEFAULT_LEVELS_STDERR, env_get("TAISEI_LOGLVLS_STDERR", NULL));

	log_init(LOG_DEFAULT_LEVELS);
	log_add_output(lvls_stdout, SDL_RWFromFP(stdout, false), log_formatter_console);
	log_add_output(lvls_stderr, SDL_RWFromFP(stderr, false), log_formatter_console);
}

static void init_log_file(void) {
	LogLevel lvls_file = log_parse_levels(LOG_DEFAULT_LEVELS_FILE, env_get("TAISEI_LOGLVLS_FILE", NULL));
	log_add_output(lvls_file, vfs_open("storage/log.txt", VFS_MODE_WRITE), log_formatter_file);

	char *logpath = vfs_repr("storage/log.txt", true);

	if(logpath != NULL) {
		char *m = strfmt(
			"For more information, see the log file at %s\n\n"
			"Please report the problem to the developers at https://taisei-project.org/ if it persists.",
			logpath
		);
		log_set_gui_error_appendix(m);
		free(m);
	} else {
		log_set_gui_error_appendix("Please report the problem to the developers at https://taisei-project.org/ if it persists.");
	}
}

/*
static SDLCALL void sdl_log(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	log_debug("[%i %i] %s", category, priority, message);
}
*/

static void init_sdl(void) {
	SDL_version v;

	if(SDL_Init(SDL_INIT_EVENTS) < 0) {
		log_fatal("SDL_Init() failed: %s", SDL_GetError());
	}

	// initialize it
	is_main_thread();

	// * TODO: refine this and make it optional
	// SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
	// SDL_LogSetOutputFunction(sdl_log, NULL);

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

static void log_system_specs(void) {
	log_info("CPU count: %d", SDL_GetCPUCount());
	// log_info("CPU type: %s", SDL_GetCPUType());
	// log_info("CPU name: %s", SDL_GetCPUName());
	log_info("CacheLine size: %d", SDL_GetCPUCacheLineSize());
	log_info("RDTSC: %d", SDL_HasRDTSC());
	log_info("Altivec: %d", SDL_HasAltiVec());
	log_info("MMX: %d", SDL_HasMMX());
	log_info("3DNow: %d", SDL_Has3DNow());
	log_info("SSE: %d", SDL_HasSSE());
	log_info("SSE2: %d", SDL_HasSSE2());
	log_info("SSE3: %d", SDL_HasSSE3());
	log_info("SSE4.1: %d", SDL_HasSSE41());
	log_info("SSE4.2: %d", SDL_HasSSE42());
	log_info("AVX: %d", SDL_HasAVX());
	log_info("AVX2: %d", SDL_HasAVX2());
#if SDL_VERSION_ATLEAST(2, 0, 6)
	log_info("NEON: %d", SDL_HasNEON());
#endif
	log_info("RAM: %d MB", SDL_GetSystemRAM());
}

static int main_singlestg(CLIAction *cli);
static int main_replay(CLIAction *cli, Replay *rpy, int idx);

int main(int argc, char **argv) {
	setlocale(LC_ALL, "C");

	Replay replay = {0};
	int replay_idx = 0;
	bool headless = false;

	htutil_init();
	init_log();

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
			tsfprintf(stdout, "%X %s: %s\n", stg->id, stg->title, stg->subtitle);
		}

		free_cli_action(&a);
		return 0;
	} else if(a.type == CLI_PlayReplay || a.type == CLI_VerifyReplay) {
		if(!replay_load_syspath(&replay, a.filename, REPLAY_READ_ALL)) {
			free_cli_action(&a);
			return 1;
		}

		replay_idx = a.stageid ? replay_find_stage_idx(&replay, a.stageid) : 0;

		if(replay_idx < 0) {
			free_cli_action(&a);
			replay_destroy(&replay);
			return 1;
		}

		if(a.type == CLI_VerifyReplay) {
			headless = true;
		}
	} else if(a.type == CLI_DumpVFSTree) {
		vfs_setup(false);

		SDL_RWops *rwops = SDL_RWFromFP(stdout, false);
		int status = 0;

		if(!rwops) {
			log_fatal("SDL_RWFromFP() failed: %s", SDL_GetError());
			log_sdl_error(LOG_ERROR, "SDL_RWFromFP");
			status = 1;
		} else if(!vfs_print_tree(rwops, a.filename)) {
			log_error("VFS error: %s", vfs_get_error());
			status = 2;
		}

		SDL_RWclose(rwops);
		vfs_shutdown();
		free_cli_action(&a);
		return status;
	}

	free_cli_action(&a);

	vfs_setup(false);

	if(headless) {
		env_set("SDL_AUDIODRIVER", "dummy", true);
		env_set("SDL_VIDEODRIVER", "dummy", true);
		env_set("TAISEI_AUDIO_BACKEND", "null", true);
		env_set("TAISEI_RENDERER", "null", true);
		env_set("TAISEI_NOPRELOAD", true, false);
		env_set("TAISEI_PRELOAD_REQUIRED", false, false);
	} else {
		init_log_file();
	}

#ifdef __EMSCRIPTEN__
	env_set("TAISEI_NOASYNC", true, true);
	// env_set("TAISEI_NOPRELOAD", true, true);
#endif

	log_info("%s %s", TAISEI_VERSION_FULL, TAISEI_VERSION_BUILD_TYPE);
	log_system_specs();
	log_lib_versions();

	config_load();

	init_sdl();
	taskmgr_global_init();
	time_init();
	init_global(&a);
	events_init();
	video_init();
	init_resources();
	r_post_init();
	draw_loading_screen();

	audio_init();
	load_resources();
	gamepad_init();
	progress_load();

	set_transition(TransLoader, 0, FADE_TIME*2);

	log_info("Initialization complete");

	atexit(taisei_shutdown);

	if(a.type == CLI_PlayReplay || a.type == CLI_VerifyReplay) {
		return main_replay(&a, &replay, replay_idx);
	}

	if(a.type == CLI_Credits) {
		credits_enter(NO_CALLCHAIN);
		eventloop_run();
		return 0;
	}

#ifdef DEBUG
	log_warn("Compiled with DEBUG flag!");

	if(a.type == CLI_SelectStage) {
		return main_singlestg(&a);
	}
#endif

	enter_menu(create_main_menu(), NO_CALLCHAIN);
	eventloop_run();
	return 0;
}

typedef struct SingleStageContext {
	PlayerMode *plrmode;
	StageInfo *stg;
} SingleStageContext;

static void main_singlestg_begin_game(CallChainResult ccr);
static void main_singlestg_end_game(CallChainResult ccr);
static void main_singlestg_cleanup(CallChainResult ccr);

static void main_singlestg_begin_game(CallChainResult ccr) {
	SingleStageContext *ctx = ccr.ctx;

	global.replay_stage = NULL;
	replay_init(&global.replay);
	global.gameover = 0;
	player_init(&global.plr);

	if(ctx->plrmode) {
		global.plr.mode = ctx->plrmode;
	}

	stage_enter(ctx->stg, CALLCHAIN(main_singlestg_end_game, ctx));
}

static void main_singlestg_end_game(CallChainResult ccr) {
	if(global.gameover == GAMEOVER_RESTART) {
		replay_destroy(&global.replay);
		main_singlestg_begin_game(ccr);
	} else {
		ask_save_replay(CALLCHAIN(main_singlestg_cleanup, ccr.ctx));
	}
}

static void main_singlestg_cleanup(CallChainResult ccr) {
	replay_destroy(&global.replay);
	free(ccr.ctx);
}

static int main_singlestg(CLIAction *a) {
	log_info("Entering stage skip mode: Stage %X", a->stageid);

	StageInfo* stg = stage_get(a->stageid);
	assert(stg); // properly checked before this

	SingleStageContext *ctx = calloc(1, sizeof(*ctx));
	ctx->plrmode = a->plrmode;
	ctx->stg = stg;

	global.diff = stg->difficulty;
	global.is_practice_mode = (stg->type != STAGE_EXTRA);

	if(a->diff) {
		global.diff = a->diff;
		log_info("Setting difficulty to %s", difficulty_name(global.diff));
	} else if(!global.diff) {
		global.diff = D_Easy;
	}

	log_info("Entering %s", stg->title);

	main_singlestg_begin_game(CALLCHAIN_RESULT(ctx, NULL));
	eventloop_run();
	return 0;
}

static void main_replay_cleanup(CallChainResult ccr) {
	replay_destroy(&global.replay);
}

int main_replay(CLIAction *cli, Replay *rpy, int idx) {
	replay_play(rpy, idx, CALLCHAIN(main_replay_cleanup, NULL));
	replay_destroy(rpy);
	eventloop_run();
	return 0;
}
