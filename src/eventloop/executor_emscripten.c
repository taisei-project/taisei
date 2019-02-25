/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "eventloop_private.h"
#include "events.h"
#include "global.h"
#include <emscripten.h>

static bool em_handle_resize_event(SDL_Event *event, void *arg);

static void em_loop_callback(void) {
	hrtime_t frame_start_time = time_get();
	global.fps.busy.last_update_time = frame_start_time;
	LogicFrameAction lframe_action = LFRAME_WAIT;
	RenderFrameAction rframe_action = RFRAME_SWAP;

	LoopFrame *frame = evloop.stack_ptr;
	lframe_action = run_logic_frame(frame);

	while(frame != evloop.stack_ptr) {
		frame = evloop.stack_ptr;
		lframe_action = run_logic_frame(frame);
	}

	fpscounter_update(&global.fps.logic);
	rframe_action = run_render_frame(frame);

	if(lframe_action == LFRAME_STOP) {
		eventloop_leave();

		if(evloop.stack_ptr == NULL) {
			events_unregister_handler(em_handle_resize_event);
			emscripten_cancel_main_loop();
		}
	}

	fpscounter_update(&global.fps.busy);
}

static bool em_handle_resize_event(SDL_Event *event, void *arg) {
	emscripten_cancel_main_loop();
	emscripten_set_main_loop(em_loop_callback, 0, false);
	emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
	return false;
}

void eventloop_run(void) {
	emscripten_set_main_loop(em_loop_callback, 0, false);
	emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
	events_register_handler(&(EventHandler) {
		em_handle_resize_event, NULL, EPRIO_SYSTEM, MAKE_TAISEI_EVENT(TE_VIDEO_MODE_CHANGED)
	});
}

#include "eventloop_private.h"
