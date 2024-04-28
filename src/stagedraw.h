/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stage.h"
#include "resource/resource.h"
#include "util/graphics.h"

typedef enum StageFBPair {
	FBPAIR_BG,
	FBPAIR_BG_AUX,
	FBPAIR_FG,
	FBPAIR_FG_AUX,
	NUM_FBPAIRS,
} StageFBPair;

typedef COEVENTS_ARRAY(
	background_drawn,
	postprocess_before_overlay,
	postprocess_after_overlay
) StageDrawEvents;

void stage_draw_preload(ResourceGroup *rg);
void stage_draw_init(void);
void stage_draw_shutdown(void);
bool stage_draw_is_initialized(void);

StageDrawEvents *stage_get_draw_events(void);

void stage_draw_hud(void);
void stage_draw_viewport(void);
void stage_draw_overlay(void);
void stage_draw_scene(StageInfo *stage);
void stage_draw_bottom_text(void);

void stage_draw_begin_noshake(void);
void stage_draw_end_noshake(void);

bool stage_should_draw_particle(Projectile *p);

void stage_display_clear_screen(const StageClearBonus *bonus);

FBPair *stage_get_fbpair(StageFBPair id) attr_returns_nonnull;
FBPair *stage_get_postprocess_fbpair(void) attr_returns_nonnull;
Framebuffer *stage_add_foreground_framebuffer(const char *label, float scale_worst, float scale_best, uint num_attachments, FBAttachmentConfig attachments[num_attachments]);
Framebuffer *stage_add_background_framebuffer(const char *label, float scale_worst, float scale_best, uint num_attachments, FBAttachmentConfig attachments[num_attachments]);
Framebuffer *stage_add_static_framebuffer(const char *label, uint num_attachments, FBAttachmentConfig attachments[num_attachments]);
