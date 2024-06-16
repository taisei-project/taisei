/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "camcontrol.h"

#include "events.h"
#include "global.h"
#include "stagetext.h"
#include "util/glm.h"
#include "video.h"
#include "entity.h"
#include "coroutine/taskdsl.h"

#define CAMCTRL_MOVE_SPEED      0.1
// for scroll wheel move speed adjustment
#define CAMCTRL_MOVE_SPEED_STEP (CAMCTRL_MOVE_SPEED * 0.25)

#define CAMCTROL_MOUSE_SENS     0.1

#define CAMCTRL_PITCH_SPEED     -CAMCTROL_MOUSE_SENS
#define CAMCTRL_YAW_SPEED       -CAMCTROL_MOUSE_SENS
#define CAMCTRL_ROLL_SPEED      1.0

#define CAMCTRL_PITCH_AXIS      0
#define CAMCTRL_YAW_AXIS        2
#define CAMCTRL_ROLL_AXIS       1

#define CAMCTRL_KEY_FORWARD     SDL_SCANCODE_W
#define CAMCTRL_KEY_BACK        SDL_SCANCODE_S
#define CAMCTRL_KEY_LEFT        SDL_SCANCODE_A
#define CAMCTRL_KEY_RIGHT       SDL_SCANCODE_D
#define CAMCTRL_KEY_UP          SDL_SCANCODE_R
#define CAMCTRL_KEY_DOWN        SDL_SCANCODE_F

// FIXME doesn't work that well with mouse-look…
#define CAMCTRL_KEY_ROLL_LEFT   SDL_SCANCODE_Q
#define CAMCTRL_KEY_ROLL_RIGHT  SDL_SCANCODE_E

#define CAMCTRL_KEY_TOGGLE_GRAB SDL_SCANCODE_F8

struct cc_state {
	SDL_Window *w;
	bool game_paused;
	bool grab_enabled;
};

INLINE float normangle(float a) {
	return a - 360.0f * floorf((a + 180.0f) / 360.0f);
}

INLINE void addangle(float *a, float d) {
	*a = normangle(*a + d);
}

static void set_mouse_grab(struct cc_state *s, bool enable) {
	enable = enable && s->grab_enabled && !s->game_paused;

	SDL_SetWindowGrab(s->w, enable);
	SDL_CaptureMouse(enable);
	SDL_SetRelativeMouseMode(enable);
	SDL_EventState(SDL_MOUSEMOTION, enable);
	SDL_EventState(SDL_MOUSEWHEEL, enable);
}

static bool keydown_event(SDL_Event *e, void *a) {
	struct cc_state *s = a;

	if(e->key.repeat) {
		return false;
	}

	if(e->key.keysym.scancode == CAMCTRL_KEY_TOGGLE_GRAB) {
		s->grab_enabled = !s->grab_enabled;
		set_mouse_grab(s, s->grab_enabled);
	}

	return false;
}

static bool mouse_event(SDL_Event *e, void *a) {
	Camera3D *cam = a;

	float *pitch = cam->rot.v + CAMCTRL_PITCH_AXIS;
	float *yaw = cam->rot.v + CAMCTRL_YAW_AXIS;

	addangle(pitch, CAMCTRL_PITCH_SPEED * e->motion.yrel);
	addangle(yaw,   CAMCTRL_YAW_SPEED   * e->motion.xrel);

	return false;
}

static bool wheel_event(SDL_Event *e, void *a) {
	float delta = CAMCTRL_MOVE_SPEED_STEP;
	float *move_speed = a;
	*move_speed += e->wheel.y * delta;
	return false;
}

static bool focus_gained_event(SDL_Event *e, void *a) {
	struct cc_state *s = a;
	set_mouse_grab(s, true);
	return false;
}

static bool focus_lost_event(SDL_Event *e, void *a) {
	struct cc_state *s = a;
	set_mouse_grab(s, false);
	return false;
}

static bool pause_event(SDL_Event *e, void *a) {
	struct cc_state *s = a;
	s->game_paused = e->user.code;
	set_mouse_grab(s, !s->game_paused);
	return false;
}

static void nodraw(EntityInterface *ent) { }

static void predraw(EntityInterface *ent, void *a) {
	if(ent) {
		// super lame hack to hide everything
		ent->draw_func = nodraw;
	}
}

TASK(cleanup, { struct cc_state *s; }) {
	ent_unhook_pre_draw(predraw);
	events_unregister_handler(keydown_event);
	events_unregister_handler(mouse_event);
	events_unregister_handler(wheel_event);
	events_unregister_handler(focus_gained_event);
	events_unregister_handler(focus_lost_event);
	events_unregister_handler(pause_event);
	set_mouse_grab(ARGS.s, false);
}

