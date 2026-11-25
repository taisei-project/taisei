/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "hirestime.h"

typedef enum SceneAction {
	SCENE_ACTION_WAIT,
	SCENE_ACTION_SKIP,
	SCENE_ACTION_SKIP_ALWAYS,
	SCENE_ACTION_STOP,
} SceneAction;

typedef SceneAction (*SceneLogicFunc)(void *context);
typedef void (*SceneRenderFunc)(void *context);
typedef void (*SceneLeaveFunc)(void *context);

typedef struct FrameTimes {
	hrtime_t start;
	hrtime_t next;
	uint target_fps;
} FrameTimes;

void eventloop_enter(
	void *context,
	SceneLogicFunc logic,
	SceneRenderFunc render,
	SceneLeaveFunc on_leave,
	uint target_fps
) attr_nonnull(1, 2, 3);

void eventloop_run(void);

FrameTimes eventloop_get_frame_times(void);

void eventloop_set_frameskip(int frameskip);
void eventloop_set_headless(bool headless);
bool eventloop_is_uncapped(void);
uint eventloop_get_frameskip(void);
