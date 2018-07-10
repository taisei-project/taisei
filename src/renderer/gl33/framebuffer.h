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

struct FramebufferImpl {
	GLuint gl_fbo;
	bool initialized;
	Texture *attachments[FRAMEBUFFER_MAX_ATTACHMENTS];
	uint attachment_mipmaps[FRAMEBUFFER_MAX_ATTACHMENTS];
	IntRect viewport;
};

void gl33_framebuffer_prepare(Framebuffer *framebuffer);

void gl33_framebuffer_create(Framebuffer *framebuffer);
void gl33_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment);
Texture *gl33_framebuffer_get_attachment(Framebuffer *framebuffer, FramebufferAttachment attachment);
uint gl33_framebuffer_get_attachment_mipmap(Framebuffer *framebuffer, FramebufferAttachment attachment);
void gl33_framebuffer_destroy(Framebuffer *framebuffer);
