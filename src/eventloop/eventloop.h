/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "hirestime.h"

typedef enum RenderFrameAction {
	RFRAME_SWAP,
	RFRAME_DROP,
} RenderFrameAction;

typedef enum LogicFrameAction {
	LFRAME_WAIT,
	LFRAME_SKIP,
	LFRAME_SKIP_ALWAYS,
	LFRAME_STOP,
} LogicFrameAction;

typedef LogicFrameAction (*LogicFrameFunc)(void *context);
typedef RenderFrameAction (*RenderFrameFunc)(void *context);
typedef void (*PostLoopFunc)(void *context);

typedef struct FrameTimes {
	hrtime_t target;
	hrtime_t start;
	hrtime_t next;
} FrameTimes;

void eventloop_enter(
	void *context,
	LogicFrameFunc frame_logic,
	RenderFrameFunc frame_render,
	PostLoopFunc on_leave,
	uint target_fps
) attr_nonnull(1, 2, 3);

void eventloop_run(void);

FrameTimes eventloop_get_frame_times(void);
