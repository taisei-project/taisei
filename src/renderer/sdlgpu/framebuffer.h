/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "texture.h"

#include "../api.h"
#include "../common/backend.h"

struct Framebuffer {
	TextureSlice attachments[FRAMEBUFFER_MAX_ATTACHMENTS];
	FramebufferAttachment output_mapping[FRAMEBUFFER_MAX_OUTPUTS];
	FloatRect viewport;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

typedef struct DefaultFramebufferState {
	TextureLoadState color;
	TextureLoadState depth;
	FloatRect viewport;
} DefaultFramebufferState;

typedef struct RenderPassOutputs {
	SDL_GPUColorTargetInfo color[FRAMEBUFFER_MAX_OUTPUTS];
	SDL_GPUTextureFormat color_formats[FRAMEBUFFER_MAX_OUTPUTS];
	SDL_GPUDepthStencilTargetInfo depth_stencil;
	SDL_GPUTextureFormat depth_format;
	uint num_color_attachments;
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
