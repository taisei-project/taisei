/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "camcontrol.h"

#include "stagetext.h"
#include "coroutine.h"
#include "util/glm.h"
#include "video.h"

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

INLINE float normangle(float a) {
	return a - 360.0f * floorf((a + 180.0f) / 360.0f);
}

INLINE void addangle(float *a, float d) {
	*a = normangle(*a + d);
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

TASK(camera_control, { Camera3D *cam; }) {
	Camera3D *cam = ARGS.cam;

	SDL_Window *w = NOT_NULL(video_get_window());
	SDL_SetWindowGrab(w, true);
	SDL_CaptureMouse(true);
	SDL_SetRelativeMouseMode(true);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEWHEEL, SDL_ENABLE);

	float move_speed = CAMCTRL_MOVE_SPEED;

	events_register_handler(
		&(EventHandler) { .proc = mouse_event, .arg = cam, .event_type = SDL_MOUSEMOTION }
	);

	events_register_handler(
		&(EventHandler) { .proc = wheel_event, .arg = &move_speed, .event_type = SDL_MOUSEWHEEL }
	);

	Font *fnt = res_font("monotiny");
	StageText *txt = stagetext_add(
		NULL, I*font_get_lineskip(fnt)/2, ALIGN_LEFT, fnt, RGB(1,1,1), 0, 9000000, 0, 0
	);

	for(;;YIELD) {
		int numkeys = 0;
		const uint8_t *keys = SDL_GetKeyboardState(&numkeys);

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
