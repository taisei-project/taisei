/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "opengl.h"
#include "texture.h"

struct Framebuffer {
	Texture *attachments[FRAMEBUFFER_MAX_ATTACHMENTS];
	uint attachment_mipmaps[FRAMEBUFFER_MAX_ATTACHMENTS];
	IntRect viewport;
	GLuint gl_fbo;
	bool initialized;
	char debug_label[R_DEBUG_LABEL_SIZE];
};

void gl33_framebuffer_prepare(Framebuffer *framebuffer);

Framebuffer* gl33_framebuffer_create(void);
void gl33_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment);
Texture *gl33_framebuffer_get_attachment(Framebuffer *framebuffer, FramebufferAttachment attachment);
uint gl33_framebuffer_get_attachment_mipmap(Framebuffer *framebuffer, FramebufferAttachment attachment);
void gl33_framebuffer_destroy(Framebuffer *framebuffer);
void gl33_framebuffer_taint(Framebuffer *framebuffer);
void gl33_framebuffer_clear(Framebuffer *framebuffer, ClearBufferFlags flags, const Color *colorval, float depthval);
void gl33_framebuffer_set_debug_label(Framebuffer *fb, const char *label);
const char* gl33_framebuffer_get_debug_label(Framebuffer* fb);
