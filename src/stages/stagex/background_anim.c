/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "background_anim.h"   // IWYU pragma: keep
#include "draw.h"

#include "random.h"
#include "stageutils.h"
#include "util/glm.h"

TASK(animate_value, { float *val; float target; float rate; }) {
	while(*ARGS.val != ARGS.target) {
		fapproach_p(ARGS.val, ARGS.target, ARGS.rate);
		YIELD;
	}
}

TASK(animate_value_asymptotic, { float *val; float target; float rate; float epsilon; }) {
	if(ARGS.epsilon == 0) {
		ARGS.epsilon = 1e-5;
	}

	while(*ARGS.val != ARGS.target) {
		fapproach_asymptotic_p(ARGS.val, ARGS.target, ARGS.rate, ARGS.epsilon);
		YIELD;
	}
}

static void animate_bg_intro(StageXDrawData *draw_data) {
	const float w = -0.005f;
	float transition = 0.0;
	float t = 0.0;

	stage_3d_context.cam.pos[2] = 2.0 * STAGEX_BG_SEGMENT_PERIOD/3;
	stage_3d_context.cam.rot.pitch = 40;
	stage_3d_context.cam.rot.roll = 10;

	float prev_x0 = stage_3d_context.cam.pos[0];
	float prev_x1 = stage_3d_context.cam.pos[1];

	for(int frame = 0;; ++frame) {
		float dt = 5.0f * (1.0f - transition);
		float rad = 4.3f * glm_ease_back_in(glm_ease_sine_out(1.0f - transition));
		stage_3d_context.cam.rot.pitch = -10 + 50 * glm_ease_sine_in(1.0f - transition);
		stage_3d_context.cam.pos[0] += rad*cosf(-w*t) - prev_x0;
		stage_3d_context.cam.pos[1] += rad*sinf(-w*t) - prev_x1;
		stage_3d_context.cam.pos[2] += w*5.6f/M_PI*dt;
		prev_x0 = stage_3d_context.cam.pos[0];
		prev_x1 = stage_3d_context.cam.pos[1];
		stage_3d_context.cam.rot.roll -= 180.0f/M_PI*w*dt;

		t += dt;

		if(frame > 60) {
			if(transition == 1.0f) {
				return;
			}

			if(frame == 61) {
				INVOKE_TASK_DELAYED(60, animate_value, &stage_3d_context.cam.vel[2], -0.38f, 0.002f);
			}

			fapproach_p(&transition, 1.0f, 1.0f/300.0f);
		}

		YIELD;
	}
}

static void animate_bg_descent(StageXDrawData *draw_data, CoEvent *stop_condition) {
	const float max_dist = 1.5f;
	const float max_spin = RAD2DEG*0.01f;
	float dist = 0.0f;
	float target_dist = 0.0f;
	float spin = 0.0f;

	CoEventSnapshot snap = coevent_snapshot(stop_condition);

	for(float a = 0; coevent_poll(stop_condition, &snap) == CO_EVENT_PENDING; a += 0.01) {
		float r = smoothmin(dist, max_dist, max_dist * 0.5);

		stage_3d_context.cam.pos[0] =  r * cos(a);
		stage_3d_context.cam.pos[1] = -r * sin(a);
		// stage_3d_context.cam.rot.roll += spin * sin(0.4331*a);
		fapproach_asymptotic_p(&draw_data->tower_spin,  spin * sin(0.4331*a), 0.02, 1e-4);
		fapproach_p(&spin, max_spin, max_spin / 240.0f);

		fapproach_asymptotic_p(&dist, target_dist, 0.01, 1e-4);
		fapproach_asymptotic_p(&target_dist, 0, 0.03, 1e-4);

		if(rng_chance(0.02)) {
			target_dist += max_dist;
		}

		YIELD;
	}
}

static void animate_bg_midboss(StageXDrawData *draw_data, CoEvent *stop_condition) {
	float camera_shift_rate = 0;

	// stagetext_add("Midboss time!", CMPLX(VIEWPORT_W, VIEWPORT_H)/2, ALIGN_CENTER, res_font("big"), RGB(1,1,1), 0, 120, 30, 30);

	CoEventSnapshot snap = coevent_snapshot(stop_condition);

	while(coevent_poll(stop_condition, &snap) == CO_EVENT_PENDING) {
	// for(;;) {
		fapproach_p(&camera_shift_rate, 1, 1.0f/120.0f);
		fapproach_asymptotic_p(&draw_data->plr_influence, 0, 0.01, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.vel[2], -0.12, 0.02, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.rot.pitch, 30, 0.01 * camera_shift_rate, 1e-4);

		float a = glm_rad(stage_3d_context.cam.rot.roll + 67.5);
		cmplxf p = cdir(a) * -3.5f;
		fapproach_asymptotic_p(&stage_3d_context.cam.pos[0], re(p), 0.01 * camera_shift_rate, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.pos[1], im(p), 0.01 * camera_shift_rate, 1e-4);

		YIELD;
	}
}

