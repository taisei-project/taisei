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
#include "opengl.h"
#include "texture.h"

struct Framebuffer {
	Texture *attachments[FRAMEBUFFER_MAX_ATTACHMENTS];
	uint attachment_mipmaps[FRAMEBUFFER_MAX_ATTACHMENTS];
	FramebufferAttachment output_mapping[FRAMEBUFFER_MAX_OUTPUTS];
	FloatRect viewport;
	GLuint gl_fbo;
	bool draw_buffers_dirty;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

extern GLuint r_attachment_to_gl_attachment[];

void gl33_framebuffer_prepare(Framebuffer *framebuffer);

Framebuffer *gl33_framebuffer_create(void);
void gl33_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment);
FramebufferAttachmentQueryResult gl33_framebuffer_query_attachment(Framebuffer *framebuffer, FramebufferAttachment attachment);
void gl33_framebuffer_outputs(Framebuffer *framebuffer, FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS], uint8_t write_mask);
void gl33_framebuffer_destroy(Framebuffer *framebuffer);
void gl33_framebuffer_taint(Framebuffer *framebuffer);
void gl33_framebuffer_clear(Framebuffer *framebuffer, BufferKindFlags flags, const Color *colorval, float depthval);
void gl33_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags);
IntExtent gl33_framebuffer_get_effective_size(Framebuffer *framebuffer);
void gl33_framebuffer_set_debug_label(Framebuffer *fb, const char *label);
const char *gl33_framebuffer_get_debug_label(Framebuffer* fb);

// FIXME this function may lie :(
GLTextureFormatInfo *gl33_framebuffer_get_format(
	Framebuffer *framebuffer, FramebufferAttachment attachment);

void gl33_framebuffer_bind_for_read(
	Framebuffer *framebuffer, FramebufferAttachment attachment);
