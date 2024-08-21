/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "../common/backend.h"

typedef struct FramebufferAttachmentLoadState {
	SDL_GpuLoadOp op;
	union {
		union {
			SDL_FColor as_sdlgpu;
			Color as_taisei;
		} color;
		float depth;
	} clear;
} FramebufferAttachmentLoadState;

typedef struct FramebufferAttachmentData {
	struct {
		Texture *texture;
		uint mip_level;
	} target;
	FramebufferAttachmentLoadState load;
} FramebufferAttachmentData;

static_assert(
	sizeof ((FramebufferAttachmentData){}).load.clear.color.as_sdlgpu ==
	sizeof ((FramebufferAttachmentData){}).load.clear.color.as_taisei);

struct Framebuffer {
	FramebufferAttachmentData attachments[FRAMEBUFFER_MAX_ATTACHMENTS];
	FramebufferAttachment output_mapping[FRAMEBUFFER_MAX_OUTPUTS];
	FloatRect viewport;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

typedef struct DefaultFramebufferState {
	FramebufferAttachmentLoadState color;
	FramebufferAttachmentLoadState depth;
	FloatRect viewport;
} DefaultFramebufferState;

typedef struct RenderPassOutputs {
	SDL_GpuColorAttachmentInfo color[FRAMEBUFFER_MAX_OUTPUTS];
	SDL_GpuTextureFormat color_formats[FRAMEBUFFER_MAX_OUTPUTS];
	SDL_GpuDepthStencilAttachmentInfo depth_stencil;
	uint num_color_attachments;
	bool have_depth_stencil;
} RenderPassOutputs;

Framebuffer *sdlgpu_framebuffer_create(void);
void sdlgpu_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment);
FramebufferAttachmentQueryResult sdlgpu_framebuffer_query_attachment(Framebuffer *framebuffer, FramebufferAttachment attachment);
void sdlgpu_framebuffer_outputs(Framebuffer *framebuffer, FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS], uint8_t write_mask);
void sdlgpu_framebuffer_destroy(Framebuffer *framebuffer);
void sdlgpu_framebuffer_taint(Framebuffer *framebuffer);
void sdlgpu_framebuffer_clear(Framebuffer *framebuffer, BufferKindFlags flags, const Color *colorval, float depthval);
void sdlgpu_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags);
IntExtent sdlgpu_framebuffer_get_size(Framebuffer *framebuffer);
void sdlgpu_framebuffer_viewport(Framebuffer *fb, FloatRect vp);
void sdlgpu_framebuffer_viewport_current(Framebuffer *fb, FloatRect *vp);
FloatRect *sdlgpu_framebuffer_viewport_pointer(Framebuffer *fb);
void sdlgpu_framebuffer_set_debug_label(Framebuffer *fb, const char *label);
const char *sdlgpu_framebuffer_get_debug_label(Framebuffer* fb);

void sdlgpu_framebuffer_flush(Framebuffer *framebuffer, uint32_t attachment_mask);
void sdlgpu_framebuffer_setup_outputs(Framebuffer *framebuffer, RenderPassOutputs *outputs);