static void animate_bg_post_midboss(StageXDrawData *draw_data, int anim_time) {
	float center_distance = -3.5f;
	float camera_shift_rate = 0;

	// stagetext_add("Midboss defeated!", CMPLX(VIEWPORT_W, VIEWPORT_H)/2, ALIGN_CENTER, res_font("big"), RGB(1,1,1), 0, 120, 30, 30);

	while(anim_time-- > 0) {
		fapproach_p(&camera_shift_rate, 1, 1.0f/120.0f);
		fapproach_asymptotic_p(&draw_data->plr_influence, 1, 0.01, 1e-4);
		fapproach_p(&stage_3d_context.cam.vel[2], -0.56f, 0.002f);
		fapproach_asymptotic_p(&center_distance, 0, 0.03, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.rot.pitch, -10, 0.01 * camera_shift_rate, 1e-4);

		float a = glm_rad(stage_3d_context.cam.rot.roll + 67.5);
		cmplxf p = cdir(a) * center_distance;
		fapproach_asymptotic_p(&stage_3d_context.cam.pos[0], re(p), 0.01 * camera_shift_rate, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.pos[1], im(p), 0.01 * camera_shift_rate, 1e-4);

		YIELD;
	}
}

TASK(animate_light, { StageXDrawData *draw_data; }) {
	StageXDrawData *draw_data = ARGS.draw_data;

	float rate = 1.0f/20.0f;

	for(;;) {
		Color c;
		c.r = psinf(draw_data->fog.t);
		c.g = 0.25f * c.r * powf(pcosf(draw_data->fog.t), 1.25f);
		c.b = c.r * c.g;

		float w = 1.0f - sqrtf(erff(8.0f * c.g));
		w = lerpf(1.0f, w, draw_data->fog.red_flash_intensity);
// 		c.b += w*w*w*w*w;

		float b = 0.1;
		c.r = lerpf(c.r, 0.3f*b, w);
		c.g = lerpf(c.g, 0.1f*b, w);
		c.b = lerpf(c.b, 0.8f*b, w);
		c.a = 1.0f;

// 		color_lerp(&c, RGBA(1, 1, 1, 1), 0.2);
		draw_data->fog.color = c;

		YIELD;
		draw_data->fog.t += rate;
	}
}

TASK(animate_bg, { StageXDrawData *draw_data; }) {
	StageXDrawData *draw_data = ARGS.draw_data;
	draw_data->fog.exponent = 4;

	INVOKE_TASK(animate_light, draw_data);

	INVOKE_TASK_DELAYED(140, animate_value, &draw_data->fog.opacity, 1.0f, 1.0f/80.0f);
	INVOKE_TASK_DELAYED(140, animate_value_asymptotic, &draw_data->fog.exponent, 42.0f, 0.004f);
	INVOKE_TASK(animate_value, &draw_data->plr_influence, 1.0f, 1.0f/120.0f);
	animate_bg_intro(draw_data);
	INVOKE_TASK_DELAYED(520, animate_value, &draw_data->fog.red_flash_intensity, 1.0f, 1.0f/320.0f);
	INVOKE_TASK_DELAYED(200, animate_value, &stage_3d_context.cam.vel[2], -0.56f, 0.002f);
	animate_bg_descent(draw_data, &draw_data->events.next_phase);
	animate_bg_midboss(draw_data, &draw_data->events.next_phase);
	animate_bg_post_midboss(draw_data, 60 * 7);
	animate_bg_descent(draw_data, &draw_data->events.next_phase);
	INVOKE_TASK(animate_value_asymptotic, &draw_data->fog.exponent, 8.0f, 0.01f);
	INVOKE_TASK(animate_value, &draw_data->tower_global_dissolution, 1.0f, 1.0f/500.0f);
	INVOKE_TASK(animate_value_asymptotic, &stage_3d_context.cam.pos[0], 0, 0.02, 1e-4);
	INVOKE_TASK(animate_value_asymptotic, &stage_3d_context.cam.pos[1], 0, 0.02, 1e-4);
}

void stagex_bg_trigger_next_phase(void) {
	StageXDrawData *draw_data = stagex_get_draw_data();
	coevent_signal(&draw_data->events.next_phase);
}

void stagex_bg_trigger_tower_dissolve(void) {
	StageXDrawData *draw_data = stagex_get_draw_data();
	INVOKE_TASK(animate_value, &draw_data->tower_partial_dissolution, 1.0f, 1.0f/600.0f);
}

void stagex_bg_init_fullstage(void) {
	StageXDrawData *draw_data = stagex_get_draw_data();

	Camera3D *cam = &stage_3d_context.cam;
// 	cam->aspect = STAGE3D_DEFAULT_ASPECT; // FIXME
// 	cam->pos[0] = 0;
// 	cam->pos[1] = 0;
// 	cam->vel[2] = -0.56f;
// 	cam->rot.v[0] = 0;

	INVOKE_TASK(animate_bg, draw_data);
}
