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
#include "fbpair.h"
#include "geometry.h"
#include "list.h"

typedef struct FramebufferResizeStrategy {
	void (*resize_func)(void *userdata, IntExtent *out_dimensions, FloatRect *out_viewport);
	void (*cleanup_func)(void *userdata);
	void *userdata;
} FramebufferResizeStrategy;

typedef struct FramebufferConfig {
	FramebufferResizeStrategy resize_strategy;
	uint num_attachments;
	FBAttachmentConfig *attachments;
} FramebufferConfig;

typedef struct ManagedFramebuffer {
	Framebuffer *fb;
	LIST_ALIGN char data[];
} ManagedFramebuffer;


typedef struct ManagedFramebufferGroup ManagedFramebufferGroup;

void fbmgr_init(void);
void fbmgr_shutdown(void);

ManagedFramebuffer *fbmgr_framebuffer_create(const char *name, const FramebufferConfig *cfg)
	attr_returns_allocated attr_nonnull(1, 2);

void fbmgr_framebuffer_destroy(ManagedFramebuffer *mfb)
	attr_nonnull(1);

Framebuffer *fbmgr_framebuffer_disown(ManagedFramebuffer *mfb)
	attr_returns_allocated attr_nonnull(1);

ManagedFramebufferGroup *fbmgr_group_create(void)
	attr_returns_allocated;

void fbmgr_group_destroy(ManagedFramebufferGroup *group)
	attr_nonnull(1);

Framebuffer *fbmgr_group_framebuffer_create(ManagedFramebufferGroup *group, const char *name, const FramebufferConfig *cfg)
	attr_returns_allocated attr_nonnull(1, 2, 3);

void fbmgr_group_fbpair_create(ManagedFramebufferGroup *group, const char *name, const FramebufferConfig *cfg, FBPair *fbpair)
	attr_nonnull(1, 2, 3, 4);

// For use as FramebufferConfig.resize_func.resize_func
// Configures the framebuffer to be as large as the main framebuffer, minus the letterboxing
// (as in video_get_viewport_size())
// Requires no cleanup
void fbmgr_resize_strategy_screensized(void *ignored, IntExtent *out_dimensions, FloatRect *out_viewport);
