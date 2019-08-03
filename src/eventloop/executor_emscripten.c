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

static FrameTimes frame_times;
static uint frame_num;

static bool em_handle_resize_event(SDL_Event *event, void *arg);

static void em_loop_callback(void) {
	LoopFrame *frame = evloop.stack_ptr;

	if(!frame) {
		events_unregister_handler(em_handle_resize_event);
		emscripten_cancel_main_loop();
		return;
	}

	if(time_get() < frame_times.next) {
		return;
	}

	frame_times.start = time_get();
	frame_times.target = frame->frametime;

	frame_times.next += frame_times.target;
	hrtime_t min_next_time = frame_times.start - 2 * frame_times.target;

	if(min_next_time > frame_times.next) {
		frame_times.next = min_next_time;
	}

	global.fps.busy.last_update_time = frame_times.start;

	LogicFrameAction lframe_action = handle_logic(&frame, &frame_times);

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

static bool em_audio_workaround(SDL_Event *event, void *arg) {
	// Workaround for Chromium:
	// https://github.com/emscripten-core/emscripten/issues/6511
	// Will start playing audio as soon as the first input occurs.

	(__extension__ EM_ASM({
		var sdl2 = Module['SDL2'];
		if(sdl2 !== undefined) {
			var audioctx = sdl2.audioContext;
			if(audioctx !== undefined) {
				audioctx.resume();
			}
		}
	}));

	events_unregister_handler(em_audio_workaround);
	return false;
}

void eventloop_run(void) {
	frame_times.next = time_get();
	emscripten_set_main_loop(em_loop_callback, 0, false);
	update_vsync();

	events_register_handler(&(EventHandler) {
		em_audio_workaround, NULL, EPRIO_SYSTEM, SDL_KEYDOWN,
	});

	events_register_handler(&(EventHandler) {
		em_handle_resize_event, NULL, EPRIO_SYSTEM, MAKE_TAISEI_EVENT(TE_VIDEO_MODE_CHANGED)
	});

	(__extension__ EM_ASM({
		Module['onFirstFrame']();
	}));
}
