/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "audio/audio.h"
#include "cli.h"
#include "coroutine/coroutine.h"
#include "credits.h"
#include "cutscenes/cutscene.h"
#include "dynstage.h"
#include "eventloop/eventloop.h"
#include "filewatch/filewatch.h"
#include "gamepad.h"
#include "global.h"
#include "log.h"
#include "menu/mainmenu.h"
#include "menu/savereplay.h"
#include "progress.h"
#include "renderer/common/models.h"
#include "renderer/common/sprite_batch.h"
#include "replay/demoplayer.h"
#include "replay/struct.h"
#include "replay/tsrtool.h"
#include "rwops/rwops_stdiofp.h"
#include "stage.h"
#include "stageobjects.h"
#include "taskmanager.h"
#include "util/env.h"
#include "util/gamemode.h"
#include "util/io.h"
#include "version.h"
#include "vfs/setup.h"
#include "video.h"
#include "watchdog.h"

#include <locale.h>
#include <png.h>
#include <zlib.h>

#undef UNICODE
#include <SDL3/SDL_main.h>

static bool watchdog_handler(SDL_Event *evt, void *arg) {
	assert(evt->type == MAKE_TAISEI_EVENT(TE_WATCHDOG));
	config_reset();
	return true;
}

attr_unused
static void taisei_shutdown(void) {
	log_info("Shutting down");

	if(!global.is_replay_verification) {
		config_save();
		progress_save();
	}

	r_release_resources();
	res_shutdown();

	if(global.is_kiosk_mode) {
		events_unregister_handler(watchdog_handler);
	}

	watchdog_shutdown();
	demoplayer_shutdown();
	progress_unload();
	stage_objpools_shutdown();
	gamemode_shutdown();
	taskmgr_global_shutdown();
	audio_shutdown();
	r_models_shutdown();
	r_sprite_batch_shutdown();
	video_shutdown();
	gamepad_shutdown();
	stageinfo_shutdown();
	config_shutdown();
	filewatch_shutdown();
	vfs_shutdown();
	events_shutdown();
	time_shutdown();
	coroutines_shutdown();
	log_queue_shutdown();
	thread_shutdown();
	log_info("Good bye");
	log_shutdown();
	SDL_Quit();
}

static void init_log(void) {
	LogLevel lvls_console = log_parse_levels(LOG_DEFAULT_LEVELS_CONSOLE, env_get("TAISEI_LOGLVLS_CONSOLE", ""));
	LogLevel lvls_stdout = lvls_console & log_parse_levels(LOG_DEFAULT_LEVELS_STDOUT, env_get("TAISEI_LOGLVLS_STDOUT", ""));
	LogLevel lvls_stderr = lvls_console & log_parse_levels(LOG_DEFAULT_LEVELS_STDERR, env_get("TAISEI_LOGLVLS_STDERR", ""));

	log_init(LOG_DEFAULT_LEVELS);
	log_add_output(lvls_stdout, SDL_RWFromFP(stdout, false), log_formatter_console);
	log_add_output(lvls_stderr, SDL_RWFromFP(stderr, false), log_formatter_console);
}

static void init_log_file(void) {
	LogLevel lvls_file = log_parse_levels(LOG_DEFAULT_LEVELS_FILE, env_get("TAISEI_LOGLVLS_FILE", ""));
	log_add_output(lvls_file, vfs_open("storage/log.txt", VFS_MODE_WRITE), log_formatter_file);

	char *logpath = vfs_repr("storage/log.txt", true);

	if(logpath != NULL) {
		char *m = strfmt(
			"For more information, see the log file at %s\n\n"
			"Please report the problem to the developers at https://taisei-project.org/ if it persists.",
			logpath
		);
		mem_free(logpath);
		log_set_gui_error_appendix(m);
		mem_free(m);
	} else {
		log_set_gui_error_appendix("Please report the problem to the developers at https://taisei-project.org/ if it persists.");
	}
}

static void init_log_filter(void) {
	SDL_IOStream *rw = vfs_open("storage/logfilter", VFS_MODE_READ);

	if(!rw) {
		return;
	}

	char buf[256];

	char *p;
	while((p = SDL_RWgets(rw, buf, sizeof(buf)))) {
		while(isspace(*p)) {
			++p;
		}

		if(*p == '#' || !*p) {
			continue;
		}

		log_add_filter_string(p);
	}

	SDL_CloseIO(rw);
}

