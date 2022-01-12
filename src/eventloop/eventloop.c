/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "eventloop_private.h"
#include "util.h"
#include "global.h"
#include "video.h"

struct evloop_s evloop;

void eventloop_enter(void *context, LogicFrameFunc frame_logic, RenderFrameFunc frame_render, PostLoopFunc on_leave, uint target_fps) {
	assert(is_main_thread());
	assume(evloop.stack_ptr < evloop.stack + EVLOOP_STACK_SIZE - 1);

	LoopFrame *frame;

	if(evloop.stack_ptr == NULL) {
		frame = evloop.stack_ptr = evloop.stack;
	} else {
		frame = ++evloop.stack_ptr;
	}

	frame->context = context;
	frame->logic = frame_logic;
	frame->render = frame_render;
	frame->on_leave = on_leave;
	frame->frametime = HRTIME_RESOLUTION / target_fps;
	frame->prev_logic_action = LFRAME_WAIT;
}

void eventloop_leave(void) {
	assert(is_main_thread());
	assume(evloop.stack_ptr != NULL);

	LoopFrame *frame = evloop.stack_ptr;

	if(evloop.stack_ptr == evloop.stack) {
		evloop.stack_ptr = NULL;
	} else {
		--evloop.stack_ptr;
	}

	if(frame->on_leave != NULL) {
		frame->on_leave(frame->context);
	}
}

FrameTimes eventloop_get_frame_times(void) {
	return evloop.frame_times;
}

LogicFrameAction run_logic_frame(LoopFrame *frame) {
	assert(frame == evloop.stack_ptr);

	if(frame->prev_logic_action == LFRAME_STOP) {
		return LFRAME_STOP;
	}

	LogicFrameAction a = frame->logic(frame->context);

	if(a != LFRAME_SKIP_ALWAYS) {
		fpscounter_update(&global.fps.logic);
	}

	if(taisei_quit_requested()) {
		a = LFRAME_STOP;
	}

	frame->prev_logic_action = a;
	return a;
}

LogicFrameAction handle_logic(LoopFrame **pframe, const FrameTimes *ftimes) {
	LogicFrameAction lframe_action;
	uint cnt = 0;

	do {
		lframe_action = run_logic_frame(*pframe);

		if(lframe_action == LFRAME_SKIP_ALWAYS) {
			lframe_action = LFRAME_SKIP;
			cnt = 0;
		}

		while(evloop.stack_ptr != *pframe) {
			*pframe = evloop.stack_ptr;
			lframe_action = run_logic_frame(*pframe);
			cnt = UINT_MAX - 1; // break out of the outer loop
		}
	} while(
		lframe_action == LFRAME_SKIP &&
		++cnt < config_get_int(CONFIG_SKIP_SPEED)
	);

	if(lframe_action == LFRAME_STOP) {
		eventloop_leave();
		*pframe = evloop.stack_ptr;

		if(*pframe == NULL) {
			return LFRAME_STOP;
		}
	}

	return lframe_action;
}

RenderFrameAction run_render_frame(LoopFrame *frame) {
	attr_unused LoopFrame *stack_prev = evloop.stack_ptr;
	r_framebuffer_clear(NULL, CLEAR_ALL, RGBA(0, 0, 0, 1), 1);
	RenderFrameAction a = frame->render(frame->context);
	assert(evloop.stack_ptr == stack_prev);

	if(a == RFRAME_SWAP) {
		video_swap_buffers();
	}

	fpscounter_update(&global.fps.render);
	return a;
}
