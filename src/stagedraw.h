/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_stagedraw_h
#define IGUARD_stagedraw_h

#include "taisei.h"

#include "stage.h"
#include "util/graphics.h"

typedef enum StageFBPair {
	FBPAIR_BG,
	FBPAIR_BG_AUX,
	FBPAIR_FG,
	FBPAIR_FG_AUX,
	NUM_FBPAIRS,
} StageFBPair;

void stage_draw_init(void);
void stage_draw_shutdown(void);
void stage_draw_hud(void);
void stage_draw_foreground(void);
void stage_draw_scene(StageInfo *stage);
bool stage_should_draw_particle(Projectile *p);

FBPair* stage_get_fbpair(StageFBPair id) attr_returns_nonnull;
Framebuffer* stage_add_foreground_framebuffer(const char *label, float scale_worst, float scale_best, uint num_attachments, FBAttachmentConfig attachments[num_attachments]);
Framebuffer* stage_add_background_framebuffer(const char *label, float scale_worst, float scale_best, uint num_attachments, FBAttachmentConfig attachments[num_attachments]);

#endif // IGUARD_stagedraw_h