static SDLCALL void sdl_log(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	const char *cat_str, *prio_str;
	LogLevel lvl = LOG_DEBUG;

	switch(category) {
		case SDL_LOG_CATEGORY_APPLICATION: cat_str = "Application"; break;
		case SDL_LOG_CATEGORY_ERROR:       cat_str = "Error"; break;
		case SDL_LOG_CATEGORY_ASSERT:      cat_str = "Assert"; break;
		case SDL_LOG_CATEGORY_SYSTEM:      cat_str = "System"; break;
		case SDL_LOG_CATEGORY_AUDIO:       cat_str = "Audio"; break;
		case SDL_LOG_CATEGORY_VIDEO:       cat_str = "Video"; break;
		case SDL_LOG_CATEGORY_RENDER:      cat_str = "Render"; break;
		case SDL_LOG_CATEGORY_INPUT:       cat_str = "Input"; break;
		case SDL_LOG_CATEGORY_TEST:        cat_str = "Test"; break;
		case SDL_LOG_CATEGORY_GPU:         cat_str = "GPU"; break;
		default:                           cat_str = "Unknown"; break;
	}

	switch(priority) {
		case SDL_LOG_PRIORITY_VERBOSE:  prio_str = "Verbose"; break;
		case SDL_LOG_PRIORITY_DEBUG:    prio_str = "Debug"; break;
		case SDL_LOG_PRIORITY_INFO:     prio_str = "Info"; lvl = LOG_INFO; break;
		case SDL_LOG_PRIORITY_WARN:     prio_str = "Warn"; lvl = LOG_WARN; break;
		case SDL_LOG_PRIORITY_ERROR:    prio_str = "Error"; lvl = LOG_ERROR; break;
		case SDL_LOG_PRIORITY_CRITICAL: prio_str = "Critical"; lvl = LOG_ERROR; break;
		default:                        prio_str = "Unknown"; break;
	}

	log_custom(lvl, "[%s, %s] %s", cat_str, prio_str, message);
}

static void init_sdl(void) {
	if(!SDL_Init(SDL_INIT_EVENTS)) {
		log_fatal("SDL_Init() failed: %s", SDL_GetError());
	}

#if defined(SDL_HINT_EMSCRIPTEN_ASYNCIFY) && defined(__EMSCRIPTEN__)
	SDL_SetHintWithPriority(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0", SDL_HINT_OVERRIDE);
#endif

	SDL_LogPriority sdl_logprio = env_get("TAISEI_SDL_LOG", SDL_LOG_PRIORITY_INFO);

	if(sdl_logprio >= SDL_LOG_PRIORITY_VERBOSE) {
		SDL_SetLogPriorities(sdl_logprio);
		SDL_SetLogOutputFunction(sdl_log, NULL);
	}

	log_info("SDL initialized");

	int v = SDL_VERSION;
	log_info("Compiled against SDL %u.%u.%u",
		SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v));

	v = SDL_GetVersion();
	log_info("Using SDL %u.%u.%u",
		SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v));

	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "Taisei Project");
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, TAISEI_VERSION);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "org.taisei_project.Taisei");
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, "https://taisei-project.org/");
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
}

static void log_lib_versions(void) {
	log_info("Compiled against zlib %s", ZLIB_VERSION);
	log_info("Using zlib %s", zlibVersion());

	log_info("Compiled against libpng %s", PNG_LIBPNG_VER_STRING);
	log_info("Using libpng %s", png_get_header_ver(NULL));
}