TASK(camera_control, { Camera3D *cam; }) {
	Camera3D *cam = ARGS.cam;

	struct cc_state estate = { };
	estate.w = NOT_NULL(video_get_window());
	estate.grab_enabled = true;
	set_mouse_grab(&estate, true);
	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, cleanup, &estate);

	float move_speed = CAMCTRL_MOVE_SPEED;

	events_register_handler(
		&(EventHandler) {
			.proc = mouse_event,
			.arg = cam,
			.event_type = SDL_MOUSEMOTION,
		}
	);

	events_register_handler(
		&(EventHandler) {
			.proc = wheel_event,
			.arg = &move_speed,
			.event_type = SDL_MOUSEWHEEL,
		}
	);

	events_register_handler(
		&(EventHandler) {
			.proc = keydown_event,
			.arg = &estate,
			.event_type = SDL_KEYDOWN,
		}
	);

	events_register_handler(
		&(EventHandler) {
			.proc = focus_gained_event,
			.arg = &estate,
			.event_type = SDL_WINDOWEVENT_FOCUS_GAINED,
		}
	);

	events_register_handler(
		&(EventHandler) {
			.proc = focus_lost_event,
			.arg = &estate,
			.event_type = SDL_WINDOWEVENT_FOCUS_LOST,
		}
	);

	events_register_handler(
		&(EventHandler) {
			.proc = pause_event,
			.arg = &estate,
			.event_type = MAKE_TAISEI_EVENT(TE_GAME_PAUSE_STATE_CHANGED)
		}
	);

	ent_hook_pre_draw(predraw, NULL);

	Font *fnt = res_font("monotiny");
	StageText *txt = stagetext_add(
		NULL, I*font_get_lineskip(fnt)/2, ALIGN_LEFT, fnt, RGB(1,1,1), 0, 9000000, 0, 0
	);

	StageText *grabtxt = stagetext_add(
		"Mouse input disabled ",
		VIEWPORT_W + I*font_get_lineskip(fnt)/2, ALIGN_RIGHT, fnt, RGB(1,0.5,0), 0, 9000000, 0, 0
	);

	for(;;YIELD) {
		int numkeys = 0;
		const uint8_t *keys = SDL_GetKeyboardState(&numkeys);

		grabtxt->text[0] = estate.grab_enabled ? 0 : 'M';

		if(numkeys < 1) {
			continue;
		}

		float m = move_speed;

		vec3 forward = {  0,  0, -m };
		vec3 right   = {  m,  0,  0 };
		vec3 up      = {  0,  m,  0 };

		mat4 camera_trans;
		glm_mat4_identity(camera_trans);
		camera3d_apply_transforms(cam, camera_trans);
		glm_mat4_inv_precise(camera_trans, camera_trans);

		glm_vec3_rotate_m4(camera_trans, forward, forward);
		glm_vec3_rotate_m4(camera_trans, right, right);
		glm_vec3_rotate_m4(camera_trans, up, up);

		if(keys[CAMCTRL_KEY_FORWARD]) glm_vec3_add(cam->pos, forward, cam->pos);
		if(keys[CAMCTRL_KEY_BACK])    glm_vec3_sub(cam->pos, forward, cam->pos);
		if(keys[CAMCTRL_KEY_RIGHT])   glm_vec3_add(cam->pos, right,   cam->pos);
		if(keys[CAMCTRL_KEY_LEFT])    glm_vec3_sub(cam->pos, right,   cam->pos);
		if(keys[CAMCTRL_KEY_UP])      glm_vec3_add(cam->pos, up,      cam->pos);
		if(keys[CAMCTRL_KEY_DOWN])    glm_vec3_sub(cam->pos, up,      cam->pos);

		float *roll = cam->rot.v + CAMCTRL_ROLL_AXIS;
		if(keys[CAMCTRL_KEY_ROLL_LEFT])  addangle(roll, -CAMCTRL_ROLL_SPEED);
		if(keys[CAMCTRL_KEY_ROLL_RIGHT]) addangle(roll,  CAMCTRL_ROLL_SPEED);

		snprintf(txt->text, sizeof(txt->text),
			"% 9.03f % 9.03f % 9.03f\n% 9.03f % 9.03f % 9.03f",
			cam->pos[0],   cam->pos[1],   cam->pos[2],
			cam->rot.v[0], cam->rot.v[1], cam->rot.v[2]
		);
	}
}

void camcontrol_init(Camera3D *cam) {
	INVOKE_TASK(camera_control, cam);
}
