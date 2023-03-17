/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "util/fbpair.h"
#include "stagedraw.h"

typedef struct Stage1DrawData {
	FBPair water_fbpair;
	ShaderProgram *water_shader;

	struct {
		float near, near_target;
		float far, far_target;
	} fog;

	struct {
		float opacity, opacity_target;
	} snow;

	float pitch_target;
} Stage1DrawData;

void stage1_drawsys_init(void);
void stage1_drawsys_shutdown(void);

Stage1DrawData *stage1_get_draw_data(void)
	attr_returns_nonnull attr_returns_max_aligned;

void stage1_draw(void);

extern ShaderRule stage1_bg_effects[];