static void log_system_specs(void) {
	log_info("CPU count: %d", SDL_GetNumLogicalCPUCores());
	// log_info("CPU type: %s", SDL_GetCPUType());
	// log_info("CPU name: %s", SDL_GetCPUName());
	log_info("CacheLine size: %d", SDL_GetCPUCacheLineSize());
	log_info("Altivec: %d", SDL_HasAltiVec());
	log_info("MMX: %d", SDL_HasMMX());
	log_info("SSE: %d", SDL_HasSSE());
	log_info("SSE2: %d", SDL_HasSSE2());
	log_info("SSE3: %d", SDL_HasSSE3());
	log_info("SSE4.1: %d", SDL_HasSSE41());
	log_info("SSE4.2: %d", SDL_HasSSE42());
	log_info("AVX: %d", SDL_HasAVX());
	log_info("AVX2: %d", SDL_HasAVX2());
	log_info("NEON: %d", SDL_HasNEON());
	log_info("RAM: %d MB", SDL_GetSystemRAM());
}

static void log_version(void) {
	log_info("%s %s", TAISEI_VERSION_FULL, TAISEI_VERSION_BUILD_TYPE);
}

typedef struct MainContext {
	CLIAction cli;
	Replay *replay_in;
	Replay *replay_out;
	SDL_IOStream *replay_out_stream;
	ResourceGroup rg;
	int replay_idx;
	uchar headless : 1;
	uchar commit_persistent_data : 1;
} MainContext;

static void main_post_vfsinit(CallChainResult ccr);
static void main_mainmenu(CallChainResult ccr);
static void main_singlestg(MainContext *mctx) attr_unused;
static void main_replay(MainContext *mctx);
static noreturn void main_vfstree(CallChainResult ccr);

static void cleanup_replay(Replay **rpy) {
	if(*rpy) {
		replay_reset(*rpy);
		mem_free(*rpy);
		*rpy = NULL;
	}
}

static Replay *alloc_replay(void) {
	return ALLOC(Replay);
}

static noreturn void main_quit(MainContext *ctx, int status) {
	res_group_release(&ctx->rg);
	free_cli_action(&ctx->cli);

	cleanup_replay(&ctx->replay_in);

	if(ctx->replay_out_stream) {
		if(ctx->replay_out) {
			if(!replay_write(
				ctx->replay_out,
				ctx->replay_out_stream,
				REPLAY_STRUCT_VERSION_WRITE
			)) {
				log_fatal("replay_write() failed");
			}
		}

		SDL_CloseIO(ctx->replay_out_stream);
		ctx->replay_out_stream = NULL;
	}

	cleanup_replay(&ctx->replay_out);

	mem_free(ctx);
	exit(status);
}

static void main_cleanup(CallChainResult ccr) {
	MenuData *m = ccr.result;

	if(m == NULL || m->state == MS_Dead) {
		main_quit(ccr.ctx, 0);
	}
}

// shut up -Wmissing-prototypes
int main(int argc, char **argv);

attr_used
int main(int argc, char **argv) {
	setlocale(LC_ALL, "");
	setlocale(LC_MESSAGES, "");
	bindtextdomain(TAISEI_BUILDCONF_GETTEXT_PACKAGE, TAISEI_BUILDCONF_LOCALE_PATH);
	textdomain(TAISEI_BUILDCONF_GETTEXT_PACKAGE);

	mem_install_sdl_callbacks();
	thread_init();
	coroutines_init();
	init_log();
	stageinfo_init(); // cli_args depends on this

#if DEBUG
	if(argc > 1 && !strcmp("tsrtool", argv[1])) {
		return tsrtool_main(argc - 1, argv + 1);
	}
#endif

	auto ctx = ALLOC(MainContext);

	// commandline arguments should be parsed as early as possible
	cli_args(argc, argv, &ctx->cli); // stage_init_array goes first!

	if(ctx->cli.type == CLI_Quit) {
		main_quit(ctx, 0);
	}

	if(ctx->cli.type == CLI_DumpStages) {
		int n = stageinfo_get_num_stages();
		for(int i = 0; i < n; ++i) {
			StageInfo *stg = stageinfo_get_by_index(i);
			tsfprintf(stdout, "%X %s: %s\n", stg->id, stg->title, stg->subtitle);
		}

		main_quit(ctx, 0);
	}

	if(ctx->cli.type == CLI_PlayReplay || ctx->cli.type == CLI_VerifyReplay) {
		ctx->replay_in = alloc_replay();

		if(!replay_load_syspath(ctx->replay_in, ctx->cli.filename, REPLAY_READ_ALL)) {
			main_quit(ctx, 1);
		}

		ctx->replay_idx = ctx->cli.stageid ? replay_find_stage_idx(ctx->replay_in, ctx->cli.stageid) : 0;

		if(ctx->replay_idx < 0) {
			main_quit(ctx, 1);
		}

		if(ctx->cli.type == CLI_VerifyReplay) {
			ctx->headless = true;
		}

		if(ctx->cli.out_replay != NULL) {
			ctx->replay_out_stream = SDL_IOFromFile(ctx->cli.out_replay, "wb");

			if(!ctx->replay_out_stream) {
				log_sdl_error(LOG_FATAL, "SDL_IOFromFile");
			}

			ctx->replay_out = alloc_replay();
		}
	} else if(ctx->cli.type == CLI_DumpVFSTree) {
		vfs_setup(CALLCHAIN(main_vfstree, ctx));
		return 0; // NO main_quit here! vfs_setup may be asynchronous.
	}

	log_info("Girls are now preparing, please wait warmly...");

	free_cli_action(&ctx->cli);
	vfs_setup(CALLCHAIN(main_post_vfsinit, ctx));
	return 0;
}

