/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "color.h"
#include "stageinfo.h"
#include "stageutils.h"

typedef struct Stage4DrawData {
	Color ambient_color;
	vec3 midboss_light_pos;

	struct {
		PBRModel corridor;
		PBRModel ground;
		PBRModel mansion;
	} models;

	struct {
		struct {
			vec3 c_base;
			vec3 c_nstate;
			vec3 c_nstate2;
		} torch_particles;

		struct {
			vec3 base;
			float r_factor;
			float g_factor;
		} torch_light;
	} corridor;

	mat4 fire_emitter_transform;
} Stage4DrawData;

void stage4_drawsys_init(void);
void stage4_drawsys_shutdown(void);

Stage4DrawData *stage4_get_draw_data(void)
	attr_returns_nonnull attr_returns_max_aligned;

void stage4_draw(void);

extern ShaderRule stage4_bg_effects[];
