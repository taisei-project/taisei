/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/fbpair.h"
#include "stagedraw.h"
#include "stageutils.h"

typedef struct Stage3DrawData {
	struct {
		PBRModel ground;
		PBRModel leaves;
		PBRModel rocks;
		PBRModel trees;
	} models;

	Texture *envmap;

	vec3 environment_color;
	vec3 ambient_color;
	vec4 fog_color;

	float boss_light_alpha;
	PointLight3D boss_light;

	float swing_strength;
	float target_swing_strength;
} Stage3DrawData;

Stage3DrawData *stage3_get_draw_data(void)
	attr_returns_nonnull attr_returns_max_aligned;

void stage3_drawsys_init(void);
void stage3_drawsys_shutdown(void);
void stage3_draw(void);

extern ShaderRule stage3_bg_effects[];
extern ShaderRule stage3_postprocess[];
