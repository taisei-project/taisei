/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_eventloop_eventloop_private_h
#define IGUARD_eventloop_eventloop_private_h

#include "taisei.h"

#include "eventloop.h"
#include "hirestime.h"

typedef struct LoopFrame LoopFrame;

#define EVLOOP_STACK_SIZE 32

struct LoopFrame {
	void *context;
	LogicFrameFunc logic;
	RenderFrameFunc render;
	PostLoopFunc on_leave;
	hrtime_t frametime;
	LogicFrameAction prev_logic_action;
};

extern struct evloop_s {
	LoopFrame stack[EVLOOP_STACK_SIZE];
	LoopFrame *stack_ptr;
} evloop;

typedef struct FrameTimes {
	hrtime_t target;
	hrtime_t start;
	hrtime_t next;
} FrameTimes;

void eventloop_leave(void);

LogicFrameAction run_logic_frame(LoopFrame *frame);
LogicFrameAction handle_logic(LoopFrame **pframe, const FrameTimes *ftimes);
RenderFrameAction run_render_frame(LoopFrame *frame);

#endif // IGUARD_eventloop_eventloop_private_h
