/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "eventloop_private.h"
#include "events.h"
#include "global.h"

#include <emscripten.h>

static uint frame_num;

static bool em_handle_resize_event(SDL_Event *event, void *arg);

static void em_loop_callback(void) {
	LoopFrame *frame = evloop.stack_ptr;

	if(!frame) {
		events_unregister_handler(em_handle_resize_event);
		emscripten_cancel_main_loop();
		return;
	}

	if(time_get() < evloop.frame_times.next) {
		return;
	}

	evloop.frame_times.start = time_get();
	evloop.frame_times.target = frame->frametime;

	evloop.frame_times.next += evloop.frame_times.target;
	hrtime_t min_next_time = evloop.frame_times.start - 2 * evloop.frame_times.target;

	if(min_next_time > evloop.frame_times.next) {
		evloop.frame_times.next = min_next_time;
	}

	global.fps.busy.last_update_time = evloop.frame_times.start;

	LogicFrameAction lframe_action = handle_logic(&frame, &evloop.frame_times);

	if(!frame || lframe_action == LFRAME_STOP) {
		return;
	}

	if(!(frame_num++ % get_effective_frameskip())) {
		run_render_frame(frame);
	}

	fpscounter_update(&global.fps.busy);
}

static void update_vsync(void) {
	switch(config_get_int(CONFIG_VSYNC)) {
		case 0: r_vsync(VSYNC_NONE); break;
		default: r_vsync(VSYNC_NORMAL); break;
	}
}

static bool em_handle_resize_event(SDL_Event *event, void *arg) {
	emscripten_cancel_main_loop();
	emscripten_set_main_loop(em_loop_callback, 0, false);
	update_vsync();
	return false;
}

void eventloop_run(void) {
	evloop.frame_times.next = time_get();
	emscripten_set_main_loop(em_loop_callback, 0, false);
	update_vsync();

	events_register_handler(&(EventHandler) {
		em_handle_resize_event, NULL, EPRIO_SYSTEM, MAKE_TAISEI_EVENT(TE_VIDEO_MODE_CHANGED)
	});

	EM_ASM({
		Module['onFirstFrame']();
	});
}
