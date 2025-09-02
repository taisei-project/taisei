/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stageinfo.h"
#include "stageutils.h"

typedef struct Stage2DrawData {
	struct {
		Color color;
		float end;
	} fog;

	struct {
		PBRModel branch;
		PBRModel grass;
		PBRModel ground;
		PBRModel water;
		PBRModel leaves;
		PBRModel rocks;
	} models;

	Texture *envmap;

	float hina_lights;
	mat4 hina_fire_emitter_transform;

	uint64_t branch_rng_seed;
} Stage2DrawData;

Stage2DrawData *stage2_get_draw_data(void)
	attr_returns_nonnull attr_returns_max_aligned;

void stage2_drawsys_init(void);
void stage2_drawsys_shutdown(void);
void stage2_draw(void);

extern ShaderRule stage2_bg_effects[];
