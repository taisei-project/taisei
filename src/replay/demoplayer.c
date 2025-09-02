/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "demoplayer.h"

#include "events.h"
#include "global.h"
#include "menu/menu.h"
#include "replay.h"
#include "replay/struct.h"
#include "transition.h"
#include "util/env.h"
#include "vfs/public.h"

#define DEMOPLAYER_DIR_PATH        "res/demos"
#define DEMOPLAYER_WAIT_TIME       (60 * FPS)
#define DEMOPLAYER_INTER_WAIT_TIME (30 * FPS)

struct {
	uint next_demo_countdown;
	uint next_demo_index;
	char **demo_files;
	size_t num_demo_files;
	int suspend_level;
	bool initialized;
} dplr;

static bool demo_path_filter(const char *path) {
	return strendswith(path, "." REPLAY_EXTENSION);
}

static bool demoplayer_check_demos(void) {
	if(!dplr.demo_files || dplr.num_demo_files == 0) {
		log_warn("No demos found");
		return false;
	}

	return true;
}

static inline int get_initial_wait_time(void) {
	return env_get("TAISEI_DEMO_TIME", DEMOPLAYER_WAIT_TIME);
}

static inline int get_inter_wait_time(void) {
	return env_get("TAISEI_DEMO_INTER_TIME", DEMOPLAYER_INTER_WAIT_TIME);
}

void demoplayer_init(void) {
	if(dplr.initialized) {
		return;
	}

	dplr.demo_files = vfs_dir_list_sorted(
		DEMOPLAYER_DIR_PATH, &dplr.num_demo_files, vfs_dir_list_order_ascending, demo_path_filter
	);

	if(demoplayer_check_demos()) {
		log_info("Found %zu demo files", dplr.num_demo_files);
	}

	dplr.next_demo_countdown = get_initial_wait_time();
	dplr.suspend_level = 1;
	dplr.initialized = true;

	demoplayer_resume();
}

void demoplayer_shutdown(void) {
	if(!dplr.initialized) {
		return;
	}

	if(dplr.suspend_level == 0) {
		demoplayer_suspend();
	}

	vfs_dir_list_free(dplr.demo_files, dplr.num_demo_files);
	dplr.demo_files = NULL;
	dplr.num_demo_files = 0;
	dplr.initialized = false;
}

typedef struct DemoPlayerContext {
	Replay rpy;
} DemoPlayerContext;

static void demoplayer_start_demo_posttransition(CallChainResult ccr);
static void demoplayer_end_demo(CallChainResult ccr);

static void demoplayer_start_demo(void) {
	auto ctx = ALLOC(DemoPlayerContext);
	CallChain cc = CALLCHAIN(demoplayer_end_demo, ctx);

	demoplayer_suspend();

	if(!demoplayer_check_demos()) {
		run_call_chain(&cc, NULL);
		return;
	}

	uint demoidx = dplr.next_demo_index;
	dplr.next_demo_index = (demoidx + 1) % dplr.num_demo_files;

	char *demo_filename = dplr.demo_files[demoidx];
	char demo_path[sizeof(DEMOPLAYER_DIR_PATH) + 1 + strlen(demo_filename)];
	snprintf(demo_path, sizeof(demo_path), DEMOPLAYER_DIR_PATH "/%s", demo_filename);

	log_debug("Staring demo %s", demo_path);

	if(!replay_load_vfspath(&ctx->rpy, demo_path, REPLAY_READ_ALL)) {
		log_error("Failed to load replay %s", demo_path);
		run_call_chain(&cc, NULL);
		return;
	}

	set_transition(TransFadeWhite, FADE_TIME * 3, FADE_TIME * 2,
		CALLCHAIN(demoplayer_start_demo_posttransition, ctx));
}

static void demoplayer_start_demo_posttransition(CallChainResult ccr) {
	DemoPlayerContext *ctx = ccr.ctx;
	CallChain end = CALLCHAIN(demoplayer_end_demo, ctx);

	if(TRANSITION_RESULT_CANCELED(ccr)) {
		run_call_chain(&end, NULL);
	} else {
		replay_play(&ctx->rpy, 0, true, end);
	}
}

static void demoplayer_end_demo(CallChainResult ccr) {
	DemoPlayerContext *ctx = ccr.ctx;
	replay_reset(&ctx->rpy);
	mem_free(ctx);
	demoplayer_resume();
}

static bool demoplayer_frame_event(SDL_Event *evt, void *arg) {
	if(!(--dplr.next_demo_countdown)) {
		demoplayer_start_demo();
		dplr.next_demo_countdown = get_inter_wait_time();
		return false;
	}

	// log_debug("%u", dplr.next_demo_countdown);
	return false;
}

static bool demoplayer_activity_event(SDL_Event *evt, void *arg) {
	switch(evt->type) {
		case SDL_EVENT_KEY_DOWN :
			goto reset;

		default: switch(TAISEI_EVENT(evt->type)) {
			case TE_GAMEPAD_BUTTON_DOWN:
			case TE_GAMEPAD_AXIS_DIGITAL:
				goto reset;
		}

		return false;
	}

reset:
	dplr.next_demo_countdown = get_initial_wait_time();
	return false;
}

void demoplayer_suspend(void) {
	if(!dplr.initialized) {
		log_debug("Not initialized");
		return;
	}

	dplr.suspend_level++;
	assert(dplr.suspend_level > 0);
	log_debug("Suspend level is now %i", dplr.suspend_level);

	if(dplr.suspend_level > 1) {
		return;
	}

	log_debug("Removing event handlers");

	events_unregister_handler(demoplayer_frame_event);
	events_unregister_handler(demoplayer_activity_event);
}

void demoplayer_resume(void) {
	if(!dplr.initialized) {
		log_debug("Not initialized");
		return;
	}

	dplr.suspend_level--;
	assert(dplr.suspend_level >= 0);
	log_debug("Suspend level is now %i", dplr.suspend_level);

	if(dplr.suspend_level > 0) {
		return;
	}

	log_debug("Installing event handlers");

	events_register_handler(&(EventHandler) {
		demoplayer_frame_event, NULL, EPRIO_LAST, MAKE_TAISEI_EVENT(TE_FRAME),
	});
	events_register_handler(&(EventHandler) {
		demoplayer_activity_event, NULL, EPRIO_SYSTEM,
	});
}
