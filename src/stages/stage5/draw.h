/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"
#include "stageutils.h"

typedef struct Stage5DrawData {
	struct {
		float light_strength;
		float light_pos;

		float rad;
		float zoffset;
		float roffset;
	} stairs;

	struct {
		PBRModel metal;
		PBRModel stairs;
		PBRModel wall;
	} models;

	Texture *env_map;
} Stage5DrawData;

void stage5_drawsys_init(void);

void iku_spell_bg(Boss *b, int t);

Stage5DrawData *stage5_get_draw_data(void);
