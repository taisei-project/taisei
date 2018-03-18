/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "render_target.h"

static GLuint r_attachment_to_gl_attachment[] = {
	[RENDERTARGET_ATTACHMENT_DEPTH] = GL_DEPTH_ATTACHMENT,
	[RENDERTARGET_ATTACHMENT_COLOR0] = GL_COLOR_ATTACHMENT0,
	[RENDERTARGET_ATTACHMENT_COLOR1] = GL_COLOR_ATTACHMENT1,
	[RENDERTARGET_ATTACHMENT_COLOR2] = GL_COLOR_ATTACHMENT2,
	[RENDERTARGET_ATTACHMENT_COLOR3] = GL_COLOR_ATTACHMENT3,
};

static_assert(sizeof(r_attachment_to_gl_attachment)/sizeof(GLuint) == RENDERTARGET_MAX_ATTACHMENTS, "");

void r_target_create(RenderTarget *target) {
	assert(target != NULL);
	memset(target, 0, sizeof(RenderTarget));
	target->impl = calloc(1, sizeof(RenderTargetImpl));
	glGenFramebuffers(1, &target->impl->gl_fbo);
}

void r_target_attach(RenderTarget *target, Texture *tex, RenderTargetAttachment attachment) {
	assert(attachment >= 0 && attachment < RENDERTARGET_MAX_ATTACHMENTS);
	assert(tex != NULL);

	GLuint gl_tex = tex ? tex->impl->gl_handle : 0;

	RenderTarget *prev_target = r_target_current();
	r_target(target);
	r_flush();
	glFramebufferTexture2D(GL_FRAMEBUFFER, r_attachment_to_gl_attachment[attachment], GL_TEXTURE_2D, gl_tex, 0);
	r_target(prev_target);

	target->attachments[attachment] = tex;
}

Texture* r_target_get_attachment(RenderTarget *target, RenderTargetAttachment attachment) {
	assert(attachment >= 0 && attachment < RENDERTARGET_MAX_ATTACHMENTS);
	return target->attachments[attachment];
}

void r_target_destroy(RenderTarget *target) {
	assert(target != NULL);
	glDeleteFramebuffers(1, &target->impl->gl_fbo);
	free(target->impl);
}
