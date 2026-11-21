/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "eventloop.h"

typedef struct Scene Scene;

#define EVLOOP_STACK_SIZE 32

struct Scene {
	void *context;
	SceneLogicFunc logic;
	SceneRenderFunc render;
	SceneLeaveFunc on_leave;
	uint target_fps;
	SceneAction prev_logic_action;
};

extern struct evloop_s {
	Scene stack[EVLOOP_STACK_SIZE];
	Scene *current_scene;
	FrameTimes frame_times;
} evloop;

void eventloop_leave(void);

SceneAction scene_handle_logic(Scene **pscene, const FrameTimes *ftimes);
void scene_render_frame(Scene *scene);