static void main_post_vfsinit(CallChainResult ccr) {
	MainContext *ctx = ccr.ctx;

	if(ctx->headless) {
		env_set("SDL_AUDIODRIVER", "dummy", true);
		env_set("SDL_VIDEODRIVER", "dummy", true);
		env_set("TAISEI_AUDIO_BACKEND", "null", true);
		env_set("TAISEI_RENDERER", "null", true);
	} else {
		init_log_file();
	}

	init_log_filter();

	log_version();
	log_system_specs();
	log_lib_versions();

	config_load();

	init_sdl();
	taskmgr_global_init();
	gamemode_init();
	time_init();
	init_global(&ctx->cli);
	events_init();

	if(global.is_kiosk_mode) {
		watchdog_init(env_get("TAISEI_KIOSK_TIMEOUT", 120 * FPS));
		events_register_handler(&(EventHandler) {
			.priority = EPRIO_SYSTEM,
			.event_type = MAKE_TAISEI_EVENT(TE_WATCHDOG),
			.proc = watchdog_handler,
		});
	}

	video_init(&(VideoInitParams) {
		.width = ctx->cli.width,
		.height = ctx->cli.height,
	});
	filewatch_init();
	res_init();
	r_models_init();
	r_sprite_batch_init();
	r_post_init();

	res_group_init(&ctx->rg);

	draw_loading_screen();
	dynstage_init_monitoring();

	audio_init();
	res_post_init();
	menu_preload(&ctx->rg);
	gamepad_init();
	progress_load();

	if(ctx->cli.unlock_all) {
		log_info("Unlocking all content because of --unlock-all");
		progress_unlock_all();
	}

	video_post_init();

	set_transition(TransLoader, 0, FADE_TIME*2, NO_CALLCHAIN);

	log_info("Initialization complete");

#ifndef __EMSCRIPTEN__
	atexit(taisei_shutdown);
#endif

	CallChain cc_cleanup = CALLCHAIN(main_cleanup, ctx);
	CallChain cc_mainmenu = CALLCHAIN(main_mainmenu, ctx);

	if(ctx->cli.type == CLI_QuitLate) {
		run_call_chain(&cc_cleanup, NULL);
		return;
	}

	if(ctx->cli.type == CLI_PlayReplay || ctx->cli.type == CLI_VerifyReplay) {
		main_replay(ctx);
		return;
	}

	if(ctx->cli.type == CLI_Credits) {
		credits_enter(cc_cleanup);
		eventloop_run();
		return;
	}

#ifdef DEBUG
	log_warn("Compiled with DEBUG flag!");

	if(ctx->cli.type == CLI_SelectStage) {
		main_singlestg(ctx);
		return;
	}

	if(ctx->cli.type == CLI_Cutscene) {
		cutscene_enter(cc_cleanup, ctx->cli.cutscene);
		eventloop_run();
		return;
	}
#endif

	if(!progress_is_cutscene_unlocked(CUTSCENE_ID_INTRO) || ctx->cli.force_intro) {
		ctx->commit_persistent_data = true;
		cutscene_enter(cc_mainmenu, CUTSCENE_ID_INTRO);
		eventloop_run();
		return;
	}

	run_call_chain(&cc_mainmenu, NULL);
	eventloop_run();
}

