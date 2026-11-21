/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "eventloop.h"
#include "eventloop_private.h"

#include "debug_text.h"
#include "global.h"
#include "thread.h"
#include "util.h"
#include "vfs/public.h"
#include "video.h"

struct evloop_s evloop;

void eventloop_enter(
	void *context, SceneLogicFunc logic, SceneRenderFunc render, SceneLeaveFunc on_leave, uint target_fps
) {
	assert(thread_current_is_main());
	assume(evloop.current_scene < evloop.stack + EVLOOP_STACK_SIZE - 1);
	assert(target_fps > 0);

	Scene *scene;

	if(evloop.current_scene == NULL) {
		scene = evloop.current_scene = evloop.stack;
	} else {
		scene = ++evloop.current_scene;
	}

	scene->context = context;
	scene->logic = logic;
	scene->render = render;
	scene->on_leave = on_leave;
	scene->target_fps = target_fps;
	scene->prev_logic_action = SCENE_ACTION_WAIT;

	vfs_sync(VFS_SYNC_STORE, NO_CALLCHAIN);
}

void eventloop_leave(void) {
	assert(thread_current_is_main());
	assume(evloop.current_scene != NULL);

	Scene *scene = evloop.current_scene;

	if(evloop.current_scene == evloop.stack) {
		evloop.current_scene = NULL;
	} else {
		--evloop.current_scene;
	}

	if(scene->on_leave != NULL) {
		scene->on_leave(scene->context);
	}
}

FrameTimes eventloop_get_frame_times(void) {
	return evloop.frame_times;
}

static SceneAction run_logic(Scene *scene) {
	assert(scene == evloop.current_scene);
	debug_text_rollback();

	if(scene->prev_logic_action == SCENE_ACTION_STOP) {
		return SCENE_ACTION_STOP;
	}

	SceneAction a = scene->logic(scene->context);

	if(a != SCENE_ACTION_SKIP_ALWAYS) {
		fpscounter_update(&global.fps.logic, time_get());
	}

	if(taisei_quit_requested()) {
		a = SCENE_ACTION_STOP;
	}

	scene->prev_logic_action = a;
	return a;
}

SceneAction scene_handle_logic(Scene **pscene, const FrameTimes *ftimes) {
	SceneAction action;
	uint cnt = 0;

	do {
		action = run_logic(*pscene);

		if(action == SCENE_ACTION_SKIP_ALWAYS) {
			action = SCENE_ACTION_SKIP;
			cnt = 0;
		}

		if(evloop.current_scene != *pscene) {
			*pscene = evloop.current_scene;
			return action;
		}
	} while(
		action == SCENE_ACTION_SKIP &&
		++cnt < config_get_int(CONFIG_SKIP_SPEED)
	);

	if(action == SCENE_ACTION_STOP) {
		eventloop_leave();
		*pscene = evloop.current_scene;

		if(*pscene == NULL) {
			return SCENE_ACTION_STOP;
		}
	}

	return action;
}

void scene_render_frame(Scene *scene) {
	attr_unused Scene *stack_prev = evloop.current_scene;
	r_begin_frame();
	r_framebuffer_clear(NULL, BUFFER_ALL, RGBA(0, 0, 0, 1), 1);
	scene->render(scene->context);
	assert(evloop.current_scene == stack_prev);
	video_finish_frame();
	r_end_frame();
}
