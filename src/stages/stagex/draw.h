/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "renderer/api.h"
#include "stageinfo.h"
#include "stageutils.h"

typedef struct StageXDrawData {
	float plr_yaw;
	float plr_pitch;
	float plr_influence;

	struct {
		Color color;
		float red_flash_intensity;
		float opacity;
		float exponent;
		float t;
	} fog;

	struct {
		Framebuffer *tower_mask;
		Framebuffer *glitch_mask;
		Framebuffer *spell_background_lq;
	} fb;

	struct {
		PBRModel metal;
		PBRModel stairs;
		PBRModel wall;
	} models;

	Texture *env_map;

	float codetex_num_segments;
	float codetex_aspect[2];
	float tower_global_dissolution;
	float tower_partial_dissolution;
	float tower_spin;

	COEVENTS_ARRAY(next_phase) events;
} StageXDrawData;

void stagex_drawsys_init(void);
void stagex_drawsys_shutdown(void);

StageXDrawData *stagex_get_draw_data(void)
	attr_returns_nonnull attr_returns_max_aligned;

bool stagex_drawing_into_glitchmask(void);
void stagex_draw_background(void);

extern ShaderRule stagex_bg_effects[];
extern ShaderRule stagex_postprocess_effects[];

#define STAGEX_BG_SEGMENT_PERIOD 11.2
#define STAGEX_BG_MAX_RANGE (64 * STAGEX_BG_SEGMENT_PERIOD)