static void main_mainmenu(CallChainResult ccr) {
	MainContext *ctx = ccr.ctx;

	if(ctx->commit_persistent_data) {
		taisei_commit_persistent_data();
	}

	demoplayer_init();
	enter_menu(create_main_menu(), CALLCHAIN(main_cleanup, ctx));
}

typedef struct SingleStageContext {
	MainContext *mctx;
	PlayerMode *plrmode;
	StageInfo *stg;
} SingleStageContext;

static void main_singlestg_begin_game(CallChainResult ccr);
static void main_singlestg_end_game(CallChainResult ccr);
static void main_singlestg_cleanup(CallChainResult ccr);

static void main_singlestg_begin_game(CallChainResult ccr) {
	SingleStageContext *ctx = ccr.ctx;
	MainContext *mctx = ctx->mctx;

	replay_reset(mctx->replay_out);
	replay_state_init_record(&global.replay.output, mctx->replay_out);
	replay_state_deinit(&global.replay.input);
	global.gameover = 0;
	player_init(&global.plr);
	stats_init(&global.plr.stats);

	if(ctx->plrmode) {
		global.plr.mode = ctx->plrmode;
	}

	stage_enter(ctx->stg, &mctx->rg, CALLCHAIN(main_singlestg_end_game, ctx));
}

static void main_singlestg_end_game(CallChainResult ccr) {
	SingleStageContext *ctx = ccr.ctx;
	MainContext *mctx = ctx->mctx;

	if(global.gameover == GAMEOVER_RESTART) {
		main_singlestg_begin_game(ccr);
	} else {
		ask_save_replay(mctx->replay_out, CALLCHAIN(main_singlestg_cleanup, ccr.ctx));
	}
}

static void main_singlestg_cleanup(CallChainResult ccr) {
	SingleStageContext *ctx = ccr.ctx;
	MainContext *mctx = ctx->mctx;
	res_group_release(&mctx->rg);
	mem_free(ccr.ctx);
	main_quit(mctx, 0);
}

static void main_singlestg(MainContext *mctx) {
	CLIAction *a = &mctx->cli;
	log_info("Entering stage skip mode: Stage %X", a->stageid);

	StageInfo *stg = stageinfo_get_by_id(a->stageid);
	assert(stg); // properly checked before this

	auto ctx = ALLOC(SingleStageContext, {
		.mctx = mctx,
		.plrmode = a->plrmode,
		.stg = stg,
	});

	global.diff = stg->difficulty;
	global.is_practice_mode = (stg->type != STAGE_EXTRA);

	if(a->diff) {
		global.diff = a->diff;
		log_info("Setting difficulty to %s", difficulty_name(global.diff));
	} else if(!global.diff) {
		global.diff = D_Easy;
	}

	log_info("Entering %s", stg->title);

	mctx->replay_out = alloc_replay();
	main_singlestg_begin_game(CALLCHAIN_RESULT(ctx, NULL));
	eventloop_run();
}

static void main_replay(MainContext *mctx) {
	if(mctx->replay_out) {
		replay_state_init_record(&global.replay.output, mctx->replay_out);
		stralloc(&mctx->replay_out->playername, mctx->replay_in->playername);
	}

	replay_play(mctx->replay_in, mctx->replay_idx, false, CALLCHAIN(main_cleanup, mctx));
	eventloop_run();
}

static void main_vfstree(CallChainResult ccr) {
	MainContext *mctx = ccr.ctx;
	SDL_IOStream *rwops = SDL_RWFromFP(stdout, false);
	int status = 0;

	if(!rwops) {
		log_fatal("SDL_RWFromFP() failed: %s", SDL_GetError());
		log_sdl_error(LOG_ERROR, "SDL_RWFromFP");
		status = 1;
	} else if(!vfs_print_tree(rwops, mctx->cli.filename)) {
		log_error("VFS error: %s", vfs_get_error());
		status = 2;
	}

	SDL_CloseIO(rwops);
	vfs_shutdown();
	main_quit(mctx, status);
}
