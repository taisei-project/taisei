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
#include "stageutils.h"

enum {
	NUM_STARS = 200
};

typedef struct Stage6DrawData {
	struct {
		float position[3*NUM_STARS];
	} stars;

	struct {
		int frames;
	} fall_over;

	struct {
		Framebuffer *aux_fb;
		FBPair fbpair;
	} baryon;

	struct {
		PBRModel rim;
		PBRModel spires;
		PBRModel stairs;
		PBRModel tower;
		PBRModel tower_bottom;

		// these don't use pbr materials
		Model *calabi_yau_quintic;
		Model *top_plate;
	} models;

	Texture *envmap;
} Stage6DrawData;

Stage6DrawData* stage6_get_draw_data(void);

void stage6_drawsys_init(void);
void stage6_drawsys_shutdown(void);
void stage6_draw(void);

void baryon_center_draw(Enemy*, int, bool);
void baryon(Enemy*, int, bool);

extern ShaderRule stage6_bg_effects[];
extern ShaderRule stage6_postprocess[];

uint stage6_towerwall_pos(Stage3D *s3d, vec3 pos, float maxrange);
void stage6_towerwall_draw(vec3 pos);

void elly_spellbg_toe(Boss*, int);
void elly_spellbg_modern_dark(Boss*, int);
void elly_spellbg_modern(Boss*, int);
void elly_spellbg_classic(Boss*, int);
void elly_global_rule(Boss*, int);
